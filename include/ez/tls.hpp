
#pragma once

#include <stdexcept>
#include <vector>

#include <ez/buffer.hpp>
#include <ez/channel.hpp>

namespace ez
{
    class tls final : public channel
    {
        static constexpr size_t impl_size = 24;
        std::aligned_storage<impl_size>::type m_impl;

        public:
        
            class alert : public std::runtime_error
            {
                public: alert(const char* _what): runtime_error(_what) {}
            };
        
            tls(channel&);
            ~tls();

            void reset();
            void close();

            static void init_server(const std::string& _apln, ez::buffer& _cert, ez::buffer& _key);

            void connect(std::string_view _host, bool _check_cert);
            void listen();
            bool handshake();
            bool is_handshake_complete() const;
            std::vector<uint8_t> pub_key() const;
        
            // channel interface
        
            size_t send(const buffer& _data) override;
            size_t send(const uint8_t* _data, size_t _size) override;
            size_t recv(buffer& _data, size_t _desired_size = 0) override;
            size_t recv(uint8_t* _data, size_t _size, size_t _desired_size = 0) override;
        
            bool can_read() const override;
    };
}
