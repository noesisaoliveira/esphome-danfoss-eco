#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "helpers.h"

namespace esphome {
namespace danfoss_eco {

void encode_hex(const uint8_t *data, size_t len, char *buff) {
    for (size_t i = 0; i < len; i++)
        sprintf(buff + (i * 2), "%02x", data[i]);
}

void parse_hex_str(const char *data, size_t str_len, uint8_t *buff) {
    for (size_t i = 0; i < str_len / 2; i++) {
        char msb = data[i * 2];
        char lsb = data[i * 2 + 1];
        auto m_val = parse_hex(msb);
        auto l_val = parse_hex(lsb);
        buff[i] = (m_val.value_or(0) << 4) | l_val.value_or(0);
    }
}

uint32_t parse_int(uint8_t *data, int start_pos) {
    return (uint32_t)data[start_pos] << 24 | (uint32_t)data[start_pos + 1] << 16 | 
           (uint32_t)data[start_pos + 2] << 8 | (uint32_t)data[start_pos + 3];
}

uint16_t parse_short(uint8_t *data, int start_pos) {
    return (uint16_t)data[start_pos] << 8 | (uint16_t)data[start_pos + 1];
}

void write_int(uint8_t *data, int start_pos, int value) {
    data[start_pos] = (value >> 24) & 0xFF;
    data[start_pos + 1] = (value >> 16) & 0xFF;
    data[start_pos + 2] = (value >> 8) & 0xFF;
    data[start_pos + 3] = value & 0xFF;
}

bool parse_bit(uint8_t data, int pos) { return (data >> pos) & 1; }
bool parse_bit(uint16_t data, int pos) { return (data >> pos) & 1; }

void set_bit(uint8_t &data, int pos, bool value) {
    if (value) data |= (1U << pos);
    else data &= ~(1U << pos);
}

void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buff) {
    for (int i = 0; i < len; i += 4) {
        int chunk_size = std::min(4, len - i);
        for (int j = 0; j < chunk_size; j++) {
            reversed_buff[i + j] = data[i + (chunk_size - 1 - j)];
        }
    }
}

uint8_t *encrypt(std::shared_ptr<Xxtea> &xxtea, uint8_t *value, uint16_t value_len) {
    uint8_t buffer[value_len], enc_buff[value_len];
    reverse_chunks(value, value_len, buffer);
    size_t out_len = value_len;
    if (xxtea->encrypt(buffer, value_len, enc_buff, &out_len) == XXTEA_STATUS_SUCCESS) {
        reverse_chunks(enc_buff, value_len, value);
    }
    return value;
}

uint8_t danfoss_char_to_hex(char chr) {
    if (chr >= '0' && chr <= '9') return chr - '0';
    if (chr >= 'A' && chr <= 'F') return chr - 'A' + 10;
    if (chr >= 'a' && chr <= 'f') return chr - 'a' + 10;
    return 0;
}

void parse_hex_str(const char *data, size_t str_len, uint8_t *buff) {
    for (size_t i = 0; i < str_len / 2; i++) {
        uint8_t msb = danfoss_char_to_hex(data[i * 2]);
        uint8_t lsb = danfoss_char_to_hex(data[i * 2 + 1]);
        buff[i] = (msb << 4) | lsb;
    }
}

} // namespace danfoss_eco
} // namespace esphome