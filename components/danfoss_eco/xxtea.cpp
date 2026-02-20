#include "xxtea.h"
#include <string.h>
#include <algorithm>

#define MX (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z))

namespace esphome {
namespace danfoss_eco {

void reverse_bytes_in_chunks(uint32_t *v, size_t count) {
    for (size_t i = 0; i < count; i++) {
        uint8_t *p = (uint8_t *)&v[i];
        std::swap(p[0], p[3]);
        std::swap(p[1], p[2]);
    }
}

void Xxtea::btea(uint32_t *v, int32_t n, uint32_t const k[4]) {
    uint32_t y, z, sum;
    uint32_t p, rounds, e;
    if (n > 1) {
        rounds = 6 + 52 / n;
        sum = 0;
        z = v[n - 1];
        do {
            sum += XXTEA_DELTA;
            e = (sum >> 2) & 3;
            for (p = 0; p < (uint32_t)n - 1; p++) {
                y = v[p + 1];
                z = v[p] += MX;
            }
            y = v[0];
            z = v[n - 1] += MX;
        } while (--rounds);
    } else if (n < -1) {
        n = -n;
        rounds = 6 + 52 / n;
        sum = rounds * XXTEA_DELTA;
        y = v[0];
        do {
            e = (sum >> 2) & 3;
            for (p = n - 1; p > 0; p--) {
                z = v[p - 1];
                y = v[p] -= MX;
            }
            z = v[n - 1];
            y = v[0] -= MX;
        } while ((sum -= XXTEA_DELTA) != 0);
    }
}

int Xxtea::set_key(uint8_t *key, size_t len) {
    if (key == nullptr || len != 16) return XXTEA_STATUS_PARAMETER_ERROR;
    memcpy(this->xxtea_key, key, 16);
    this->status_ = XXTEA_STATUS_SUCCESS;
    return XXTEA_STATUS_SUCCESS;
}

int Xxtea::encrypt(uint8_t *data, size_t len, uint8_t *buf, size_t *maxlen) {
    if (data == nullptr || len <= 0 || (len % 4) != 0) return XXTEA_STATUS_PARAMETER_ERROR;
    if (len > MAX_XXTEA_DATA8) return XXTEA_STATUS_SIZE_ERROR;
    memset(this->xxtea_data, 0, MAX_XXTEA_DATA8);
    memcpy(this->xxtea_data, data, len);
    reverse_bytes_in_chunks(this->xxtea_data, len / 4);
    btea(this->xxtea_data, (int32_t)(len / 4), this->xxtea_key);
    reverse_bytes_in_chunks(this->xxtea_data, len / 4);
    memcpy(buf, this->xxtea_data, len);
    *maxlen = len;
    return XXTEA_STATUS_SUCCESS;
}

int Xxtea::encrypt(uint8_t *data, size_t len, uint8_t *buf) {
    size_t ml = MAX_XXTEA_DATA8;
    return this->encrypt(data, len, buf, &ml);
}

int Xxtea::decrypt(uint8_t *data, size_t len) {
    if (data == nullptr || len <= 0 || (len % 4) != 0) return XXTEA_STATUS_PARAMETER_ERROR;
    if (len > MAX_XXTEA_DATA8) return XXTEA_STATUS_SIZE_ERROR;
    memset(this->xxtea_data, 0, MAX_XXTEA_DATA8);
    memcpy(this->xxtea_data, data, len);
    reverse_bytes_in_chunks(this->xxtea_data, len / 4);
    btea(this->xxtea_data, -(int32_t)(len / 4), this->xxtea_key);
    reverse_bytes_in_chunks(this->xxtea_data, len / 4);
    memcpy(data, this->xxtea_data, len);
    return XXTEA_STATUS_SUCCESS;
}

int Xxtea::decrypt(uint8_t *data, size_t len, uint8_t *buf) {
    memcpy(buf, data, len);
    return this->decrypt(buf, len);
}

} // namespace danfoss_eco
} // namespace esphome