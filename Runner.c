#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

// State.
int needs_relaunch = 0; // Set on update.
pid_t app_pid = -1;  // Pid of ./ircserv
time_t last_binary_mtime = 0; // Timestamp for tracking binary updates for relaunch.

// Runs `git pull`.
void Update(void);

// Keeps `./ircserv` running and manages it's restarts.
void Run();

// Sends a message over webhook.
void WebhookSendMsg();

// Sleep delay in seconds.
int const default_sleep_delay_sec = 10;
int sleep_delay_sec = default_sleep_delay_sec;

int main(void)
{
    while (1)
    {
        Update();
        Run();
        sleep(sleep_delay_sec);
        sleep_delay_sec = default_sleep_delay_sec; // reset the delay to default
    }
}

void Update()
{
    // Run git pull
    pid_t pid = fork();

    if (pid == -1)
    {
        fprintf(stderr, "Error: fork() returned %s.\r\n", strerror(errno));
        sleep_delay_sec = 60;
        return;
    }

    if (pid == 0)
    {
        // never thought of this one before
        close(STDOUT_FILENO);
        open("/dev/null", O_WRONLY);

        if (-1 == execvp("git", (char *const[]){"git", "pull", NULL}))
        {
            fprintf(stderr, "Error: execvp() returned %s.\r\n", strerror(errno));
            _exit(1);
        }
    }
    else
    {
        int exit_status;
        if (-1 == waitpid(pid, &exit_status, 0))
        {
            fprintf(stderr, "Error: waitpid() returned %s.\r\n", strerror(errno));
            sleep_delay_sec = 60;
            return;
        }

        if (WIFEXITED(exit_status))
        {
            if (WEXITSTATUS(exit_status) != 0)
            {
                fprintf(stderr, "Error: `git pull` failed. Exit status %i.\r\n", WEXITSTATUS(exit_status));
                sleep_delay_sec = 60;
                return;
            }
        }
        else
        {
            fprintf(stderr, "Abnormal process termination of `git pull`. Exit status: %i\r\n", WTERMSIG(exit_status));
            sleep_delay_sec = 60;
            return;
        }
    }

    // Run xmake build
    pid = fork();
    if (pid == -1)
    {
        fprintf(stderr, "Error: fork() returned %s.\r\n", strerror(errno));
        sleep_delay_sec = 60;
        return;
    }

    if (pid == 0)
    {
        // never thought of this one before
        close(STDOUT_FILENO);
        open("/dev/null", O_WRONLY);

        if (-1 == execvp("xmake", (char *const[]){"xmake", "build", NULL}))
        {
            fprintf(stderr, "Error: execvp() returned %s.\r\n", strerror(errno));
            _exit(1);
        }
    }
    else
    {
        int exit_status;
        if (-1 == waitpid(pid, &exit_status, 0))
        {
            fprintf(stderr, "Error: waitpid() returned %s.\r\n", strerror(errno));
            sleep_delay_sec = 60;
            return;
        }

        if (WIFEXITED(exit_status))
        {
            if (WEXITSTATUS(exit_status) != 0)
            {
                fprintf(stderr, "Error: `xmake build` failed. Exit status %i.\r\n", WEXITSTATUS(exit_status));
                sleep_delay_sec = 60;
                return;
            }
        }
        else
        {
            fprintf(stderr, "Abnormal process termination of `xmake build`. Exit status: %i\r\n", WTERMSIG(exit_status));
            sleep_delay_sec = 60;
            return;
        }
    }

    // Check mtime
    struct stat st;
    if (0 == stat("./ircserv", &st))
    {
        if (st.st_mtime != last_binary_mtime)
        {
            needs_relaunch = 1;
            last_binary_mtime = st.st_mtime;
        }
    }
    else
    {
        fprintf(stderr, "Error: stat() returned %s.\r\n", strerror(errno));
    }
}

void Run(void)
{
    if (needs_relaunch)
    {
        if (app_pid != -1)
        {
            int ret = kill(app_pid, SIGKILL);
            int exit_status;
            if (0 != ret)
            {
                fprintf(stderr, "Error: kill() returned %s.\r\n", strerror(errno));
                sleep_delay_sec = 60;
                return;
            }

            if (-1 == waitpid(app_pid, &exit_status, 0))
            {
                fprintf(stderr, "Error: waitpid() returned %s.\r\n", strerror(errno));
                sleep_delay_sec = 60;
                return;
            }
        }
        app_pid = fork();

        if (app_pid == -1)
        {
            fprintf(stderr, "Error: fork() returned %s.\r\n", strerror(errno));
            sleep_delay_sec = 60;
            return;
        }

        if (app_pid == 0)
        {
            if (-1 == execvp("./ircserv", (char *const[]){"ircserv", "6667", "secret", NULL}))
            {
                fprintf(stderr, "Error: execvp() returned %s.\r\n", strerror(errno));
                _exit(1);
            }
        }
        else
        {
            needs_relaunch = 0;
            return;
        }
    }
    else // Check if the process is still running.
    {
        int exit_status;
        int res = waitpid(app_pid, &exit_status, WNOHANG);
        if (-1 == res)
        {
            fprintf(stderr, "Error: waitpid() returned %s.\r\n", strerror(errno));
            sleep_delay_sec = 60;
            return;
        }
        else if (0 == res)
        {
            return; // Process is running normally.
        }

        needs_relaunch = 1;
        app_pid = -1;
        if (WIFEXITED(exit_status))
        {
            if (WEXITSTATUS(exit_status) != 0)
            {
                fprintf(stderr, "Abnormal process termination of `ircserv`. Exit status: %i\r\n", WEXITSTATUS(exit_status));
                sleep_delay_sec = 1;
                return;
            }
        }
        else
        {
            fprintf(stderr, "Abnormal process termination of `ircserv`. Exit status: %i\r\n", WTERMSIG(exit_status));
            sleep_delay_sec = 1;
            return;
        }
    }
}