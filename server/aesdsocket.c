#include <signal.h> // Include signal handling library
#include <fcntl.h> // Include file control options
#include <stdio.h> // Include standard input/output library
#include <stdlib.h> // Include standard library for memory allocation, process control, etc.
#include <string.h> // Include string manipulation functions
#include <sys/socket.h> // Include socket programming library
#include <netinet/in.h> // Include Internet address family definitions
#include <arpa/inet.h> // Include definitions for internet operations
#include <unistd.h> // Include POSIX API for system calls
#include <syslog.h> // Include system logging library
#include <errno.h> // Include error handling definitions

#define PORT 9000 // Define the port number for the server
#define BACKLOG 10 // Define the maximum number of pending connections in the queue
#define FILE_PATH "/var/tmp/aesdsocketdata" // Define the file path for storing received data

int server_socket = -1; // Global variable for the server socket descriptor
int client_socket = -1; // Global variable for the client socket descriptor
int file_fd = -1; // Global variable for the file descriptor
volatile sig_atomic_t running = 1; // Global flag to indicate if the server is running

void cleanup() {
    // Close the client socket if it is open
    if (client_socket != -1) close(client_socket);
    // Close the server socket if it is open
    if (server_socket != -1) close(server_socket);
    // Close the file descriptor if it is open
    if (file_fd != -1) {
        close(file_fd);
        syslog(LOG_INFO, "Closed file descriptor\n");
    }
    // Remove the temporary file
    remove(FILE_PATH);
    // Log a message indicating the server is exiting
    syslog(LOG_INFO, "Caught signal, exiting");
    // Close the system logger
    closelog();
}

void handle_signal(int sig, siginfo_t *info, void *context) {
    (void)info;
    (void)context;
    syslog(LOG_USER, "Caught signal %d, exiting", sig);
    running = 0;
    if (server_socket != -1) {
        shutdown(server_socket, SHUT_RDWR);
    }
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(EXIT_SUCCESS); // Parent exits
    if (setsid() < 0) exit(EXIT_FAILURE); // Create new session
    if (chdir("/") < 0) exit(EXIT_FAILURE); // Change working directory
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    const int SOCKET_CHUNK_SIZE = 1024;
    int socket_buffer_size = SOCKET_CHUNK_SIZE;

    int total_bytes_received;

    char *socket_buffer = malloc(socket_buffer_size); // Allocate initial buffer
    if (!socket_buffer) {
        syslog(LOG_ERR, "Failed to allocate initial buffer");
        return -1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction act = { 0 };
    act.sa_sigaction = &handle_signal;
    act.sa_flags = SA_SIGINFO;

    // Handle signals
    if (sigaction(SIGTERM, &act, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    // Check for daemon mode
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemonize();
    }

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        free(socket_buffer);
        return -1;
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        syslog(LOG_ERR,"setsockopt error: %s\n", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        syslog(LOG_ERR, "Socket bind failed: %s", strerror(errno));
        cleanup();
        free(socket_buffer);
        return -1;
    }

    // Listen for connections
    if (listen(server_socket, BACKLOG) == -1) {
        perror("Socket listen failed");
        syslog(LOG_ERR, "Socket listen failed: %s", strerror(errno));
        cleanup();
        free(socket_buffer);
        return -1;
    }

    file_fd = open(FILE_PATH, O_RDWR | O_CREAT, 0666);

    if (file_fd == -1) {
        perror("Failed to open file");
        syslog(LOG_ERR, "Failed to open file %s: %s", FILE_PATH, strerror(errno));
        cleanup();
        free(socket_buffer);
        return -1;
    }

    while (running) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if (client_socket == -1) {
            if (errno == EINTR) continue; // Interrupted by signal, check running flag
            if (!running) break;
            perror("Socket accept failed");
            syslog(LOG_ERR, "Socket accept failed: %s", strerror(errno));
            break;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        // Receive data dynamically
        total_bytes_received = recv(client_socket, socket_buffer, socket_buffer_size, 0);

        while (total_bytes_received == socket_buffer_size) {
            socket_buffer = realloc(socket_buffer, socket_buffer_size + SOCKET_CHUNK_SIZE);

            if (!socket_buffer) {
                syslog(LOG_ERR, "Unable to realloc %i bytes more", socket_buffer_size + SOCKET_CHUNK_SIZE);
                cleanup();
                exit(EXIT_FAILURE);
            }

            ssize_t current_bytes_received = recv(client_socket, socket_buffer + socket_buffer_size, SOCKET_CHUNK_SIZE, 0);
            total_bytes_received += current_bytes_received;
            socket_buffer_size = socket_buffer_size + SOCKET_CHUNK_SIZE;
        }

        int nr = write(file_fd, socket_buffer, total_bytes_received);

        syslog(LOG_DEBUG, "Writing '%s' (%i bytes)", (char *)socket_buffer, total_bytes_received);

        if (nr == -1) {
            syslog(LOG_ERR,"Error writing data, %s", strerror(errno));
            cleanup();
            exit(EXIT_FAILURE);
        }

        int filesize = (int)lseek(file_fd, 0, SEEK_END);
        lseek(file_fd, 0, SEEK_SET);

        char *rdfile = malloc(filesize);

        if (read(file_fd, rdfile, filesize) == -1) {
            syslog(LOG_ERR,"Error reading file, %s\n", strerror(errno));
            cleanup();
            exit(EXIT_FAILURE);
        }

        int bytes_sent = send(client_socket, rdfile, filesize, 0);

        if (bytes_sent == -1) {
            syslog(LOG_ERR,"Error sending data, %s\n", strerror(errno));
            cleanup();
            free(rdfile);
            exit(EXIT_FAILURE);
        }
        free(rdfile);

        syslog(LOG_DEBUG, "Sent %i bytes from file to client\n", bytes_sent);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));

        if (close(client_socket) == -1) { // Close client socket after handling the connection
            syslog(LOG_ERR, "Error closing client socket: %s", strerror(errno));
            cleanup();
            exit(EXIT_FAILURE);
        }
        client_socket = -1;
    }

    cleanup(); // Cleanup server resources only when exiting the loop
    free(socket_buffer);
    return 0;
}
