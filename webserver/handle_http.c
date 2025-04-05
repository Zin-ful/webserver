#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include "handle_files.c"

#define PORT 8080
#define RESERVED 8192
#define WEBROOT "./main_pages"
 char movie[128];
//for video streaming
void send_video(int client_socket, const char *video_path, const char *header_buffer) {

    FILE *file = fopen(video_path, "rb");
    printf("VIDEO %s\n", video_path);
    if (!file) {
        printf("\n###############\nno file at path\n###############\n");
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/html\r\n\r\n"
                                "<html><body>video file does not exist<html><body>";
        write(client_socket, not_found, strlen(not_found));
        return;  // Remove fclose(file) since file is NULL
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Parse Range header if present
    long start = 0, end = file_size - 1;
    char *range = NULL;
    
    if (header_buffer) {
        // Look for Range: bytes=X-Y in the header
        range = strstr(header_buffer, "Range: bytes=");
        if (range) {
            range += 13; // Move past "Range: bytes="
            sscanf(range, "%ld-%ld", &start, &end);
            if (end == 0 || end >= file_size) {
                end = file_size - 1;
            }
            printf("Range request: %ld-%ld\n", start, end);
            fseek(file, start, SEEK_SET);
        }
    }

    char response_header[1024];
    if (range) {
        // Send partial content response
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 206 Partial Content\r\n"
                 "Content-Type: video/mp4\r\n"
                 "Accept-Ranges: bytes\r\n"
                 "Content-Range: bytes %ld-%ld/%ld\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: keep-alive\r\n\r\n",
                 start, end, file_size, (end - start + 1));
    } else {
        // Send full content response
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: video/mp4\r\n"
                 "Accept-Ranges: bytes\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: keep-alive\r\n\r\n",
                 file_size);
    }
    
    send(client_socket, response_header, strlen(response_header), 0);

    // Actual video content
    char buffer[RESERVED];
    long remaining = end - start + 1;
    ssize_t result;

    while (remaining > 0) {
        size_t chunk_size = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
        size_t read_bytes = fread(buffer, 1, chunk_size, file);
        if (read_bytes <= 0) {
            break;  // End of file or error
        }

        result = send(client_socket, buffer, read_bytes, MSG_NOSIGNAL);
        if (result <= 0) {
            if (errno == EPIPE) {
                printf("Client disconnected (EPIPE error)\n");
            } else {
                perror("Error sending data");
            }
            break;
        }

        remaining -= read_bytes;
    }

    fclose(file);
}




//send response is for other functions. like indexing a file system.
void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    char response[RESERVED];
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
    printf("SENDING FILE %s\n", filepath);
    FILE *file = fopen(filepath, "r");
    if (!file) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<html><body style='background-color:black;'><p style='color:white;'>Nice try bucko but that page doesnt exist</p><html><body>";
        write(client_socket, not_found, strlen(not_found));
        return;
    }
    const char *header = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n";
    write(client_socket, header, strlen(header));

    char buffer[RESERVED];
        printf("reached buffer\n", buffer);
    while (fgets(buffer, sizeof(buffer), file)) {
        write(client_socket, buffer, strlen(buffer));
    }
    printf("\n\nBUFFER %s\n\n", buffer);
    fclose(file);
}

//preps the search query
void prep_file_index(int client_socket, char *query) {
    char output[RESERVED];
    printf("QUERY %s\n", query);
    char *param = strstr(query, "?");
    if (!param) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query 001</h1>");
        return;
    }
    param++;
    char *folder_end = strstr(param, "=");
    if (!folder_end) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query 002</h1>");
        return;
    }
    *folder_end = '\0';
    printf("FOLDER %s\n", param);
    char *input = folder_end + 1;
    char *input_end = strstr(input, "&");
    if (input_end) {
        *input_end = '\0';
    }
    printf("INPUT %s\n", input);
    index_folder(input, param, output, sizeof(output));
    send_response(client_socket, "200 OK", "text/html", output);
}


void prep_video(int client_socket, char *req) {
    char output[RESERVED];
    printf("QUERY %s\n", req);
    char *folder = strstr(req, "/");
    if (!folder) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query 003</h1>");
        return;
    }
    folder++; // moves to "player/"
    char *folder_end = strstr(folder, "/");
    if (!folder_end) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query 004</h1>");
        return;
    }
    *folder_end = '\0'; // "player"
    printf("FOLDER %s\n", folder);
    char *page = folder_end + 1; // "player.hmtl?video.mp4
    char *page_end = strstr(page, "?");
    *page_end = '\0'; // "player.html"
    char *video = page_end + 1; // "video.mp4"
    printf("PAGE %s\n", page);
    printf("VIDEO %s\n", video);
    snprintf(movie, sizeof(movie), "movies/%s", video);
    printf("MOVIE %s\n", movie);
    char bo[RESERVED] =
    "<html><body style='background-color:black;'><video width='1280' height='720' controls>\r\n"
    "<source src='\r";

    char dy[RESERVED] = 
    "' type='video/mp4'>\r\n"
    "</video><html><body>\r\n\r\n";

    strcat(bo, folder);
    strcat(bo, "/");
    strcat(bo, video);
    strcat (bo, dy);
    printf("BO-DY %s\n", bo);
    send_response(client_socket, "200 OK", "text/html", bo);
}

void handle_client(int client_socket) {
    char buffer[RESERVED];
    read(client_socket, buffer, sizeof(buffer) - 1);
    // Split into method and path
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);
    // Default to index
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }
    printf("\nBUFFER %s\nMETHOD %s\nPATH %s\n", buffer, method, path);
    if (strstr(buffer, "GET /search?movies")) {
        prep_file_index(client_socket, path);
    } else if (strstr(buffer, "GET /search?library")) {
        prep_file_index(client_socket, path);
    } else if (strstr(buffer, "GET /player/player.html")) {
        prep_video(client_socket, path);
    } else if (strstr(buffer, ".mp4")) {
        // Pass the entire header buffer to handle Range requests
        send_video(client_socket, movie, buffer);
    } else {
        char filepath[RESERVED];
        snprintf(filepath, sizeof(filepath), "%s%s", WEBROOT, path);
        send_file(client_socket, filepath);
    }
    close(client_socket);
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
