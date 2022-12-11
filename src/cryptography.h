#include "rsa.h"

char* decrypt_data(_connection* con, char* data, size_t size)
{
    if (size % sizeof(int) != 0u)
    {
        dp_error("chiper error\n");
        return NULL;
    }

    return rsa_decrypt(&con->key.priv, (int*)data, (int)(size / sizeof(int)));
}

char* encrypt_data(_connection* con, char* data, size_t size)
{
    return (char*)rsa_encrypt(&con->key.pub, data, (int)size);
}