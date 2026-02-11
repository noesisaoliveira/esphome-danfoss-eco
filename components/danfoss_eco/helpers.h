#pragma once

#include "esphome/core/helpers.h"
#include <esp_gattc_api.h>

namespace esphome {
namespace danfoss_eco {

void parse_hex_str(const char *data, size_t str_len, uint8_t *buff);
void encode_hex(const uint8_t *data, size_t len, char *buff);

uint32_t parse_int(const uint8_t *data, int start_pos);
uint16_t parse_short(const uint8_t *data, int start_pos);
void write_int(uint8_t *data, int start_pos, int value);

bool parse_bit(uint8_t data, int pos);
bool parse_bit(uint16_t data, int pos);
void set_bit(uint8_t &data, int pos, bool value);

void reverse_chunks(uint8_t *data, int len, uint8_t *reversed_buff);
void copy_address(uint64_t mac, esp_bd_addr_t bd_addr);

} // namespace danfoss_eco
} // namespace esphome