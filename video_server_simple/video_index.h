#ifndef VIDEO_INDEX_H
#define VIDEO_INDEX_H

void build_video_index(const char *search_term, const char *folder_type, char *output, int max_size);
void build_directory_listing(const char *dir_path, char *output, int max_size);

#endif