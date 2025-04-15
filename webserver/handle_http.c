#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "handle_files.c"
#include "strops.c"

#define PORT 8080
#define RESERVED 8192
#define WEBROOT "./main_pages"
#define MAX_THREADS 20  // Maximum number of worker threads

/*
ALL MULTI-THREADING WAS DONE BY AI - CLAUDE 3.7

I also had it refine my comments. Nothing else was touched.
*/

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
int thread_count = 0;

// Client request queue
typedef struct client_request {
    int client_socket;
    struct client_request *next;
} client_request_t;

client_request_t *request_queue_head = NULL;
client_request_t *request_queue_tail = NULL;

void queue_add(int client_socket) {
    client_request_t *request = malloc(sizeof(client_request_t));
    request->client_socket = client_socket;
    request->next = NULL;

    pthread_mutex_lock(&queue_mutex);
    
    if (request_queue_tail == NULL) {
        request_queue_head = request_queue_tail = request;
    } else {
        request_queue_tail->next = request;
        request_queue_tail = request;
    }
    
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

// Get the next client request from the queue
int queue_get() {
    int client_socket = -1;
    
    pthread_mutex_lock(&queue_mutex);
    
    while (request_queue_head == NULL) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    
    client_request_t *request = request_queue_head;
    client_socket = request->client_socket;
    request_queue_head = request->next;
    
    if (request_queue_head == NULL) {
        request_queue_tail = NULL;
    }
    
    pthread_mutex_unlock(&queue_mutex);
    free(request);
    
    return client_socket;
}

// For video streaming
void send_video(int client_socket, const char *video_path, const char *header_buffer) {
    printf("STEP 4: IN SEND VIDEO\nVIDEO %s\n", video_path);
    FILE *file = fopen(video_path, "rb");

    if (!file) {
        printf("\n###############\nno file at path\n###############\n");
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/html\r\n\r\n"
                                "<html><body>video file does not exist<html><body>";
        write(client_socket, not_found, strlen(not_found));
        return;
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


void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    char response[RESERVED];
    printf("STEP 4: IN SEND RESPONSE\n");
    int length = snprintf(response, sizeof(response),
    "HTTP/1.1 %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n"
    "Connection: close\r\n\r\n"
    "%s", status, content_type, strlen(body), body);

    send(client_socket, response, length, 0);
}


void send_file(int client_socket, const char *filepath) {
    printf("STEP 2: IN SEND FILE %s\n", filepath);
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
    printf("reached buffer\n");
    while (fgets(buffer, sizeof(buffer), file)) {
        write(client_socket, buffer, strlen(buffer));
    }
    printf("\n\nBUFFER %s\n\n", buffer);
    fclose(file);
}


void prep_file_index(int client_socket, char *query) { //rebuild
    char output[RESERVED];
    printf("STEP 2: IN PREP FILE INDEX\nQUERY %s\n", query);
    char *item = strstr(query, "?");
    if (!item) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query Code: 001</h1>");
        return;
    }
    item++;
    char *folder_end = strstr(item, "=");
    if (!folder_end) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query Code: 002</h1>");
        return;
    }
    *folder_end = '\0';
    printf("FOLDER %s\n", item);
    char *input = folder_end + 1;
    char *input_end = strstr(input, "&");
    if (input_end) {
        *input_end = '\0';
    }
    printf("INPUT %s\n", input);
    index_folder(input, item, output, sizeof(output));
    send_response(client_socket, "200 OK", "text/html", output);
}

void prep_video(int client_socket, char *path) {
    size_t path_size = sizeof(path);
    char output[RESERVED];
    printf("STEP 2: IN PREP VIDEO\nQUERY %s\n", path);
    char *folder = strstr(path, "/");
    if (!folder) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query Code: 003</h1>");
        return;
    }
    folder++; // moves to "player/"
    char *folder_end = strstr(folder, "/");
    if (!folder_end) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query Code: 004</h1>");
        return;
    }
    *folder_end = '\0'; // "player"
    printf("FOLDER %s\n", folder);
    char *page = folder_end + 1; // "player.html?video.mp4
    char *page_end = strstr(page, "?");
    if (!page_end) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query Code: 005</h1>");
        return;
    }
    *page_end = '\0'; // "player.html"
    char *video = page_end + 1; // "video.mp4"
    printf("PAGE %s\nVIDEO %s\nMOVIE %s\n", page, video, path);

    
    char bo[RESERVED] =
    "<html>\r\n<body style='background-color:white;'>\r\n"
    "<p>You are watching: ";
    strcat(bo, video);
    strcat(bo, "<a href='/' style='color:powderblue;'><h2>Home</h2></a></p>\r\n"
    "<video width='1280' height='720' controls>\r\n"
    "<source src='\r");
    
    char dy[RESERVED] = 
    "' type='video/mp4'>\r\n"
    "</video></body></html>\r\n\r\n";
    
    strcat(bo, folder);
    strcat(bo, "/");
    strcat(bo, video);
    strcat(bo, dy);
    printf("BO-DY %s\n", bo);
    send_response(client_socket, "200 OK", "text/html", bo);
}


void download_file(int client_socket, const char *path, const char *name, const char *header_buffer) {
    printf("STEP 4: IN SEND FILE\nFILE %s\n", path);
    FILE *file = fopen(path, "rb");
    char content_type[32];
    if (find(path, "epub") != 0) {
        strcpy(content_type, "application/epub+zip");
    } else if (find(path, "pdf") != 0) {
        strcpy(content_type, "application/pdf");
    } else if (find(path, "txt") != 0) {
        strcpy(content_type, "text/plain");
    } else {
        fclose(file);
    }
    if (!file) {
        printf("\n###############\nno file at path\n###############\n");
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/html\r\n\r\n"
                                "<html><body>file does not exist<html><body>";
        write(client_socket, not_found, strlen(not_found));
        return;
    }
    
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
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
                 "Content-Type: %s\r\n"
                 "Content-Disposition: attachment; filename=\'%s\'\r\n"
                 "Accept-Ranges: bytes\r\n"
                 "Content-Range: bytes %ld-%ld/%ld\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: keep-alive\r\n\r\n",
                 name, content_type, start, end, file_size, (end - start + 1));
    } else {    
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Disposition: attachment; filename=\'%s\'\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: keep-alive\r\n\r\n",
                 name, content_type, file_size);
    }
    send(client_socket, response_header, strlen(response_header), 0);
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        int sent = send(client_socket, buffer, bytes_read, 0);
        if (sent == -1) {
            perror("Disconnect");
            break;
        }
    }
    fclose(file);
}


void handle_client(int client_socket) {
    char buffer[RESERVED];
    char movie_path[612];
    printf("STEP 1: IN PREP FILE INDEX\n");
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    buffer[bytes_read] = '\0';

    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);
    dcopy(path, movie_path);

    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    } else if (find(path, "television") != 0 || find(path, "movies") != 0 || find(path, "library") != 0) {
        dstrip(path, "/", 1);
    }
    printf("\nBUFFER %s\nMETHOD %s\nPATH %s\n", buffer, method, path);
    
    if (strstr(buffer, "GET /search?movies") || strstr(buffer, "GET /search?television")) {
        prep_file_index(client_socket, path);
    
    } else if (strstr(buffer, "GET /search?library")) {
        prep_file_index(client_socket, path);
    
    } else if (strstr(buffer, "GET /television/") && !strstr(buffer, ".mp4")) {
        char output[RESERVED];
        index_folder("", path, output, sizeof(output));
        send_response(client_socket, "200 OK", "text/html", output);
    
    } else if (strstr(buffer, ".epub") || strstr(buffer, ".pdf") || strstr(buffer, ".txt") || strstr(buffer, ".py") || strstr(buffer, ".c")) {
        char *name = pstrip(path, "library/", 1);
        download_file(client_socket, path, name, buffer);
        free(name);
    } else if (strstr(buffer, "GET /player/player.html")) {
        prep_video(client_socket, path);
    
    } else if (strstr(buffer, ".mp4")) {
            printf("Direct video request - processing: %s\n", path);
            // Check if the file exists before trying to send it
            FILE *check = fopen(path, "rb");
            if (check) {
                fclose(check);
                send_video(client_socket, path, buffer);
            } else {
                printf("File not found: %s\n", path);
                send_response(client_socket, "404 Not Found", "text/html", 
                        "<html><body>Video file not found Code: 006</body></html>");
            }
    } else {
        char filepath[RESERVED];
        snprintf(filepath, sizeof(filepath), "%s%s", WEBROOT, path);
        send_file(client_socket, filepath);
    }
    
    close(client_socket);
}

void *worker_thread(void *arg) {
    while (1) {
        int client_socket = queue_get();
        if (client_socket >= 0) {
            handle_client(client_socket);
        }
    }
    return NULL;
}

void init_thread_pool(int num_threads) {
    pthread_t thread_id;
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&thread_id, NULL, worker_thread, NULL) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);
        thread_count++;
    }
    
    printf("Thread pool initialized with %d threads\n", thread_count);
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
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
    
    init_thread_pool(MAX_THREADS);
    
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("Client connecting: %d\n", client_socket);
        
        queue_add(client_socket);
    }
    
    close(server_fd);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    
    return 0;
}
