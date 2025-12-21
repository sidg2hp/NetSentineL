#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "proxy_parse.h"

ParsedRequest parse_request(char *request_buffer) {
    ParsedRequest req = {0};
    char full_url[1024];
    
    // 1. Read Request Line: METHOD URL VERSION
    if (sscanf(request_buffer, "%15s %1023s %15s", req.method, full_url, req.version) != 3) {
        return req;
    }

    strcpy(req.path, full_url);

    // 2. Parse URL to extract HOST and PORT
    char *host_start = full_url;
    char *protocol_end = strstr(full_url, "://");
    
    if (protocol_end) {
        host_start = protocol_end + 3;
    }
    
    char *path_start = strchr(host_start, '/');
    if (path_start) {
        int host_len = path_start - host_start;
        strncpy(req.host, host_start, host_len);
        req.host[host_len] = '\0';
    } else {
        strcpy(req.host, host_start);
    }

    // 3. Check for Port in Host
    char *port_start = strchr(req.host, ':');
    if (port_start) {
        *port_start = '\0';
        req.port = atoi(port_start + 1);
    } else {
        req.port = 80; // Default HTTP port
    }

    return req;
}