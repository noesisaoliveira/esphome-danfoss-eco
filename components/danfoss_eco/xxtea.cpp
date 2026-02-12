#include "xxtea.h"
#include <xxtea-iot-crypt.h>
#include <string.h>

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
    int ret = XXTEA_STATUS_GENERAL_ERROR;
    
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
    xxtea_encrypt(this->xxtea_data, l, (const uint32_t *)this->xxtea_key);

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
    xxtea_decrypt(this->xxtea_data, l, (const uint32_t *)this->xxtea_key);
    
    // Copy Decrypted Data back to buffer
    memcpy((void *)data, (const void *)this->xxtea_data, len);
    
    return XXTEA_STATUS_SUCCESS;
}