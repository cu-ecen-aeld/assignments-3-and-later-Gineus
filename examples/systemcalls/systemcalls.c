#include "systemcalls.h"
#include <errno.h>    // for errno
#include <stdlib.h>   // for system()
#include <stdio.h>    // for fprintf()
#include <syslog.h>   // for syslog()
#include <unistd.h>   // for fork(), execv()
#include <sys/wait.h> // for wait()
#include <string.h>   // for strerror
#include <fcntl.h>    // for O_WRONLY, O_TRUNC, O_CREAT

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    const int result = system(cmd);

    if (result != 0)
    {
        syslog(LOG_ERR, "Command '%s' failed with return code %d", cmd, result);
        return false;
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    int i;
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    pid_t pid = fork();
    if (pid == -1)
    {
        syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
        va_end(args);
        return false;
    }
    else if (pid == 0)
    {
        // Inside the child process
        execv(command[0], command);
        // If execv returns, it must have failed
        syslog(LOG_ERR, "Execv failed: %s", strerror(errno));
        exit(1);
    }
    else
    {
        // Inside the parent process
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            syslog(LOG_ERR, "Waitpid failed: %s", strerror(errno));
            va_end(args);
            return false;
        }
        if (status == 0)
        {
            va_end(args);
            return true;
        }
        else
        {
            syslog(LOG_ERR, "Command failed with status %d", status);
            va_end(args);
            return false;
        }
    }
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);

    if (fd < 0)
    {
        syslog(LOG_ERR, "open '%s' failed: %s", outputfile, strerror(errno));
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
        va_end(args);
        return false;
    }
    else if (pid == 0)
    {
        // Inside the child process
        const int stdout_fd = 1;

        if (dup2(fd, stdout_fd) < 0)
        {
            syslog(LOG_ERR, "dup2 '%d' failed: %s", fd, strerror(errno));
            exit(1);
        }

        close(fd);
        execvp(command[0], command);
        // If execv returns, it must have failed
        syslog(LOG_ERR, "execvp '%s' failed: %s", command[0], strerror(errno));
        exit(1);
    }
    else
    {
        // Inside the parent process
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            syslog(LOG_ERR, "Waitpid failed: %s", strerror(errno));
            va_end(args);
            return false;
        }
        if (status == 0)
        {
            va_end(args);
            return true;
        }
        else
        {
            syslog(LOG_ERR, "Command failed with status %d", status);
            va_end(args);
            return false;
        }
    }

    va_end(args);

    return true;
}
