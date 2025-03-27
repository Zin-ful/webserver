#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>


void index_folder (const char *query, const char *dir, char *output, size_t max_output) {
    DIR *current_dir = opendir(*dir);
    if (!dir) {
    snprintf(output, max_output, "<h1>Error: Directory not found</h1>");
    return;
    }
    struct dirent *entry;
    snprintf(output, max_output, "<h3>result: </h3><ul>");
    while ((entry = readdir(current_dir))) {
        if (strstr(entry->d_name, query)) {
            strncat(output, "<li>", max_output - strlen(output) - 1);
            strncat(output, entry->d_name, max_output - strlen(output) - 1);
            strncat(output, "</li>", max_output - strlen(output) - 1);
        }
    }
    strncat(output, "</ul>", max_output - strlen(output) - 1);
    closedir(dir);
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
