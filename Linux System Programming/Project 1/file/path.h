extern char BACKUP_DIR[];
extern char USER_DIR[];
extern char EXECUTE_DIR[];


extern void get_absolute_path(const char *path, char *ret);

extern const char *get_relative_path(const char *path, const char *base);

extern const char *get_date_in_path(const char *path);

extern void get_parent_directory(const char *path, char *ret);

extern int check_path_in_directory(const char *path, const char *directory);

extern void remove_filename_date(const char *path, char *ret);

extern int is_directory(const char *path);
