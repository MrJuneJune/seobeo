#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       
#include <sys/socket.h>
#include <netinet/in.h>   
#include <arpa/inet.h>   

// --- Custom Libs --- 
#include "lib/juneper.h"

#define PORT 6969 
#define BUFFER_SIZE 8192
#define RESPONSE_HEADER "HTTP/1.1 200 OK\r\n"\
  "Content-Type: text/html\r\n"\
  "Connection: close\r\n"\
  "\r\n"
#define GET_HEADER "GET "
#define REFERER_HEADER "Referer: "


int main() {
    // 1. Create socket
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char header_buffer[BUFFER_SIZE] = {0};
    char response_buffer[BUFFER_SIZE] = {0};
    char path[1024] = {0};
    char file_path[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Listen
    if (listen(server_fd, 1) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("HTTP Server listening on port %d...\n", PORT);

    while (1) {
        // 5. Accept new connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // 6. Read request (optional)
        memset(header_buffer, 0, BUFFER_SIZE);
        read(client_fd, header_buffer, BUFFER_SIZE - 1);
        printf("🔗 Request:\n%s\n", header_buffer);
        int start_pos = FindChildrenFromParentGeneric(
            header_buffer, BUFFER_SIZE,
            GET_HEADER, strlen(GET_HEADER),
            sizeof(char)
        );

        memset(path, 0, 1024);
        if (start_pos != -1) {
            ExtractPathFromReferer(header_buffer + start_pos, path);
            printf("📂 Referer Path: %s\n", path);
            printf("paths%s",path);
        } else {
            printf("⚠️  Referer header not found.\n");
        }

        // 7. Open HTML file
        memset(file_path, 0, 1024);
        if (strcmp(path, "/") == 0) {
            snprintf(file_path, sizeof(file_path), "paths/index.html");
        } else {
            snprintf(file_path, sizeof(file_path), "paths%s.html", path);
        }
        FILE *html_file = fopen(file_path, "r");
        if (!html_file) {
            printf("⚠️ Could not open %s", file_path);
            close(client_fd);
            continue;
        }

        // 8. Send headers
        send(client_fd, RESPONSE_HEADER, strlen(RESPONSE_HEADER), 0);

        // 9. Send file contents
        while (fgets(response_buffer, BUFFER_SIZE, html_file)) {
            send(client_fd, response_buffer, strlen(response_buffer), 0);
        }

        fclose(html_file);
        close(client_fd);
    }

    // 6. Cleanup
    close(client_fd);
    close(server_fd);
    return 0;
}
