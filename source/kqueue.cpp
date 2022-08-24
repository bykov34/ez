
#include <ez/events.hpp>

#include <thread>
#include <atomic>
#include <chrono>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/pipe.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

using namespace std::chrono_literals;

// ------------------------------------------------------------------------------------------

static inline bool kqueue_add(int _kqueue, int fd, bool _write = true)
{
    if (_write)
    {
        struct kevent ev[2];
        EV_SET(&ev[0], fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, 0);
        EV_SET(&ev[1], fd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, 0);
        if (int n = kevent(_kqueue, ev, 2, 0, 0, 0); n == -1)
            return false;
    }
    else
    {
        struct kevent ev;
        EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, 0);
        if (int n = kevent(_kqueue, &ev, 1, 0, 0, 0); n == -1)
            return false;
    }
    
    return true;
}

// ------------------------------------------------------------------------------------------

namespace ez {

struct event_loop::impl
{
    int                 m_kqueue = -1;
    int                 m_stop_fd[2] = { -1, - 1 };
    on_event_t          m_event_fn;
    on_timeout_t        m_timeout_fn;
    void*               m_param = nullptr;
    
    ~impl();

    void process_events(int _kq, int _stopfd, int _timeout);
    bool process_fd(int _fd, struct kevent&);
};

// ------------------------------------------------------------------------------------------

event_loop::event_loop() : m_impl(new impl)
{
}

void event_loop::init()
{
    m_impl->m_kqueue = kqueue();
    fcntl(m_impl->m_kqueue, F_SETFD, FD_CLOEXEC);
}

event_loop::~event_loop()
{
    delete m_impl;
}

event_loop::impl::~impl()
{
    if (m_kqueue != -1)
        close(m_kqueue);
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

void event_loop::add_fd(int _fd)
{
    if (!kqueue_add(m_impl->m_kqueue, _fd))
        throw std::runtime_error("can't add server to kqueue");
}

// ------------------------------------------------------------------------------------------

void event_loop::stop()
{
    if (m_impl->m_stop_fd[0] != -1 && m_impl->m_stop_fd[1] != -1)
    {
        uint64_t c = 1;
        write(m_impl->m_stop_fd[1], &c, 8);
    }
}

// ------------------------------------------------------------------------------------------

void event_loop::start(int _timeout)
{
    if (m_impl->m_kqueue == -1)
        throw std::runtime_error("kqueue() main error");

    if ( pipe(m_impl->m_stop_fd) == 0)
    {
        fcntl(m_impl->m_stop_fd[0], F_SETFD, FD_CLOEXEC);
        fcntl(m_impl->m_stop_fd[1], F_SETFD, FD_CLOEXEC);
        kqueue_add(m_impl->m_kqueue, m_impl->m_stop_fd[0], true);
    }
    else
        throw std::runtime_error("can't create main pipe()");

    m_impl->process_events(m_impl->m_kqueue, m_impl->m_stop_fd[0], _timeout); // blocks this thread

    // after calling stop()
    
    close(m_impl->m_stop_fd[0]);
    close(m_impl->m_stop_fd[1]);
    
    m_impl->m_stop_fd[0] = m_impl->m_stop_fd[1] = -1;
}

// ------------------------------------------------------------------------------------------

void event_loop::impl::process_events(int _kq, int _stopfd, int _timeout)
{
    constexpr int num_events = 100;
    struct kevent events[num_events];
    for (;;)
    {
        int result = 0; 
        if (_timeout > 0)
        {
            timespec ts;
            if (_timeout < 1000)
            {
                ts.tv_sec = 0;
                ts.tv_nsec = _timeout * 1000000;
            }
            else
            {
                ts.tv_sec = _timeout / 1000;
                ts.tv_nsec = (_timeout - ts.tv_sec * 1000) * 1000000;
            }
           
            result = kevent(_kq, 0, 0, events, num_events, &ts);
        }
        else
            result = kevent(_kq, 0, 0, events, num_events, nullptr);
            
        if (result < 0 && errno == EINTR)
            continue;
        else if (result < 0)
        {
            if (errno == EACCES)
                throw std::runtime_error("kevent() worker error EACCES");
            else if (errno == EFAULT)
                throw std::runtime_error("kevent() worker error EFAULT");
            else if (errno == EBADF)
                throw std::runtime_error("kevent() worker error EBADF");
            else if (errno == EINVAL)
                throw std::runtime_error("kevent() worker error EINVAL");
            else if (errno == ENOENT)
                throw std::runtime_error("kevent() worker error ENOENT");
            else if (errno == ENOMEM)
                throw std::runtime_error("kevent() worker error ENOMEM");
            else if (errno == ESRCH)
                throw std::runtime_error("kevent() worker error ESRCH");
        }
        else if (result == 0)
        {
            if (m_timeout_fn)
                m_timeout_fn(m_param);
                
            continue;
        }

        for (int i = 0; i < result; ++i)
        {
            auto fd = static_cast<int>(events[i].ident);
            if (fd == _stopfd)
                return;

            process_fd(fd, events[i]);
        }
    }
}

// ------------------------------------------------------------------------------------------

bool event_loop::impl::process_fd(int _fd, struct kevent& _ev)
{
    if (_ev.flags & EV_ERROR)
    {
        switch (_ev.data)
        {
            case ENOENT:
            case EINVAL:
            case EBADF:
            case EPERM:
            case EPIPE:
            {
                m_event_fn(m_param, _fd, event_e::error);
                return false;
            }
        }
    }
    
    if(_ev.filter == EVFILT_READ) // ready to read
    {
        m_event_fn(m_param, _fd, event_e::read);
    }
    else if (_ev.filter == EVFILT_WRITE) // ready to write
    {
        m_event_fn(m_param, _fd, event_e::write);
    }
    
    if (_ev.flags & EV_EOF)
    {
        m_event_fn(m_param, _fd, event_e::closed);
        return false;
    }
    
    return true;
}

}


