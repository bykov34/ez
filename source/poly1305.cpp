
#include <ez/poly1305.hpp>
#include <ez/hmac.hpp>
#include <ez/common.hpp>

namespace ez {

struct poly1305::impl
{
    uint32_t    m_r[4];
    uint32_t    m_s[4];
    uint64_t    m_a[8];
    uint8_t     m_buffer[17];
    uint32_t    m_digest[4];
    size_t      m_size;
    
    void set_key(const uint8_t* _key, size_t _key_size);
    void update(const uint8_t* _data, size_t _size);
    void complete();
    void transform();
};

// ------------------------------------------------------------------------------------------

poly1305::poly1305() : m_impl(new impl)
{
}

poly1305::~poly1305() // TODO: cleanup
{
    delete m_impl;
}

// ------------------------------------------------------------------------------------------

std::vector<uint8_t> poly1305::get()
{
    std::vector<uint8_t> result(digest_size);
    copy_to(result.data());
    return result;
}

void poly1305::copy_to(uint8_t* _result)
{
    memcpy(_result, m_impl->m_digest, digest_size);
}

bool poly1305::compare_with(const uint8_t* _data) // TODO: replace with secure_compare
{
    return memcmp(m_impl->m_digest, _data, digest_size);
}

poly1305& poly1305::calculate(const uint8_t* _data, size_t _size)
{
    update(_data, _size);
    return complete();
}

poly1305& poly1305::calculate(const char* _data, size_t _size)
{
    return calculate(reinterpret_cast<const uint8_t*>(_data), _size);
}

void poly1305::update(const uint8_t* _data, size_t _size)
{
    m_impl->update(_data, _size);
}

poly1305& poly1305::complete()
{
    m_impl->complete();
    return *this;
}

// ------------------------------------------------------------------------------------------

void poly1305::set_key(const uint8_t* _key, size_t _key_size)
{
    m_impl->m_r[0] = LOAD32LE(_key);
    m_impl->m_r[1] = LOAD32LE(_key + 4);
    m_impl->m_r[2] = LOAD32LE(_key + 8);
    m_impl->m_r[3] = LOAD32LE(_key + 12);
    m_impl->m_s[0] = LOAD32LE(_key + 16);
    m_impl->m_s[1] = LOAD32LE(_key + 20);
    m_impl->m_s[2] = LOAD32LE(_key + 24);
    m_impl->m_s[3] = LOAD32LE(_key + 28);
    
    m_impl->m_r[0] &= 0x0FFFFFFF;
    m_impl->m_r[1] &= 0x0FFFFFFC;
    m_impl->m_r[2] &= 0x0FFFFFFC;
    m_impl->m_r[3] &= 0x0FFFFFFC;
    
    m_impl->m_a[0] = 0;
    m_impl->m_a[1] = 0;
    m_impl->m_a[2] = 0;
    m_impl->m_a[3] = 0;
    m_impl->m_a[4] = 0;
    m_impl->m_a[5] = 0;
    m_impl->m_a[6] = 0;
    m_impl->m_a[7] = 0;
    m_impl->m_size = 0;
}

// ------------------------------------------------------------------------------------------

void poly1305::impl::update(const uint8_t* _data, size_t _size)
{
    auto* p = reinterpret_cast<uint8_t*>(m_buffer);
    while(_size > 0)
    {
        size_t n = std::min(_size, block_size - m_size);
        memcpy(p + m_size, _data, n);

        m_size += n;
        _data += n;
        _size -= n;

        if(m_size == block_size)
        {
            transform();
            m_size = 0;
        }
    }
}

// ------------------------------------------------------------------------------------------

void poly1305::impl::transform()
{
    uint32_t a[5];
    uint32_t r[4];
    auto n = m_size;

    //Add one bit beyond the number of octets. For a 16-byte block,
    //this is equivalent to adding 2^128 to the number. For the shorter
    //block, it can be 2^120, 2^112, or any power of two that is evenly
    //divisible by 8, all the way down to 2^8
    m_buffer[n++] = 0x01;

    //If the resulting block is not 17 bytes long (the last block),
    //pad it with zeros
    while(n < 17)
        m_buffer[n++] = 0x00;

    //Read the block as a little-endian number
    a[0] = LOAD32LE(m_buffer);
    a[1] = LOAD32LE(m_buffer + 4);
    a[2] = LOAD32LE(m_buffer + 8);
    a[3] = LOAD32LE(m_buffer + 12);
    a[4] = m_buffer[16];

    //Add this number to the accumulator
    m_a[0] += a[0];
    m_a[1] += a[1];
    m_a[2] += a[2];
    m_a[3] += a[3];
    m_a[4] += a[4];

    //Propagate the carry
    m_a[1] += m_a[0] >> 32;
    m_a[2] += m_a[1] >> 32;
    m_a[3] += m_a[2] >> 32;
    m_a[4] += m_a[3] >> 32;

    //We only consider the least significant bits
    a[0] = m_a[0] & 0xFFFFFFFF;
    a[1] = m_a[1] & 0xFFFFFFFF;
    a[2] = m_a[2] & 0xFFFFFFFF;
    a[3] = m_a[3] & 0xFFFFFFFF;
    a[4] = m_a[4] & 0xFFFFFFFF;

    //Copy r
    r[0] = m_r[0];
    r[1] = m_r[1];
    r[2] = m_r[2];
    r[3] = m_r[3];

    //Multiply the accumulator by r
    m_a[0] = (uint64_t) a[0] * r[0];
    m_a[1] = (uint64_t) a[0] * r[1] + (uint64_t) a[1] * r[0];
    m_a[2] = (uint64_t) a[0] * r[2] + (uint64_t) a[1] * r[1] + (uint64_t) a[2] * r[0];
    m_a[3] = (uint64_t) a[0] * r[3] + (uint64_t) a[1] * r[2] + (uint64_t) a[2] * r[1] + (uint64_t) a[3] * r[0];
    m_a[4] = (uint64_t) a[1] * r[3] + (uint64_t) a[2] * r[2] + (uint64_t) a[3] * r[1] + (uint64_t) a[4] * r[0];
    m_a[5] = (uint64_t) a[2] * r[3] + (uint64_t) a[3] * r[2] + (uint64_t) a[4] * r[1];
    m_a[6] = (uint64_t) a[3] * r[3] + (uint64_t) a[4] * r[2];
    m_a[7] = (uint64_t) a[4] * r[3];

    //Propagate the carry
    m_a[1] += m_a[0] >> 32;
    m_a[2] += m_a[1] >> 32;
    m_a[3] += m_a[2] >> 32;
    m_a[4] += m_a[3] >> 32;
    m_a[5] += m_a[4] >> 32;
    m_a[6] += m_a[5] >> 32;
    m_a[7] += m_a[6] >> 32;

    //Save the high part of the accumulator
    a[0] = m_a[4] & 0xFFFFFFFC;
    a[1] = m_a[5] & 0xFFFFFFFF;
    a[2] = m_a[6] & 0xFFFFFFFF;
    a[3] = m_a[7] & 0xFFFFFFFF;

    //We only consider the least significant bits
    m_a[0] &= 0xFFFFFFFF;
    m_a[1] &= 0xFFFFFFFF;
    m_a[2] &= 0xFFFFFFFF;
    m_a[3] &= 0xFFFFFFFF;
    m_a[4] &= 0x00000003;

    //Perform fast modular reduction (first pass)
    m_a[0] += a[0];
    m_a[0] += (a[0] >> 2) | (a[1] << 30);
    m_a[1] += a[1];
    m_a[1] += (a[1] >> 2) | (a[2] << 30);
    m_a[2] += a[2];
    m_a[2] += (a[2] >> 2) | (a[3] << 30);
    m_a[3] += a[3];
    m_a[3] += (a[3] >> 2);

    //Propagate the carry
    m_a[1] += m_a[0] >> 32;
    m_a[2] += m_a[1] >> 32;
    m_a[3] += m_a[2] >> 32;
    m_a[4] += m_a[3] >> 32;

    //Save the high part of the accumulator
    a[0] = m_a[4] & 0xFFFFFFFC;

    //We only consider the least significant bits
    m_a[0] &= 0xFFFFFFFF;
    m_a[1] &= 0xFFFFFFFF;
    m_a[2] &= 0xFFFFFFFF;
    m_a[3] &= 0xFFFFFFFF;
    m_a[4] &= 0x00000003;

    //Perform fast modular reduction (second pass)
    m_a[0] += a[0];
    m_a[0] += a[0] >> 2;

    //Propagate the carry
    m_a[1] += m_a[0] >> 32;
    m_a[2] += m_a[1] >> 32;
    m_a[3] += m_a[2] >> 32;
    m_a[4] += m_a[3] >> 32;

    //We only consider the least significant bits
    m_a[0] &= 0xFFFFFFFF;
    m_a[1] &= 0xFFFFFFFF;
    m_a[2] &= 0xFFFFFFFF;
    m_a[3] &= 0xFFFFFFFF;
    m_a[4] &= 0x00000003;
    
}

// ------------------------------------------------------------------------------------------

void poly1305::impl::complete()
{
    uint32_t mask;
    uint32_t b[4];

    if(m_size != 0)
        transform();

    //Save the accumulator
    b[0] = m_a[0] & 0xFFFFFFFF;
    b[1] = m_a[1] & 0xFFFFFFFF;
    b[2] = m_a[2] & 0xFFFFFFFF;
    b[3] = m_a[3] & 0xFFFFFFFF;

    //Compute a + 5
    m_a[0] += 5;

    //Propagate the carry
    m_a[1] += m_a[0] >> 32;
    m_a[2] += m_a[1] >> 32;
    m_a[3] += m_a[2] >> 32;
    m_a[4] += m_a[3] >> 32;

    //If (a + 5) >= 2^130, form a mask with the value 0x00000000. Else,
    //form a mask with the value 0xffffffff
    mask = ((m_a[4] & 0x04) >> 2) - 1;

    //Select between ((a - (2^130 - 5)) % 2^128) and (a % 2^128)
    m_a[0] = (m_a[0] & ~mask) | (b[0] & mask);
    m_a[1] = (m_a[1] & ~mask) | (b[1] & mask);
    m_a[2] = (m_a[2] & ~mask) | (b[2] & mask);
    m_a[3] = (m_a[3] & ~mask) | (b[3] & mask);

    //Finally, the value of the secret key s is added to the accumulator
    m_a[0] += m_s[0];
    m_a[1] += m_s[1];
    m_a[2] += m_s[2];
    m_a[3] += m_s[3];

    //Propagate the carry
    m_a[1] += m_a[0] >> 32;
    m_a[2] += m_a[1] >> 32;
    m_a[3] += m_a[2] >> 32;
    m_a[4] += m_a[3] >> 32;

    //We only consider the least significant bits
    b[0] = m_a[0] & 0xFFFFFFFF;
    b[1] = m_a[1] & 0xFFFFFFFF;
    b[2] = m_a[2] & 0xFFFFFFFF;
    b[3] = m_a[3] & 0xFFFFFFFF;

    //The result is serialized as a little-endian number, producing
    //the 16 byte tag
    STORE32LE(b[0], m_digest);
    STORE32LE(b[1], m_digest + 1);
    STORE32LE(b[2], m_digest + 2);
    STORE32LE(b[3], m_digest + 3);

    m_a[0] = 0;
    m_a[1] = 0;
    m_a[2] = 0;
    m_a[3] = 0;
    m_a[4] = 0;
    m_a[5] = 0;
    m_a[6] = 0;
    m_a[7] = 0;

    //Clear r and s
    m_r[0] = 0;
    m_r[1] = 0;
    m_r[2] = 0;
    m_r[3] = 0;
    m_s[0] = 0;
    m_s[1] = 0;
    m_s[2] = 0;
    m_s[3] = 0;
}

}
