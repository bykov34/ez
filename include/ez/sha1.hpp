
#pragma once

#include <inttypes.h>
#include <vector>

namespace ez {

class sha1 final
{
    public:
    
        inline static const size_t digest_size = 20;
        inline static const size_t block_size = 64;
    
        sha1();
        ~sha1();
        void reset();
    
        sha1& calculate(const uint8_t* _data, size_t _size);
        sha1& calculate(const char* _data, size_t _size);
    
        void update(const uint8_t* _data, size_t _size);
        void update(const char* _data, size_t _size);
    
        sha1& complete();
    
        std::vector<uint8_t> get();
        void copy_to(uint8_t* _result);
        bool compare_with(const uint8_t* _data);
    
    private:

        struct impl; impl* m_impl;
};

}
