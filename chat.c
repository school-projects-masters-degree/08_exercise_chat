#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
// How to compile
// gcc chat.c -o chat

// Create Pipes
// mkfifo p1 p2
#define BUFFER_SIZE 1024
static int global_count = 0;
pthread_t reader_thread, writer_thread;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void increment_global_count()
{
    pthread_mutex_lock(&count_mutex);
    global_count++;
    pthread_mutex_unlock(&count_mutex);
}

void *writer_func(void *arg)
{
    char *pipe_name = (char *)arg;
    int fd_pipe1 = open(pipe_name, O_WRONLY);
    if (fd_pipe1 == -1)
    {
        perror("Error opening pipe in reader thread");
        pthread_exit(NULL);
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL)
    {
        size_t len = strlen(buffer);

        // Check if the last character is not a newline and the buffer is full
        if (buffer[len - 1] != '\n' && len == BUFFER_SIZE - 1)
        {
            increment_global_count();
            printf("\nTotal lines: %d\n", global_count);
            perror("Error: Input line is too long\n");
            exit(EXIT_FAILURE);
        }

        if (write(fd_pipe1, buffer, len) == -1)
        {
            perror("Write error in reader thread");
            exit(EXIT_FAILURE);
        }
        increment_global_count();
    }

    close(fd_pipe1);
    pthread_exit(NULL);
}

void *reader_func(void *arg)
{
    char *pipe_name = (char *)arg;
    int fd_pipe2 = open(pipe_name, O_RDONLY);
    if (fd_pipe2 == -1)
    {
        perror("Error opening pipe in writer thread");
        pthread_exit(NULL);
    }

    char buffer[BUFFER_SIZE];
    int num_read;
    while ((num_read = read(fd_pipe2, buffer, BUFFER_SIZE)) > 0)
    {
        write(STDOUT_FILENO, buffer, num_read);
        increment_global_count();
    }

    if (num_read == -1)
    {
        perror("Read error in writer thread");
        exit(0);
    }

    if (num_read == 0)
    {
        // Detected EOF - other end closed
        // Increment count for the last line
        increment_global_count();
        // Print total lines
        printf("\nTotal lines: %d\n", global_count);
        printf("Writer detected EOF. Exiting.\n");
        exit(0);
    }

    close(fd_pipe2);
    pthread_exit(NULL);
}

void signal_handler(int sig)
{

    // Increment count for the SIGINT signal
    increment_global_count();

    // Otherwise we would count +1 on the other side
    // And we would have a total lines count mismatch
    pthread_cancel(reader_thread);
    pthread_cancel(writer_thread);
    pthread_join(reader_thread, NULL);
    pthread_join(writer_thread, NULL);
    printf("\nTotal lines: %d\n", global_count);
    exit(0);
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        printf("Usage: %s <name> <name>\n", argv[0]);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("Error setting up signal handler");
        return 1;
    }

    if (pthread_create(&reader_thread, NULL, writer_func, (void *)argv[1]) != 0)
    {
        perror("Error creating reader thread");
        return 1;
    }

    if (pthread_create(&writer_thread, NULL, reader_func, (void *)argv[2]) != 0)
    {
        perror("Error creating writer thread");
        return 1;
    }

    pthread_join(reader_thread, NULL);
    pthread_join(writer_thread, NULL);

    pthread_mutex_destroy(&count_mutex);
    return 0;
}