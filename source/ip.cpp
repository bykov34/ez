
#include <ez/ip.hpp>
#include <cinttypes>
#include <cctype>
#include <stdexcept>
//#include <netdb.h>

namespace ez
{
    ipv4_t::ipv4_t(std::string_view _source)
    {
        bool at_least_one_symbol = false;
        uint8_t symbol, string_index = 0, result_index = 0;
        uint16_t data = 0;
        size_t string_length = _source.size();
        uint8_t* result = reinterpret_cast<uint8_t*>(&S_addr);
        const char* string = _source.data();
        
        while ( string_index < string_length )
        {
            symbol = string[string_index];
            if ( std::isdigit ( symbol ) != 0 )
            {
                symbol -= '0';
                data   = data * 10 + symbol;
                if ( data > UINT8_MAX )
                {
                    // 127.0.0.256
                    throw std::runtime_error("ERROR_IPV4_DATA_OVERFLOW");
                }
                at_least_one_symbol = true;
            }
            else if ( symbol == '.' )
            {
                if ( result_index < 3 )
                {
                    if ( at_least_one_symbol )
                    {
                        result[result_index] = static_cast<uint8_t>(data);
                        data = 0;
                        result_index ++;
                        at_least_one_symbol = false;
                    }
                    else
                    {
                        // 127.0..1
                        throw std::runtime_error("ERROR_IPV4_NO_SYMBOL");
                    }
                }
                else
                {
                    // 127.0.0.1.2
                    throw std::runtime_error("ERROR_IPV4_INPUT_OVERFLOW");
                }
            } else
            {
                // 127.*
                throw std::runtime_error("ERROR_IPV4_INVALID_SYMBOL");
            }
            string_index++;
        }
        
        if ( result_index == 3 )
        {
            if ( at_least_one_symbol )
            {
                result[result_index] = static_cast<uint8_t>(data);
            }
            else
            {
                // 127.0.0.
                throw std::runtime_error("ERROR_IPV4_NOT_ENOUGH_INPUT");
            }
        }
        else
        {
            // result_index will be always less than 3
            // 127.0
            throw std::runtime_error("ERROR_IPV4_NOT_ENOUGH_INPUT");
        }
    }

    ipv4_t resolve(const std::string& _name)
    {
        /*struct in_addr* ip = 0;
        struct hostent* hp = 0;
        //hp = gethostbyname(_name.c_str());
        if (hp == 0)
            throw std::runtime_error("cannot resolve host");

        for(unsigned i = 0; hp->h_addr_list[i] != NULL; ++i )
        {
            ip = (struct in_addr*) hp->h_addr_list[i];
            break;
        }

        if (ip == 0)
            throw std::runtime_error("resolved IP address not found");

        return { (uint32_t) (*ip).s_addr };*/
        
        return {};
    }
}


