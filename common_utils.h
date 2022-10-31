#pragma once

_packet* construct_packet(const char* path, _packet_type type, size_t size)
{
    size_t path_size = (strlen(path) + 1ull) * sizeof(*path);
    _packet* packet = malloc(sizeof(_packet));
    packet->type = type;
    packet->size = size;
    memcpy(packet->path, path, path_size);
    return packet;
}

bool receive_data(_socket sk, char** data, size_t size)
{
    return recv(sk, *data = malloc(size), size, 0) == size;
}

bool transmit_data(_socket sk, char* data, size_t size)
{
    return send(sk, data, size, 0) == size;
}

bool receive_packet(_socket sk, _packet** packet)
{
    return receive_data(sk, packet, sizeof(_packet));
}

bool transmit_packet(_socket sk, _packet* packet)
{
    return transmit_data(sk, packet, sizeof(_packet));
}

bool load_file(char** buffer, long* size, const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == INVALID_HANDLE_VALUE) return false;

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (*size > 0)
    {
        *buffer = malloc(*size);
        fread(*buffer, *size, 1, file);
    }

    return fclose(file), true;
}

bool save_file(char* buffer, long size, const char* path)
{
    FILE* file = fopen(path, "wb+");
    if (file == INVALID_HANDLE_VALUE) return false;

    fwrite(buffer, size, 1, file);
    return fclose(file), true;
}

int network_startup()
{
#if defined(_WIN32)
    WSADATA wsadata;
    return WSAStartup(MAKEWORD(2, 2), &wsadata);
#else
    return 0;
#endif
}

int network_shutdown()
{
#if defined(_WIN32)
    return WSACleanup();
#else
    return 0;
#endif
}