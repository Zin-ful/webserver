#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>


void index_folder (const char *query, const char *dir, char *output, size_t max_output) {
    printf("\nDIR %s\nITEM %s\n", dir, query);
    DIR *current_dir = opendir(dir);
    if (!current_dir) {
    snprintf(output, max_output, "<h1>Error: Directory not found</h1>");
    return;
    }
    struct dirent *entry;
    snprintf(output, max_output, "<!DOCTYPE html>\r\n<html>\r\n<body style='background-color:black;'>\r\n<a href='/' style='color:powderblue;'><h2>Home</h2></a><h2>result: </h2>\r\n<ul>");
    while ((entry = readdir(current_dir))) {
        if (strcmp(dir," movies")) {
            if (strstr(entry->d_name, query)) {
                printf("File found in: %s\n%s\n", dir, entry->d_name);
                if (strstr(".mp4", entry->d_name)) {
                    strncat(output, "<li><a href=player/player.html? style='color:powderblue;'>", max_output - strlen(output) - 1);
                    strncat(output, entry->d_name, max_output - strlen(output) - 1);
                    strncat(output, ">", max_output - strlen(output) - 1);
                    strncat(output, entry->d_name, max_output - strlen(output) - 1);
                    strncat(output, "</a></li>", max_output - strlen(output) - 1);
                } else {
                    strncat(output, "<li><a href=television/", max_output - strlen(output) - 1);
                    strncat(output, entry->d_name, max_output - strlen(output) - 1);
                    strncat(output, ">", max_output - strlen(output) - 1);
                    strncat(output, entry->d_name, max_output - strlen(output) - 1);
                    strncat(output, "</a></li>", max_output - strlen(output) - 1);
                }
            }
        } else {
            if (strstr(entry->d_name, query)) {
                printf("true\n");
                strncat(output, "<li>", max_output - strlen(output) - 1);
                strncat(output, entry->d_name, max_output - strlen(output) - 1);
                strncat(output, "</li>", max_output - strlen(output) - 1);
            }
        }
    }
    strncat(output, "</ul>\r\n</body>\r\n</html>\r\n", max_output - strlen(output) - 1);
    printf("OUTPUT %s", output);
    closedir(current_dir);
}

void capture_suggest(const char *suggestion) {
    FILE *file = fopen("log/suggestion.txt", "a");
    if (!file) {
        perror("Suggestions log failure");
        return;
    }
    fprintf(file, "%s\n", suggestion);
    fclose(file);
}
