
#include <ez/events.hpp>

#include <thread>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/prctl.h>
#include <sys/eventfd.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <atomic>
#include <string.h>
#include <stdexcept>

using namespace std::chrono_literals;

// ------------------------------------------------------------------------------------------

static inline bool epoll_add(int _epoll, int _fd, unsigned _ev = 0)
{
    struct epoll_event ev{};
    if (_ev == 0)
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLET;
    else
        ev.events = _ev;

    ev.data.fd = _fd;

    int n = epoll_ctl(_epoll, EPOLL_CTL_ADD, _fd, &ev);
    if (n != 0) return false;

    return true;
}

// ------------------------------------------------------------------------------------------

namespace ez {

struct event_loop::impl
{
    on_event_t          m_event_fn;
    on_timeout_t        m_timeout_fn;
    void*               m_param = nullptr;
    int                 m_epoll = -1;
    int                 m_stop_fd = -1;

    ~impl();

    void process_events(int _kq, int _stopfd, int _timeout);
    void process_fd(int _fd, uint32_t _flags);
};

// ------------------------------------------------------------------------------------------

event_loop::event_loop() : m_impl(new impl)
{
}

void event_loop::init()
{
    m_impl->m_epoll = epoll_create(1);
}

event_loop::~event_loop()
{
    delete m_impl;
}

event_loop::impl::~impl()
{
    if (m_epoll != -1)
        close(m_epoll);
}

void event_loop::stop()
{
    if (m_impl->m_stop_fd == -1)
        return;

    uint64_t c = 1;
    write(m_impl->m_stop_fd, &c, 8);
}

void event_loop::add_fd(int _fd)
{
    if(!epoll_add(m_impl->m_epoll, _fd))
        throw std::runtime_error("can't add fd to epoll");
}

void event_loop::on_event(void* _param, on_event_t _ev)
{
    m_impl->m_param = _param;
    m_impl->m_event_fn = _ev;
}

void event_loop::on_timeout(on_timeout_t _ev)
{
    m_impl->m_timeout_fn = _ev;
}

// ------------------------------------------------------------------------------------------

void event_loop::start(int _timeout)
{
    if (m_impl->m_epoll == -1)
        throw std::runtime_error("epoll_create() main error");

    m_impl->m_stop_fd = eventfd(0, 0);
    if (m_impl->m_stop_fd == -1)
        throw std::runtime_error("eventfd() main error");

    epoll_add(m_impl->m_epoll, m_impl->m_stop_fd, EPOLLIN);

    m_impl->process_events(m_impl->m_epoll, m_impl->m_stop_fd, _timeout);

    close(m_impl->m_stop_fd);
    m_impl->m_stop_fd = -1;
}

// ------------------------------------------------------------------------------------------

void event_loop::impl::process_events(int _ep, int _stopfd, int _timeout)
{
    const int num_events = 100;
    epoll_event events[num_events];
    for(;;)
    {
        int result = 0;
        if (_timeout > 0 )
            result = epoll_wait(_ep, events, num_events, _timeout);
        else
            result = epoll_wait(_ep, events, num_events, -1);

        if (result < 0 && errno == EINTR)
            continue;
        else if (result < 0)
        {
            if (errno == EFAULT)
                throw std::runtime_error("epoll_wait() worker error EFAULT");
            else if (errno == EBADF)
                throw std::runtime_error("epoll_wait() worker error EBADF");
            else if (errno == EINVAL)
                throw std::runtime_error("epoll_wait() worker error EINVAL");
        }
        else if (result == 0)
        {
            if (m_timeout_fn)
                m_timeout_fn(m_param);

            continue;
        }

        for (int i = 0; i < result; ++i)
        {
            auto fd = events[i].data.fd;

            if (fd == _stopfd)
                return;

            process_fd(fd, events[i].events);
        }
    }
}

// ------------------------------------------------------------------------------------------

void event_loop::impl::process_fd(int _fd, uint32_t _flags)
{
    if (!m_event_fn)
        return;

    if (_flags & EPOLLRDHUP)
    {
        m_event_fn(m_param, _fd, event_e::closed);
        return;
    }

    if ((_flags & EPOLLERR) || (_flags & EPOLLHUP))
    {
        m_event_fn(m_param, _fd, event_e::error);
        return;
    }

    if (_flags & EPOLLIN)
        m_event_fn(m_param, _fd, event_e::read);

    if (_flags & EPOLLOUT)
        m_event_fn(m_param, _fd, event_e::write);
}


}



