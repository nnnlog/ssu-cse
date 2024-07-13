#include "../data/linked_list.h"

extern int is_MD5;

//extern const char *BACKUP_DIR;


extern void init_filesystem(char *executable_path);

extern int remove_backup_directory_safely(const char *path);


/// 파일 입출력 관련
extern unsigned char *read_file(const char *path, int *length);

extern void write_file(const char *path, const unsigned char *content, int length);

extern void copy_file(const char *dst, const char *src);

extern off_t get_filesize(const char *path);


/// 파일 스캔 관련
extern struct Node *find_all_files(const char *path, int is_recursive, int is_first);

extern struct Node *find_same_name_files(const char *path);

