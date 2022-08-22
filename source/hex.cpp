
#include <ez/hex.hpp>
#include <stdexcept>

namespace ez::hex {

size_t encoded_size(size_t _src_size)
{
    return _src_size * 2;
}

void encode(const uint8_t* _src, size_t _src_len, char* _result, bool upper)
{
    char* bfr = _result;
    const uint8_t *d = _src;
    const char hex[] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
    const char hex_up[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

    for (size_t i = 0; i < _src_len; ++i)
    {
        if (upper)
        {
            *bfr++ = hex_up[(*d & 0xf0) >> 4];
            *bfr++ = hex_up[*d & 0x0f];
        }
        else
        {
            *bfr++ = hex[(*d & 0xf0) >> 4];
            *bfr++ = hex[*d & 0x0f];
        }
        d++;
    }
}

std::string encode(const uint8_t* _src, size_t _src_len, bool upper)
{
    std::string result;
    result.resize(_src_len * 2);
    encode(_src, _src_len, result.data(), upper);
    return result;
}

std::string encode(const std::vector<uint8_t>& _src, bool upper)
{
    return encode(_src.data(), _src.size(), upper);
}

void encode(const std::vector<uint8_t>& _src, char* _result, bool upper)
{
    encode(_src.data(), _src.size(), _result, upper);
}

// ---------------------------------------------------------------------------------------------------------------------------------

void decode(const char* _src, size_t _src_len, uint8_t* _result)
{
    if (_src_len % 2 != 0)
        throw std::runtime_error("hex: invalid source");
    
    auto* bfr = _result;

    for (size_t i = 0; i < _src_len; i+=2)
    {
        char s1 = *_src;
        char s2 = *(_src+1);
        
        if ('0' <= s1 && s1 <= '9') *bfr = (s1 - '0') << 4;
        else if ('a' <= s1 && s1 <= 'f') *bfr = (s1 - 'a' + 10) << 4;
        else if ('A' <= s1 && s1 <= 'F') *bfr = (s1 - 'A' + 10) << 4;
        
        if ('0' <= s2 && s2 <= '9') *bfr |= (s2 - '0');
        else if ('a' <= s2 && s2 <= 'f') *bfr |= (s2 - 'a' + 10);
        else if ('A' <= s2 && s2 <= 'F') *bfr |= (s2 - 'A' + 10);

        _src+=2; bfr++;
    }
}

void decode(std::string_view _src, uint8_t* _result)
{
    decode(_src.data(), _src.size(), _result);
}

std::vector<uint8_t> decode(std::string_view _src)
{
    if (_src.size() % 2 != 0)
        throw std::runtime_error("hex: invalid source");
    
    std::vector<uint8_t> result(_src.size()/2);
    decode(_src, result.data());
    return result;
}

}

