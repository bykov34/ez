#pragma once

#include <inttypes.h>
#include <string>
#include <string_view>
#include <vector>

namespace ez::hex {

size_t encoded_size(size_t _src_size);

void encode(const uint8_t* _src, size_t _src_len, char* _result, bool upper = false);
void encode(const std::vector<uint8_t>& _src, char* _result, bool upper = false);
std::string encode(const uint8_t* _src, size_t _src_len, bool upper = false);
std::string encode(const std::vector<uint8_t>& _src, bool upper = false);

void decode(const char* _src, size_t _src_len, uint8_t* _result);
void decode(std::string_view _src, uint8_t* _result);
std::vector<uint8_t> decode(std::string_view _src);

}
