#define CLIENT_DEBUG
#ifndef CLIENT_DEBUG

#include "includes.h"

#include "defines.h"
#include "common_utils.h"
#include "cryptography.h"

void transmit_file(_socket sk, _packet* packet)
{
    log("[+] client requested a file download\n");

    char* buffer;
    long size;

    if (load_file(&buffer, &size, packet->path))
    {
        _packet* temp = construct_packet(packet->path, _packet_file_download, packet->client_id, size);

        log("[+] file loaded: \n");
        log("    size: 0x%x bytes\n", size);
        log("    path: %s\n", packet->path);

        if (transmit_packet(sk, temp))
        {
            log("[+] packet sent successfully\n");

            if (load_keys_transmit_encrypt_data(sk, buffer, size, packet->client_id))
                log("[+] data sent successfully\n");
            else
                log("[-] data sent error: %x\n", _last_error_code);
        }
        else
            log("[-] packet sent error: %x\n", _last_error_code);


        free(temp);
        free(buffer);
    }
    else
        log("[-] load file error: %x\n", _last_error_code);
}

void receive_file(_socket sk, _packet* packet)
{
    log("[+] client requested a file upload\n", packet->path);

    char* data;

    if (load_keys_receive_decrypt_data(sk, &data, packet->size, packet->client_id))
    {
        log("[+] file received: \n");
        log("    size: 0x%x bytes\n", packet->size);
        log("    path: %s\n", packet->path);

        if (save_file(data, packet->size, packet->path))
            log("[+] data saved successfully\n");
        else
            log("[-] save file error: %x\n", _last_error_code);
    }
    else
        log("[-] receive data error: %x\n", _last_error_code);

    free(data);
}

void on_packet_received(_socket sk, _packet* packet)
{
    switch (packet->type)
    {
        case _packet_file_download:
            return transmit_file(sk, packet);
        case _packet_file_upload:
            return receive_file(sk, packet);
    }

    log("[!] client requested an unknown operation\n");
}

_socket socket_create()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

void socket_setup(_sockaddr_in* sa, ushort port)
{
    sa->sin_family = AF_INET;
    sa->sin_port = htons(port);
    sa->sin_addr.s_addr = INADDR_ANY;
}

int socket_bind(_socket sk, _sockaddr_in* sa)
{
    return bind(sk, sa, sizeof(*sa));
}

int socket_listen(_socket sk)
{
    return listen(sk, 4);
}

_socket socket_accept(_socket sk, _sockaddr_in* sa)
{
    int len = sizeof(*sa);
    return accept(sk, sa, &len);
}

void handle_client(_socket cl)
{
    log("[+] client %x connected!\n", cl);

    _packet* packet;

    if (receive_packet(cl, &packet))
    {
        log("[+] packet received successfully\n");
        on_packet_received(cl, packet);
        free(packet);
    }

    socket_close(cl);
    log("[+] client %x disconnected!\n\n", cl);
}

void process_packet_handling(_socket sk, _sockaddr_in* sa)
{
    while (true)
    {
        _socket cl = socket_accept(sk, sa);
        if (cl == SOCKET_ERROR) continue;

        thread_create(&handle_client, cl);
    }
}

int process_server(ushort port)
{
    _socket sk = socket_create();
    if (sk == SOCKET_ERROR) return _last_error_code;

    _sockaddr_in sa;
    socket_setup(&sa, port);

    if (socket_bind(sk, &sa) == SOCKET_ERROR)
        return _last_error_code;

    if (socket_listen(sk) == SOCKET_ERROR)
        return _last_error_code;

    process_packet_handling(sk, &sa);
    return socket_close(sk);
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    if (argc != 2)
    {
        log("usage:\n");
        log("      server <port>\n");
        return 0;
    }

    ushort port = strtoull(argv[1], 0, 10);

    int code = network_startup();
    if (code) return _last_error_code;

    int status = process_server(port);
    return network_shutdown(), status;
}
#endif