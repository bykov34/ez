
#pragma once

namespace ez
{
    class event_loop
    {
        public:

            enum class event_e
            {
                error,
                read,
                write,
                closed
            };

            event_loop();
            void init();
            ~event_loop();
        
            using on_event_t = void(*)(void*, int, event_e, unsigned);
            using on_timeout_t = void(*)(void*, unsigned);

            unsigned workers() const;

            void start(unsigned _workers, int _timeout = -1);
            void stop();

            void add_fd(int _fd);
            void add_fd(int _fd, unsigned _worker);

            void remove_fd(int _fd, unsigned _worker);

            void on_event(void* _param, on_event_t);
            void on_timeout(on_timeout_t);

        private:

            struct impl; impl* m_impl;
    };
}


