
#include <string.h>

#include <ez/buffer.hpp>

namespace ez
{
    struct buffer::impl
    {
        uint8_t* m_data = nullptr;
        size_t m_size = 0;
        size_t m_orig_size = 0;
        size_t m_position = 0;
        bool m_own_data = true;

        unsigned m_refs;
        void inc_ref() { ++m_refs; }
        void dec_ref() { --m_refs; if (m_refs == 0) { if (m_own_data) delete [] m_data; delete this; } }
    };
    
    buffer::buffer() : m_impl(new impl)
    {
        m_impl->m_refs = 1;
    }
    
    buffer::buffer(const buffer& _right) : m_impl(nullptr)
    {
        *this = _right;
    }
    
    const buffer& buffer::operator = (const buffer& _right)
    {
        if (this == &_right || _right.m_impl == nullptr) return *this;
        
        if (m_impl)
            m_impl->dec_ref();
        
        m_impl = _right.m_impl;
        m_impl->inc_ref();
        
        return *this;
    }
    
    buffer::buffer(size_t _size) : m_impl(new impl)
    {
        m_impl->m_refs = 1;
        m_impl->m_data = new unsigned char[_size];
        memset(m_impl->m_data, 0, _size);
        m_impl->m_size = _size;
        m_impl->m_orig_size = _size;
    }

    buffer::buffer(const std::vector<std::uint8_t>& _vec) : m_impl(new impl)
    {
        m_impl->m_refs = 1;
        m_impl->m_data = new uint8_t[_vec.size()];
        m_impl->m_size = _vec.size();
        m_impl->m_orig_size = _vec.size();
        memcpy(m_impl->m_data, _vec.data(), _vec.size());
    }
    
    buffer::buffer(const void *_data, size_t _size, bool _copy) : m_impl(new impl)
    {
        const uint8_t* dp = reinterpret_cast<const uint8_t*>(_data);
        m_impl->m_refs = 1;
        if (_copy)
        {
            m_impl->m_data = new unsigned char[_size];
            memcpy(m_impl->m_data, dp, _size);
            m_impl->m_own_data = true;
        }
        else
        {
            m_impl->m_data = (uint8_t*) dp;
            m_impl->m_own_data = false;
        }

        m_impl->m_size = _size;
        m_impl->m_orig_size = _size;
    }

    buffer::buffer(const std::string& _str) : m_impl(new impl)
    {
        m_impl->m_refs = 1;
        m_impl->m_data = new uint8_t[_str.size()];
        m_impl->m_size = _str.size();
        m_impl->m_orig_size = _str.size();
        memcpy(m_impl->m_data, _str.c_str(), _str.size());
    }

    buffer::buffer(const std::string_view _str)
    {
        m_impl->m_refs = 1;
        m_impl->m_data = new uint8_t[_str.size()];
        m_impl->m_size = _str.size();
        m_impl->m_orig_size = _str.size();
        memcpy(m_impl->m_data, _str.data(), _str.size());
    }
    
    buffer::buffer(const char* _str) : m_impl(new impl)
    {
        m_impl->m_refs = 1;
        size_t len = strlen(_str);
        m_impl->m_data = new uint8_t[len];
        m_impl->m_size = len;
        m_impl->m_orig_size = len;
        memcpy(m_impl->m_data, _str, len);
    }
    
    buffer::~buffer()
    {
        m_impl->dec_ref();
    }
    
    uint8_t* buffer::ptr() const
    {
        return m_impl->m_data + m_impl->m_position;
    }

    const char* buffer::c_str() const
    {
        return reinterpret_cast<const char*>(m_impl->m_data) + m_impl->m_position;
    }

    size_t buffer::size() const
    {
        return m_impl->m_size - m_impl->m_position;;
    }
    
    void buffer::set_size(size_t _size)
    {
        if (_size <= m_impl->m_orig_size)
            m_impl->m_size = _size;
    }
    
    size_t buffer::position() const
    {
        return m_impl->m_position;
    }
    
    void buffer::set_position(size_t _pos)
    {
        if (_pos > m_impl->m_size)
        {
            m_impl->m_position = m_impl->m_size;
            return;
        }
        
        m_impl->m_position = _pos;
    }
}

