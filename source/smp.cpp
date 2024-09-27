
#include <string.h>

#include <ez/smp.hpp>
#include <ez/buffer.hpp>

#define MAX_HEADER 1024*1024*10
#define MAX_DATA 1024*1024*10
#define MAGIC "SMPm"

namespace ez {

struct smp::impl
{
    state_e         m_state = state_e::waiting_meta;
    buffer          m_header;
    buffer          m_body;
    buffer          m_send_buffer;
    bool            m_new_header = false;
    char            m_meta[12];
    uint32_t        m_header_size = 0;
    uint32_t        m_body_size = 0;
    channel&        m_channel;

    impl(channel& _ch) : m_channel(_ch)
    {
        reset();
    }

    void reset();
    void send_more();
    smp::message_t recv();
    void send(const buffer& _header, const buffer& _body);
};

// ------------------------------------------------------------------------------------------

smp::smp(channel& _ch) : m_impl(new impl(_ch))
{
}

smp::~smp()
{
    delete m_impl;
}

smp::state_e smp::state() const
{
    return m_impl->m_state;
}

void smp::reset()
{
    m_impl->reset();
}

void smp::impl::reset()
{
    m_state = state_e::waiting_meta;
    if (m_new_header || m_header.size() < sizeof(m_meta))
        m_header = buffer(4096);
    m_new_header = false;
    m_body = buffer();
    m_header_size = 0;
    m_body_size = 0;
    m_header.set_position(0);
}

bool smp::have_body() const
{
    return m_impl->m_body_size != 0;
}

// ------------------------------------------------------------------------------------------

void smp::send_more()
{
    m_impl->send_more();
}

void smp::impl::send_more()
{
    if (m_state == state_e::sending_data)
    {
        if (auto sz = m_channel.send(m_send_buffer); sz == m_send_buffer.size())
        {
            m_state = state_e::send_complete;
            m_send_buffer = buffer();
        }
        else if (sz > 0)
            m_send_buffer.set_position(m_send_buffer.position() + sz);
    }
}

// ------------------------------------------------------------------------------------------

void smp::send(const buffer& _header, const buffer& _body)
{
    m_impl->send(_header, _body);
}

void smp::impl::send(const buffer& _header, const buffer& _body)
{
    m_state = state_e::sending_data;
    
    uint32_t header_size = static_cast<uint32_t>(_header.size());
    uint32_t body_size = static_cast<uint32_t>(_body.size());

    if (header_size == 0 || header_size > MAX_HEADER)
        throw error("invalid header size");

    if (body_size > MAX_DATA)
        throw error("invalid body size");

    // todo send separately

    m_send_buffer = buffer(12 + header_size + body_size);

    memcpy(m_send_buffer.ptr(), MAGIC, 4);
    memcpy(m_send_buffer.ptr()+4, &header_size, 4);
    memcpy(m_send_buffer.ptr()+8, &body_size, 4);
    memcpy(m_send_buffer.ptr()+12, _header.ptr(), _header.size());
    if (body_size > 0)
        memcpy(m_send_buffer.ptr()+12+header_size, _body.ptr(), _body.size());

    if(auto sz = m_channel.send(m_send_buffer); sz == m_send_buffer.size())
    {
        m_state = state_e::send_complete;
        m_send_buffer = buffer();
    }
    else if (sz > 0)
        m_send_buffer.set_position(m_send_buffer.position() + sz);
}

// ------------------------------------------------------------------------------------------

smp::message_t smp::recv()
{
    return m_impl->recv();
}

smp::message_t smp::impl::recv()
{
    switch (m_state)
    {
        case state_e::sending_data:
            throw error("smp: can't recv while sending");

        case state_e::send_complete:
            reset();
            [[fallthrough]];

        case state_e::waiting_meta:
        {
            auto need = sizeof(m_meta) - m_header.position();
            if (auto sz = m_channel.recv(m_header, need); sz == need) // recv meta size
            {
                m_header.set_position(0);
                memcpy(m_meta, m_header.ptr(), sizeof(m_meta));
                if (memcmp(m_meta, MAGIC, 4) == 0)
                {
                    m_header_size = *reinterpret_cast<uint32_t*>(&m_meta[4]);
                    m_body_size = *reinterpret_cast<uint32_t*>(&m_meta[8]);

                    if (m_header_size == 0 || m_header_size > MAX_HEADER)
                        throw error("smp: wrong data format");

                    if (m_header_size > m_header.size())
                    {
                        m_new_header = true;
                        m_header = buffer(m_header_size);
                    }

                    m_state = state_e::waiting_header;
                }
                else
                    throw error("smp: wrong data format");
            }
            else if (sz > 0)
                m_header.set_position(m_header.position() + sz);
            else // would block
                return smp::message_t();

        } [[fallthrough]];

        case state_e::waiting_header:
        {
            auto need = m_header_size - m_header.position();
            if (auto sz = m_channel.recv(m_header, need); sz == need)
            {
                m_header.set_position(0);
                m_header.set_size(m_header_size); // shrink buffer
            }
            else if (sz > 0)
                m_header.set_position(m_header.position() + sz);
            else // would block
                return smp::message_t();

            if (m_body_size > 0 && m_body_size <= MAX_DATA)
            {
                m_body = buffer(m_body_size);
                m_state = state_e::waiting_body;
            }
            else if (m_body_size > 0)
                throw error("smp: body size is bigger than allowed");
            else
                return { m_header, buffer() };

        } [[fallthrough]];

        case state_e::waiting_body:
        {
            auto need = m_body_size - m_body.position();
            if (auto sz = m_channel.recv(m_body, need); sz == need)
            {
                m_body.set_position(0);
                m_state = state_e::waiting_meta;
                return { m_header, m_body };
            }
            else if (sz > 0)
                m_body.set_position(m_body.position() + sz);
            else // would block
                return smp::message_t();
        }
    }

    throw std::runtime_error("smp: should never come here");
}


}
