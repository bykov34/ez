
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
        
            using on_event_t = void(*)(void*, int, event_e);
            using on_timeout_t = void(*)(void*);

            void start(int _timeout = -1);
            void stop();

            void add_fd(int _fd);

            void on_event(void* _param, on_event_t);
            void on_timeout(on_timeout_t);

        private:

            struct impl; impl* m_impl;
    };
}


