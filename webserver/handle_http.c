#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "handle_files.c"

#define PORT 8080

#define WEBROOT "./main_pages"


//for video streaming
void send_video(int client_socket, const char *video_path, const char *head) {
    FILE *file = fopen(video_path, "rb");
    if (!file) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body>video file does not exist<html><body>";
        write(client_socket, not_found, strlen(not_found));
        return;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    long start = 0, end = file_size - 1;
    if (head) {
        sscanf(head, "bytes=%ld-%ld", &start, &end);
        if (end == 0 || end >= file_size) end = file_size - 1;
        fseek(file, start, SEEK_SET);
    }
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 206 Partial Content\r\n"
        "Content-Type: video/mp4\r\n"
        "Accept-Ranges bytes\r\n"
        "Content-Range: bytes\r\n"
        "Connection: keep-alive", end - start + 1, start, end, file_size);
    send(client_socket, header, strlen(header), 0);

    //actual vid content
    char buffer[4096];
    long remaining = end - start + 1;
    while (remaining > 0) {
        size_t chunk_size = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
        size_t read_bytes = fread(buffer, 1, chunk_size, file);
        if (read_bytes < 0) break;
        send(client_socket, buffer, read_bytes, 0);
        remaining -= read_bytes;
    }
    fclose(file);
}



//send response is for other functions. like indexing a file system.
void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    char response[16284];
    int length = snprintf(response, sizeof(response),
    "HTTP/1.1 %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n"
    "Connection: close\r\n\r\n"
    "%s", status, content_type, strlen(body), body);

    send(client_socket, response, length, 0);
}

//sends html files
void send_file(int client_socket, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<html><body>Nice try bucko but that page doesnt exist<html><body>";
    write(client_socket, not_found, strlen(not_found));
    return;
    }
    const char *header = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n";
    write(client_socket, header, strlen(header));

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
        write(client_socket, buffer, strlen(buffer));
    }
    fclose(file);
}

//preps the search query
void prep_file_index(int client_socket, char *query) {
    char output[16284];
    char *param = strstr(query, "q=");
    if (param) {
        param += 2;
        char *end = strchr(param, "&");
        if (end) {
            *end = '\0';
        }
        char folder = param;
        index_folder(query, folder, output, sizeof(output));
        send_response(client_socket, "200 OK", "text/html", output);
    }
}

void handle_client(int client_socket) {
    char buffer[1024];
    read(client_socket, buffer, sizeof(buffer) - 1);
    //split
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);

    //default to index
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }
    printf("%s\n", buffer);
    if (strstr(buffer, "GET /search?movies")) {
        prep_file_index(client_socket, buffer);
    } else {

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s%s", WEBROOT, path);
        send_file(client_socket, filepath);
        close(client_socket);
    }
    //add request for media
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
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
    printf("Server started on port %d\n", PORT);
    while (1) {
        client_socket = accept(server_fd, (struct sockkaddr*)&address, &addr_len);
        printf("client connecting: %d\n", client_socket);
        handle_client(client_socket);

    }
    close(server_fd);
    return 0;
}
