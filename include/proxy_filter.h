#ifndef PROXY_FILTER_H
#define PROXY_FILTER_H

// Returns 1 if the host is blocked, 0 if allowed
int is_blocked(char *host);

#endif