#include "xxtea.h"
#include <string.h>

#define MX (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z))

// XXTEA block tea algorithm (Corrected Block TEA)
void Xxtea::btea(uint32_t *v, int32_t n, uint32_t const k[4])
{
    uint32_t y, z, sum;
    uint32_t p, rounds, e;

    if (n > 1)
    {
        // Encryption
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
        // Decryption
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
            sum -= XXTEA_DELTA;
        } while (--rounds);
    }
}

int Xxtea::set_key(uint8_t *key, size_t len)
{
    this->status_ = XXTEA_STATUS_GENERAL_ERROR;

    // Parameter Check
    if (key == NULL || len <= 0 || len > MAX_XXTEA_KEY8)
    {
        this->status_ = XXTEA_STATUS_PARAMETER_ERROR;
        return this->status_;
    }

    size_t osz = (len + 3) / 4;  // Calculate 32-bit words needed

    // Check for Size Errors
    if (osz > MAX_XXTEA_KEY32)
    {
        this->status_ = XXTEA_STATUS_SIZE_ERROR;
        return this->status_;
    }

    // Clear the Key
    memset((void *)this->xxtea_key, 0, MAX_XXTEA_KEY8);

    // Copy the Key from Buffer
    memcpy((void *)this->xxtea_key, (const void *)key, len);

    // We have Success
    this->status_ = XXTEA_STATUS_SUCCESS;

    return this->status_;
}

int Xxtea::encrypt(uint8_t *data, size_t len, uint8_t *buf, size_t *maxlen)
{
    if (data == NULL || len <= 0 || len > MAX_XXTEA_DATA8 ||
        buf == NULL || maxlen == NULL || *maxlen <= 0 || *maxlen < len)
    {
        return XXTEA_STATUS_PARAMETER_ERROR;
    }
    
    // Calculate the Length needed for the 32bit Buffer
    int32_t l = (len + 3) / 4;

    // Check if More than expected space is needed
    if (l > MAX_XXTEA_DATA32 || *maxlen < (size_t)(l * 4))
    {
        return XXTEA_STATUS_SIZE_ERROR;
    }

    // Clear the Data
    memset((void *)this->xxtea_data, 0, MAX_XXTEA_DATA8);

    // Copy the Data from Buffer
    memcpy((void *)this->xxtea_data, (const void *)data, len);

    // Perform Encryption
    btea(this->xxtea_data, l, this->xxtea_key);

    // Copy Encrypted Data back to buffer
    memcpy((void *)buf, (const void *)this->xxtea_data, (l * 4));

    // Assign the Length
    *maxlen = l * 4;

    return XXTEA_STATUS_SUCCESS;
}

int Xxtea::decrypt(uint8_t *data, size_t len)
{
    if (data == NULL || len <= 0 || (len % 4) != 0)
    {
        return XXTEA_STATUS_PARAMETER_ERROR;
    }
    
    if (len > MAX_XXTEA_DATA8)
    {
        return XXTEA_STATUS_SIZE_ERROR;
    }
    
    // Copy the Data into Processing Array
    memset((void *)this->xxtea_data, 0, MAX_XXTEA_DATA8);
    memcpy((void *)this->xxtea_data, (const void *)data, len);
    
    // Get the Actual Size in 32bits - Negative for Decryption
    int32_t l = -((int32_t)len / 4);
    
    // Perform Decryption
    btea(this->xxtea_data, l, this->xxtea_key);
    
    // Copy Decrypted Data back to buffer
    memcpy((void *)data, (const void *)this->xxtea_data, len);
    
    return XXTEA_STATUS_SUCCESS;
}