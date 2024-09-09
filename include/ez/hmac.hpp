
#pragma once

#include <string.h>

#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5C

namespace ez {

template <class HashType>
class hmac
{
    using self_type = hmac<HashType>;
    HashType m_hash;
    uint8_t m_key[HashType::block_size];
    uint8_t m_digest[HashType::digest_size];
    
    public:
        
        hmac() = default;
    
        hmac(const uint8_t* _key, size_t _key_size)
        {
            set_key(_key, _key_size);
        }
    
        void set_key(const uint8_t* _key, size_t _key_size)
        {
            if(_key_size > HashType::block_size)
            {
                m_hash.calculate(_key, _key_size).copy_to(m_key);
                memset(m_key + HashType::digest_size, 0, HashType::block_size - HashType::digest_size);
            }
            else
            {
                memcpy(m_key, _key, _key_size);
                memset(m_key + _key_size, 0, HashType::block_size - _key_size);
            }

            for(unsigned i = 0; i < HashType::block_size; ++i)
                m_key[i] ^= HMAC_IPAD;

            m_hash.reset();
            m_hash.update(m_key, HashType::block_size);
        }
        
        void update(const uint8_t* _data, size_t _size)
        {
            m_hash.update(_data, _size);
        }
    
        self_type& complete()
        {
            m_hash.complete().copy_to(m_digest);

            for(unsigned i = 0; i < HashType::block_size; ++i)
                m_key[i] ^= HMAC_IPAD ^ HMAC_OPAD;

            m_hash.reset();
            m_hash.update(m_key, HashType::block_size);
            m_hash.update(m_digest, HashType::digest_size);
            m_hash.complete().copy_to(m_digest);
            
            return *this;
        }
    
        void copy_to(uint8_t* _result)
        {
            memcpy(_result, m_digest, HashType::digest_size);
        }
    
};




}
