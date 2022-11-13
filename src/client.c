//#define SERVER_DEBUG
#ifndef SERVER_DEBUG

#include "includes.h"

#include "defines.h"
#include "common_utils.h"
#include "cryptography.h"

_client_id g_client_id;

bool transmit_file(_socket sk, const char* from, const char* to)
{
    char* buffer;
    long size;

    if (load_file(&buffer, &size, from))
    {
        log("[+] file loaded: \n");
        log("    size: 0x%x bytes\n", size);
        log("    from: %s\n", from);
        log("    to: %s\n", to);

        _packet* request = construct_packet(to, _packet_file_upload, g_client_id, size);

        if (transmit_packet(sk, request))
        {
            log("[+] packet sent successfully\n");
            load_file(&buffer, &size, from);

            if (transmit_encrypt_data(sk, buffer, size))
                log("[+] data sent successfully\n");
            else
                log("[-] data sent error: %x\n", _last_error_code);
        }
        else
            log("[-] packet sent error: %x\n", _last_error_code);

        free(request);
        free(buffer);
    }
    else
        log("[-] load file error: %x\n", _last_error_code);

    return false;
}

bool receive_file(_socket sk, const char* from, const char* to)
{
    _packet* response;
    _packet* request = construct_packet(from, _packet_file_download, g_client_id, 0);

    if (transmit_packet(sk, request))
        log("[+] packet sent successfully\n");
    else
        log("[-] packet sent error: %x\n", _last_error_code);

    if (transmit_packet(sk, request) && receive_packet(sk, &response))
    {
        char* data;

        if (receive_decrypt_data(sk, &data, response->size))
        {
            save_file(data, response->size, to);
            free(data);
            free(response);
            free(request);
            return true;
        }

        free(response);
    }

    free(request);
    return false;
}

_socket socket_create()
{
    return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

void socket_setup(const char* ip, _sockaddr_in* sa, ushort port)
{
    sa->sin_family = AF_INET;
    sa->sin_port = htons(port);
    sa->sin_addr.s_addr = inet_addr(ip);
}

_connection socket_connect(_socket sk, _sockaddr_in* sa)
{
    int len = sizeof(*sa);
    return connect(sk, sa, &len);
}

int receive(const char* ip, ushort port, const char* from, const char* to)
{
    _socket sk = socket_create();
    if (sk == SOCKET_ERROR) return _last_error_code;

    _sockaddr_in sa;
    socket_setup(ip, &sa, port);

    _connection cn = socket_connect(sk, &sa);
    if (cn == CONNECTION_ERROR) return _last_error_code;

    if (receive_file(sk, from, to))
    {
        log("[+] file received successfully\n");
        return socket_close(sk);
    }

    int code = _last_error_code;
    return socket_close(sk), code;
}

int transmit(const char* ip, ushort port, const char* from, const char* to)
{
    _socket sk = socket_create();
    if (sk == SOCKET_ERROR) return _last_error_code;

    _sockaddr_in sa;
    socket_setup(ip, &sa, port);

    _connection cn = socket_connect(sk, &sa);
    if (cn == CONNECTION_ERROR) return _last_error_code;

    if (transmit_file(sk, from, to))
    {
        log("[+] file sent successfully\n");
        return socket_close(sk);
    }

    int code = _last_error_code;
    return socket_close(sk), code;
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    if (argc == 2)
    {
        _client_id client_id = strtoull(argv[1], 0, 10);
        rsa_generate_keys(&g_rsa_data);
        rsa_save_client_keys(&g_rsa_data, client_id);
        return 0;
    }

    if (argc != 7)
    {
        log("usage:\n");
        log("      client <client_id> <ip> <port> [r/t] <from> <to>\n");
        log("      or\n");
        log("      client <client_id>\n");
        return 0;
    }

    int code = network_startup();
    if (code) return _last_error_code;

    g_client_id   = strtoull(argv[1], 0, 10);
    if (!initialize_client_rsa(g_client_id)) return 1;

    const char* ip         = argv[2];
    ushort port            = strtoull(argv[3], 0, 10);
    const char* direction  = argv[4];
    const char* from       = argv[5];
    const char* to         = argv[6];

    if (direction[0] == 'r')
        code = receive(ip, port, from, to);
    else if (direction[0] == 't')
        code = transmit(ip, port, from, to);
    else
        log("[-] unknown operation\n");

    log("[?] exit code: %x\n", code);
    return network_shutdown();
}
#endif