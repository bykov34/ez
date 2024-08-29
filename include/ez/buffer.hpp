
#pragma once

#include <inttypes.h>
#include <string>
#include <string_view>
#include <vector>

namespace ez
{
    class buffer
    {
        public:
        
            buffer();
            ~buffer();
        
            buffer(const buffer& _right);
            const buffer& operator = (const buffer& _right);
        
            explicit buffer(size_t _size);
            buffer(const std::vector<uint8_t>&);
            buffer(const void* _data, size_t _size, bool _copy = true);
            buffer(const std::string& _str);
            buffer(const std::string_view _str);
            buffer(const char* _str);

            uint8_t* ptr() const;
            const char* c_str() const;
            size_t size() const;

            void set_size(size_t _size);
        
            size_t position() const;
            void set_position(size_t _pos);
        
        private:
        
            struct impl; struct impl* m_impl;
    };
}
