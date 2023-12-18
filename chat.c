#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// Create Pipes 
// mkfifo p1 p2
#define BUFFER_SIZE 1024
static int global_count = 0;
pthread_t reader_thread, writer_thread;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int fd_pipe1, fd_pipe2;

void increment_global_count() {
    pthread_mutex_lock(&count_mutex);
    global_count++;
    pthread_mutex_unlock(&count_mutex);
}

void *reader_func(void *arg) {
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        write(fd_pipe1, buffer, strlen(buffer));
        increment_global_count();
    }
    return NULL;
}

void *writer_func(void *arg) {
    char buffer[BUFFER_SIZE];
    int num_read;
    while ((num_read = read(fd_pipe2, buffer, BUFFER_SIZE)) > 0) {
        write(STDOUT_FILENO, buffer, num_read);
        increment_global_count();
    }
    return NULL;
}

void signal_handler(int sig) {
    pthread_cancel(reader_thread);
    pthread_cancel(writer_thread);
    pthread_join(reader_thread, NULL);
    pthread_join(writer_thread, NULL);
    printf("\n Total lines: %d\n", global_count);
    exit(0);
}

int main(int argc, int *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <name> <name>\n", argv[0]);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        goto Error;
    }

    fd_pipe1 = open(argv[1], O_WRONLY);
    if (fd_pipe1 == -1) {
        perror("Error opening pipe1");
        goto Error;
    }

    fd_pipe2 = open(argv[2], O_RDONLY);
    if (fd_pipe2 == -1) {
        perror("Error opening pipe2");
        goto Error;
    }

    if (pthread_create(&reader_thread, NULL, reader_func, NULL) != 0) {
        perror("Error creating reader thread");
        goto Error;
    }

    if (pthread_create(&writer_thread, NULL, writer_func, NULL) != 0) {
        perror("Error creating writer thread");
        goto Error;
    }

    pthread_join(reader_thread, NULL);
    pthread_join(writer_thread, NULL);

    close(fd_pipe1);
    close(fd_pipe2);
    pthread_mutex_destroy(&count_mutex);
    return 0;

    return 0;


Error:
    if (fd_pipe1 != -1) close(fd_pipe1);
    if (fd_pipe2 != -1) close(fd_pipe2);
    return 1;

}



