
#include <ez/tls.hpp>
#include <memory>

#include <tls.h>
#include <tls_internal.h>
#include <openssl/x509.h>

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
    
    ssize_t recv(uint8_t* _data, size_t _size, size_t _desired_size = 0);
    ssize_t send(const uint8_t* _data, size_t _size);
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

bool tls::can_read() const
{
    const auto& _this = get_const(&m_impl);
    return _this.m_channel.can_read();
}

bool tls::is_nonblocking() const
{
    const auto& _this = get_const(&m_impl);
    return _this.m_channel.is_nonblocking();
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
    if (recvd >= 0)
        return recvd;
        
    if (recvd == -2)
        return TLS_WANT_POLLIN;
    
    return -1;
}

ssize_t write_cb(struct ::tls* _ctx, const void* _buf, size_t _buflen, void* _cb_arg)
{
    auto _this = reinterpret_cast<impl*>(_cb_arg);
    
    auto sent = _this->m_channel.send(reinterpret_cast<const uint8_t*>(_buf), _buflen);
    if (sent >= 0)
        return sent;
        
    if (sent == -3)
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
    
    struct ::tls* ctx = (struct ::tls*) m_context;
    
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
    if (_this.m_context == nullptr)
        return false;
        
    struct ::tls* ctx = (struct ::tls*) _this.m_context;
    return (ctx->state & TLS_HANDSHAKE_COMPLETE);
}

std::vector<uint8_t> tls::pub_key() const
{
    auto& _this = get_const(&m_impl);
    if (_this.m_context == nullptr)
        return {};
    
    struct ::tls* ctx = (struct ::tls*) _this.m_context;
    if (ctx->ssl_peer_cert == NULL)
        return {};
    
    ASN1_BIT_STRING* pubkey = X509_get0_pubkey_bitstr(ctx->ssl_peer_cert);
    
    size_t size = pubkey->length;
    const unsigned char* data = pubkey->data;
    return { data, data + size };
}

// ------------------------------------------------------------------------------------------
// channel interface

ssize_t tls::send(const buffer& _buff)
{
    auto& _this = get(&m_impl);
    return _this.send(_buff.ptr(), _buff.size());
}

ssize_t tls::send(const uint8_t* _data, size_t _size)
{
    auto& _this = get(&m_impl);
    return _this.send(_data, _size);
}

ssize_t impl::send(const uint8_t* _data, size_t _size)
{
    for (;;)
    {
        if (auto res = tls_write(m_context, _data, _size); res >= 0)
        {
            return res;
        }
        else if (res == TLS_WANT_POLLOUT)
        {
            if (m_channel.is_nonblocking())
                return -3;
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

ssize_t tls::recv(buffer& _buff, size_t _desired_size)
{
    auto& _this = get(&m_impl);
    return _this.recv(_buff.ptr(), _buff.size(), _desired_size);
}

ssize_t tls::recv(uint8_t* _data, size_t _size, size_t _desired_size)
{
    auto& _this = get(&m_impl);
    return _this.recv(_data, _size);
}

ssize_t impl::recv(uint8_t* _data, size_t _size, size_t _desired_size)
{
    if (_size < _desired_size)
        throw channel::error("recv fail: buffer is too small for desired size");
        
    for (;;)
    {
        ssize_t result = -1;
        if (_desired_size > 0)
            result = tls_read(m_context, _data, _desired_size);
        else
            result = tls_read(m_context, _data, _size);
            
        if (result >= 0)
        {
            return result;
        }
        else if (result == TLS_WANT_POLLIN)
        {
            if (m_channel.is_nonblocking())
                return -2;
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
