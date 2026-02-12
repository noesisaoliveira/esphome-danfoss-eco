#pragma once

#include <stdint.h>
#include <stddef.h>

// key Size is always fixed
#define MAX_XXTEA_KEY8 16
// 32 Bit
#define MAX_XXTEA_KEY32 4
// DWORD Size of Data Buffer
#define MAX_XXTEA_DATA8 64
#define MAX_XXTEA_DATA32 (MAX_XXTEA_DATA8 / 4)

#define XXTEA_STATUS_NOT_INITIALIZED -1
#define XXTEA_STATUS_SUCCESS 0
#define XXTEA_STATUS_GENERAL_ERROR -2
#define XXTEA_STATUS_PARAMETER_ERROR -3
#define XXTEA_STATUS_SIZE_ERROR -4

#define XXTEA_DELTA 0x9E3779B9

class Xxtea
{
public:
    Xxtea() : status_(XXTEA_STATUS_NOT_INITIALIZED){};

    int set_key(uint8_t *key, size_t len);

    int encrypt(uint8_t *data, size_t len, uint8_t *buf, size_t *maxlen);
    int encrypt(uint8_t *data, size_t len, uint8_t *buf);  // Overload for device_data.h
    int decrypt(uint8_t *data, size_t len);
    int decrypt(uint8_t *data, size_t len, uint8_t *buf);  // Overload for device_data.h

    int status() { return this->status_; }

private:
    void btea(uint32_t *v, int32_t n, uint32_t const k[4]);
    
    int status_;
    uint32_t xxtea_data[MAX_XXTEA_DATA32];
    uint32_t xxtea_key[MAX_XXTEA_KEY32];
};