
#include <string.h>

#include <ez/sha2.hpp>
#include <ez/common.hpp>

namespace ez {

struct sha2_256::impl
{
    uint32_t m_digest[8];
    uint32_t m_buffer[16];
    size_t   m_size;
    uint64_t m_total_size;
    
    void update(const uint8_t* _data, size_t _size);
    void complete();
    void transform();
};

// ------------------------------------------------------------------------------------------

sha2_256::sha2_256() : m_impl(new impl)
{
    reset();
}

sha2_256::~sha2_256()
{
    // TODO: cleanup
    delete m_impl;
}

// ------------------------------------------------------------------------------------------

std::vector<uint8_t> sha2_256::get()
{
    std::vector<uint8_t> result(digest_size);
    copy_to(result.data());
    return result;
}

void sha2_256::copy_to(uint8_t* _result)
{
    memcpy(_result, m_impl->m_digest, digest_size);
}

bool sha2_256::compare_with(const uint8_t* _data) // TODO: replace with secure_compare
{
    return memcmp(m_impl->m_digest, _data, digest_size) == 0;
}

sha2_256& sha2_256::calculate(const uint8_t* _data, size_t _size)
{
    update(_data, _size);
    return complete();
}

sha2_256& sha2_256::calculate(const char* _data, size_t _size)
{
    return calculate(reinterpret_cast<const uint8_t*>(_data), _size);
}

void sha2_256::update(const uint8_t* _data, size_t _size)
{
    m_impl->update(_data, _size);
}

sha2_256& sha2_256::complete()
{
    m_impl->complete();
    return *this;
}

// ------------------------------------------------------------------------------------------

#define W(t) w[(t) & 0x0F]
#define CH(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define SIGMA1(x) (ROR32(x, 2) ^ ROR32(x, 13) ^ ROR32(x, 22))
#define SIGMA2(x) (ROR32(x, 6) ^ ROR32(x, 11) ^ ROR32(x, 25))
#define SIGMA3(x) (ROR32(x, 7) ^ ROR32(x, 18) ^ SHR32(x, 3))
#define SIGMA4(x) (ROR32(x, 17) ^ ROR32(x, 19) ^ SHR32(x, 10))

static const uint8_t padding[64] =
{
   0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint32_t k[64] =
{
   0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
   0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
   0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
   0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
   0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
   0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
   0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
   0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

// ------------------------------------------------------------------------------------------

void sha2_256::reset()
{
    m_impl->m_size = 0;
    m_impl->m_total_size = 0;
    m_impl->m_digest[0] = 0x6A09E667;
    m_impl->m_digest[1] = 0xBB67AE85;
    m_impl->m_digest[2] = 0x3C6EF372;
    m_impl->m_digest[3] = 0xA54FF53A;
    m_impl->m_digest[4] = 0x510E527F;
    m_impl->m_digest[5] = 0x9B05688C;
    m_impl->m_digest[6] = 0x1F83D9AB;
    m_impl->m_digest[7] = 0x5BE0CD19;
}

// ------------------------------------------------------------------------------------------

void sha2_256::impl::transform()
{
    uint32_t a = m_digest[0];
    uint32_t b = m_digest[1];
    uint32_t c = m_digest[2];
    uint32_t d = m_digest[3];
    uint32_t e = m_digest[4];
    uint32_t f = m_digest[5];
    uint32_t g = m_digest[6];
    uint32_t h = m_digest[7];
    uint32_t *w = m_buffer;

    for(unsigned t = 0; t < 16; t++)
        w[t] = SWAPINT32(w[t]);

    for(unsigned t = 0; t < 64; t++)
    {
        if(t >= 16)
            W(t) += SIGMA4(W(t + 14)) + W(t + 9) + SIGMA3(W(t + 1));

        uint32_t temp1 = h + SIGMA2(e) + CH(e, f, g) + k[t] + W(t);
        uint32_t temp2 = SIGMA1(a) + MAJ(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    m_digest[0] += a;
    m_digest[1] += b;
    m_digest[2] += c;
    m_digest[3] += d;
    m_digest[4] += e;
    m_digest[5] += f;
    m_digest[6] += g;
    m_digest[7] += h;
}

// ------------------------------------------------------------------------------------------

void sha2_256::impl::update(const uint8_t* _data, size_t _size)
{
    auto* p = reinterpret_cast<uint8_t*>(m_buffer);
    while(_size > 0)
    {
        size_t n = std::min(_size, block_size - m_size);
        memcpy(p + m_size, _data, n);

        m_size += n;
        m_total_size += n;
        _data = (_data + n);
        _size -= n;

        if(m_size == block_size)
        {
            transform();
            m_size = 0;
        }
    }
}

// ------------------------------------------------------------------------------------------

void sha2_256::impl::complete()
{
    size_t padding_size = 64 + 56 - m_size;
    uint64_t total_size = m_total_size * 8;

    if(m_size < 56)
        padding_size = 56 - m_size;

    update(padding, padding_size);

    m_buffer[14] = SWAPINT32((uint32_t) (total_size >> 32));
    m_buffer[15] = SWAPINT32((uint32_t) total_size);

    transform();

    for(unsigned i = 0; i < 8; i++)
        m_digest[i] = SWAPINT32(m_digest[i]);
}





}
