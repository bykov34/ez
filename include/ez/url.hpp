
#pragma once

#include <inttypes.h>
#include <string>
#include <string_view>
#include <vector>

namespace ez::url {

size_t encode(const char* _src, size_t _src_len, char* _result);
std::string encode(const char* _src, size_t _src_len);
std::string encode(std::string_view _src);

size_t decode(std::string_view _src, char* _result);
std::string decode(std::string_view _src);

}
