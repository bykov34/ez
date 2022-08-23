
#include <ez/tls.hpp>
#include <memory>

#include <tls.h>

#if defined _MSC_VER && defined _M_X64
using ssize_t = long long;
#endif

namespace ez {

struct impl
{
    struct ::tls*          m_context;
    struct ::tls_config*   m_config;
    channel&               m_channel;
    
    impl(channel& _ch) : m_context(nullptr), m_config(nullptr), m_channel(_ch) {}

    void connect(std::string_view _host, bool _check_cert);
    void listen();
    
    size_t recv(uint8_t* _data, size_t _size, size_t _desired_size);
    size_t send(const uint8_t* _data, size_t _size);
};

::tls* g_scontext = nullptr;
tls_config*   g_sconfig = nullptr;

void tls::init_server(const std::string& _apln, ez::buffer& _cert, ez::buffer& _key)
{
    g_scontext = tls_server();
    g_sconfig = tls_config_new();

    tls_config_set_protocols(g_sconfig, TLS_PROTOCOL_TLSv1_3);
    tls_config_set_cert_mem(g_sconfig, _cert.ptr(), _cert.size());
    tls_config_set_key_mem(g_sconfig, _key.ptr(), _key.size());
    
    if (!_apln.empty())
        tls_config_set_alpn(g_sconfig, _apln.c_str());
    
    tls_configure(g_scontext, g_sconfig);
}

inline impl& get(void* _storage) noexcept
{
    return *reinterpret_cast<impl*>(_storage);
}

inline const impl& get_const(const void* _storage) noexcept
{
    return *reinterpret_cast<const impl*>(_storage);
}

// ------------------------------------------------------------------------------------------

tls::tls(channel& _ch)
{
    auto sz = sizeof(impl);
    auto al = alignof(impl);
        
    static_assert(sizeof(impl) <= tls::impl_size, "impl size mismatch");
    static_assert(alignof(impl) == std::alignment_of<impl>::value, "impl alignment mismatch");
    new (&m_impl) impl(_ch); // placement new to our storage
}

tls::~tls()
{
    auto& _this = get(&m_impl);
    
    if (_this.m_context)
        tls_free(_this.m_context);
        
    if (_this.m_config)
        tls_config_free(_this.m_config);
    
    _this.~impl();
}

// ------------------------------------------------------------------------------------------

void tls::reset()
{
    auto& _this = get(&m_impl);
    if (_this.m_context)
        tls_reset(_this.m_context);
}

void tls::close()
{
    auto& _this = get(&m_impl);
    if (_this.m_context)
        tls_close(_this.m_context);
}

// ------------------------------------------------------------------------------------------

ssize_t read_cb(struct ::tls* _ctx, void* _buf, size_t _buflen, void* _cb_arg)
{
    auto _this = reinterpret_cast<impl*>(_cb_arg);
    
    auto recvd = _this->m_channel.recv(reinterpret_cast<uint8_t*>(_buf), _buflen);
    if (recvd > 0)
        return recvd;
        
    if (recvd == 0)
        return TLS_WANT_POLLIN;
    
    return -1;
}

ssize_t write_cb(struct ::tls* _ctx, const void* _buf, size_t _buflen, void* _cb_arg)
{
    auto _this = reinterpret_cast<impl*>(_cb_arg);
    
    auto sent = _this->m_channel.send(reinterpret_cast<const uint8_t*>(_buf), _buflen);
    if (sent > 0)
        return sent;
        
    if (sent == 0)
        return TLS_WANT_POLLOUT;
    
    return -1;
}

// ------------------------------------------------------------------------------------------

void tls::connect(std::string_view _host, bool _check_cert)
{
    auto& _this = get(&m_impl);
    _this.connect(_host, _check_cert);
}

void impl::connect(std::string_view _host, bool _check_cert)
{
    if (!m_context)
        m_context = tls_client();

    if (!m_config)
        m_config = tls_config_new();
    
    tls_config_set_protocols(m_config, TLS_PROTOCOL_TLSv1_2 | TLS_PROTOCOL_TLSv1_3);
    if (!_check_cert)
    {
        tls_config_insecure_noverifycert(m_config);
        tls_config_insecure_noverifyname(m_config);
    }
    
    tls_configure(m_context, m_config);
    tls_connect_cbs(m_context, read_cb, write_cb, this, _host.data());
}

// ------------------------------------------------------------------------------------------

void tls::listen()
{
    auto& _this = get(&m_impl);
    _this.listen();
}

void impl::listen()
{
    tls_accept_cbs(g_scontext, &m_context, read_cb, write_cb, this);
}

// ------------------------------------------------------------------------------------------

bool tls::handshake()
{
    auto& _this = get(&m_impl);
    if (_this.m_context == nullptr)
        return false;
    
    return tls_handshake(_this.m_context) == 0;
}

bool tls::is_handshake_complete() const
{
    auto& _this = get_const(&m_impl);
    return tls_peer_is_handhake_complete(_this.m_context) != 0;
}

std::vector<uint8_t> tls::pub_key() const
{
    auto& _this = get_const(&m_impl);
    if (_this.m_context == nullptr)
        return {};
    
    size_t size = 0;
    auto pubkey = tls_peer_cert_pubkey(_this.m_context, &size);
    return { pubkey, pubkey + size };
}

// ------------------------------------------------------------------------------------------
// channel interface

size_t tls::send(const buffer& _buff)
{
    auto& _this = get(&m_impl);
    return _this.send(_buff.ptr(), _buff.size());
}

size_t tls::send(const uint8_t* _data, size_t _size)
{
    auto& _this = get(&m_impl);
    return _this.send(_data, _size);
}

size_t impl::send(const uint8_t* _data, size_t _size)
{
    ssize_t total_sent = 0;
    for (;;)
    {
        auto res = tls_write(m_context, _data + total_sent, _size - total_sent);

        if (res > 0)
        {
            total_sent += res;
            if (total_sent == _size) // all data is sent
                return total_sent;
        }
        else if (res == TLS_WANT_POLLOUT)
        {
            continue;
        }
        else
        {
            const char* err = tls_error(m_context);
            if (err != nullptr)
            {
                std::string what = "tls: "; what += err;
                throw channel::error(what.c_str());
            }
            
            throw channel::error("tls: unknown error");
        }
    }
}

// ------------------------------------------------------------------------------------------

size_t tls::recv(buffer& _buff, size_t _desired_size)
{
    auto& _this = get(&m_impl);
    return _this.recv(_buff.ptr(), _buff.size(), _desired_size);
}

size_t tls::recv(uint8_t* _data, size_t _size, size_t _desired_size)
{
    auto& _this = get(&m_impl);
    return _this.recv(_data, _size, _desired_size);
}

size_t impl::recv(uint8_t* _data, size_t _size, size_t _desired_size)
{
    //if (_size == 0)
    //    throw error(m_fd, "recv fail: buffer is full or empty");

    //if (_size < _desired_size)
    //    throw error(m_fd, "recv fail: buffer is too small for desired size");

    size_t total_recv = 0;
    for (;;)
    {
        ssize_t rcv = -1;

        if (_desired_size > 0)
            rcv = tls_read(m_context, _data + total_recv, _desired_size - total_recv);
        else
            rcv = tls_read(m_context, _data + total_recv, _size - total_recv);

        if (rcv > 0)
        {
            total_recv += rcv;
            if (_desired_size > 0 && total_recv < _desired_size)
                continue;
            
            return total_recv;
        }
        else if (rcv == TLS_WANT_POLLIN)
        {
            continue;
        }
        else
        {
            const char* err = tls_error(m_context);
            if (err != nullptr)
            {
                std::string what = "tls: "; what += err;
                throw channel::error(what.c_str());
            }
            
            throw channel::error("tls: unknown error");
        }
    }
}

} // namespace ez
