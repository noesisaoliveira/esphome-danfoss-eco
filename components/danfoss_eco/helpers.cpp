#include "helpers.h"
#include "esphome/core/log.h"
#include <cstdio>
#include <cstring>

namespace esphome {
namespace danfoss_eco {

static uint8_t hex_to_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

void parse_hex_str(const char *data, size_t str_len, uint8_t *buff) {
  for (size_t i = 0; i < str_len / 2; i++) {
    uint8_t msb = hex_to_nibble(data[i * 2]);
    uint8_t lsb = hex_to_nibble(data[i * 2 + 1]);
    buff[i] = (msb << 4) | lsb;
  }
}

void encode_hex(const uint8_t *data, size_t len, char *buff) {
  for (size_t i = 0; i < len; i++) {
    sprintf(buff + i * 2, "%02x", data[i]);
  }
  buff[len * 2] = '\0';
}

uint32_t parse_int(const uint8_t *data, int start_pos) {
  return (uint32_t)data[start_pos] | (uint32_t)data[start_pos + 1] << 8 | 
         (uint32_t)data[start_pos + 2] << 16 | (uint32_t)data[start_pos + 3] << 24;
}

uint16_t parse_short(const uint8_t *data, int start_pos) {
  return (uint16_t)data[start_pos] | (uint16_t)data[start_pos + 1] << 8;
}

void write_int(uint8_t *data, int start_pos, int value) {
  data[start_pos] = value & 0xFF;
  data[start_pos + 1] = (value >> 8) & 0xFF;
  data[start_pos + 2] = (value >> 16) & 0xFF;
  data[start_pos + 3] = (value >> 24) & 0xFF;
}

bool parse_bit(uint8_t data, int pos) { return (data >> pos) & 1; }
bool parse_bit(uint16_t data, int pos) { return (data >> pos) & 1; }

void set_bit(uint8_t &data, int pos, bool value) {
  if (value) data |= (1 << pos);
  else data &= ~(1 << pos);
}

void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buff) {
  for (int i = 0; i < len / 4; i++) {
    reversed_buff[i * 4] = data[i * 4 + 3];
    reversed_buff[i * 4 + 1] = data[i * 4 + 2];
    reversed_buff[i * 4 + 2] = data[i * 4 + 1];
    reversed_buff[i * 4 + 3] = data[i * 4];
  }
}

void copy_address(uint64_t mac, esp_bd_addr_t bd_addr) {
  for (int i = 0; i < 6; i++) {
    bd_addr[i] = (mac >> (40 - i * 8)) & 0xFF;
  }
}

} // namespace danfoss_eco
} // namespace esphome