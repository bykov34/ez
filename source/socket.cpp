

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
    //m_port = _right.m_port;
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

void socket::set_nonblocking(bool _flag)
{
    if (m_fd == -1)
        return;

#if defined(__APPLE__) || defined(__linux__)
    
    int opts;
    if ((opts = fcntl(m_fd, F_GETFL)) < 0)
        throw channel::error("can't get socket options");

    if (_flag)
        opts = opts | O_NONBLOCK;
    else
        opts = opts & ~O_NONBLOCK;

    if (fcntl(m_fd, F_SETFL, opts) < 0)
        throw channel::error("can't set socket options");

#endif

    m_nonblocking = _flag;
}

// ------------------------------------------------------------------------------------------

void socket::set_close_on_exec()
{
    if (m_fd == -1)
        return;
    
    fcntl(m_fd, F_SETFD, FD_CLOEXEC);
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

size_t socket::send(const buffer& _data)
{
    return send(_data.ptr(), _data.size());
}

size_t socket::send(const uint8_t* _data, size_t _size)
{
    if (_size == 0)
        return false;

    if (m_state != socket::state::connected)
        throw socket::error("send fail: socket is not connected");

    size_t total_sent = 0;
    for (;;)
    {
        auto res = ::send(m_fd, (const char*)(_data + total_sent), _size - total_sent, SEND_FLAGS);

        if (res > 0)
        {
            total_sent += res;
            if (total_sent == _size) // all data is sent
                return total_sent;
        }
        else if (res == -1)
        {
            if (would_block())
            {
                if (m_nonblocking)
                    return total_sent; // >= 0

                throw socket::timeout();
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

size_t socket::recv(buffer& _destination, size_t _desired_size)
{
    return recv(_destination.ptr(), _destination.size(), _desired_size);
}

size_t socket::recv(uint8_t* _data, size_t _size, size_t _desired_size)
{
    if (m_state != socket::state::connected)
        throw socket::error("recv fail: socket is not connected");

    if (_size == 0)
        throw socket::error("recv fail: buffer is full or empty");

    if (_size < _desired_size)
        throw socket::error("recv fail: buffer is too small for desired size");

    size_t total_recv = 0;
    for (;;)
    {
        ssize_t rcv = -1;

        if (_desired_size > 0)
            rcv = ::recv(m_fd, (char*)(_data + total_recv), _desired_size - total_recv, 0);
        else
            rcv = ::recv(m_fd, (char*)(_data + total_recv), _size - total_recv, 0);

        if (rcv > 0)
        {
            total_recv += rcv;
            if (_desired_size > 0 && total_recv < _desired_size)
                continue;
            
            return total_recv;
        }
        else if (rcv == -1)
        {
            if (would_block()) // TODO: windows error codes
            {
                if (m_nonblocking)
                    return total_recv;

                throw socket::timeout();
            }
            else if (errno == EINTR) // wait and try again
                continue;
            else
                throw socket::error("socket: unknown error");
        }
        else // rcv == 0 -> closed by remote side
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
        auto res = ::accept(m_fd, reinterpret_cast<sockaddr*>(&remoteAddr), &addrlen);
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
                std::string err = "unknown error while accepting client: " + std::to_string(errno);
                throw socket::error(err.c_str());
            }
        }
    }
}

// ------------------------------------------------------------------------------------------

void socket::listen(ipv4_t _address, uint16_t _port, size_t _max_clients, bool _share)
{
    if (m_fd == -1)
    {
        m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
        //::shutdown(m_fd, SHUT_RDWR);
        ::close(m_fd);
#elif defined (WIN32)
        ::closesocket(m_fd);
#endif
        m_fd = -1;
    }
    
    m_state = socket::state::disconnected;
}

// ------------------------------------------------------------------------------------------

void socket::connect(ipv4_t _address, uint16_t _port, unsigned _timeout, ipv4_t _bind_to)
{
    if (m_fd == -1)
    {
        m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
        set_nonblocking(false);
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

    m_state = socket::state::connecting;

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

} // namespace ez


