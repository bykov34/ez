
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace ez {

class poly1305 final
{
    public:
    
        inline static const size_t digest_size = 16;
        inline static const size_t block_size = 16;
    
        poly1305();
        ~poly1305();
    
        void set_key(const uint8_t* _key, size_t _key_size);
    
        poly1305& calculate(const uint8_t* _data, size_t _size);
        poly1305& calculate(const char* _data, size_t _size);
    
        void update(const uint8_t* _data, size_t _size);
        void update(const char* _data, size_t _size);
    
        poly1305& complete();
    
        std::vector<uint8_t> get();
        void copy_to(uint8_t* _result);
        bool compare_with(const uint8_t* _data);
    
    private:

        struct impl; impl* m_impl;
};

}
