

#include <thread>
#include <string.h>

#if defined(__linux__)
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0
#endif

#include <fcntl.h>
#include <errno.h>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/un.h>

#elif defined(WIN32)

#pragma comment(lib, "ws2_32.lib")

#include <winsock.h>
#include <windows.h>
#include <winerror.h>

using socklen_t = int;
using ssize_t = long long;

struct wsa_startup
{
    wsa_startup()
    {
        WSADATA wsaData;
        memset(&wsaData, 0, sizeof(wsaData));
        WSAStartup(MAKEWORD(2, 0), &wsaData);
    }
};

static wsa_startup wsa;

#endif

#include <ez/socket.hpp>
#include <ez/buffer.hpp>

bool would_block()
{
#if defined(__APPLE__) || defined(__linux__)
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
#elif defined(WIN32)
    auto err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK)
#endif
        return true;

    return false;
}

bool connection_reset()
{
#if defined(__APPLE__) || defined(__linux__)
    if (errno == EPIPE || errno == ECONNRESET)
#elif defined(WIN32)
    if (WSAGetLastError() == WSAECONNRESET)
#endif
        return true;
    
    return false;
}

// ------------------------------------------------------------------------------------------

namespace ez {

socket::socket()
{
}

socket::socket(fd_t _fd, state _state)
{
    m_fd = _fd;
    m_state = _state;
}

socket::socket(socket&& _right)
{
    m_fd = _right.m_fd;
    m_state = _right.m_state;
    m_port = _right.m_port;
    m_nonblocking = _right.m_nonblocking;
    
    _right.m_fd = -1; // prevent old socket close on destruction
}

socket::~socket()
{
    close();
}

// ------------------------------------------------------------------------------------------

socket::fd_t socket::fd() const
{
    return m_fd;
}

bool socket::is(state _st) const
{
    return m_state == _st;
}

uint16_t socket::port() const
{
    return m_port;
}

bool socket::can_read() const
{
    if (m_fd == -1)
        return false;
    
    int nread;
    ioctl(m_fd, FIONREAD, &nread);
    
    return nread > 0;
}

/*bool socket::is_nonblocking() const
{
    return m_nonblocking;
}*/

void socket::set_nonblocking(bool _flag)
{
    if (m_fd == -1)
        return;

#if defined(__APPLE__) || defined(__linux__)
    
    int opts;
    if ((opts = fcntl(m_fd, F_GETFL)) < 0)
        throw channel::error("can't get socket options");

    if (_flag)
        opts |= O_NONBLOCK;
    else
        opts &= ~O_NONBLOCK;

    if (fcntl(m_fd, F_SETFL, opts) < 0)
        throw channel::error("can't set socket options");

#elif defined(WIN32)

    u_long iMode = _flag ? 1 : 0;

    auto iResult = ioctlsocket(m_fd, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
        throw error("can't set non-blocking status");
#endif

    m_nonblocking = _flag;
}

bool socket::is_nonblocking() const
{
    return m_nonblocking;
}

// ------------------------------------------------------------------------------------------

void socket::set_timeout(unsigned _timeout)
{
    if (m_fd == -1)
        return;

#if defined(__APPLE__) || defined(__linux__)

    struct timeval timeout;
    timeout.tv_sec = _timeout;
    timeout.tv_usec = 0;
    setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

#elif defined(WIN32)

    int t = _timeout * 1000;
    setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof(timeout));
    setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&t, sizeof(timeout));

#endif

}

// ------------------------------------------------------------------------------------------

ssize_t socket::send(const buffer& _data)
{
    return send(_data.ptr(), _data.size());
}

ssize_t socket::send(const uint8_t* _data, size_t _size)
{
    if (_size == 0)
        return 0;

    if (m_state != socket::state::connected)
        throw socket::error("send fail: socket is not connected");

    for (;;)
    {
        if (auto res = ::send(m_fd, (const char*)(_data), _size, SEND_FLAGS); res >= 0)
        {
            return res;
        }
        else if (res == -1)
        {
            if (would_block())
            {
                if (m_nonblocking)
                    return -3;

                throw timeout();
            }
            else if (errno == EINTR) // wait and try again
            {
                continue;
            }
            else if (connection_reset())
            {
                close();
                if (m_nonblocking)
                    return 0; // event_loop will report about close
                else
                    throw socket::error("socket: disconnected");
            }
            else
            {
                throw socket::error("socket: unknown error");
            }
        }
    }
    
    // should never come here
}

// ------------------------------------------------------------------------------------------

ssize_t socket::recv(buffer& _destination, size_t _desired_size)
{
    return recv(_destination.ptr(), _destination.size(), _desired_size);
}

ssize_t socket::recv(uint8_t* _data, size_t _size, size_t _desired_size)
{
    if (m_state != socket::state::connected)
        throw socket::error("recv fail: socket is not connected");

    if (_size == 0)
        throw socket::error("recv fail: buffer is full or empty");
        
    if (_size < _desired_size)
        throw socket::error("recv fail: buffer is too small for desired size");

    for (;;)
    {
        ssize_t result = -1;
        if (_desired_size > 0)
            result = ::recv(m_fd, (char*)(_data), _desired_size, 0);
        else
            result = ::recv(m_fd, (char*)(_data), _size, 0);
            
        if (result > 0)
        {
            return result;
        }
        else if (result < 0)
        {
            if (would_block()) // TODO: windows error codes
            {
                if (m_nonblocking)
                    return -2;

                throw socket::timeout();
            }
            else if (errno == EINTR) // wait and try again
                continue;
            else
                throw socket::error("socket: unknown error");
        }
        else // result == 0 -> closed by remote side
        {
            close();
            if (m_nonblocking)
                return 0; // event_loop will report about close
            else
                throw socket::error("socket: disconnected");
        }
    }
    
    // should never come here
}

// ------------------------------------------------------------------------------------------

socket socket::accept()
{
    if (m_state != socket::state::listening)
        throw socket::error("accept fail: socket is not listening");

    sockaddr_in remoteAddr;
    socklen_t addrlen = sizeof(sockaddr_in);

    for (;;)
    {
#ifdef SOCK_CLOEXEC
        auto res = ::accept4(m_fd, reinterpret_cast<sockaddr*>(&remoteAddr), &addrlen, SOCK_CLOEXEC);
#else
        auto res = ::accept(m_fd, reinterpret_cast<sockaddr*>(&remoteAddr), &addrlen);
#endif
        if (res >= 0)
        {
            ipv4_t a;
            a.S_addr = remoteAddr.sin_addr.s_addr;
            //id_t id = { a , ntohs(remoteAddr.sin_port), _this.m_id.server_address, _this.m_id.server_port };
            
            return socket(res, state::connected);
        }
        else if (res == -1)
        {
            if (would_block())
            {
                if (m_nonblocking)
                    return socket();

                throw socket::error("EAGAIN on blocking socket");
            }
            else if (errno == EINTR)
            {
                continue;
            }
            else
            {
                throw socket::error("unknown error while accepting client");
            }
        }
    }
}

// ------------------------------------------------------------------------------------------

void socket::listen(ipv4_t _address, uint16_t _port, size_t _max_clients, bool _share)
{
    if (m_fd == -1)
    {
#ifdef SOCK_CLOEXEC
        m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
#else
        m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
        if (m_fd == -1)
            throw socket::error("can't create socket");
    }

    if (_share)
    {
        int set = 1;
        setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char*) &set, sizeof(set));
#if defined(__APPLE__) || defined(__linux__)
        setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, (const char*) &set, sizeof(set));
#endif
    }

    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(_port);
    local_addr.sin_addr.s_addr = _address.S_addr;

    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&local_addr), sizeof(sockaddr_in)) == -1)
    {
        throw socket::error("can't bind to selected address");
    }

    if (::listen(m_fd, static_cast<int>(_max_clients)) == -1)
    {
        std::string s = "can't listen on socket, errno=" + std::to_string(errno);
        throw socket::error(s.c_str());
    }

    m_port = _port;
    m_state = socket::state::listening;
}

// ------------------------------------------------------------------------------------------

void socket::listen(std::string_view _adr, size_t _max_clients, bool _share)
{
    if (m_fd == -1)
    {
        m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_fd == -1)
            throw socket::error("can't create socket");
    }

    if (_share)
    {
        int set = 1;
        setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char*) &set, sizeof(set));
#if defined(__APPLE__) || defined(__linux__)
        setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, (const char*) &set, sizeof(set));
#endif
    }
    
    struct sockaddr_un local_addr{};
    
    auto sz = std::max(_adr.size(), sizeof(local_addr.sun_path)-1);

    local_addr.sun_family = AF_UNIX;
#if !defined(__linux__)
    local_addr.sun_len = sizeof(sockaddr_un);
#endif
    _adr.copy(local_addr.sun_path, sz);
    
    sz += sizeof(local_addr.sun_family);

    unlink(local_addr.sun_path);
    
    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&local_addr), sz) == -1)
    {
        throw socket::error("can't bind to selected address");
    }
    
    if (::listen(m_fd, static_cast<int>(_max_clients)) == -1)
    {
        std::string s = "can't listen on socket, errno=" + std::to_string(errno);
        throw socket::error(s.c_str());
    }
    
    m_state = socket::state::listening;
}

// ------------------------------------------------------------------------------------------

void socket::close()
{
    if (m_fd != -1)
    {
#if defined(__APPLE__) || defined(__linux__)
        ::shutdown(m_fd, SHUT_RDWR);
        ::close(m_fd);
#elif defined (WIN32)
        ::closesocket(m_fd);
#endif
        m_fd = -1;
    }
    
    m_state = socket::state::disconnected;
}

void socket::set_close_on_exec()
{
    if (m_fd != -1)
        fcntl(m_fd, F_SETFD, FD_CLOEXEC);
}

// ------------------------------------------------------------------------------------------

void socket::connect(ipv4_t _address, uint16_t _port, unsigned _timeout, ipv4_t _bind_to)
{
    if (m_fd == -1)
    {
#ifdef SOCK_CLOEXEC
        m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
#else
        m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
        if (m_fd == -1)
            throw socket::error("can't create socket");
    }

    auto fd = m_fd;

    if (_bind_to.S_addr != 0)
    {
        sockaddr_in local_addr{};
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = _bind_to.S_addr;

        if (bind(fd, (struct sockaddr*)&local_addr, sizeof(sockaddr_in)) == -1)
        {
            throw std::runtime_error("can't bind to selected address");
        }
    }

#if defined(__APPLE__)
    int set = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
#endif
    
    set_nonblocking(true);

    m_state = socket::state::connecting;

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = _address.S_addr; // adr->s_addr;
    sin.sin_port = htons(_port);

    int res = ::connect(fd, reinterpret_cast<sockaddr*>(&sin), sizeof(sin));

    if (res == 0)
    {
        m_state = socket::state::connected;
        return;
    }
    else if (res == -1)
    {
        if (would_block())
        {
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(fd, &fdset);

            struct timeval tmptv {};
            tmptv.tv_sec = _timeout;
            tmptv.tv_usec = 0;

            int resultsize = select(fd + 1, nullptr, &fdset, nullptr, &tmptv);

            if (resultsize < 0)
            {
                throw socket::error("select() error in connect()");
            }
            else if (resultsize == 0)
            {
                throw socket::timeout();
            }

            int so_error;
            socklen_t len = sizeof so_error;

            int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*) &so_error, &len);

            if (ret != 0 || so_error != 0)
            {
                throw socket::timeout();
            }

            m_state = socket::state::connected;
        }
        else
        {
            auto s = std::string("error during connect(), errno=") + std::to_string(errno);
            throw socket::error(s.c_str());
        }
    }

    set_nonblocking(false);
}

// ------------------------------------------------------------------------------------------

void socket::connect(std::string_view _adr, unsigned _timeout)
{
    if (m_fd == -1)
    {
        m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (m_fd == -1)
            throw socket::error("can't create socket");
    }

    auto fd = m_fd;

#if defined(__APPLE__)
    int set = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
#endif
    
    set_nonblocking(true);

    struct sockaddr_un local_addr{};
    
    auto sz = std::max(_adr.size(), sizeof(local_addr.sun_path)-1);

    local_addr.sun_family = AF_UNIX;
#if !defined(__linux__)
    local_addr.sun_len = sizeof(sockaddr_un);
#endif
    _adr.copy(local_addr.sun_path, sz);
    
    sz += sizeof(local_addr.sun_family);

    int res = ::connect(fd, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr));

    if (res == 0)
    {
        m_state = socket::state::connected;
        return;
    }
    else if (res == -1)
    {
        if (would_block())
        {
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(fd, &fdset);

            struct timeval tmptv {};
            tmptv.tv_sec = _timeout;
            tmptv.tv_usec = 0;

            int resultsize = select(fd + 1, nullptr, &fdset, nullptr, &tmptv);

            if (resultsize < 0)
            {
                throw socket::error("select() error in connect()");
            }
            else if (resultsize == 0)
            {
                throw socket::timeout();
            }

            int so_error;
            socklen_t len = sizeof so_error;

            int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*) &so_error, &len);

            if (ret != 0 || so_error != 0)
            {
                throw socket::timeout();
            }

            m_state = socket::state::connected;
        }
        else
        {
            auto s = std::string("error during connect(), errno=") + std::to_string(errno);
            throw socket::error(s.c_str());
        }
    }

    set_nonblocking(false);
}

// ------------------------------------------------------------------------------------------

void socket::connect_async(ipv4_t _address, uint16_t _port, ipv4_t _bind_to)
{
    if (m_fd == -1)
    {
#ifdef SOCK_CLOEXEC
        m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
#else
        m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
        if (m_fd == -1)
            throw socket::error("can't create socket");
    }

    auto fd = m_fd;
    if (m_state == socket::state::disconnected)
    {
        #if defined(__APPLE__)
            int set = 1;
            setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
        #endif
        
        set_nonblocking(true);
        
        m_state = socket::state::connecting;

        sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = _address.S_addr; // adr->s_addr;
        sin.sin_port = htons(_port);

        int res = ::connect(fd, reinterpret_cast<sockaddr*>(&sin), sizeof(sin));

        if (res == 0)
        {
            m_state = socket::state::connected;
            return;
        }
        else if (res == -1)
        {
            if (would_block())
            {
                return;
            }
            else
            {
                auto s = std::string("error during connect_async(), errno=") + std::to_string(errno);
                throw socket::error(s.c_str());
            }
        }
    }
    else if (m_state == socket::state::connecting)
    {
        int val; socklen_t len = sizeof(val);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*) &val, &len);
        if (val == 0)
            m_state = socket::state::connected;
        else
        {
            auto s = std::string("error during connect_async(), errno=") + std::to_string(val);
            throw socket::error(s.c_str());
        }
    }
}

} // namespace ez


