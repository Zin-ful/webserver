#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "video_index.h"
#include "strops.h"

int is_video_file(const char *filename) {
    return (strstr(filename, ".mp4") != NULL || strstr(filename, ".mkv") != NULL);
}

void build_video_index(const char *search_term, const char *folder_type, char *output, int max_size) {
    printf("(CODE video_index.c - html_0) building search index\n");
    DIR *dir = opendir(folder_type);
    if (!dir) {
        snprintf(output, max_size, "<h1>Error: Directory '%s' not found</h1>", folder_type);
        return;
    }
    char search_form[512];
    snprintf(search_form, sizeof(search_form),
        "<form action='/search' method='GET' style='color:white'>\n"
        "Search: <input type='text' name='%s' value='%s'>\n"
        "<button type='submit'>Search</button>\n"
        "</form>", folder_type, search_term);
    
    snprintf(output, max_size, 
        "<!DOCTYPE html>\n<html>\n"
        "<body style='background-color:black; color:white;'>\n"
        "%s\n"
        "<a href='/' style='color:powderblue;'><h2>← Home</h2></a>\n"
        "<h2>Search Results:</h2>\n<ul>",
        search_form);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        //skip hidden files and directories
        if (entry->d_name[0] == '.') continue;
        
        //check if entry matches search term if provided
        if (strlen(search_term) > 0 && !strstr(entry->d_name, search_term)) continue;
        
        if (is_video_file(entry->d_name)) {
            printf("(CODE video_index.c - html_1) adding movie to list..\n");
            char clean_name[256];
            strcpy(clean_name, entry->d_name);
            char *dot = strrchr(clean_name, '.');
            if (dot) *dot = '\0';
            simple_replace(clean_name, "_", " ");
            char link[1024];
            snprintf(link, sizeof(link), 
                "<li><a style='color:powderblue;' href='/%s/%s'>%s</a></li>",
                folder_type, entry->d_name, clean_name);
            
            if (strlen(output) + strlen(link) < max_size - 100) {
                strcat(output, link);
            }
        } else {
            printf("(CODE video_index.c - html_2) adding series to list..\n");
            // Directory - create browse link (for TV shows)
            if (strcmp(folder_type, "television") == 0) {
                char clean_name[256];
                strcpy(clean_name, entry->d_name);
                simple_replace(clean_name, "_", " ");
                
                char link[1024];
                snprintf(link, sizeof(link), 
                    "<li><a style='color:powderblue;' href='/television/%s/'>📁 %s</a></li>",
                    entry->d_name, clean_name);
                
                if (strlen(output) + strlen(link) < max_size - 100) {
                    strcat(output, link);
                }
            }
        }
    }
    
    strcat(output, "</ul></body></html>");
    closedir(dir);
}

void build_directory_listing(const char *dir_path, char *output, int max_size) {
    printf("(CODE video_index.c - html_3) adding episodes to list..\n");
    DIR *dir = opendir(dir_path);
    if (!dir) {
        snprintf(output, max_size, "<h1>Error: Directory not found</h1>");
        return;
    }
    
    //extract name for display
    char show_name[256];
    strcpy(show_name, dir_path);
    char *last_slash = strrchr(show_name, '/');
    if (last_slash) {
        strcpy(show_name, last_slash + 1);
    }
    simple_replace(show_name, "_", " ");
    
    snprintf(output, max_size,
        "<!DOCTYPE html>\n<html>\n"
        "<body style='background-color:black; color:white;'>\n"
        "<a href='/' style='color:powderblue;'><h2>← Home</h2></a>\n"
        "<a href='/television/' style='color:powderblue;'><h3>← TV Shows</h3></a>\n"
        "<h2>%s Episodes:</h2>\n<ul>", show_name);
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        if (is_video_file(entry->d_name)) {
            char clean_name[256];
            strcpy(clean_name, entry->d_name);
            
            // Remove extension
            char *dot = strrchr(clean_name, '.');
            if (dot) *dot = '\0';
            
            // Clean up filename
            simple_replace(clean_name, "_", " ");
            
            char link[1024];
            snprintf(link, sizeof(link), 
                "<li><a style='color:powderblue;' href='/%s/%s'>%s</a></li>",
                dir_path, entry->d_name, clean_name);
            
            if (strlen(output) + strlen(link) < max_size - 100) {
                strcat(output, link);
            }
        }
    }
    
    strcat(output, "</ul></body></html>");
    closedir(dir);
}