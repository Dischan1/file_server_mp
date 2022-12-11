#ifndef ENABLE_SERVER

#include "includes.h"

#include "common_utils.h"
#include "cryptography.h"
#include "ctcp_client.h"

bool transmit_file(_connection* con, const char* from, const char* to)
{
    char* buffer;
    long size;

    if (load_file(&buffer, &size, from))
    {
        _rpc_status status = transmit_rpc(con, _rpc_file_upload);

        if (status != _rpc_status_accepted)
        {
            free(buffer);
            return false;
        }

        if (!transmit_string(con, to))
        {
            free(buffer);
            return false;
        }

        if (!transmit_data(con, buffer, size))
        {
            free(buffer);
            return false;
        }

        free(buffer);
    }

    return false;
}

bool receive_file(_connection* con, const char* from, const char* to)
{
    _rpc_status status = transmit_rpc(con, _rpc_file_download);
    if (status != _rpc_status_accepted) return false;

    if (!transmit_string(con, from))
        return false;

    char* data;
    size_t size;

    if (receive_data(con, &data, &size))
    {
        save_file(data, size, to);
        return free(data), true;
    }

    return false;
}

bool list_directory(_connection* con, const char* path)
{
    _rpc_status status = transmit_rpc(con, _rpc_list_directory);
    if (status != _rpc_status_accepted) return false;

    if (!transmit_string(con, path))
        return false;

    char* data;
    size_t size;

    if (receive_data(con, &data, &size))
    {
        printf("\n\n%s\n\n", data);
        return free(data), true;
    }

    return false;
}

bool transmit_message(_connection* con, const char* message)
{
    _rpc_status status = transmit_rpc(con, _rpc_message);
    if (status != _rpc_status_accepted) return false;

    return transmit_string(con, message);
}

bool client_connect(_connection* con)
{
    _rpc_status status = transmit_rpc(con, _rpc_connect);
    if (status != _rpc_status_accepted) return false;

    _rsa_data rsa;
    rsa_generate_keys(&rsa);
    rsa_get_private(&rsa, &con->key.priv);

    _rsa_public pub;
    rsa_get_public(&rsa, &pub);

    if (!transmit_data(con, &pub, sizeof(pub)))
        return false;

    if (!receive_data_sized(con, &con->key.pub, sizeof(con->key.pub)))
        return false;

    return con->encryption = true;
}

bool client_disconnect(_connection* con)
{
    return transmit_rpc(con, _rpc_disconnect) == _rpc_status_accepted;
}

bool client_register(_connection* con, char* passphrase)
{
    _rpc_status status = transmit_rpc(con, _rpc_register);
    if (status != _rpc_status_accepted) return false;

    return transmit_string(con, passphrase);
}

bool client_login(_connection* con, char* passphrase)
{
    _rpc_status status = transmit_rpc(con, _rpc_login);
    if (status != _rpc_status_accepted) return false;

    return transmit_string(con, passphrase);
}

void process_client_receive(_connection* con)
{
    while (con->is_active)
    {
        int fromlen = sizeof(con->sockaddr_in);

        int result = recvfrom(con->socket, &con->packet,
            sizeof(con->packet), 0, &con->sockaddr_in, &fromlen);

        bool is_packet_size_valid =
            result != SOCKET_DATA_EMPTY &&
            result >= sizeof(_ctcp_header) &&
            result <= sizeof(_ctcp_packet);

        if (!is_packet_size_valid)  continue;
        if (con->packet.header.ACK) continue;

        if (!ctcp_is_checksum_valid(&con->packet))
        {
            dp_error("invalid checksum\n");
            continue;
        }

        dp_status(result > 0, "%4x: sv->cl->%x\n", con->guid, result);

        if (con->packet.header.index == con->index_next)
        {
            if (con->encryption && con->packet.header.payload_size)
            {
                char* dec = decrypt_data(con, con->packet.payload, con->packet.header.payload_size);
                if (!dec) continue;

                memcpy(con->packet.payload, dec, con->packet.header.payload_size);
                free(dec);

                con->packet.header.payload_size       = con->packet.header.payload_size / 4u;
                con->packet.header.payload_size_total = con->packet.header.payload_size_total / 4u;
            }

            //con->packet = packet;

            ctcp_write_frame_queue(&con->input, &con->packet);
            con->index_next += 1u;
        }

        ctcp_send_ack(con);
    }

    ctcp_dispose_connection(con);
}

void process_client_transmit(_connection* con)
{
    while (con->is_active)
    {
        if (!con->output.count) continue;

        _ctcp_packet* cache = ctcp_peak_frame_queue(&con->output);
        cache->header.index = con->index_next;

        if (con->encryption && cache->header.payload_size)
        {
            char* enc = encrypt_data(con, cache->payload, cache->header.payload_size);
            if (!enc) continue;

            cache->header.payload_size       = cache->header.payload_size * sizeof(int);
            cache->header.payload_size_total = cache->header.payload_size_total * sizeof(int);

            memcpy(cache->payload, enc, cache->header.payload_size);
            free(enc);
        }

        con->packet.header.ACK = false;
        ctcp_subscribe_packet(cache);

        int result = -1;

        while (!con->packet.header.ACK || con->packet.header.index != cache->header.index)
        {
            uint32_t packet_size = sizeof(cache->header) + cache->header.payload_size;

            result = sendto(con->socket, cache, packet_size,
                0, &con->sockaddr_in, sizeof(con->sockaddr_in));

            sleep(10);
        }

        con->index_next += 1u;

        cache->header.ACK = con->packet.header.ACK;
        ctcp_skip_frame_queue(&con->output);

        dp_status(result > 0, "%4x: cl->sv->%x\n", con->guid, result);
    }

    ctcp_dispose_connection(con);
}

int print_usage()
{
    printf("usage:\n");
    printf("      client <ip> <port> <passphrase> [r/t] <from> <to>\n");
    printf("      or\n");
    printf("      client <ip> <port> <passphrase> l <path>\n");
    printf("      or\n");
    printf("      client <ip> <port> <passphrase> m <message>\n");
    printf("      or\n");
    printf("      client <ip> <port> <passphrase> g\n");
    return 0;
}

int validate_arguments(int argc, char* argv[])
{
    if (argc == 1) return -1;

    if (argc >= 5)
    {
        switch (argv[4][0])
        {
            case 'r': case 't':
            {
                if (argc != 7)
                    return -2;
                break;
            }
            case 'l':
            {
                if (argc != 6)
                    return -2;
                break;
            }
            case 'm':
            {
                if (argc != 6)
                    return -2;
                break;
            }
            case 'g':
            {
                if (argc != 5)
                    return -2;
                break;
            }
            default:
                return -2;
        }
    }
    else
        return -3;

    return 0;
}

_connection gcon;

int main(int argc, char* argv[])
{
    /*

    argv[0]   executable_name
                    |
    argv[1]         ip
                    |
    argv[2]        port
                    |
    argv[3]     passphrase
                    |
    argv[4]        [r/t/ - - - - /l - - - - /m - - - - /g]
                    |             |          |
    argv[5]        from          path     message
                    |
    argv[6]         to

    */

    srand(time(NULL));

    int code = 0;

    if (code = validate_arguments(argc, argv))
        return code < 0 ? print_usage() : code;

    if (code = network_startup())
        return _last_error_code;

    _socket sk = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    _sockaddr_in sa;
    sa.sin_family      = AF_INET;
    sa.sin_port        = htons(strtoull(argv[2], 0, 10));
    sa.sin_addr.s_addr = inet_addr(argv[1]);

    _connection* con = ctcp_setup_connection(&gcon);
    con->socket      = sk;
    con->sockaddr_in = sa;

    thread_create(&process_client_receive, con);
    thread_create(&process_client_transmit, con);

    if (!client_connect(con))
    {
        dp_error("connect error\n");
        return 2;
    }

    if (argv[3][0] != 'g')
    {
        if (!client_login(con, argv[3]))
        {
            dp_error("register error\n");
            return 2;
        }
    }

    if (argv[4][0] == 't')
    {
        if (transmit_file(con, argv[5], argv[6]))
            dp_success("file sent successfully\n");
    }
    else if (argv[4][0] == 'r')
    {
        if (receive_file(con, argv[5], argv[6]))
            dp_success("file received successfully\n");
    }
    else if (argv[4][0] == 'l')
    {
        if (list_directory(con, argv[5]))
            dp_success("string received successfully\n");
    }
    else if (argv[4][0] == 'm')
    {
        if (transmit_message(con, argv[5], argv[6]))
            dp_success("string transmitted successfully\n");
    }
    else if (argv[4][0] == 'g')
    {
        if (client_register(con, argv[3]))
            dp_success("client registred successfully\n");
    }
    else
        dp_error("unknown operation\n");

    client_disconnect(con);

    ctcp_dispose_connection(con);
    socket_close(sk);

    dp_status(code == 0, "exit code: %x\n", code);
    return network_shutdown();
}
#endif // ENABLE_SERVER