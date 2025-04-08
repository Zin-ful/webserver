#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
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

// Thread pool variables
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

// Add a client request to the queue
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
    FILE *file = fopen(video_path, "rb");
        
    printf("VIDEO %s\n", video_path);
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

// Send response for other functions
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

// Sends HTML files
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
    printf("reached buffer\n");
    while (fgets(buffer, sizeof(buffer), file)) {
        write(client_socket, buffer, strlen(buffer));
    }
    printf("\n\nBUFFER %s\n\n", buffer);
    fclose(file);
}

// Preps the search query
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

// Function to prepare video player HTML
void prep_video(int client_socket, char *req, char *movie_path, size_t path_size) {
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
    char *page = folder_end + 1; // "player.html?video.mp4
    char *page_end = strstr(page, "?");
    if (!page_end) {
        send_response(client_socket, "400 Bad Request", "text/html","<h1>Invalid Query 005</h1>");
        return;
    }
    *page_end = '\0'; // "player.html"
    char *video = page_end + 1; // "video.mp4"
    printf("PAGE %s\n", page);
    printf("VIDEO %s\n", video);
    
    // Store the movie path in the provided buffer
    snprintf(movie_path, path_size, "movies/%s", video);
    printf("MOVIE %s\n", movie_path);
    
    char bo[RESERVED] =
    "<html>\r\n<body style='background-color:black;'>\r\n<p><a href='/' style='color:powderblue;'><h2>Home</h2></a></p>\r\n"
    "<video width='1280' height='720' controls>\r\n"
    "<source src='\r";

    char dy[RESERVED] = 
    "' type='video/mp4'>\r\n"
    "</video><html><body>\r\n\r\n";

    strcat(bo, folder);
    strcat(bo, "/");
    strcat(bo, video);
    strcat(bo, dy);
    printf("BO-DY %s\n", bo);
    send_response(client_socket, "200 OK", "text/html", bo);
}

// Main client handling function
void handle_client(int client_socket) {
    char buffer[RESERVED];
    char movie_path[612] = {0}; // Local buffer for movie path
    
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }
    
    buffer[bytes_read] = '\0';
    
    // Split into method and path
    char method[8], path[256];
    sscanf(buffer, "%s %s", method, path);
    dcopy(path, movie_path);
    // Default to index
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }
    
    printf("\nBUFFER %s\nMETHOD %s\nPATH %s\n", buffer, method, path);
    
    if (strstr(buffer, "GET /search?movies") || strstr(buffer, "GET /search?television")) {
        prep_file_index(client_socket, path);
    } else if (strstr(buffer, "GET /search?library")) {
        prep_file_index(client_socket, path);
    } else if (strstr(buffer, "GET /television/") && !strstr(buffer, ".mp4")) {
        char output[RESERVED];
        char item[RESERVED];

        psplit(path, "f", item, movie_path);
        printf("item: %s movie path: %s path: %s \n", item, movie_path, path);
        dstrip(path , "/", 1);
        index_folder("", path, output, sizeof(output));
        send_response(client_socket, "200 OK", "text/html", output);
    } else if (strstr(buffer, "GET /player/player.html")) {
        prep_video(client_socket, path, movie_path, sizeof(movie_path));
    } else if (strstr(buffer, ".mp4")) {
 // For direct MP4 requests
    char *video_filename = strrchr(path, '/'); // Get the filename portion after the last '/'
    
    if (video_filename) {
        video_filename++; // Skip the '/' character
        
        // Safely construct the movie path
        snprintf(movie_path, 2560, "movies/%s", video_filename);
        printf("Direct video request - processing: %s\n", movie_path);
        
        // Check if the file exists before trying to send it
        FILE *check = fopen(movie_path, "rb");
        if (check) {
            fclose(check);
            send_video(client_socket, movie_path, buffer);
        } else {
            printf("File not found: %s\n", movie_path);
            send_response(client_socket, "404 Not Found", "text/html", 
                     "<html><body>Video file not found</body></html>");
        }
    } else {
        send_response(client_socket, "400 Bad Request", "text/html", 
                     "<html><body>Invalid video path format</body></html>");
    }
    } else {
        char filepath[RESERVED];
        snprintf(filepath, sizeof(filepath), "%s%s", WEBROOT, path);
        send_file(client_socket, filepath);
    }
    
    close(client_socket);
}

// Worker thread function
void *worker_thread(void *arg) {
    while (1) {
        int client_socket = queue_get();
        if (client_socket >= 0) {
            handle_client(client_socket);
        }
    }
    return NULL;
}

// Initialize the thread pool
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
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    
    // Create socket
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
    
    // Configure address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server started on port %d\n", PORT);
    
    // Initialize thread pool
    init_thread_pool(MAX_THREADS);
    
    // Main accept loop
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        printf("Client connecting: %d\n", client_socket);
        
        // Add client to the request queue
        queue_add(client_socket);
    }
    
    // Clean up (although we never reach here in this example)
    close(server_fd);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    
    return 0;
}
