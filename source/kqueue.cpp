
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

struct worker
{
    int         stop_fd[2] = { -1, - 1 };
    int         kqueue_fd = -1;

    std::atomic<bool> running {false};
    std::atomic<bool> stopping {false};

    void stop() { close(kqueue_fd); close(stop_fd[0]); close(stop_fd[1]); }
};

struct event_loop::impl
{
    int                 m_kqueue = -1;
    int                 m_stop_fd[2] = { -1, - 1 };
    unsigned            m_workers = 0;
    on_event_t          m_event_fn;
    on_timeout_t        m_timeout_fn;
    void*               m_param = nullptr;
    worker*             m_worker = nullptr;
    
    ~impl();

    void start_workers(unsigned int _workers, int _timeout);
    void process_events(int _kq, int _stopfd, int _worker, int _timeout);
    bool process_fd(int _fd, struct kevent&, unsigned _worker);
};

// ------------------------------------------------------------------------------------------

event_loop::event_loop() : m_impl(new impl)
{
}

void event_loop::init()
{
    m_impl->m_kqueue = kqueue();
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

unsigned event_loop::workers() const
{
    return m_impl->m_workers;
}

void event_loop::add_fd(int _fd)
{
    if (!kqueue_add(m_impl->m_kqueue, _fd))
        throw std::runtime_error("can't add server to kqueue");
}

void event_loop::add_fd(int _fd, unsigned _worker)
{
    if (_worker < m_impl->m_workers)
        if (!kqueue_add(m_impl->m_worker[_worker].kqueue_fd, _fd))
             throw std::runtime_error("can't add client to worker kqueue");
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

void event_loop::impl::start_workers(unsigned int _workers, int _timeout)
{
    m_workers = _workers;
    
    if (_workers == 0)
        return;
    
    m_worker = new worker[_workers];

    auto wrk = [this, _timeout](unsigned w)
    {
//        prctl(PR_SET_NAME, std::string("Worker {}" + std::to_string(w)).c_str(),0,0,0);

        m_worker[w].kqueue_fd = kqueue();
        if (m_worker[w].kqueue_fd == -1)
        {
            throw std::runtime_error("kqueue() worker error");
            return;
        }
        
        if (pipe(m_worker[w].stop_fd) == 0)
        {
            kqueue_add(m_worker[w].kqueue_fd, m_worker[w].stop_fd[0], true);
        }
        else
            throw std::runtime_error("can't create worker pipe()");
        
        m_worker[w].running = true;
        process_events(m_worker[w].kqueue_fd, m_worker[w].stop_fd[0], w, _timeout); // blocks this thread
        close(m_worker[w].kqueue_fd);
        close(m_worker[w].stop_fd[0]);
        close(m_worker[w].stop_fd[1]);
        m_worker[w].running = false;
    };

    for (unsigned i = 0; i < _workers; ++i)
        std::thread(wrk, i).detach(); // start worker
}

// ------------------------------------------------------------------------------------------

void event_loop::start(unsigned _workers, int _timeout)
{
    if (m_impl->m_kqueue == -1)
        throw std::runtime_error("kqueue() main error");

    if ( pipe(m_impl->m_stop_fd) == 0)
    {
        kqueue_add(m_impl->m_kqueue, m_impl->m_stop_fd[0], true);
    }
    else
        throw std::runtime_error("can't create main pipe()");

    m_impl->start_workers(_workers, 0);
    m_impl->process_events(m_impl->m_kqueue, m_impl->m_stop_fd[0], -1, _timeout); // blocks this thread

    // after calling stop()
    
    close(m_impl->m_stop_fd[0]);
    close(m_impl->m_stop_fd[1]);
    
    m_impl->m_stop_fd[0] = m_impl->m_stop_fd[1] = -1;

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
                    write(m_impl->m_worker[i].stop_fd[1], &c, 8);
                }
            }
        }

        std::this_thread::sleep_for(100ms);
    }

    m_impl->m_workers = 0;
    delete [] m_impl->m_worker;
}

// ------------------------------------------------------------------------------------------

void event_loop::impl::process_events(int _kq, int _stopfd, int _worker, int _timeout)
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
                m_timeout_fn(m_param, _worker);
                
            continue;
        }

        for (int i = 0; i < result; ++i)
        {
            auto fd = static_cast<int>(events[i].ident);
            if (fd == _stopfd)
                return;

            process_fd(fd, events[i], _worker);
        }
    }
}

// ------------------------------------------------------------------------------------------

bool event_loop::impl::process_fd(int _fd, struct kevent& _ev, unsigned _worker)
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
                m_event_fn(m_param, _fd, event_e::error, _worker);
                return false;
            }
        }
    }
    
    if(_ev.filter == EVFILT_READ) // ready to read
    {
        m_event_fn(m_param, _fd, event_e::read, _worker);
    }
    else if (_ev.filter == EVFILT_WRITE) // ready to write
    {
        m_event_fn(m_param, _fd, event_e::write, _worker);
    }
    
    if (_ev.flags & EV_EOF)
    {
        m_event_fn(m_param, _fd, event_e::closed, _worker);
        return false;
    }
    
    return true;
}

}


