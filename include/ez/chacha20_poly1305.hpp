
#pragma once

#include <cstdint>
#include <stddef.h>

namespace ez {

class chacha20_poly1305 final
{
    public:
    
        inline static const size_t key_size = 16;
        inline static const size_t block_size = 8;
    
        chacha20_poly1305();
        ~chacha20_poly1305();
    
        size_t encrypted_size(size_t _in_size);
        size_t decrypted_size(size_t _in_size);
    
        void set_key(const uint8_t* _key, size_t _key_size);
        void set_iv(const uint8_t* _iv, size_t _iv_size);
        void set_aad(const uint8_t* _aad, size_t _aad_size);
        void set_tag(const uint8_t* _tag, size_t _tag_size);
    
        void get_tag(uint8_t* _tag);
    
        void encrypt(const uint8_t* _input, size_t _in_size, uint8_t* _output);
        bool decrypt(const uint8_t* _input, size_t _in_size, uint8_t* _output);
    
    private:
        
        struct impl; impl* m_impl;
};


}
