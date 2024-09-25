
#include <unistd.h>
#include <thread>
#include <stdexcept>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std::chrono_literals;

#include <ez/daemon.hpp>

// --------------------------------------------------------------------------------------------------------------------

bool switch_to_daemon()
{
    auto pid = fork();
    if (pid < 0) throw std::runtime_error("Unable to fork()");
    if (pid > 0) return true;

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    return false;
}

// --------------------------------------------------------------------------------------------------------------------

bool self_restart_loop(const char* _executable, const char* _param)
{
    for(;;)
    {
        auto pid = fork();
        if (pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status))
            {
                if (WEXITSTATUS(status) == 0)
                    return true;
                else if (WEXITSTATUS(status) == 1)
                    execlp(_executable, _executable, _param, (const char*) nullptr);
            }

            std::this_thread::sleep_for(5s);
        }
        else if (pid == -1)
            throw std::runtime_error("Unable to fork()");
        else break;
    }
    
    return false;
}
