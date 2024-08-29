
#include <ez/events.hpp>

#include <thread>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/prctl.h>
#include <sys/eventfd.h>
#include <fcntl.h>
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

struct worker
{
    int         stop_fd = -1;
    int         epoll_fd = -1;

    std::atomic<bool> running {false};
    std::atomic<bool> stopping {false};
};

struct event_loop::impl
{
    on_event_t          m_event_fn;
    on_timeout_t        m_timeout_fn;
    void*               m_param = nullptr;
    worker*             m_worker = nullptr;
    int                 m_epoll = -1;
    int                 m_stop_fd = -1;
    unsigned            m_workers = 0;

    ~impl();

    void start_workers(unsigned int _workers, int _timeout);
    void process_events(int _kq, int _stopfd, unsigned _worker, int _timeout);
    void process_fd(int _fd, uint32_t _flags, unsigned _worker);
};

// ---------------------------------------------------------------------------------------------------------------------------------

event_loop::event_loop() : m_impl(new impl)
{
}

void event_loop::init()
{
    m_impl->m_epoll = epoll_create1(EPOLL_CLOEXEC);
    if (m_impl->m_epoll == -1)
        throw std::runtime_error("epoll_create() error");
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

unsigned event_loop::workers() const
{
    return m_impl->m_workers;
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

void event_loop::add_fd(int _fd, unsigned _worker)
{
    if (_worker < m_impl->m_workers)
    {
        auto efd = m_impl->m_worker[_worker].epoll_fd;
        if (!epoll_add(efd, _fd))
        {
            std::string err = "can't add client: " + std::to_string(_fd) +
                              " to worker epoll: " + std::to_string(efd) +
                              ", errno: " + std::to_string(errno);

            throw std::runtime_error(err.c_str());
        }
    }
    else
        throw std::runtime_error("wrong worker");
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

void event_loop::impl::start_workers(unsigned int _workers, int _timeout)
{
    m_workers = _workers;
    m_worker = new worker[_workers];

    auto wrk = [this, _timeout](unsigned w)
    {
        m_worker[w].running = true;
        m_worker[w].epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (m_worker[w].epoll_fd == -1)
            throw std::runtime_error("epoll_create() worker error");

        m_worker[w].stop_fd = eventfd(0, EFD_CLOEXEC);
        if (m_worker[w].stop_fd == -1)
            throw std::runtime_error("eventfd() worker error");

        std::string name = "worker " + std::to_string(w);
        prctl(PR_SET_NAME, name.c_str(),0,0,0);

        epoll_add(m_worker[w].epoll_fd, m_worker[w].stop_fd, EPOLLIN);
        process_events(m_worker[w].epoll_fd, m_worker[w].stop_fd, w, _timeout); // blocks this thread
        close(m_worker[w].epoll_fd);
        close(m_worker[w].stop_fd);
        m_worker[w].running = false;
    };

    for (unsigned i = 0; i < _workers; ++i)
        std::thread(wrk, i).detach(); // start worker
}

// ------------------------------------------------------------------------------------------

void event_loop::start(unsigned _workers, int _timeout)
{
    if (m_impl->m_epoll == -1)
        throw std::runtime_error("epoll_create() main error");

    m_impl->m_stop_fd = eventfd(0, EFD_CLOEXEC);
    if (m_impl->m_stop_fd == -1)
        throw std::runtime_error("eventfd() main error");

    epoll_add(m_impl->m_epoll, m_impl->m_stop_fd, EPOLLIN);

    m_impl->start_workers(_workers, 0);
    m_impl->process_events(m_impl->m_epoll, m_impl->m_stop_fd, -1, _timeout);

    close(m_impl->m_stop_fd);
    m_impl->m_stop_fd = -1;

    if (m_impl->m_workers == 0)
        return;

    bool found = false;
    while(!found) // wait until all workers stop
    {
        for (unsigned i = 0; i < m_impl->m_workers; ++i)
        {
            if (m_impl->m_worker[i].running)
            {
                found = true;
                if (!m_impl->m_worker[i].stopping)
                {
                    m_impl->m_worker[i].stopping = true;
                    uint64_t c = 1;
                    write(m_impl->m_worker[i].stop_fd, &c, 8);
                }
            }
        }

        std::this_thread::sleep_for(100ms);
    }

    m_impl->m_workers = 0;
    delete [] m_impl->m_worker;
}

// ------------------------------------------------------------------------------------------

void event_loop::impl::process_events(int _ep, int _stopfd, unsigned _worker, int _timeout)
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
                m_timeout_fn(m_param, _worker);

            continue;
        }

        for (int i = 0; i < result; ++i)
        {
            auto fd = events[i].data.fd;

            if (fd > 0 && fd == _stopfd && events[i].events & EPOLLIN)
            {
                uint64_t c = 0;
                if (read(fd, &c, 8) == 8 && c == 1)
                    return;
            }

            process_fd(fd, events[i].events, _worker);
        }
    }
}

// ------------------------------------------------------------------------------------------

void event_loop::impl::process_fd(int _fd, uint32_t _flags, unsigned _worker)
{
    if (!m_event_fn)
        return;

    if (_flags & EPOLLRDHUP)
    {
        m_event_fn(m_param, _fd, event_e::closed, _worker);
        return;
    }

    if ((_flags & EPOLLERR) || (_flags & EPOLLHUP))
    {
        m_event_fn(m_param, _fd, event_e::error, _worker);
        return;
    }

    if (_flags & EPOLLIN)
        m_event_fn(m_param, _fd, event_e::read, _worker);

    if (_flags & EPOLLOUT)
        m_event_fn(m_param, _fd, event_e::write, _worker);
}


}



