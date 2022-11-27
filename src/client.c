//#define SERVER_DEBUG
#ifndef SERVER_DEBUG

#include "includes.h"

#include "common_utils.h"
#include "cryptography.h"
#include "ctcp_client.h"

_client_id g_client_id;

bool transmit_file(_connection* con, const char* from, const char* to)
{
    char* buffer;
    long size;

    if (load_file(&buffer, &size, from))
    {
        _packet request;
        construct_packet(&request, to, _packet_file_upload, g_client_id, size);

        if (transmit_packet(con, &request))
        {
            load_file(&buffer, &size, from);
            transmit_encrypt_data(con, buffer, size);
        }

        free(buffer);
    }

    return false;
}

bool receive_file(_connection* con, const char* from, const char* to)
{
    _packet request;

    construct_packet(&request, from, _packet_file_download, g_client_id, 0);
    transmit_packet(con, &request);

    char* data;
    size_t size;

    if (receive_decrypt_data(con, &data, &size))
    {
        save_file(data, size, to);
        free(data);
        return true;
    }

    return false;
}

bool list_directory(_connection* con, const char* path)
{
    _packet request;

    construct_packet(&request, path, _packet_list_directory, g_client_id, 0);
    transmit_packet(con, &request);

    char* data;
    size_t size;

    if (receive_decrypt_data(con, &data, &size))
    {
        printf("\n\n%s\n\n", data);
        free(data);
        return true;
    }

    return false;
}

bool ctcp_on_packet_received(_connection* con, _ctcp_packet* packet)
{
    con->ping_last = time(NULL);
    ctcp_write_frame_queue(&con->input, packet);
}

void process_client_receive(_connection* con)
{
    while (true)
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

        ctcp_write_frame_queue(&con->input, &con->packet);
        con->packet.header.ACK          = true;
        con->packet.header.payload_size = 0u;

        ctcp_subscribe_packet(&con->packet);
        result = sendto(con->socket, &con->packet, sizeof(con->packet.header),
            0, &con->sockaddr_in, sizeof(con->sockaddr_in));
    }
}

void process_client_transmit(_connection* con)
{
    while (true)
    {
        if (!con->output.count) continue;

        _ctcp_packet* cache = ctcp_peak_frame_queue(&con->output);

        int result = -1;

        con->packet.header.ACK = false;
        ctcp_subscribe_packet(cache);

        while (!con->packet.header.ACK)
        {
            result = sendto(con->socket, cache, sizeof(*cache),
                0, &con->sockaddr_in, sizeof(con->sockaddr_in));

            sleep(10);
        }

        cache->header.ACK = con->packet.header.ACK;
        ctcp_skip_frame_queue(&con->output);

        dp_status(result > 0, "%4x: cl->sv->%x\n", con->guid, result);
    }
}

int print_usage()
{
    printf("usage:\n");
    printf("      client <client_id> <ip> <port> [r/t] <from> <to>\n");
    printf("      or\n");
    printf("      client <client_id> <ip> <port> l <path>\n");
    printf("      or\n");
    printf("      client <client_id>\n");
    return 0;
}

int validate_arguments(int argc, char* argv[])
{
    if (argc == 1) return -1;

    if (argc == 2)
    {
        _client_id client_id = strtoull(argv[1], 0, 10);
        rsa_generate_keys(&g_rsa_data);
        rsa_save_client_keys(&g_rsa_data, client_id);
        return 1;
    }

    if (argc >= 2)
    {
        g_client_id = strtoull(argv[1], 0, 10);
        if (!initialize_client_rsa(g_client_id)) return -1;
    }

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
            default:
                return -2;
        }
    }
    else
        return -3;

    return 0;
}

int main(int argc, char* argv[])
{
    /*

    argv[0]   executable_name
                    |
    argv[1]      client_id
                    |
    argv[2]        ip
                    |
    argv[3]        port
                    |
    argv[4]        [r/t/ - - - - /l]
                    |             |
    argv[5]       from           path
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
    sa.sin_port        = htons(strtoull(argv[3], 0, 10));
    sa.sin_addr.s_addr = inet_addr(argv[2]);

    _connection* con = ctcp_allocate_connection();
    con->socket      = sk;
    con->sockaddr_in = sa;

    thread_create(&process_client_receive, con);
    thread_create(&process_client_transmit, con);

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
    else
        dp_error("unknown operation\n");

    ctcp_dispose_connection(con);
    socket_close(sk);

    dp_status(code == 0, "exit code: %x\n", code);
    return network_shutdown();
}
#endif