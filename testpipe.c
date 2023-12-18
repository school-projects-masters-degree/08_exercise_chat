#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <pipe1> <pipe2>\n", argv[0]);
        return 1;
    }

    int pipe1_fd = open(argv[1], O_WRONLY);
    if (pipe1_fd == -1)
    {
        perror("Open pipe1");
        return 1;
    }

    int pipe2_fd = open(argv[2], O_RDONLY);
    if (pipe2_fd == -1)
    {
        perror("Open pipe2");
        close(pipe1_fd);
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        if (write(pipe1_fd, line, strlen(line)) == -1)
        {
            perror("Write to pipe1 failed");
            break;
        }

        ssize_t nread = read(pipe2_fd, line, sizeof(line) - 1);
        if (nread == -1)
        {
            perror("Read from pipe2 failed");
            break;
        }

        line[nread] = '\0';
        fputs(line, stdout);
        fflush(stdout);
    }

    close(pipe1_fd);
    close(pipe2_fd);
    return 0;
}
