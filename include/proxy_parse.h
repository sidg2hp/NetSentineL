#ifndef PROXY_PARSE_H
#define PROXY_PARSE_H

typedef struct {
    char method[16];      // GET, POST, CONNECT
    char host[256];       // www.example.com
    int port;             // 80
    char path[1024];      // /index.html
    char version[16];     // HTTP/1.1
} ParsedRequest;

// Function prototype
ParsedRequest parse_request(char *request_buffer);

#endif