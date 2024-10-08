
#pragma once

#include <stdexcept>
#include <string>

#include <ez/buffer.hpp>
#include <ez/channel.hpp>

namespace ez
{
    class smp final
    {
        struct impl; impl* m_impl;
        public:
        
            enum class state_e
            {
                waiting_meta,
                waiting_header,
                waiting_body,
                sending_data,
                send_complete
            };

            class error : public std::runtime_error
            {
                public: error(const std::string& _what) : runtime_error(_what) {}
            };

            smp(channel&);
            ~smp();
        
            smp(const smp& _right) = delete;
            const smp& operator = (const smp& _right) = delete;
        
            state_e state() const;
            void reset();

            bool have_body() const;
            struct message_t { ez::buffer header; ez::buffer body; };
            void send(const buffer& _header, const buffer& _data);
            message_t recv();
            void send_more();
    };
}
