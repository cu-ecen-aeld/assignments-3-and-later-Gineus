#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <writefile> <writestr>\n", argv[0]);
        return 1;
    }

    openlog("writer", LOG_PID, LOG_USER);
    char *writefile = argv[1];
    char *writestr = argv[2];
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    closelog();

    if (errno != 0)
    {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        perror("Error");
        return 1;
    }

    return 0;
}