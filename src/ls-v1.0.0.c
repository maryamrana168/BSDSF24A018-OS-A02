
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"
#define COLOR_RESET   "\033[0m"

enum DisplayMode {
    DISPLAY_COLUMNS,
    DISPLAY_HORIZONTAL,
    DISPLAY_LONG
};

static int compare_names(const void *a, const void *b);
static void do_ls(const char *dir, enum DisplayMode mode,
                  int recursive_flag);
static void print_permissions(mode_t mode, char permissions[11]);
static void print_colorized_name(const char *dir, const char *name);
static void print_long_entry(const char *dir, const char *name);
static void print_columns(const char *dir, char **names, size_t count);
static void print_horizontal(const char *dir, char **names, size_t count);
static char *make_path(const char *dir, const char *name);
static int is_archive(const char *name);
static int is_dot_entry(const char *name);

/* Compare filenames alphabetically. */
static int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char * const *)a;
    const char *name_b = *(const char * const *)b;

    return strcmp(name_a, name_b);
}

/* Ignore . and .. during directory traversal. */
static int is_dot_entry(const char *name)
{
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

/* Check common archive extensions. */
static int is_archive(const char *name)
{
    const char *dot = strrchr(name, '.');

    if (dot == NULL)
        return 0;

    return strcmp(dot, ".tar") == 0 ||
           strcmp(dot, ".gz") == 0 ||
           strcmp(dot, ".zip") == 0 ||
           strcmp(dot, ".rar") == 0 ||
           strcmp(dot, ".7z") == 0;
}

/* Build parent/name safely. */
static char *make_path(const char *dir, const char *name)
{
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    int has_slash = dir_len > 0 && dir[dir_len - 1] == '/';

    if (dir_len > SIZE_MAX - name_len - 2) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    size_t length = dir_len + name_len + (has_slash ? 1 : 2);
    char *path = malloc(length);

    if (path == NULL) {
        perror("malloc");
        return NULL;
    }

    if (has_slash)
        snprintf(path, length, "%s%s", dir, name);
    else
        snprintf(path, length, "%s/%s", dir, name);

    return path;
}

/* Print Unix file permissions. */
static void print_permissions(mode_t mode, char permissions[11])
{
    permissions[0] = S_ISDIR(mode)  ? 'd' :
                     S_ISLNK(mode)  ? 'l' :
                     S_ISCHR(mode)  ? 'c' :
                     S_ISBLK(mode)  ? 'b' :
                     S_ISFIFO(mode) ? 'p' :
                     S_ISSOCK(mode) ? 's' : '-';

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_ISUID) ?
                     ((mode & S_IXUSR) ? 's' : 'S') :
                     ((mode & S_IXUSR) ? 'x' : '-');

    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_ISGID) ?
                     ((mode & S_IXGRP) ? 's' : 'S') :
                     ((mode & S_IXGRP) ? 'x' : '-');

    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_ISVTX) ?
                     ((mode & S_IXOTH) ? 't' : 'T') :
                     ((mode & S_IXOTH) ? 'x' : '-');

    permissions[10] = '\0';
}

/* Print a filename with color according to its file type. */
static void print_colorized_name(const char *dir, const char *name)
{
    char *path = make_path(dir, name);

    if (path == NULL) {
        printf("%s", name);
        return;
    }

    struct stat info;

    /* lstat checks the link itself instead of following it. */
    if (lstat(path, &info) == -1) {
        printf("%s", name);
        free(path);
        return;
    }

    const char *color = COLOR_RESET;

    if (S_ISLNK(info.st_mode))
        color = COLOR_PINK;
    else if (S_ISDIR(info.st_mode))
        color = COLOR_BLUE;
    else if (S_ISREG(info.st_mode) && (info.st_mode & (S_IXUSR |
                                                       S_IXGRP |
                                                       S_IXOTH)))
        color = COLOR_GREEN;
    else if (S_ISREG(info.st_mode) && is_archive(name))
        color = COLOR_RED;
    else if (!S_ISREG(info.st_mode))
        color = COLOR_REVERSE;

    printf("%s%s%s", color, name, COLOR_RESET);

    free(path);
}

/* Print a file's details in long format. */
static void print_long_entry(const char *dir, const char *name)
{
    char *path = make_path(dir, name);

    if (path == NULL) {
        printf("%s\n", name);
        return;
    }

    struct stat info;

    if (lstat(path, &info) == -1) {
        perror(path);
        free(path);
        return;
    }

    char permissions[11];
    print_permissions(info.st_mode, permissions);

    struct passwd *owner = getpwuid(info.st_uid);
    struct group *group = getgrgid(info.st_gid);

    char time_buffer[64];
    struct tm *time_info = localtime(&info.st_mtime);

    if (time_info != NULL)
        strftime(time_buffer, sizeof(time_buffer),
                 "%b %d %H:%M", time_info);
    else
        strcpy(time_buffer, "unknown time");

    printf("%s %3lu %-8s %-8s %8lld %s ",
           permissions,
           (unsigned long)info.st_nlink,
           owner ? owner->pw_name : "?",
           group ? group->gr_name : "?",
           (long long)info.st_size,
           time_buffer);

    print_colorized_name(dir, name);

    if (S_ISLNK(info.st_mode)) {
        char target[PATH_MAX];
        ssize_t length = readlink(path, target, sizeof(target) - 1);

        if (length >= 0) {
            target[length] = '\0';
            printf(" -> %s", target);
        }
    }

    putchar('\n');
    free(path);
}

/* Print names in columns. */
static void print_columns(const char *dir, char **names, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        print_colorized_name(dir, names[i]);

        if ((i + 1) % 3 == 0 || i + 1 == count)
            putchar('\n');
        else
            printf("\t");
    }
}

/* Print names horizontally on one line. */
static void print_horizontal(const char *dir, char **names, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        print_colorized_name(dir, names[i]);

        if (i + 1 < count)
            printf("  ");
    }

    putchar('\n');
}

/*
 * Feature 7: recursively list a directory.
 * Base case: if the directory cannot be opened, stop this call.
 */
static void do_ls(const char *dir, enum DisplayMode mode,
                  int recursive_flag)
{
    DIR *directory = opendir(dir);

    if (directory == NULL) {
        perror(dir);
        return;
    }

    char **names = NULL;
    size_t count = 0;
    size_t capacity = 0;
    struct dirent *entry;

    /* Read directory entries into a dynamically growing array. */
    errno = 0;

    while ((entry = readdir(directory)) != NULL) {
        if (is_dot_entry(entry->d_name))
            continue;

        /* Preserve the previous behavior of hiding dotfiles. */
        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 16 : capacity * 2;

            if (new_capacity < capacity ||
                new_capacity > SIZE_MAX / sizeof(*names)) {
                fprintf(stderr, "Too many directory entries\n");
                break;
            }

            char **new_names = realloc(
                names, new_capacity * sizeof(*names));

            if (new_names == NULL) {
                perror("realloc");
                break;
            }

            names = new_names;
            capacity = new_capacity;
        }

        names[count] = strdup(entry->d_name);

        if (names[count] == NULL) {
            perror("strdup");
            break;
        }

        count++;
    }

    if (entry == NULL && errno != 0)
        perror("readdir");

    closedir(directory);

    /* Sort the names alphabetically. */
    qsort(names, count, sizeof(*names), compare_names);

    /* Print a directory heading for recursive mode. */
    if (recursive_flag)
        printf("%s:\n", dir);

    /* Display the current directory's entries. */
    if (mode == DISPLAY_LONG) {
        for (size_t i = 0; i < count; i++)
            print_long_entry(dir, names[i]);
    } else if (mode == DISPLAY_HORIZONTAL) {
        print_horizontal(dir, names, count);
    } else {
        print_columns(dir, names, count);
    }

    /*
     * Recursion: inspect each entry and visit actual directories.
     * lstat prevents following symbolic links to directories.
     */
    if (recursive_flag) {
        for (size_t i = 0; i < count; i++) {
            char *path = make_path(dir, names[i]);

            if (path == NULL)
                continue;

            struct stat info;

            if (lstat(path, &info) == -1) {
                perror(path);
                free(path);
                continue;
            }

            if (S_ISDIR(info.st_mode)) {
                putchar('\n');
                do_ls(path, mode, recursive_flag);
            }

            free(path);
        }
    }

    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
}

/* Program entry point. */
int main(int argc, char *argv[])
{
    enum DisplayMode mode = DISPLAY_COLUMNS;
    int recursive_flag = 0;
    int option;

    /* Parse -l, -x, and -R. */
    while ((option = getopt(argc, argv, "lxR")) != -1) {
        switch (option) {
        case 'l':
            mode = DISPLAY_LONG;
            break;

        case 'x':
            mode = DISPLAY_HORIZONTAL;
            break;

        case 'R':
            recursive_flag = 1;
            break;

        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory ...]\n",
                    argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* Use the current directory when no directory is supplied. */
    if (optind == argc) {
        do_ls(".", mode, recursive_flag);
    } else {
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1)
                printf("%s:\n", argv[i]);

            do_ls(argv[i], mode, recursive_flag);

            if (i + 1 < argc)
                putchar('\n');
        }
    }

    return EXIT_SUCCESS;
}
