
#include <string.h>
#include <stdexcept>
#include <vector>

#include <ez/chacha20_poly1305.hpp>
#include <ez/chacha20.hpp>
#include <ez/poly1305.hpp>
#include <ez/common.hpp>

namespace ez {

struct chacha20_poly1305::impl
{
    chacha20 m_cipher;
    poly1305 m_mac;
    
    std::vector<uint8_t> m_aad;
    std::vector<uint8_t> m_tag;
    
    
    void encrypt(const uint8_t *_input, size_t _in_size, uint8_t *_output);
    bool decrypt(const uint8_t *_input, size_t _in_size, uint8_t *_output);
};

// ------------------------------------------------------------------------------------------

chacha20_poly1305::chacha20_poly1305() : m_impl(new impl)
{
    m_impl->m_cipher.set_rounds(20);
}

chacha20_poly1305::~chacha20_poly1305() // TODO: cleanup
{
    delete m_impl;
}

size_t chacha20_poly1305::encrypted_size(size_t _in_size)
{
    return _in_size;
}

size_t chacha20_poly1305::decrypted_size(size_t _in_size)
{
    return _in_size;
}

void chacha20_poly1305::set_key(const uint8_t* _key, size_t _key_size)
{
    m_impl->m_cipher.set_key(_key, _key_size);
}

void chacha20_poly1305::set_iv(const uint8_t* _iv, size_t _iv_size)
{
    m_impl->m_cipher.set_iv(_iv, _iv_size);
}

void chacha20_poly1305::set_aad(const uint8_t* _aad, size_t _aad_size)
{
    m_impl->m_aad.assign(_aad, _aad + _aad_size);
}

void chacha20_poly1305::set_tag(const uint8_t* _tag, size_t _tag_size)
{
    m_impl->m_tag.assign(_tag, _tag + _tag_size);
}

void chacha20_poly1305::get_tag(uint8_t* _tag)
{
    m_impl->m_mac.copy_to(_tag);
}

void chacha20_poly1305::encrypt(const uint8_t* _input, size_t _in_size, uint8_t* _output)
{
    m_impl->encrypt(_input, _in_size, _output);
}

bool chacha20_poly1305::decrypt(const uint8_t *_input, size_t _in_size, uint8_t *_output)
{
    return m_impl->decrypt(_input, _in_size, _output);
}

// ------------------------------------------------------------------------------------------

void chacha20_poly1305::impl::encrypt(const uint8_t* _input, size_t _in_size, uint8_t* _output)
{
    uint8_t temp[32];

    m_cipher.encrypt(nullptr, 32, temp);
    m_cipher.encrypt(nullptr, 32, nullptr);
    m_cipher.encrypt(_input, _in_size, _output);

    m_mac.set_key(temp, 32);
    m_mac.update(m_aad.data(), m_aad.size());

    if ((m_aad.size() % 16) != 0)
    {
        auto padding_size = 16 - (m_aad.size() % 16);
        memset(temp, 0, padding_size);
        m_mac.update(temp, padding_size);
    }

    m_mac.update(_output, _in_size);

    if ((_in_size % 16) != 0)
    {
        auto padding_size = 16 - (_in_size % 16);
        memset(temp, 0, padding_size);
        m_mac.update(temp, padding_size);
    }

    STORE64LE(m_aad.size(), temp);
    m_mac.update(temp, sizeof(uint64_t));
    
    STORE64LE(_in_size, temp);
    m_mac.update(temp, sizeof(uint64_t));
    
    m_mac.complete();
}

// ------------------------------------------------------------------------------------------

bool chacha20_poly1305::impl::decrypt(const uint8_t *_input, size_t _in_size, uint8_t *_output)
{
    if (m_tag.empty())
        return false;
    
    uint8_t temp[32];

    m_cipher.encrypt(nullptr, 32, temp);
    m_cipher.encrypt(nullptr, 32, nullptr);

    m_mac.set_key(temp, 32);
    m_mac.update(m_aad.data(), m_aad.size());

    if ((m_aad.size() % 16) != 0)
    {
        auto padding_size = 16 - (m_aad.size() % 16);
        memset(temp, 0, padding_size);
        m_mac.update(temp, padding_size);
    }

    m_mac.update(_input, _in_size);

    if ((_in_size % 16) != 0)
    {
        auto padding_size = 16 - (_in_size % 16);
        memset(temp, 0, padding_size);
        m_mac.update(temp, padding_size);
    }

    STORE64LE(m_aad.size(), temp);
    m_mac.update(temp, sizeof(uint64_t));
    
    STORE64LE(_in_size, temp);
    m_mac.update(temp, sizeof(uint64_t));
    
    m_mac.complete();
 
    const auto& mac = m_mac.get();
    uint8_t mask = 0;
 
    m_cipher.decrypt(_input, _in_size, _output);
    
    for (auto i = 0; i < m_tag.size(); i++)
        mask |= mac[i] ^ m_tag[i];

    return mask == 0;
}


}
