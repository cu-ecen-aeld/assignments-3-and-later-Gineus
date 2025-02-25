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

    FILE *file = fopen(writefile, "w");
    if (file == NULL)
    {
        syslog(LOG_ERR, "Failed to open file %s: %s", writefile, strerror(errno));
        closelog();
        return 1;
    }

    if (fprintf(file, "%s", writestr) < 0)
    {
        syslog(LOG_ERR, "Failed to write to file %s: %s", writefile, strerror(errno));
        fclose(file);
        closelog();
        return 1;
    }

    fclose(file);
    closelog();

    return 0;
}