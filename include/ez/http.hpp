
#pragma once

#include <string_view>
#include <tuple>
#include <unordered_map>
#include <stdexcept>

#include <ez/buffer.hpp>
#include <ez/channel.hpp>

namespace ez
{
    class http
    {
        public:

            class error : public std::runtime_error
            {
                public: error(const std::string& _what) : runtime_error(_what) {}
            };
        
            http(channel& _ch);
            ~http();
        
            http(const http& _right) = delete;
            const http& operator = (const http& _right) = delete;

            enum class state_e
            {
                waiting_header,
                waiting_body,
                sending_body,
                send_complete
            };
        
            using headers_t = std::unordered_multimap<std::string, std::string>;
            using response_t = std::tuple<unsigned, std::string, headers_t, buffer>;
            using request_t = std::tuple<std::string, std::string, headers_t, buffer>;

            state_e state() const;
            void reset();
            void set_channel(channel& _ch);

            // client

            response_t recv_response();
            void send_request(const request_t& _request);

            // server
        
            request_t recv_request();
            void send_response(const response_t& _response);
            void send_more();

        private:
        
            struct impl; impl* m_impl;
    };
}
