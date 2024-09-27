
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
            
            virtual ssize_t send(const buffer&) = 0;
            virtual ssize_t send(const uint8_t* _data, size_t _size) = 0;
            virtual ssize_t recv(buffer& _buffer, size_t _desired_size = 0) = 0;
            virtual ssize_t recv(uint8_t* _data, size_t _size, size_t _desired_size = 0) = 0;
        
            virtual bool can_read() const = 0;
            virtual bool is_nonblocking() const = 0;
    };
}
