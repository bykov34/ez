
#include <ez/http.hpp>

#include <string.h>
#include "picohttpparser.c"
#include <map>
#include <charconv>
#include <functional>
#include <algorithm>

namespace ez {

struct http::impl
{
    phr_header m_pheaders[30];
    phr_chunked_decoder m_chunked_decoder{};
    size_t m_num_headers = 0;
    char *m_method_ptr = nullptr, *m_path_ptr = nullptr, *m_message_ptr = nullptr;
    size_t m_method_len = 0, m_path_len = 0, m_message_len = 0;

    state_e             m_state = state_e::waiting_header;
    http::headers_t     m_headers;
    unsigned            m_recv_buffer_size = 8192;
    unsigned            m_max_request_body_size = 10*1024*1024;
    int                 m_status = 0;
    unsigned            m_body_size = 0;
    unsigned            m_header_size = 0;
    unsigned            m_chunk_size = 0;
    std::string         m_method;
    std::string         m_path;
    std::string         m_message;
    bool                m_new_body_buffer = false;
    bool                m_chunked = false;

    std::reference_wrapper<channel> m_channel;
    
    buffer          m_buffer;
    buffer          m_body;
    buffer          m_send_buffer;

    impl(channel& _ch);

    void reset();
    void send_more();
    void send_request(std::string_view _method, std::string_view _path, const headers_t& _hdrs, buffer _body);
    void send_response(unsigned _code, std::string_view _message, const headers_t& _hdrs, buffer _body);
    
    bool isequal(const std::string& a, const std::string& b)
    {
        return std::equal(a.begin(), a.end(),
                          b.begin(), b.end(),
                          [](char a, char b) {
                              return std::tolower(a) == std::tolower(b);
                          });
    }
  
    request_t recv();
    response_t recv_response();
    request_t recv_request();
};

// -----------------------------------------------------------------------------------------------------------

http::http(channel& _ch) : m_impl(new impl(_ch))
{
}

http::~http()
{
    delete m_impl;
}

http::impl::impl(channel& _ch) : m_channel(_ch)
{
    m_buffer = buffer(m_recv_buffer_size);
    reset();
}

void http::set_channel(channel& _ch)
{
    m_impl->m_channel = _ch;
}

// -----------------------------------------------------------------------------------------------------------
// for async channel - continue send when ready

void http::send_more()
{
    m_impl->send_more();
}

void http::impl::send_more()
{
    if (m_state == state_e::sending_body)
    {
        if (auto sz = m_channel.get().send(m_send_buffer); sz == m_send_buffer.size())
        {
            m_state = state_e::send_complete;
            m_send_buffer = buffer();
        }
        else if (sz > 0)
            m_send_buffer.set_position(m_send_buffer.position() + sz);
    }
}

// -----------------------------------------------------------------------------------------------------------

void http::send_request(const http::request_t& _request)
{
    const auto& [_method, _path, _headers, _body] = _request;
    return m_impl->send_request(_method, _path, _headers, _body);
}

// -----------------------------------------------------------------------------------------------------------

void http::impl::send_request(std::string_view _method, std::string_view _path, const headers_t& _hdrs, buffer _body)
{
    m_state = state_e::sending_body;
    std::string req(_method.data(), _method.size());
    req += " ";
    req += _path;
    req += " HTTP/1.1";
    req += "\r\n";

    for (const auto& [key, value]: _hdrs)
    {
        if (value.empty())
            continue;
        
        req += key;
        req += ": ";
        req += value;
        req += "\r\n";
    }
    
    if (_body.size() > 0)
    {
        req += "Content-Length: " + std::to_string(_body.size());
        req += "\r\n";
    }

    req += "\r\n";
    
    m_send_buffer = buffer(req.size() + _body.size());

    memcpy(m_send_buffer.ptr(), req.c_str(), req.size());
    memcpy(m_send_buffer.ptr() + req.size(), _body.ptr(), _body.size());

    if (auto sz = m_channel.get().send(m_send_buffer); sz == m_send_buffer.size())
    {
        m_state = state_e::send_complete;
        m_send_buffer = buffer();
    }
    else if (sz > 0)
        m_send_buffer.set_position(m_send_buffer.position() + sz);
}

// -----------------------------------------------------------------------------------------------------------

http::state_e http::state() const
{
    return m_impl->m_state;
}

void http::reset()
{
    m_impl->reset();
}

void http::http::impl::reset()
{
    m_state = state_e::waiting_header;
    m_body_size = 0;
    m_buffer.set_position(0);
    m_buffer.set_size(m_recv_buffer_size);
    m_body = buffer();
    m_send_buffer = buffer();
    m_headers.clear();
    m_new_body_buffer = false;
    m_chunked = false;
    memset(&m_chunked_decoder, 0, sizeof(m_chunked_decoder));
}

// -----------------------------------------------------------------------------------------------------------
// this method can be called several times before it returns with result

http::request_t http::recv_request()
{
    return m_impl->recv_request();
}

http::http::request_t http::http::impl::recv_request()
{
    switch (m_state)
    {
        case state_e::sending_body:
            throw error("http: can't recv while sending data");

        case state_e::send_complete:
            reset();
            [[fallthrough]];

        case state_e::waiting_header:
        {
            size_t data_size = 0;
            for (;;) // recv data portion (size unknown, so we loop until necessary amount received)
            {
                if (auto block_size = m_channel.get().recv(m_buffer, 0); block_size > 0)
                {
                    int minor_version = 0;
                    m_buffer.set_position(0);
                    m_num_headers = sizeof(m_pheaders) / sizeof(phr_header);
                    int ret = phr_parse_request((const char*) m_buffer.ptr(), data_size + block_size,
                                                (const char**) &m_method_ptr, &m_method_len,
                                                (const char**) &m_path_ptr, &m_path_len, &minor_version,
                                                m_pheaders, &m_num_headers, data_size);
                    data_size += block_size;
                    if (ret > 0) // all headers parsed
                    {
                        m_header_size = static_cast<unsigned>(ret);
                        m_method = std::string_view(m_method_ptr, m_method_len);
                        m_path = std::string_view(m_path_ptr, m_path_len);
                        m_buffer.set_position(m_header_size);
                        
                        for (unsigned i = 0; i < m_num_headers; ++i)
                        {
                            auto field = std::string(m_pheaders[i].name, m_pheaders[i].name_len);
                            auto value = std::string(m_pheaders[i].value, m_pheaders[i].value_len);

                            m_headers.insert({field, value});

                            if (isequal(field,"Content-Length"))
                            {
                                unsigned size = 0;
                                if (auto [p, ec] = std::from_chars(value.data(), value.data() + value.size(), size); ec == std::errc())
                                    m_body_size = size;
                                else
                                    throw error("http: can't parse 'Content-Length' value");
                            }
                            else if (isequal(field,"Transfer-Encoding"))
                            {
                                if (value == "chunked")
                                {
                                    m_chunked = true;
                                    m_body_size = m_max_request_body_size;
                                }
                            }
                        }

                        if (m_body_size > 0) // must have body
                        {
                            if (m_body_size <= data_size - m_header_size) // body already received
                            {
                                m_buffer.set_size(m_header_size+m_body_size); // shrink buffer
                                return std::make_tuple(m_method, m_path, m_headers, m_buffer);
                            }
                            
                            if (m_body_size <= m_max_request_body_size)
                            {
                                m_state = state_e::waiting_body;

                                if (m_buffer.size() < m_body_size) // alloc new if size is not enough
                                {
                                    m_new_body_buffer = true;
                                    m_body = buffer(m_body_size);
                                    auto tail_size = data_size - m_buffer.position();
                                    if (tail_size > 0)
                                    {
                                        memcpy(m_body.ptr(), m_buffer.ptr(), tail_size);
                                        if (m_chunked)
                                        {
                                            m_body.set_position(0);
                                            m_body.set_size(tail_size);
                                        }
                                        else
                                            m_body.set_position(tail_size);
                                    }
                                }
                                else
                                    m_buffer.set_position(data_size);

                                break; // exit loop
                            }
                            else
                                throw error("http: body is bigger than allowed");
                        }
                        else
                            return std::make_tuple(m_method, m_path, m_headers, buffer());
                    }
                    else if (ret == -1)
                        throw error("http: error parsing http headers");

                    if (data_size == m_buffer.size())
                        throw error("http: request is too big");
                    
                    m_buffer.set_position(data_size);
                }
                else // would block
                {
                    return http::http::request_t();
                }

            } //loop

        } [[fallthrough]];

        case state_e::waiting_body:
        {
            if (m_chunked)
            {
                for (;;)
                {
                    size_t size = m_body.size();
                    auto ret = phr_decode_chunked(&m_chunked_decoder, (char*) m_body.ptr(), &size);
                    if (ret == -2)
                    {
                        m_body.set_size(m_max_request_body_size);
                        m_body.set_position(m_body.position()+size);
                        
                        if (auto sz = m_channel.get().recv(m_body, 0); sz > 0)
                        {
                            m_body.set_size(m_body.position()+sz);
                        }
                        else // would block
                        {
                            m_body.set_size(m_body.position());
                            return http::http::request_t();
                        }
                    }
                    else if (ret > 0)
                    {
                        m_body.set_size(m_body.position()+size);
                        m_body.set_position(0);
                        return std::make_tuple(m_method, m_path, m_headers, m_body);
                    }
                    else if (ret == -1)
                        throw error("http: unable to parse chunked content");
                }
            }
            else if (m_new_body_buffer)
            {
                auto need = m_body_size - m_body.position();
                if (auto sz = m_channel.get().recv(m_body, need); sz == need)
                {
                    m_body.set_position(0);
                    return std::make_tuple(m_method, m_path, m_headers, m_body);
                }
                else // would block
                {
                    m_body.set_position(m_body.position()+sz);
                    return http::http::request_t();
                }
            }
            else
            {
                auto need = m_body_size + m_header_size - m_buffer.position();
                if (auto sz = m_channel.get().recv(m_buffer, need); sz == need)
                {
                    m_buffer.set_position(m_header_size);
                    m_buffer.set_size(m_header_size+m_body_size); // shrink buffer
                    return std::make_tuple(m_method, m_path, m_headers, m_buffer);
                }
                else // would block
                {
                    m_buffer.set_position(m_buffer.position()+sz);
                    return http::http::request_t();
                }
            }
        }
            
        default:
            throw error("invalid state");
    }
}

// -----------------------------------------------------------------------------------------------------------

http::response_t http::recv_response()
{
    return m_impl->recv_response();
}

http::response_t http::impl::recv_response()
{
    switch (m_state)
    {
        case state_e::sending_body:
            throw error("http: can't recv while sending data");

        case state_e::send_complete:
            reset();
            [[fallthrough]];

        case state_e::waiting_header:
        {
            size_t data_size = 0;
            for (;;) // recv data portion (size unknown, so we loop)
            {
                if (auto block_size = m_channel.get().recv(m_buffer, 0); block_size > 0)
                {
                    int minor_version = 0;
                    m_buffer.set_position(0);
                    m_num_headers = sizeof(m_pheaders) / sizeof(phr_header);
                    auto ret = phr_parse_response((const char*) m_buffer.ptr(), data_size + block_size, &minor_version,
                                                 &m_status, (const char **) &m_message_ptr, &m_message_len,
                                                 m_pheaders, &m_num_headers, data_size);
                    
                    data_size += block_size;
                    if (ret > 0) // all headers parsed
                    {
                        m_header_size = static_cast<unsigned>(ret);
                        m_message = std::string_view(m_message_ptr, m_message_len);
                        m_buffer.set_position(m_header_size);
                        
                        for (unsigned i = 0; i < m_num_headers; ++i)
                        {
                            auto field = std::string(m_pheaders[i].name, m_pheaders[i].name_len);
                            auto value = std::string(m_pheaders[i].value, m_pheaders[i].value_len);

                            m_headers.insert({field, value});

                            if (isequal(field,"Content-Length"))
                            {
                                unsigned size = 0;
                                if (auto [p, ec] = std::from_chars(value.data(), value.data() +
                                                                   value.size(), size); ec == std::errc())
                                    m_body_size = size;
                                else
                                    throw error("http: can't parse 'Content-Length' value");
                            }
                            else if (isequal(field,"Transfer-Encoding"))
                            {
                                if (value == "chunked")
                                {
                                    m_chunked = true;
                                    m_body_size = m_max_request_body_size;
                                }
                            }
                        }

                        if (m_body_size == 0)
                            return std::make_tuple(m_status, m_message, m_headers, buffer());
                        
                        if (m_body_size <= data_size - m_header_size) // body already received
                        {
                            m_buffer.set_size(m_header_size + m_body_size); // shrink buffer
                            return std::make_tuple(m_status, m_message, m_headers, m_buffer);
                        }
                        
                        if (m_body_size <= m_max_request_body_size)
                        {
                            m_state = state_e::waiting_body;
                            
                            if (m_buffer.size() < m_body_size) // alloc new if size is not enough
                            {
                                m_new_body_buffer = true;
                                m_body = buffer(m_body_size);
                                auto tail_size = data_size - m_buffer.position();
                                if (tail_size > 0)
                                {
                                    memcpy(m_body.ptr(), m_buffer.ptr(), tail_size);
                                    if (m_chunked)
                                    {
                                        m_body.set_position(0);
                                        m_body.set_size(tail_size);
                                    }
                                    else
                                        m_body.set_position(tail_size);
                                }
                            }
                            else
                                m_buffer.set_position(data_size);

                            break; // exit loop
                        }
                        else
                            throw error("http: body is bigger than allowed");
                    }
                    else if (ret == -1)
                        throw error("http: error parsing http headers");
                    
                    if (data_size == m_buffer.size())
                        throw error("http: request is too big");
                    
                    m_buffer.set_position(data_size);
                }
                else // would block
                {
                    return http::response_t();
                }

            } // loop

        } [[fallthrough]];

        case state_e::waiting_body:
        {
            if (m_chunked)
            {
                for (;;)
                {
                    size_t size = m_body.size();
                    auto ret = phr_decode_chunked(&m_chunked_decoder, (char*) m_body.ptr(), &size);
                    if (ret == -2)
                    {
                        m_body.set_size(m_max_request_body_size);
                        m_body.set_position(m_body.position()+size);
                        
                        if (auto sz = m_channel.get().recv(m_body, 0); sz > 0)
                        {
                            m_body.set_size(m_body.position()+sz);
                        }
                        else // would block
                        {
                            m_body.set_size(m_body.position());
                            return http::response_t();
                        }
                    }
                    else if (ret > 0)
                    {
                        m_body.set_size(m_body.position()+size);
                        m_body.set_position(0);
                        return std::make_tuple(m_status, m_message, m_headers, m_body);
                    }
                    else if (ret == -1)
                        throw error("http: unable to parse chunked content");
                }
            }
            else if (m_new_body_buffer)
            {
                auto need = m_body_size - m_body.position();
                if (auto sz = m_channel.get().recv(m_body, need); sz == need)
                {
                    m_body.set_position(0);
                    return std::make_tuple(m_status, m_message, m_headers, m_body);
                }
                else // would block
                {
                    m_body.set_position(m_body.position()+sz);
                    return http::response_t();
                }
            }
            else
            {
                auto need = m_body_size + m_header_size - m_buffer.position();
                if (auto sz = m_channel.get().recv(m_buffer, need); sz == need)
                {
                    m_buffer.set_position(m_header_size);
                    m_buffer.set_size(m_header_size+m_body_size); // shrink buffer
                    return std::make_tuple(m_status, m_message, m_headers, m_buffer);
                }
                else // would block
                {
                    m_buffer.set_position(m_buffer.position()+sz);
                    return http::response_t();
                }
            }
        }
            
        default:
            throw error("invalid state");
    }
}

// -----------------------------------------------------------------------------------------------------------

void http::send_response(const response_t& _response)
{
    const auto& [_code, _text, _headers, _body] = _response;
    m_impl->send_response(_code, _text, _headers, _body);
}

// -----------------------------------------------------------------------------------------------------------

void http::impl::send_response(unsigned _code, std::string_view _text, const headers_t& _hdrs, buffer _body)
{
    m_state = state_e::sending_body;

    std::string resp = "HTTP/1.1 " + std::to_string(_code);
    resp += " ";
    resp += _text;
    resp += "\r\n";

    for (auto& [key, value]: _hdrs)
    {
        resp += key;
        resp += ": ";
        resp += value;
        resp += "\r\n";
    }

    if (_body.size() > 0)
    {
        resp += "Content-Length: " + std::to_string(_body.size());
        resp += "\r\n";
    }

    resp += "\r\n";
    
    auto size = resp.size() + _body.size();
    m_send_buffer = buffer(size);

    memcpy(m_send_buffer.ptr(), resp.c_str(), resp.size());
    memcpy(m_send_buffer.ptr() + resp.size(), _body.ptr(), _body.size());

    if (auto sz = m_channel.get().send(m_send_buffer); sz == m_send_buffer.size())
    {
        m_state = state_e::send_complete;
        m_send_buffer = buffer();
    }
    else if (sz > 0)
        m_send_buffer.set_position(m_send_buffer.position() + sz);
}


}
