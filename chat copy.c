#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

pthread_t reader_thread, writer_thread;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int global_count = 0;
int pipe1_fd, pipe2_fd;

void *reader_thread_func(void *arg)
{
    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        if (write(pipe1_fd, line, strlen(line)) == -1)
        {
            perror("Write to pipe1 failed");
            break;
        }
        pthread_mutex_lock(&count_mutex);
        global_count++;
        pthread_mutex_unlock(&count_mutex);
    }
    return NULL;
}

void *writer_thread_func(void *arg)
{
    char line[1024];
    ssize_t nread;
    while ((nread = read(pipe2_fd, line, sizeof(line) - 1)) > 0)
    {
        line[nread] = '\0';
        fputs(line, stdout);
        fflush(stdout); // Ensure immediate output
        pthread_mutex_lock(&count_mutex);
        global_count++;
        pthread_mutex_unlock(&count_mutex);
    }
    return NULL;
}

void sigint_handler(int sig)
{
    pthread_cancel(reader_thread);
    pthread_cancel(writer_thread);
    pthread_join(reader_thread, NULL);
    pthread_join(writer_thread, NULL);
    printf("Total lines processed: %d\n", global_count);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <pipe1> <pipe2>\n", argv[0]);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    pipe1_fd = open(argv[1], O_WRONLY);
    if (pipe1_fd == -1)
    {
        perror("Open pipe1");
        return 1;
    }

    pipe2_fd = open(argv[2], O_RDONLY);
    if (pipe2_fd == -1)
    {
        perror("Open pipe2");
        close(pipe1_fd);
        return 1;
    }

    if (pthread_create(&reader_thread, NULL, reader_thread_func, (void *)argv[1]) != 0)
    {
        perror("pthread_create reader");
        close(pipe1_fd);
        close(pipe2_fd);
        return 1;
    }

    if (pthread_create(&writer_thread, NULL, writer_thread_func, (void *)argv[2]) != 0)
    {
        perror("pthread_create writer");
        pthread_cancel(reader_thread);
        close(pipe1_fd);
        close(pipe2_fd);
        return 1;
    }

    pthread_join(reader_thread, NULL);
    pthread_join(writer_thread, NULL);

    close(pipe1_fd);
    close(pipe2_fd);
    return 0;
}
