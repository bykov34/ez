
#include <ez/base64.hpp>

static const char to_base64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const uint8_t from_base64[128] =
{
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255,  62, 255,  63,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,   0, 255, 255, 255,
    255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255,  63,
    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255
};

namespace ez::base64 {

size_t encoded_size(size_t _src_size)
{
    size_t ret_size = _src_size;
    while ((ret_size % 3) != 0)
        ++ret_size;

    return 4 * ret_size / 3;
}

void encode(const uint8_t* _src, size_t _src_size, char* _result)
{
    size_t missing = 0;
    size_t ret_size = _src_size;
    while ((ret_size % 3) != 0)
    {
        ++ret_size;
        ++missing;
    }

    ret_size = 4*ret_size/3;
    auto* buf = _result;

    for (size_t i = 0; i < ret_size/4; ++i)
    {
        const size_t index = i*3;
        const uint8_t b3_0 = (index+0 < _src_size) ? _src[index+0] : 0;
        const uint8_t b3_1 = (index+1 < _src_size) ? _src[index+1] : 0;
        const uint8_t b3_2 = (index+2 < _src_size) ? _src[index+2] : 0;

        const uint8_t b4_0 =                        ((b3_0 & 0xfc) >> 2);
        const uint8_t b4_1 = ((b3_0 & 0x03) << 4) + ((b3_1 & 0xf0) >> 4);
        const uint8_t b4_2 = ((b3_1 & 0x0f) << 2) + ((b3_2 & 0xc0) >> 6);
        const uint8_t b4_3 = ((b3_2 & 0x3f) << 0);

        *buf++ = (to_base64[b4_0]);
        *buf++ = (to_base64[b4_1]);
        *buf++ = (to_base64[b4_2]);
        *buf++ = (to_base64[b4_3]);
    }

    for (size_t i = 0; i < missing; ++i)
        _result[ret_size - i - 1] = '=';
}

std::string encode(const uint8_t* _src, size_t _src_size)
{
    std::string result;
    result.resize(encoded_size(_src_size));
    encode(_src, _src_size, result.data());
    return result;
}

std::string encode(const std::vector<uint8_t>& _src)
{
    return encode(_src.data(), _src.size());
}

void encode(const std::vector<uint8_t>& _src, char* _result)
{
    encode(_src.data(), _src.size(), _result);
}

std::string encode(std::string_view _src)
{
    return encode(reinterpret_cast<const uint8_t*>(_src.data()), _src.size());
}

// ---------------------------------------------------------------------------------------------------------------------------------

size_t decoded_size(size_t _src_size)
{
    while ((_src_size % 4) != 0)
        ++_src_size;
    
    return 3*_src_size/4;
}

size_t decode(std::string_view _src, uint8_t* _result)
{
    size_t encoded_size = _src.size();
    while ((encoded_size % 4) != 0)
        ++encoded_size;

    const size_t N = _src.size();
    auto* buf = _result;

    for (size_t i = 0; i < encoded_size; i += 4)
    {
        const uint8_t b4_0 = (          _src[i+0] <= 'z') ? from_base64[static_cast<uint8_t>(_src[i+0])] : 0xff;
        const uint8_t b4_1 = (i+1 < N && _src[i+1] <= 'z') ? from_base64[static_cast<uint8_t>(_src[i+1])] : 0xff;
        const uint8_t b4_2 = (i+2 < N && _src[i+2] <= 'z') ? from_base64[static_cast<uint8_t>(_src[i+2])] : 0xff;
        const uint8_t b4_3 = (i+3 < N && _src[i+3] <= 'z') ? from_base64[static_cast<uint8_t>(_src[i+3])] : 0xff;

        const uint8_t b3_0 = ((b4_0 & 0x3f) << 2) + ((b4_1 & 0x30) >> 4);
        const uint8_t b3_1 = ((b4_1 & 0x0f) << 4) + ((b4_2 & 0x3c) >> 2);
        const uint8_t b3_2 = ((b4_2 & 0x03) << 6) + ((b4_3 & 0x3f) >> 0);

        if (b4_1 != 0xff) *buf++ = b3_0;
        if (b4_2 != 0xff) *buf++ = b3_1;
        if (b4_3 != 0xff) *buf++ = b3_2;
    }
    
    return buf - _result;
}

std::vector<uint8_t> decode(std::string_view _src)
{
    std::vector<uint8_t> result;
    result.resize(decoded_size(_src.size()));
    auto size = decode(_src, result.data());
    result.resize(size);
    return result;
}












}
