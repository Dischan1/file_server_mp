#include "rsa.h"

_rsa_data g_rsa_data;

bool receive_decrypt_data(_socket sk, char** data, size_t size)
{
    char* buffer = NULL;

    if (!receive_data(sk, &buffer, size * sizeof(int)))
    {
        log("[-] data receive error\n");
        return false;
    }

    *data = rsa_decrypt(&g_rsa_data, buffer, size);
    free(buffer);

    if (!*data)
    {
        log("[-] decrypt data error\n");
        return false;
    }

    log("[+] data decrypted successfully\n");
    return true;
}

bool transmit_encrypt_data(_socket sk, char* data, size_t size)
{
    int* buffer = rsa_encrypt(&g_rsa_data, data, size);
    char* dec = rsa_decrypt(&g_rsa_data, buffer, size);

    if (!buffer)
    {
        log("[-] encrypt data error\n");
        return false;
    }

    if (!transmit_data(sk, buffer, size * sizeof(int)))
    {
        log("[-] data transmit error\n");
        return false;
    }

    free(buffer);

    log("[+] data encrypted successfully\n");
    return true;
}

bool load_keys_receive_decrypt_data(_socket sk, char** data, size_t size, _client_id client_id)
{
    bool status = true;
    status &= status && rsa_load_client_keys(&g_rsa_data, client_id);

    if (!status)
    {
        log("[-] client keys not found\n");
        return false;
    }

    status &= status && receive_decrypt_data(sk, data, size);

    if (!status)
    {
        log("[-] data receive error\n");
        return false;
    }

    return status;
}

bool load_keys_transmit_encrypt_data(_socket sk, char* data, size_t size, _client_id client_id)
{
    bool status = true;
    status &= status && rsa_load_client_keys(&g_rsa_data, client_id);

    if (!status)
    {
        log("[-] client keys not found\n");
        return false;
    }

    status &= status && transmit_encrypt_data(sk, data, size);

    if (!status)
    {
        log("[-] data transmit error\n");
        return false;
    }

    return status;
}

bool initialize_client_rsa(_client_id client_id)
{
    if (!rsa_load_client_keys(&g_rsa_data, client_id))
    {
        log("[-] client keys not found\n");
        return false;
    }
    else
        log("[+] client keys loaded\n");

    return true;
}