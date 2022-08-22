#pragma once

#include <chrono>

namespace ez
{
    class timer
    {
        using clock_t = std::chrono::steady_clock;
        using ms = std::chrono::milliseconds;
        
        public:
        
            void reset()
            {
                m_start = clock_t::now();
                m_started = true;
            }
        
            void stop() { m_started = false; }
        
            ms elapsed() const
            {
                if (!m_started)
                    return std::chrono::duration_values<ms>::zero();
                
                return std::chrono::duration_cast<ms>(clock_t::now() - m_start);
            }
        
            template <class T>
            bool operator == (const T& _right) const { return elapsed() == _right; }
        
            template <class T>
            bool operator <= (const T& _right) const { return elapsed() <= _right; }

            template <class T>
            bool operator >= (const T& _right) const { return elapsed() >= _right; }
 
            template <class T>
            bool operator < (const T& _right) const { return elapsed() < _right; }

            template <class T>
            bool operator > (const T& _right) const { return elapsed() > _right; }
        
            bool started() const { return m_started; }
            operator bool() const { return m_started; }
        
        private:
        
            clock_t::time_point m_start;
            bool m_started = false;
    };
}

using namespace std::chrono_literals;
