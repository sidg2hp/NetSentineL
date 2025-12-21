#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "proxy_filter.h"

#define CONFIG_FILE "config/blocked_domains.txt"

int is_blocked(char *host) {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        // If config is missing, default to allow
        return 0; 
    }

    char line[256];
    int blocked = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0; // Strip newlines

        if (strcmp(line, host) == 0) {
            blocked = 1;
            break;
        }
    }

    fclose(file);
    return blocked;
}