#include "rsa.h"

_rsa_data g_rsa_data;

bool receive_decrypt_data(_connection* con, char** data, size_t* size)
{
    char* buffer = NULL;

    if (!receive_data(con, &buffer, size))
    {
        dp_error("data receive error\n");
        return false;
    }

    *data = rsa_decrypt(&g_rsa_data, buffer, *size);
    free(buffer);

    if (!*data)
    {
        dp_error("decrypt data error\n");
        return false;
    }

    if (*size % sizeof(int) != 0u)
    {
        dp_error("chiper error\n");
        return free(*data), false;
    }

    *size /= sizeof(int);
    return true;
}

bool transmit_encrypt_data(_connection* con, char* data, size_t size)
{
    int* buffer = rsa_encrypt(&g_rsa_data, data, size);
    char* dec   = rsa_decrypt(&g_rsa_data, buffer, size);

    if (!buffer)
    {
        dp_error("encrypt data error\n");
        return false;
    }

    if (!transmit_data(con, buffer, size * sizeof(int)))
    {
        dp_error("data transmit error\n");
        return false;
    }

    free(buffer);
    return true;
}

bool load_keys_receive_decrypt_data(_connection* con, char** data, size_t* size, _client_id client_id)
{
    bool status = true;
    status &= status && rsa_load_client_keys(&g_rsa_data, client_id);

    if (!status)
    {
        dp_error("client keys not found\n");
        return false;
    }

    status &= status && receive_decrypt_data(con, data, size);

    if (!status)
    {
        dp_error("data receive error\n");
        return false;
    }

    return status;
}

bool load_keys_transmit_encrypt_data(_connection* con, char* data, size_t size, _client_id client_id)
{
    bool status = true;
    status &= status && rsa_load_client_keys(&g_rsa_data, client_id);

    if (!status)
    {
        dp_error("client keys not found\n");
        return false;
    }

    status &= status && transmit_encrypt_data(con, data, size);

    if (!status)
    {
        dp_error("data transmit error\n");
        return false;
    }

    return status;
}

bool initialize_client_rsa(_client_id client_id)
{
    if (!rsa_load_client_keys(&g_rsa_data, client_id))
    {
        dp_error("client keys not found\n");
        return false;
    }

    return true;
}