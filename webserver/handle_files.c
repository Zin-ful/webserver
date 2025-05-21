#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>


void index_folder (const char *query, const char *dir, char *output, size_t max_output) {
    printf("    STEP THREE: Creating file list from requested directory\n");
    DIR *current_dir = opendir(dir);
    if (!current_dir) {
        snprintf(output, max_output, "<h1>Error: Directory not found Code: 007</h1>");
        return;
    }
    struct dirent *entry;
    if (strstr(dir, "movie")) {
        snprintf(output, max_output, "<!DOCTYPE html>\r\n<html>\r\n<body style='background-color:black;'>\r\n"
        "<form action='/search' method='GET' style='color:white'>\r\n"
        "search:\r\n"
        "<input type='text' name='movies'>\r\n"
        "<button type='submit'>search</button>\r\n"
        "</form>\r\n"
        "<a href='/' style='color:powderblue;'><h2>Home</h2></a><h2 style='color:white'>result: </h2>\r\n<ul>");
     } else {
         snprintf(output, max_output, "<!DOCTYPE html>\r\n<html>\r\n<body style='background-color:black;'>\r\n"
        "<form style='color:white' action='/search' method='GET'>\r\n"
        "search:\r\n"
        "<input type='text' name='television'>\r\n"
        "<button type='submit'>search</button>\r\n"
        "</form>\r\n"
        "<a href='/' style='color:powderblue;'><h2>Home</h2></a><h2 style='color:white'>result: </h2>\r\n<ul>");
     }
    while ((entry = readdir(current_dir))) {
        if (strcmp(dir," movies") || strcmp(dir, " television")) {
            if (strstr(entry->d_name, query)) {
                if (strstr(entry->d_name, ".mp4") || strstr(entry->d_name, ".webm")) {
                    strncat(output, "<li><a style='color:powderblue;' href=", max_output - strlen(output) - 1);
                    strncat(output, dir, max_output - strlen(output) - 1);
                    strncat(output, "/", max_output - strlen(output) - 1);
                    strncat(output, entry->d_name, max_output - strlen(output) - 1);
                    strncat(output, ">", max_output - strlen(output) - 1);
                    char cache[256];
                    snprintf(cache, sizeof(cache), "%s", entry->d_name);
                    if (strstr(entry->d_name, ".mp4")) {
                        dstrip(cache, ".mp4", 1);
                    } else if (strstr(entry->d_name, ".webm")) {
                        dstrip(cache, ".webm", 1);
                    }
                    simple_replace(cache, "_", " ");
                    strncat(output, cache, max_output - strlen(output) - 1);
                    strncat(output, "</a></li>", max_output - strlen(output) - 1);
                } else if (entry->d_name != ".." || entry->d_name != ".") { //idk why the fuck this doesnt work but it doesnt
                        if (strstr(dir, "television/")) {
                            dstrip(dir, "television/", 1);
                        }                        strncat(output, "<li><a style='color:powderblue' href=", max_output - strlen(output) - 1);
                        strncat(output, dir, max_output - strlen(output) - 1);
                        strncat(output, "/", max_output - strlen(output) - 1);
                        strncat(output, entry->d_name, max_output - strlen(output) - 1);
                        strncat(output, ">", max_output - strlen(output) - 1);
                        char cache[256];
                        snprintf(cache, sizeof(cache), "%s", entry->d_name);
                        replace_all(cache, "_", " ");
                        strncat(output, cache, max_output - strlen(output) - 1);
                        strncat(output, "</a></li>", max_output - strlen(output) - 1);
              
                }
            }
        
        } else {
            if (strstr(entry->d_name, query)) {
                strncat(output, "<li><a style='color:powderblue;' download href=", max_output - strlen(output) - 1);
                strncat(output, dir, max_output - strlen(output) - 1);
                strncat(output, ">", max_output - strlen(output) - 1);
                strncat(output, entry->d_name, max_output - strlen(output) - 1);
                strncat(output, "</a></li>", max_output - strlen(output) - 1);
            }
        }
    }
    strncat(output, "</ul>\r\n</body>\r\n</html>\r\n", max_output - strlen(output) - 1);
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
