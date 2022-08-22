
#pragma once

#include <string_view>

namespace ez
{
    struct ipv4_t
    {
        union
        {
            struct { uint8_t s_b1, s_b2, s_b3, s_b4; } S_un_b;
            uint32_t S_addr;
        };

        ipv4_t(uint32_t _saddr)
        {
            S_addr = _saddr;
        }
        
        ipv4_t(uint8_t _1 = 0, uint8_t _2 = 0, uint8_t _3 = 0, uint8_t _4 = 0)
        {
            S_un_b.s_b1 = _1; S_un_b.s_b2 = _2; S_un_b.s_b3 = _3; S_un_b.s_b4 = _4;
        }
        
        explicit ipv4_t(std::string_view _source);
    };

    ipv4_t resolve(std::string_view _name);
}

