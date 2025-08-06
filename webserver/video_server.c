//SIMPLIFIED VIDEO STREAMING WEBSERVER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "video_index.h"
#include "strops.h"

#define PORT 8080
#define BUFFER_SIZE 8192
#define WEBROOT "./main_pages"
#define MAX_THREADS 20

// ========================= THREADING SYSTEM =========================
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

typedef struct {
    int socket;
    struct client_request *next;
} client_request;

client_request *queue_head = NULL;
client_request *queue_tail = NULL;

void add_client(int socket) {
    printf("(CODE pt1) adding new client\n");
    client_request *new_client = malloc(sizeof(client_request));
    new_client->socket = socket;
    new_client->next = NULL;

    pthread_mutex_lock(&queue_mutex);
    if (queue_tail == NULL) {
        queue_head = queue_tail = new_client;
    } else {
        queue_tail->next = new_client;
        queue_tail = new_client;
    }
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

int get_client() {
    printf("(CODE pt2) removing current client\n");
    pthread_mutex_lock(&queue_mutex);
    while (queue_head == NULL) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }
    
    client_request *client = queue_head;
    int socket = client->socket;
    queue_head = client->next;
    if (queue_head == NULL) queue_tail = NULL;
    
    pthread_mutex_unlock(&queue_mutex);
    free(client);
    return socket;
}

void* worker_thread(void* arg) {
    printf("(CODE init2) starting worker thread\n");
    while (1) {
        int socket = get_client();
        handle_client_request(socket);
    }
    return NULL;
}

void start_threads() {
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, worker_thread, NULL);
        pthread_detach(thread);
    }
    printf("(CODE init1) started %d worker threads\n", MAX_THREADS);
}

// ========================= MAIN SERVER =========================
int main() {
    signal(SIGPIPE, SIG_IGN);
    
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket failed");
        exit(1);
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("(CODE init0) video Server started on port %d\n", PORT);
    start_threads();
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_socket >= 0) {
            printf("\n\n\n(CODE pt0) accepting client\n");
            add_client(client_socket);
        }
    }
    
    close(server_socket);
    return 0;
}

// ========================= SIMPLE HTTP HELPERS =========================
void send_simple_response(int socket, char *status, char *content_type, char *body) {
    printf("(CODE utils_1) sending http header of type: %s\n", status);
    char response[BUFFER_SIZE];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s", status, content_type, strlen(body), body);
    send(socket, response, len, 0);
}

void send_404(int socket) {
    printf("(CODE utils_2) 404 occured\n");
    char *body = "<html><body style='background-color:black;color:white;'>"
                 "<h1>404 - Video Not Found</h1></body></html>";
    send_simple_response(socket, "404 Not Found", "text/html", body);
}

// ========================= HTML FILE SERVING =========================
void serve_html(int socket, char *file_path) {
    printf("(CODE html_1) sending html file: %s\n", file_path);

    FILE *file = fopen(file_path, "r");
    if (!file) {
        send_404(socket);
        return;
    }
    
    char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    send(socket, header, strlen(header), 0);
    
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file)) {
        send(socket, buffer, strlen(buffer), 0);
    }
    fclose(file);
}

// ========================= VIDEO STREAMING =========================
void stream_video(int socket, char *video_path, char *headers) {
    printf("(CODE vid0) preparing to stream: %s\n", video_path);
    FILE *file = fopen(video_path, "rb");
    if (!file) {
        send_404(socket);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //range request for video seeking
    long start = 0, end = file_size - 1;
    char *range = strstr(headers, "Range: bytes=");
    if (range) {
        range += 13; //skip "Range: bytes="
        sscanf(range, "%ld-%ld", &start, &end);
        if (end <= 0 || end >= file_size) end = file_size - 1;
        fseek(file, start, SEEK_SET);
    }

    //headers
    char response[1024];
    char *content_type = strstr(video_path, ".mkv") ? "video/x-matroska" : "video/mp4";
    
    if (range) {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 206 Partial Content\r\n"
                 "Content-Type: %s\r\n"
                 "Accept-Ranges: bytes\r\n"
                 "Content-Range: bytes %ld-%ld/%ld\r\n"
                 "Content-Length: %ld\r\n\r\n",
                 content_type, start, end, file_size, (end - start + 1));
    } else {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Accept-Ranges: bytes\r\n"
                 "Content-Length: %ld\r\n\r\n",
                 content_type, file_size);
    }
    send(socket, response, strlen(response), 0);

    //actually streaming
    printf("(CODE init_vid) starting stream\n");
    char buffer[BUFFER_SIZE];
    long bytes_left = end - start + 1;
    while (bytes_left > 0) {
        int chunk_size = (bytes_left < BUFFER_SIZE) ? bytes_left : BUFFER_SIZE;
        int bytes_read = fread(buffer, 1, chunk_size, file);
        if (bytes_read <= 0) break;
        
        if (send(socket, buffer, bytes_read, MSG_NOSIGNAL) <= 0) break;
        bytes_left -= bytes_read;
    }
    fclose(file);
}

void serve_video_player(int socket, char *video_path) {
    printf("(CODE vid1) using video player\n");
    char player_html[2048];
    char *filename = strrchr(video_path, '/');
    if (filename) filename++; else filename = video_path;
    
    snprintf(player_html, sizeof(player_html),
             "<!DOCTYPE html>\n"
             "<html>\n<head><title>Video Player</title></head>\n"
             "<body style='background-color:black; color:white; text-align:center;'>\n"
             "<h2>Now Playing: %s</h2>\n"
             "<a href='/' style='color:powderblue;'>← Home</a><br><br>\n"
             "<video width='1280' height='720' controls>\n"
             "<source src='%s' type='video/mp4'>\n"
             "</video>\n</body></html>",
             filename, video_path);
    
    send_simple_response(socket, "200 OK", "text/html", player_html);
}

// ========================= MAIN REQUEST HANDLER =========================
void handle_client_request(int socket) {
    printf("(CODE main) handling request\n");
    char buffer[BUFFER_SIZE];
    char method[16], path[512];
    
    int bytes_read = read(socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(socket);
        return;
    }
    buffer[bytes_read] = '\0';
    
    sscanf(buffer, "%s %s", method, path);
    printf("Request: %s %s\n", method, path);
    
    if (strcmp(method, "GET") != 0) {
        send_404(socket);
        close(socket);
        return;
    }
    //default to home
    if (strcmp(path, "/") == 0) {
        char home_path[256];
        snprintf(home_path, sizeof(home_path), "%s/index.html", WEBROOT);
        serve_html(socket, home_path);
    }
    
    //searching
    else if (strstr(path, "/search?movies=") || strstr(path, "/search?television=")) {
        char output[BUFFER_SIZE];
        char search_term[256] = "";
        char *folder_type = strstr(path, "movies") ? "movies" : "television";
        char *equals = strchr(path, '=');
        if (equals) {
            strcpy(search_term, equals + 1);
            char *amp = strchr(search_term, '&');
            if (amp) *amp = '\0';
        }
        
        build_video_index(search_term, folder_type, output, sizeof(output));
        send_simple_response(socket, "200 OK", "text/html", output);
    }
    
    //browsing television (since it has nested directories)
    else if (strstr(path, "/television/") && !strstr(path, ".mp4") && !strstr(path, ".mkv")) {
        char dir_path[512];
        strcpy(dir_path, path + 1); // Remove leading slash
        char output[BUFFER_SIZE];
        build_directory_listing(dir_path, output, sizeof(output));
        send_simple_response(socket, "200 OK", "text/html", output);
    }
    
    //direct video streaming
    else if (strstr(path, ".mp4") || strstr(path, ".mkv")) {
        char video_path[512];
        strcpy(video_path, path + 1); //skip slash +1
        
        //check if browser is requesting video stream or player page
        if (strstr(buffer, "Accept: text/html")) {
            serve_video_player(socket, path);
        } else {
            stream_video(socket, video_path, buffer);
        }
    }
    
    //html files
    else if (strstr(path, ".html")) {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s%s", WEBROOT, path);
        serve_html(socket, file_path);
    }
    
    //everything else is 404
    else {
        send_404(socket);
    }
    
    close(socket);
}
