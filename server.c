#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void log_connection(const char *client_ip, const char *message) {
    FILE *log_file = fopen("/tmp/tcp-server.log", "a");
    if (log_file == NULL) {
        perror("log failed");
        return;
    }

    // Get current timestamp
    time_t now = time(NULL);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0'; // remove newline from ctime

    fprintf(log_file, "[%s] %s - %s\n", timestamp, client_ip, message);
    fclose(log_file);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    
    // Read client message
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
    if (bytes_read < 0) {
        perror("read failed");
        close(client_fd);
        return;
    }
    
    log_connection("client", buffer);
    printf("[Child PID %d] Received: %s\n", getpid(), buffer);
    
    // Send response
    char response[BUFFER_SIZE*2];
    snprintf(response, sizeof(response), 
             "Hello from server! Your message was: %s\n", buffer);
    write(client_fd, response, strlen(response));
    
    close(client_fd);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Allow port reuse — avoids "Address already in use" error
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[Server PID %d] Listening on port %d...\n", getpid(), PORT);

    // Reap zombie child processes automatically
    signal(SIGCHLD, SIG_IGN);

    // Infinite loop — keep accepting connections
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&address,
                              (socklen_t *)&addrlen);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // Get client IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
        log_connection(client_ip, "connected");
        printf("[Parent PID %d] New connection from %s\n", getpid(), client_ip);

        // Fork a child to handle this client
        pid_t pid = fork();

        if (pid == 0) {
            // Child process — handle the client
            close(server_fd); // child doesn't need the server socket
            handle_client(client_fd);
            exit(0);
        } else if (pid > 0) {
            // Parent process — go back to accepting
            close(client_fd); // parent doesn't need this client socket
        } else {
            perror("fork failed");
        }
    }

    close(server_fd);
    return 0;
}