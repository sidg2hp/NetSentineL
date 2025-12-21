#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "proxy_parse.h"
#include "proxy_filter.h"

#define PORT 8888
#define BUFFER_SIZE 4096
#define LOG_FILE "proxy.log"

// --- Helper: Logging ---
void log_request(char *client_ip, char *request_url, int status_code) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp == NULL) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(fp, "[%s] Client: %s | Request: %s | Status: %d\n", time_str, client_ip, request_url, status_code);
    fclose(fp);
}

// --- Helper: Connect to Upstream ---
int connect_to_upstream(char *host, int port) {
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    if ((server = gethostbyname(host)) == NULL) return -1;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;
    return sock;
}

// --- Worker Thread ---
void *handle_client(void *args) {
    int client_socket = *((int*)args);
    char *client_ip = (char*)args + sizeof(int);
    
    char buffer[BUFFER_SIZE] = {0};
    int valread = read(client_socket, buffer, BUFFER_SIZE);

    if (valread > 0) {
        ParsedRequest req = parse_request(buffer);
        
        // 1. Filter Check
        if (is_blocked(req.host)) {
            printf("[Thread %ld] BLOCKED: %s\n", pthread_self(), req.host);
            log_request(client_ip, req.host, 403);

            char *forbidden = "HTTP/1.1 403 Forbidden\r\n\r\n<h1>403 Forbidden</h1><p>Blocked by Proxy</p>";
            send(client_socket, forbidden, strlen(forbidden), 0);
            
            close(client_socket);
            free(args);
            return NULL;
        }

        printf("[Thread %ld] Forwarding to %s:%d\n", pthread_self(), req.host, req.port);

        // 2. Forwarding Logic
        int upstream_socket = connect_to_upstream(req.host, req.port);
        
        if (upstream_socket != -1) {
            send(upstream_socket, buffer, valread, 0);

            char response[BUFFER_SIZE];
            int bytes;
            while ((bytes = read(upstream_socket, response, BUFFER_SIZE)) > 0) {
                send(client_socket, response, bytes, 0);
            }
            
            log_request(client_ip, req.host, 200);
            close(upstream_socket);
        } else {
            char *err = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
            send(client_socket, err, strlen(err), 0);
            log_request(client_ip, req.host, 502);
        }
    }

    close(client_socket);
    free(args);
    return NULL;
}

// --- Main Listener ---
int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Proxy Server (Multi-threaded & Filtered) listening on port %d...\n", PORT);

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        void *args = malloc(sizeof(int) + INET_ADDRSTRLEN);
        int *new_sock = (int*)args;

        if ((*new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("Accept failed");
            free(args);
            continue;
        }

        char *ip_str = (char*)args + sizeof(int);
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, args) < 0) {
            perror("Thread creation failed");
            free(args);
        }
        pthread_detach(thread_id);
    }
    return 0;
}