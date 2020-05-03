/* updaterevision.c
 *
 * Public domain. This program uses git commands command to get
 * various bits of repository status for a particular directory
 * and writes it into a header file so that it can be used for a
 * project's versioning.
 */

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

// Used to strip newline characters from lines read by fgets.
void stripnl(char *str)
{
    if (*str != '\0')
    {
        size_t len = strlen(str);
        if (str[len - 1] == '\n')
        {
            str[len - 1] = '\0';
        }
    }
}

int main(int argc, char **argv)
{
    char vertag[128], lastlog[128], lasthash[128], *hash = NULL;
    FILE *stream = NULL;
    int gotrev = 0, needupdate = 1;

    vertag[0] = '\0';
    lastlog[0] = '\0';

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <path to gitinfo.h>\n", argv[0]);
        return 1;
    }

    // Use git describe --tags to get a version string. If we are sitting directly
    // on a tag, it returns that tag. Otherwise it returns <most recent tag>-<number of
    // commits since the tag>-<short hash>.
    // Use git log to get the time of the latest commit in ISO 8601 format and its full hash.
    stream = popen("git describe --tags && git log -1 --format=%ai*%h", "r");

    if (NULL != stream)
    {
        if (fgets(vertag, sizeof vertag, stream) == vertag &&
            fgets(lastlog, sizeof lastlog, stream) == lastlog)
        {
            stripnl(vertag);
            stripnl(lastlog);
            gotrev = 1;
        }

        pclose(stream);
    }

    if (gotrev)
    {
        hash = strchr(lastlog, '*');
        if (hash != NULL)
        {
            *hash = '\0';
            hash++;
        }
    }
    if (hash == NULL)
    {
        fprintf(stderr, "Failed to get commit info: %s\n", strerror(errno));
        strcpy(vertag, "<unknown version>");
        lastlog[0] = '\0';
        lastlog[1] = '0';
        lastlog[2] = '\0';
        hash = lastlog + 1;
    }

    stream = fopen (argv[1], "r");
    if (stream != NULL)
    {
        if (!gotrev)
        { // If we didn't get a revision but the file does exist, leave it alone.
            fclose (stream);
            return 0;
        }
        // Read the revision that's in this file already. If it's the same as
        // what we've got, then we don't need to modify it and can avoid rebuilding
        // dependant files.
        if (fgets(lasthash, sizeof lasthash, stream) == lasthash)
        {
            stripnl(lasthash);
            if (strcmp(hash, lasthash + 3) == 0)
            {
                needupdate = 0;
            }
        }
        fclose (stream);
    }

    if (needupdate)
    {
        stream = fopen (argv[1], "w");
        if (stream == NULL)
        {
            return 1;
        }
        fprintf(stream,
                "// %s\n"
                "//\n"
                "// This file was automatically generated by the\n"
                "// updaterevision tool. Do not edit by hand.\n"
                "\n"
                "#define GIT_DESCRIPTION \"%s\"\n"
                "#define GIT_HASH \"%s\"\n"
                "#define GIT_TIME \"%s\"\n",
                hash, vertag, hash, lastlog);
        fclose(stream);
        fprintf(stderr, "%s updated to commit %s.\n", argv[1], vertag);
    }
    else
    {
        fprintf (stderr, "%s is up to date at commit %s.\n", argv[1], vertag);
    }

    return 0;
}
