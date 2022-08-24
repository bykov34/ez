
#pragma once

#include "channel.hpp"
#include "ip.hpp"

namespace ez
{
    class buffer;

    class socket final : public channel
    {
        public:

#ifdef _WIN64
            using fd_t = uint64_t;
#else
            using fd_t = int;
#endif
            enum class state : uint8_t
            {
                disconnected,
                connecting,
                connected,
                listening
            };

            socket();
            socket(fd_t _fd, state);
            socket(socket&& _right);
            socket(const socket& _right) = delete;
            const socket& operator = (const socket& _right) = delete;
            ~socket();

            fd_t fd() const;
            bool is(state) const;
            void close();

            void set_timeout(unsigned _timeout);
            void set_nonblocking(bool _flag);
            void set_close_on_exec();

            void listen(ipv4_t _address, uint16_t _port, size_t _max_clients, bool _share);
            void listen(std::string_view _adr, size_t _max_clients, bool _share);
        
            void connect(ipv4_t _address, uint16_t _port, unsigned _timeout, ipv4_t _bind_to = ipv4_t());
            void connect(std::string_view _adr, unsigned _timeout);
        
            socket accept();

            // channel interface

            size_t send(const buffer& _data) override;
            size_t send(const uint8_t* _data, size_t _size) override;
            size_t recv(buffer& _data, size_t _desired_size = 0) override;
            size_t recv(uint8_t* _data, size_t _size, size_t _desired_size = 0) override;
    
        private:
            
            fd_t                    m_fd = -1;
            socket::state           m_state = socket::state::disconnected;
            bool                    m_nonblocking = false;
    };
}
