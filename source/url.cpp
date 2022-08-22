
#include <ez/url.hpp>
#include <ez/hex.hpp>
#include <stdexcept>

namespace ez::url {

size_t encode(const char* _src, size_t _src_len, char* _result)
{
    size_t size = 0;
    char* buf = _result;
    
    for (size_t i = 0; i < _src_len; ++i)
    {
        char c = *(_src+i);
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
            c == '*' || c == '\'' || c == '(' || c == ')')
        {
            *buf++ = c;
            size++;
        }
        else if (c == ' ')
        {
            *buf++ = '+';
            size++;
        }
        else
        {
            *buf++ = '%';
            hex::encode(reinterpret_cast<const uint8_t*>(_src+i), 1, buf++, true);
            buf++;
            size+=3;
        }
    }
    
    return size;
}

std::string encode(const char* _src, size_t _src_len)
{
    std::string result;
    result.resize(_src_len * 3);
    result.resize(encode(_src, _src_len, result.data()));
    return result;
}

std::string encode(std::string_view _src)
{
    return encode(_src.data(), _src.size());
}

// ---------------------------------------------------------------------------------------------------------------------------------

size_t decode(std::string_view _src, char* _result)
{
    size_t size = 0;
    char* buf = _result;
    const char* data = _src.data();
    
    for (size_t i = 0; i < _src.size(); ++i)
    {
        char c = *(data+i);
        
        if (c == '%')
        {
            hex::decode(data+i+1, 2, reinterpret_cast<uint8_t*>(buf++));
            i+=2;
        }
        else if (c == '+') *buf++ = ' ';
        else *buf++ = c;
        
        size++;
    }
    
    return size;
}

std::string decode(std::string_view _src)
{
    std::string result;
    result.resize(_src.size());
    result.resize(decode(_src, result.data()));
    return result;
}

}

