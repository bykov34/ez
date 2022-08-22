
#pragma once

#include <stddef.h>
#include <stdexcept>

namespace ez
{
    class buffer;
    class channel
    {
        public:
        
            class error : public std::runtime_error
            {
                public: error(const char* _what): runtime_error(_what) {}
            };
        
            class timeout : public std::runtime_error
            {
                public: timeout(): runtime_error("timeout") {}
            };
            
            virtual size_t send(const buffer&) = 0;
            virtual size_t send(const uint8_t* _data, size_t _size) = 0;
            virtual size_t recv(buffer& _buff, size_t _desired_size = 0) = 0;
            virtual size_t recv(uint8_t* _data, size_t _size, size_t _desired_size = 0) = 0;
    };
}
