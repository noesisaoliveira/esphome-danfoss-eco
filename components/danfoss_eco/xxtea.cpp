#include "xxtea.h"
#include <string.h>
#include <algorithm>

#define MX (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z))

void Xxtea::btea(uint32_t *v, int32_t n, uint32_t const k[4])
{
    uint32_t y, z, sum;
    uint32_t p, rounds, e;

    if (n > 1)
    {
        rounds = 6 + 52 / n;
        sum = 0;
        z = v[n - 1];
        do
        {
            sum += XXTEA_DELTA;
            e = (sum >> 2) & 3;
            for (p = 0; p < (uint32_t)n - 1; p++)
            {
                y = v[p + 1];
                z = v[p] += MX;
            }
            y = v[0];
            z = v[n - 1] += MX;
        } while (--rounds);
    }
    else if (n < -1)
    {
        n = -n;
        rounds = 6 + 52 / n;
        sum = rounds * XXTEA_DELTA;
        y = v[0];
        do
        {
            e = (sum >> 2) & 3;
            for (p = n - 1; p > 0; p--)
            {
                z = v[p - 1];
                y = v[p] -= MX;
            }
            z = v[n - 1];
            y = v[0] -= MX;
        } while ((sum -= XXTEA_DELTA) != 0);
    }
}

int Xxtea::set_key(uint8_t *key, size_t len)
{
    if (key == NULL || len != 16) return XXTEA_STATUS_PARAMETER_ERROR;
    memcpy(this->xxtea_key, key, 16);
    this->status_ = XXTEA_STATUS_SUCCESS;
    return XXTEA_STATUS_SUCCESS;
}

int Xxtea::encrypt(uint8_t *data, size_t len, uint8_t *buf, size_t *maxlen)
{
    if (data == NULL || len <= 0 || (len % 4) != 0) return XXTEA_STATUS_PARAMETER_ERROR;

    memset((void *)this->xxtea_data, 0, MAX_XXTEA_DATA8);
    memcpy((void *)this->xxtea_data, (const void *)data, len);

    // Reverter Chunks antes de encriptar (Big Endian -> Little Endian)
    for (size_t i = 0; i < len / 4; i++) {
        uint8_t *p = (uint8_t *)&this->xxtea_data[i];
        std::swap(p[0], p[3]);
        std::swap(p[1], p[2]);
    }

    int32_t l = (int32_t)len / 4;
    btea(this->xxtea_data, l, this->xxtea_key);

    // Reverter Chunks depois de encriptar (Little Endian -> Big Endian)
    for (size_t i = 0; i < len / 4; i++) {
        uint8_t *p = (uint8_t *)&this->xxtea_data[i];
        std::swap(p[0], p[3]);
        std::swap(p[1], p[2]);
    }

    memcpy((void *)buf, (const void *)this->xxtea_data, len);
    *maxlen = len;
    return XXTEA_STATUS_SUCCESS;
}

int Xxtea::decrypt(uint8_t *data, size_t len)
{
    if (data == NULL || len <= 0 || (len % 4) != 0) return XXTEA_STATUS_PARAMETER_ERROR;
    
    memset((void *)this->xxtea_data, 0, MAX_XXTEA_DATA8);
    memcpy((void *)this->xxtea_data, (const void *)data, len);
    
    // 1. Reverter Chunks antes de desencriptar
    for (size_t i = 0; i < len / 4; i++) {
        uint8_t *p = (uint8_t *)&this->xxtea_data[i];
        std::swap(p[0], p[3]);
        std::swap(p[1], p[2]);
    }

    int32_t l = -((int32_t)len / 4);
    btea(this->xxtea_data, l, this->xxtea_key);
    
    // 2. Reverter Chunks depois de desencriptar
    for (size_t i = 0; i < len / 4; i++) {
        uint8_t *p = (uint8_t *)&this->xxtea_data[i];
        std::swap(p[0], p[3]);
        std::swap(p[1], p[2]);
    }
    
    memcpy((void *)data, (const void *)this->xxtea_data, len);
    return XXTEA_STATUS_SUCCESS;
}