
#pragma once

#include <inttypes.h>
#include <string>
#include <string_view>
#include <vector>

namespace ez::base64 {

size_t encoded_size(size_t _src_size);
size_t decoded_size(size_t _src_size);

void encode(const uint8_t* _src, size_t _src_len, char* _result);
void encode(const std::vector<uint8_t>& _src, char* _result);
std::string encode(const uint8_t* _src, size_t _src_len);
std::string encode(const std::vector<uint8_t>& _src);
std::string encode(std::string_view _src);

size_t decode(std::string_view _src, uint8_t* _result);
std::vector<uint8_t> decode(std::string_view _src);

}
