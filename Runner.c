#include <iso646.h>
#include <stdlib.h>
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
int app_read_fd = -1; // File descriptor for reading the server logs (stdout and stderr).
char const* remote_branch;

// Runs `git pull`.
void Update(void);

// Keeps `./ircserv` running and manages it's restarts.
void Run(void);

// Read stdout and stderr into Webhook.
void PullLogs(void);

// Sleep delay in seconds.
int const default_sleep_delay_sec = 10;
int sleep_delay_sec = default_sleep_delay_sec;

// Webhook.
size_t const webhook_message_size_limit = 1900;
size_t const webhook_buffer_allocation_size = webhook_message_size_limit * 6 + 32;
char const* webhook_secret;
char* webhook_buffer = NULL;
size_t webhook_buffer_len = 0;

int  WebhookInit(char const* InWebhookSecret);
void WebhookShutdown(void);
void WebhookSendMsg(char const* InMsg);
void WebhookFlushMsgs(void);

int main(int argc, char const** argv)
{
    if (argc < 3) // Need at least 3 arguments.
    {
        fprintf(stderr, "Invalid amount of arguments.\r\n"
            "Usage: %s <WEBHOOK_URL_SECRET> <remote/branch>\r\n", argc == 1 ? argv[0] : "<program>");
        return 1;
    }

    remote_branch = argv[2];
    if (not WebhookInit(argv[1]))
    {
        fprintf(stderr, "Fatal error: Failed to initialize Webhook.\r\n");
        return 1;
    }

    while (1)
    {
        PullLogs();
        Update();
        Run();
        sleep(sleep_delay_sec);
        sleep_delay_sec = default_sleep_delay_sec; // reset the delay to default
        WebhookFlushMsgs();
    }
}

void Update()
{
    // Run git fetch origin
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

        if (-1 == execvp("git", (char *const[]){"git", "fetch", "origin", NULL}))
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
                fprintf(stderr, "Error: `git fetch origin` failed. Exit status %i.\r\n", WEXITSTATUS(exit_status));
                sleep_delay_sec = 60;
                return;
            }
        }
        else
        {
            fprintf(stderr, "Abnormal process termination of `git fetch origin`. Exit status: %i\r\n", WTERMSIG(exit_status));
            sleep_delay_sec = 60;
            return;
        }
    }

    // Run git reset
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

        if (-1 == execvp("git", (char *const[]){"git", "reset", "--hard", (char*)remote_branch, NULL}))
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
                fprintf(stderr, "Error: `git reset` failed. Exit status %i.\r\n", WEXITSTATUS(exit_status));
                sleep_delay_sec = 60;
                return;
            }
        }
        else
        {
            fprintf(stderr, "Abnormal process termination of `git reset`. Exit status: %i\r\n", WTERMSIG(exit_status));
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
            if (app_read_fd != -1)
            {
                close(app_read_fd);
                app_read_fd = -1;
            }
        }
        int pipes[2];

        if (-1 == pipe(pipes))
        {
            fprintf(stderr, "Error: pipe() returned %s.\r\n", strerror(errno));
            sleep_delay_sec = 60;
            return;
        }

        app_pid = fork();

        if (app_pid == -1)
        {
            fprintf(stderr, "Error: fork() returned %s.\r\n", strerror(errno));
            close(pipes[0]);
            close(pipes[1]);
            sleep_delay_sec = 60;
            return;
        }

        if (app_pid == 0)
        {
            close(pipes[0]);
            dup2(pipes[1], STDOUT_FILENO);
            dup2(pipes[1], STDERR_FILENO);
            close(pipes[1]);

            if (-1 == execvp("./ircserv", (char *const[]){"ircserv", "6667", "secret", NULL}))
            {
                fprintf(stderr, "Error: execvp() returned %s.\r\n", strerror(errno));
                _exit(1);
            }
        }
        else
        {
            needs_relaunch = 0;
            app_read_fd = pipes[0];
            fcntl(app_read_fd, F_SETFL, O_NONBLOCK);
            close(pipes[1]);
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

        if (app_read_fd != -1)
        {
            close(app_read_fd);
            app_read_fd = -1;
        }
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

void PullLogs(void)
{
    if (-1 == app_read_fd)
    {
        return;
    }

    char readbuf[4096];
    while (1)
    {
        ssize_t n = read(app_read_fd, readbuf, sizeof(readbuf));
        if (n <= 0) break;

        readbuf[n] = 0;
        WebhookSendMsg(readbuf);
    }
}

// --- Discord Webhook.

// Returns bytes written, not including the NULL-terminator.
static int EscapeLogStringJSON(char *InDst, int DstCap, const char *InSrc, int InSrcLen)
{
    int j = 0;
    for (int i = 0; i < InSrcLen && j < DstCap - 6; i++) {
        unsigned char c = InSrc[i];
        switch (c) {
        case '"':  InDst[j++] = '\\'; InDst[j++] = '"';  break;
        case '\\': InDst[j++] = '\\'; InDst[j++] = '\\'; break;
        case '\n': InDst[j++] = '\\'; InDst[j++] = 'n';  break;
        case '\r': InDst[j++] = '\\'; InDst[j++] = 'r';  break;
        case '\t': InDst[j++] = '\\'; InDst[j++] = 't';  break;
        case '\b': InDst[j++] = '\\'; InDst[j++] = 'b';  break;
        case '\f': InDst[j++] = '\\'; InDst[j++] = 'f';  break;
        default:
            if (c < 0x20) {
                // control chars -> \u00XX
                j += snprintf(InDst + j, DstCap - j, "\\u%04x", c);
            } else {
                InDst[j++] = c;
            }
            break;
        }
    }
    return j;
}

int WebhookInit(char const* InWebhookSecret)
{
    webhook_secret = InWebhookSecret;
    webhook_buffer = malloc(webhook_buffer_allocation_size);
    if (not webhook_buffer)
    {
        return 0;
    }

    return 1; // Success!
}

void WebhookShutdown(void)
{
    free(webhook_buffer);
    webhook_buffer = NULL;
}

// Queues message to webhook, may flush the message buffer if it gets full.
void WebhookSendMsg(char const* InMsg)
{
    size_t const msg_len = strlen(InMsg);

    // If InMsg is so big it doesn't fit the entire buffer.
    if (msg_len > webhook_message_size_limit)
    {
        WebhookFlushMsgs();
        memcpy(webhook_buffer, InMsg, webhook_message_size_limit);
        webhook_buffer_len = webhook_message_size_limit;
        WebhookFlushMsgs();
        return;
    }

    // If InMsg and already buffered messages do not fit together.
    if (webhook_buffer_len + msg_len > webhook_message_size_limit)
    {
        WebhookFlushMsgs();
    }

    // There's enough space to buffer InMsg message.
    memcpy(webhook_buffer + webhook_buffer_len, InMsg, msg_len);
    webhook_buffer_len += msg_len;
}

void WebhookFlushMsgs(void)
{
    if (webhook_buffer_len == 0)
    {
        return;
    }

    char json[4096];
    int off = 0;
    off += snprintf(json + off, sizeof(json) - off, "{\"content\":\"");
    off += EscapeLogStringJSON(json + off, sizeof(json) - off, webhook_buffer, webhook_buffer_len);
    off += snprintf(json + off, sizeof(json) - off, "\"}");

    // pipe JSON into curl via stdin
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST -H 'Content-Type: application/json' --data @- '%s'",
        webhook_secret);

    FILE *curl = popen(cmd, "w");
    if (curl)
    {
        fwrite(json, 1, off, curl);
        pclose(curl);
    }
    webhook_buffer_len = 0;
}