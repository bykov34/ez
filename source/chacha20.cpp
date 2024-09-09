
#include <stdexcept>
#include <algorithm>

#include <ez/chacha20.hpp>
#include <ez/common.hpp>

namespace ez {

struct chacha20::impl
{
    unsigned rounds = 20;
    size_t pos = 0;
    
    uint32_t state[16];
    uint32_t block[16];
  
    void transform();
    void set_key(const uint8_t* _key, size_t _key_size);
    void set_iv(const uint8_t* _iv, size_t _iv_size);
    void crypt(const uint8_t* _input, size_t _size, uint8_t* _output);
};

// ------------------------------------------------------------------------------------------

chacha20::chacha20() : m_impl(new impl)
{
}

chacha20::~chacha20() // TODO: cleanup
{
    delete m_impl;
}

size_t chacha20::encrypted_size(size_t _in_size)
{
    return _in_size;
}

size_t chacha20::decrypted_size(size_t _in_size)
{
    return _in_size;
}

void chacha20::set_key(const uint8_t* _key, size_t _key_size)
{
    m_impl->set_key(_key, _key_size);
}

void chacha20::set_iv(const uint8_t* _iv, size_t _iv_size)
{
    m_impl->set_iv(_iv, _iv_size);
}

void chacha20::set_rounds(unsigned int _rounds)
{
    if (_rounds != 8 && _rounds != 12 && _rounds != 20)
        throw std::runtime_error("invalid number of rounds");
    
    m_impl->rounds = _rounds;
}

void chacha20::encrypt(const uint8_t *_input, size_t _in_size, uint8_t *_output)
{
    m_impl->crypt(_input, _in_size, _output);
}

void chacha20::decrypt(const uint8_t *_input, size_t _in_size, uint8_t *_output)
{
    m_impl->crypt(_input, _in_size, _output);
}

// ------------------------------------------------------------------------------------------

void chacha20::impl::set_key(const uint8_t *key, size_t _key_size)
{
    if (_key_size != 16 && _key_size != 32)
        throw std::runtime_error("invalid key size");
    
   if(_key_size == 16)
   {
      state[0] = 0x61707865;
      state[1] = 0x3120646E;
      state[2] = 0x79622D36;
      state[3] = 0x6B206574;

      state[4] = LOAD32LE(key);
      state[5] = LOAD32LE(key + 4);
      state[6] = LOAD32LE(key + 8);
      state[7] = LOAD32LE(key + 12);

      state[8] = LOAD32LE(key);
      state[9] = LOAD32LE(key + 4);
      state[10] = LOAD32LE(key + 8);
      state[11] = LOAD32LE(key + 12);
   }
   else // 32
   {
      state[0] = 0x61707865;
      state[1] = 0x3320646E;
      state[2] = 0x79622D32;
      state[3] = 0x6B206574;

      state[4] = LOAD32LE(key);
      state[5] = LOAD32LE(key + 4);
      state[6] = LOAD32LE(key + 8);
      state[7] = LOAD32LE(key + 12);
      state[8] = LOAD32LE(key + 16);
      state[9] = LOAD32LE(key + 20);
      state[10] = LOAD32LE(key + 24);
      state[11] = LOAD32LE(key + 28);
   }
}

// ------------------------------------------------------------------------------------------

void chacha20::impl::set_iv(const uint8_t* _iv, size_t _iv_size)
{
    if (_iv_size != 8 && _iv_size != 12)
        throw std::runtime_error("invalid iv size");

   if(_iv_size == 8)
   {
      state[12] = 0;
      state[13] = 0;
      state[14] = LOAD32LE(_iv);
      state[15] = LOAD32LE(_iv + 4);
   }
   else // 12
   {
      state[12] = 0;
      state[13] = LOAD32LE(_iv);
      state[14] = LOAD32LE(_iv + 4);
      state[15] = LOAD32LE(_iv + 8);
   }
}

// ------------------------------------------------------------------------------------------

void chacha20::impl::crypt(const uint8_t* _input, size_t _size, uint8_t* _output)
{
    while(_size > 0)
    {
        if(pos == 0 || pos >= 64)
        {
            transform();
            state[12]++;

            if(state[12] == 0)
                state[13]++;

            pos = 0;
        }

        auto n = std::min(_size, 64 - pos);

        if(_output != nullptr)
        {
            uint8_t* k = reinterpret_cast<uint8_t *>(block) + pos;
            if(_input != nullptr)
            {
                for (size_t i = 0; i < n; i++)
                    _output[i] = _input[i] ^ k[i];

                _input += n;
            }
            else
            {
                for (size_t i = 0; i < n; i++)
                    _output[i] = k[i];
            }

            _output += n;
        }

        pos += n;
        _size -= n;
    }
}

// ------------------------------------------------------------------------------------------

#define CHACHA_QUARTER_ROUND(a, b, c, d) \
{ \
   a += b; \
   d ^= a; \
   d = ROL32(d, 16); \
   c += d; \
   b ^= c; \
   b = ROL32(b, 12); \
   a += b; \
   d ^= a; \
   d = ROL32(d, 8); \
   c += d; \
   b ^= c; \
   b = ROL32(b, 7); \
}

void chacha20::impl::transform()
{
    uint32_t *w = block;
    
    for(auto i = 0; i < 16; i++)
        w[i] = state[i];

    for(unsigned i = 0; i < rounds; i += 2)
    {
        CHACHA_QUARTER_ROUND(w[0], w[4], w[8], w[12]);
        CHACHA_QUARTER_ROUND(w[1], w[5], w[9], w[13]);
        CHACHA_QUARTER_ROUND(w[2], w[6], w[10], w[14]);
        CHACHA_QUARTER_ROUND(w[3], w[7], w[11], w[15]);

        CHACHA_QUARTER_ROUND(w[0], w[5], w[10], w[15]);
        CHACHA_QUARTER_ROUND(w[1], w[6], w[11], w[12]);
        CHACHA_QUARTER_ROUND(w[2], w[7], w[8], w[13]);
        CHACHA_QUARTER_ROUND(w[3], w[4], w[9], w[14]);
    }

    for(auto i = 0; i < 16; i++)
        w[i] += state[i];

    //for(i = 0; i < 16; i++)
    //    w[i] = htole32(w[i]);
}

}
