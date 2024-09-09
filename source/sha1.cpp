
#include <string.h>

#include <ez/sha1.hpp>
#include <ez/common.hpp>

namespace ez {

struct sha1::impl
{
    uint32_t m_digest[5];
    uint32_t m_buffer[16];
    size_t   m_size;
    uint64_t m_total_size;
    
    void update(const uint8_t* _data, size_t _size);
    void complete();
    void transform();
};

// ------------------------------------------------------------------------------------------

sha1::sha1() : m_impl(new impl)
{
    reset();
}
 
sha1::~sha1() // TODO: cleanup
{
    delete m_impl;
}

// ------------------------------------------------------------------------------------------

std::vector<uint8_t> sha1::get()
{
    std::vector<uint8_t> result(digest_size);
    copy_to(result.data());
    return result;
}

void sha1::copy_to(uint8_t* _result)
{
    memcpy(_result, m_impl->m_digest, digest_size);
}

bool sha1::compare_with(const uint8_t* _data) // TODO: replace with secure_compare
{
    return memcmp(m_impl->m_digest, _data, sha1::digest_size) == 0;
}

sha1& sha1::calculate(const uint8_t* _data, size_t _size)
{
    update(_data, _size);
    return complete();
}

sha1& sha1::calculate(const char* _data, size_t _size)
{
    return calculate(reinterpret_cast<const uint8_t*>(_data), _size);
}

void sha1::update(const uint8_t* _data, size_t _size)
{
    m_impl->update(_data, _size);
}

sha1& sha1::complete()
{
    m_impl->complete();
    return *this;
}

// ------------------------------------------------------------------------------------------

#define W(t) w[(t) & 0x0F]
#define CH(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define PARITY(x, y, z) ((x) ^ (y) ^ (z))
#define MAJ(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))

static const uint8_t padding[64] =
{
   0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint32_t k[4] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };

// ------------------------------------------------------------------------------------------

void sha1::reset()
{
    m_impl->m_size = 0;
    m_impl->m_total_size = 0;
    m_impl->m_digest[0] = 0x67452301;
    m_impl->m_digest[1] = 0xefcdab89;
    m_impl->m_digest[2] = 0x98badcfe;
    m_impl->m_digest[3] = 0x10325476;
    m_impl->m_digest[4] = 0xc3d2e1f0;
}

// ------------------------------------------------------------------------------------------

void sha1::impl::transform()
{
    uint32_t a = m_digest[0];
    uint32_t b = m_digest[1];
    uint32_t c = m_digest[2];
    uint32_t d = m_digest[3];
    uint32_t e = m_digest[4];
    uint32_t *w = m_buffer;

    for (uint8_t i = 0; i < 16; i++)
         w[i] = SWAPINT32(w[i]);

    for (uint8_t i = 0; i < 80; i++)
    {
        if(i >= 16)
            W(i) = ROL32(W(i + 13) ^ W(i + 8) ^ W(i + 2) ^ W(i), 1);

        uint32_t temp;
        
        if(i < 20)      temp = ROL32(a, 5) + CH(b, c, d) + e + W(i) + k[0];
        else if(i < 40) temp = ROL32(a, 5) + PARITY(b, c, d) + e + W(i) + k[1];
        else if(i < 60) temp = ROL32(a, 5) + MAJ(b, c, d) + e + W(i) + k[2];
        else            temp = ROL32(a, 5) + PARITY(b, c, d) + e + W(i) + k[3];

        e = d;
        d = c;
        c = ROL32(b, 30);
        b = a;
        a = temp;
    }

    m_digest[0] += a;
    m_digest[1] += b;
    m_digest[2] += c;
    m_digest[3] += d;
    m_digest[4] += e;
}

// ------------------------------------------------------------------------------------------

void sha1::impl::update(const uint8_t* _data, size_t _size)
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

void sha1::impl::complete()
{
    size_t padding_size = 64 + 56 - m_size;
    uint64_t total_size = m_total_size * 8;

    if(m_size < 56)
        padding_size = 56 - m_size;

    update(padding, padding_size);

    m_buffer[14] = SWAPINT32((uint32_t) (total_size >> 32));
    m_buffer[15] = SWAPINT32((uint32_t) total_size);

    transform();

    for(unsigned i = 0; i < 5; i++)
        m_digest[i] = SWAPINT32(m_digest[i]);
}





}
