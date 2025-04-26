#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h> // Include syslog header
#include <signal.h> // Include signal header

#define PORT 9000
#define BACKLOG 10
#define BUFFER_SIZE 1024 * 1024
#define SOCKET_ERROR -1

volatile sig_atomic_t stop_server = 0; // Flag to indicate server should stop

void handle_signal(int signal) {
    (void)signal; // Mark parameter as unused to suppress warning
    syslog(LOG_INFO, "Caught signal, exiting"); // Log signal caught
    stop_server = 1; // Set flag to stop the server
}

int main() {
    // Register signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    const char *out_file = "/var/tmp/aesdsocketdata";

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(SOCKET_ERROR);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(SOCKET_ERROR);
    }

    if (listen(server_socket, BACKLOG) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(SOCKET_ERROR);
    }

    printf("Server is listening on port %d\n", PORT);
    openlog("aesdsocket", LOG_PID, LOG_USER); // Open syslog

    while (!stop_server) { // Loop until stop_server is set
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (stop_server) break; // Exit loop if interrupted by signal
            perror("Accept failed");
            continue;
        }

        printf("Accepted connection from %s\n", inet_ntoa(client_addr.sin_addr));
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr)); // Log accepted connection
        
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        FILE *file = fopen(out_file, "a+");

        if (file == NULL) {
            perror("File open failed");
            close(server_socket);
            exit(SOCKET_ERROR);
        }

        while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0) {
            ssize_t start = 0;
            for (size_t i = 0; i < (size_t)bytes_read; i++) {
                if (buffer[i] == '\n') {
                    buffer[i + 1] = '\0'; 
                    if (fwrite(&buffer[start], 1, i - start + 1, file) < i - start + 1) {
                        perror("File write failed");
                        close(client_socket);
                        fclose(file);
                        exit(SOCKET_ERROR);
                    }
                    start = i + 1;

                    // Send the full content of the file back to the client
                    fflush(file); // Ensure all data is written to the file
                    fseek(file, 0, SEEK_SET); // Rewind the file to the beginning
                    char send_buffer[BUFFER_SIZE];
                    size_t bytes_to_send;
                    while ((bytes_to_send = fread(send_buffer, 1, sizeof(send_buffer), file)) > 0) {
                        if (write(client_socket, send_buffer, bytes_to_send) < 0) {
                            perror("Write to client failed");
                            close(client_socket);
                            fclose(file);
                            exit(SOCKET_ERROR);
                        }
                    }
                }
            }
        }

        if (bytes_read < 0) {
            perror("Read failed");
        }

        close(client_socket);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr)); // Log closed connection
        fclose(file);
    }

    printf("Shutting down server...\n");
    syslog(LOG_INFO, "Server shutting down");

    // Delete the file /var/tmp/aesdsocketdata
    if (remove(out_file) != 0) {
        perror("Failed to delete file");
        syslog(LOG_ERR, "Failed to delete file %s", out_file);
    } else {
        syslog(LOG_INFO, "Deleted file %s", out_file);
    }

    closelog(); // Close syslog
    close(server_socket);
    return 0;
}