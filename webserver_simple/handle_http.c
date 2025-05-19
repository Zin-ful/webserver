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

#define PORT 80
#define BUFFER_SIZE 8192
#define WEBROOT "./main_pages"
#define MAX_THREADS 20  // Maximum number of worker threads

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
int thread_count = 0;
int start = 0;

typedef struct client_request {
    int client_socket;
    struct client_request *next;
} client_request_t;

client_request_t *request_queue_head = NULL;
client_request_t *request_queue_tail = NULL;

void queue_add(int client_socket) {
    client_request_t *request = malloc(sizeof(client_request_t));
    printf("\nSETUP: Adding client to thread queue\n");
    if (!request) {
        perror("Failed to allocate memory for request");
        close(client_socket);
        return;
    }

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

int queue_get() {
    if (start) {
        printf("\nSETUP: Removing client from queue\n");
    }
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

void *worker_thread(void *arg) {
    if (start) {
        printf("\nSETUP: Thread started\n");
    }
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
    printf("\nSETUP: Initilazing threads\n");
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

void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    char response[BUFFER_SIZE];
    int length = snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s", status, content_type, strlen(body), body);

    send(client_socket, response, length, 0);
}

void send_not_found(int client_socket, const char *message) {
    printf("ERROR: Requested page that doesnt exist.\n");
    const char *not_found_message = message ? message : 
        "<html><body style='background-color:black;'><p style='color:white;'>Nice try bucko but that page doesn't exist</p></body></html>";

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s", strlen(not_found_message), not_found_message);

    write(client_socket, response, strlen(response));
}

void send_bad_request(int client_socket, const char *error_code) {
    printf("ERROR: Incorrect request format (likely a server-side issue)\n");
    char message[100];
    snprintf(message, sizeof(message), "<h1>Invalid Query Code: %s</h1>", error_code);
    send_response(client_socket, "400 Bad Request", "text/html", message);
}

void send_file(int client_socket, const char *filepath) {
    printf("    STEP TWO: Sending HTML page from 'Main Pages'\n");
    FILE *file = fopen(filepath, "r");
    if (!file) {
        send_not_found(client_socket, NULL);
        return;
    }

    const char *header = "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n";
    write(client_socket, header, strlen(header));

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file)) {
        write(client_socket, buffer, strlen(buffer));
    }

    fclose(file);
}

void send_video(int client_socket, const char *video_path, const char *header_buffer) {
    printf("    STEP THREE: Request for .mp4 file, parsing (for byte-range) and sending video data\n");
    FILE *file = fopen(video_path, "rb");

    if (!file) {
        send_not_found(client_socket, "<html><body>Video file does not exist</body></html>");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Parse Range header if present
    long start = 0, end = file_size - 1;
    char *range = NULL;

    if (header_buffer) {
        range = strstr(header_buffer, "Range: bytes=");
        if (range) {
            range += 13; // Move past "Range: bytes="
            sscanf(range, "%ld-%ld", &start, &end);
            if (end == 0 || end >= file_size) {
                end = file_size - 1;
            }
            fseek(file, start, SEEK_SET);
        }
    }

    // Prepare response header
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

    // Stream video content
    char buffer[BUFFER_SIZE];
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

void prep_video(int client_socket, char *path) {
    printf("    STEP TWO: Prepping video\n");
    char *folder = strstr(path, "/");
    if (!folder) {
        send_bad_request(client_socket, "003");
        return;
    }
    folder++; // moves to "player/"

    char *folder_end = strstr(folder, "/");
    if (!folder_end) {
        send_bad_request(client_socket, "004");
        return;
    }
    *folder_end = '\0'; // "player"

    char *page = folder_end + 1; // "player.html?video.mp4
    char *page_end = strstr(page, "?");
    if (!page_end) {
        send_bad_request(client_socket, "005");
        return;
    }
    *page_end = '\0'; // "player.html"
    char *video = page_end + 1; // "video.mp4"
    // Build the HTML response with video player
    char html_response[BUFFER_SIZE];
    snprintf(html_response, sizeof(html_response),
        "<html>\r\n<body style='background-color:black;'>\r\n"
        "<form action='/search' method='GET' style='color:white'>\r\n"
        "Search:\r\n"
        "<input type='text' name='movies'>\r\n"
        "<button type='submit'>Search</button>\r\n"
        "</form>\r\n"
        "<p style='color:white'>You are watching: %s"
        "<a href='/' style='color:powderblue;'><h2>Home</h2></a></p>\r\n"
        "<video style='width: 1080px; height: 720px;' controls>\r\n"
        "<source src='/stream?file=%s/%s' type='video/mp4'>\r\n"
        "</video></body></html>\r\n\r\n",
        video, folder, video);

    send_response(client_socket, "200 OK", "text/html", html_response);
}

void prep_file_index(int client_socket, char *query) {
    char output[BUFFER_SIZE];
    printf("    STEP TWO: Prepping file list\n");
    char *item = strstr(query, "?");
    if (!item) {
        send_bad_request(client_socket, "001");
        return;
    }
    item++;

    char *folder_end = strstr(item, "=");
    if (!folder_end) {
        send_bad_request(client_socket, "002");
        return;
    }
    *folder_end = '\0';

    char *input = folder_end + 1;
    char *input_end = strstr(input, "&");
    if (input_end) {
        *input_end = '\0';
    }
    // Generate directory listing HTML
    index_folder(input, item, output, sizeof(output));
    send_response(client_socket, "200 OK", "text/html", output);
}

void handle_client(int client_socket) {
    printf("    STEP ONE: Parsing and passing to functions...\n");
    char buffer[BUFFER_SIZE];
    char movie_path[512];

    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }

    buffer[bytes_read] = '\0';

    // Parse request method and path
    char method[256], path[256];
    sscanf(buffer, "%s %s", method, path);
    dcopy(path, movie_path);

    // Normalize path
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    } else if (find(path, "television") != 0 || find(path, "movies") != 0 || find(path, "library") != 0) {
        dstrip(path, "/", 1);
    }
    // Handle different request types
    if (strstr(buffer, "GET /search?movies") || strstr(buffer, "GET /search?television") ||
        strstr(buffer, "GET /search?library")) {
        prep_file_index(client_socket, path);
        
    } else if (strstr(buffer, "GET /television/") && !strstr(buffer, ".mp4")) {
        char output[BUFFER_SIZE];
        index_folder("", path, output, sizeof(output));
        send_response(client_socket, "200 OK", "text/html", output);

    } else if (strstr(buffer, "GET /player/player.html")) {
        prep_video(client_socket, path);
        
    } else if (strstr(buffer, ".mp4")) {
        send_video(client_socket, path, buffer);
        
    } else {
        char filepath[BUFFER_SIZE];
        snprintf(filepath, sizeof(filepath), "%s%s", WEBROOT, path);
        send_file(client_socket, filepath);
    }

    close(client_socket);
}

int main() {
    // Ignore SIGPIPE signals to prevent crashes when clients disconnect
    signal(SIGPIPE, SIG_IGN);

    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    // Create server socket
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

    // Bind socket to port
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

    // Initialize thread pool
    init_thread_pool(MAX_THREADS);

    // Main server loop
    start = 1;
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("STARTING: Request incoming from client: %d\n", client_socket);
        queue_add(client_socket);
    }

    // Clean up resources (this code is never reached in practice)
    close(server_fd);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);

    return 0;
}
