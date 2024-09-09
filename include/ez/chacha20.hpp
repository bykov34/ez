
#pragma once

#include <cstdint>
#include <stddef.h>

namespace ez {

class chacha20 final
{
    public:
    
        inline static const size_t key_size = 16;
        inline static const size_t block_size = 8;
    
        chacha20();
        ~chacha20();
    
        size_t encrypted_size(size_t _in_size);
        size_t decrypted_size(size_t _in_size);
    
        void set_key(const uint8_t* _key, size_t _key_size);
        void set_iv(const uint8_t* _iv, size_t _iv_size);
        void set_rounds(unsigned _rounds);
    
        void encrypt(const uint8_t* _input, size_t _in_size, uint8_t* _output);
        void decrypt(const uint8_t* _input, size_t _in_size, uint8_t* _output);
    
    private:
        
        struct impl; impl* m_impl;
    
};








}
