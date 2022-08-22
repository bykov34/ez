#pragma once

#include <atomic>
#include <thread>

namespace ez
{
	class spin_lock
	{
			std::atomic_flag locked = ATOMIC_FLAG_INIT;

		public:

			void lock()
			{
                while (locked.test_and_set(std::memory_order_acquire));
			}

            bool try_lock()
            {
                return !locked.test_and_set(std::memory_order_acquire);
            }
        
			void unlock()
			{
				locked.clear(std::memory_order_release);
			}
	};
}
