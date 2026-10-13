
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

/* ANSI escape codes for colorized output. */
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"
#define COLOR_RESET   "\033[0m"

/* Available display modes. */
enum DisplayMode
{
    DISPLAY_VERTICAL,
    DISPLAY_LONG,
    DISPLAY_HORIZONTAL
};

/* Function declarations. */
int compare_names(const void *a, const void *b);
void do_ls(const char *dir, enum DisplayMode mode);
void print_permissions(mode_t mode, char permissions[11]);
void print_colorized_name(const char *dir, const char *name);
void print_long_entry(const char *dir, const char *name);
void print_columns(const char *dir, char **names, size_t count);
void print_horizontal(const char *dir, char **names, size_t count);

/* Main function. */
int main(int argc, char *argv[])
{
    enum DisplayMode mode = DISPLAY_VERTICAL;
    int option;

    /* Read command-line options: -l and -x. */
    while ((option = getopt(argc, argv, "lx")) != -1)
    {
        switch (option)
        {
            case 'l':
                mode = DISPLAY_LONG;
                break;

            case 'x':
                mode = DISPLAY_HORIZONTAL;
                break;

            default:
                fprintf(stderr,
                        "Usage: %s [-l | -x] [directory ...]\n",
                        argv[0]);
                return 1;
        }
    }

    /* If no directory is given, list the current directory. */
    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        /* Process each directory argument. */
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], mode);
            puts("");
        }
    }

    return 0;
}

/* Convert file permission bits into a string. */
void print_permissions(mode_t mode, char permissions[11])
{
    permissions[0] = S_ISDIR(mode) ? 'd' :
                     S_ISLNK(mode) ? 'l' :
                     S_ISCHR(mode) ? 'c' :
                     S_ISBLK(mode) ? 'b' :
                     S_ISFIFO(mode) ? 'p' :
                     S_ISSOCK(mode) ? 's' : '-';

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';

    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';

    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';

    permissions[10] = '\0';
}

/* Compare filenames for alphabetical sorting. */
int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char **)a;
    const char *name_b = *(const char **)b;

    return strcmp(name_a, name_b);
}

/*
 * Print a filename with a color determined by its file type.
 * lstat() identifies a symbolic link without following its target.
 */
void print_colorized_name(const char *dir, const char *name)
{
    char path[4096];
    struct stat file_info;
    const char *color = "";
    size_t length = strlen(name);

    int written = snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (written < 0 || (size_t)written >= sizeof(path))
    {
        printf("%s", name);
        return;
    }

    if (lstat(path, &file_info) == -1)
    {
        /* If metadata cannot be obtained, print without color. */
        printf("%s", name);
        return;
    }

    mode_t mode = file_info.st_mode;

    /* Symbolic links must be checked before their targets. */
    if (S_ISLNK(mode))
    {
        color = COLOR_PINK;
    }
    /* Special files use reverse video. */
    else if (S_ISCHR(mode) || S_ISBLK(mode) ||
             S_ISFIFO(mode) || S_ISSOCK(mode))
    {
        color = COLOR_REVERSE;
    }
    /* Directories are blue. */
    else if (S_ISDIR(mode))
    {
        color = COLOR_BLUE;
    }
    /* Recognized archive extensions are red. */
    else if ((length >= 4 &&
              strcmp(name + length - 4, ".tar") == 0) ||
             (length >= 4 &&
              strcmp(name + length - 4, ".zip") == 0) ||
             (length >= 3 &&
              strcmp(name + length - 3, ".gz") == 0))
    {
        color = COLOR_RED;
    }
    /* Files with any execute permission bit are green. */
    else if (S_ISREG(mode) &&
             (mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
    {
        color = COLOR_GREEN;
    }

    /* Reset formatting after each colored filename. */
    printf("%s%s%s",
           color,
           name,
           color[0] != '\0' ? COLOR_RESET : "");
}

/* Print one file in long-listing format. */
void print_long_entry(const char *dir, const char *name)
{
    char path[4096];
    struct stat file_info;
    char permissions[11];
    char time_text[32];

    int written = snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (written < 0 || (size_t)written >= sizeof(path))
    {
        fprintf(stderr, "Path too long: %s/%s\n", dir, name);
        return;
    }

    if (lstat(path, &file_info) == -1)
    {
        perror(path);
        return;
    }

    print_permissions(file_info.st_mode, permissions);

    struct passwd *owner = getpwuid(file_info.st_uid);
    struct group *group = getgrgid(file_info.st_gid);
    struct tm *time_info = localtime(&file_info.st_mtime);

    if (time_info == NULL ||
        strftime(time_text, sizeof(time_text), "%b %e %H:%M",
                 time_info) == 0)
    {
        strcpy(time_text, "unknown time");
    }

    /* Keep metadata uncolored and color only the filename. */
    printf("%s %2lu %-8s %-8s %8lld %s ",
           permissions,
           (unsigned long)file_info.st_nlink,
           owner ? owner->pw_name : "?",
           group ? group->gr_name : "?",
           (long long)file_info.st_size,
           time_text);

    print_colorized_name(dir, name);
    putchar('\n');
}

/* Default display: down-then-across (vertical columns). */
void print_columns(const char *dir, char **names, size_t count)
{
    if (count == 0)
        return;

    size_t max_length = 0;

    /* Find the longest filename. */
    for (size_t i = 0; i < count; i++)
    {
        size_t length = strlen(names[i]);

        if (length > max_length)
            max_length = length;
    }

    size_t column_width = max_length + 2;
    size_t terminal_width = 80;

    /* Detect terminal width. Use 80 if detection fails. */
    struct winsize window;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 &&
        window.ws_col > 0)
    {
        terminal_width = window.ws_col;
    }

    /* Calculate the number of columns that can fit. */
    size_t columns = terminal_width / column_width;

    if (columns == 0)
        columns = 1;

    if (columns > count)
        columns = count;

    /* Calculate the number of rows. */
    size_t rows = (count + columns - 1) / columns;

    /*
     * Print down each column, then move across.
     * Index = row + column * rows.
     */
    for (size_t row = 0; row < rows; row++)
    {
        for (size_t col = 0; col < columns; col++)
        {
            size_t index = row + col * rows;

            if (index < count)
            {
                print_colorized_name(dir, names[index]);

                /*
                 * Calculate padding from the filename length.
                 * ANSI escape codes do not count as visible characters.
                 */
                if (col != columns - 1 && index + rows < count)
                {
                    size_t padding =
                        column_width - strlen(names[index]);

                    printf("%*s", (int)padding, "");
                }
            }
        }

        putchar('\n');
    }
}

/* Horizontal display: left-to-right, wrapping at terminal width. */
void print_horizontal(const char *dir, char **names, size_t count)
{
    if (count == 0)
        return;

    size_t terminal_width = 80;
    size_t position = 0;

    /* Detect terminal width. Use 80 if detection fails. */
    struct winsize window;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 &&
        window.ws_col > 0)
    {
        terminal_width = window.ws_col;
    }

    for (size_t i = 0; i < count; i++)
    {
        size_t name_length = strlen(names[i]);

        /* Wrap before printing if the next name will not fit. */
        if (position > 0 &&
            position + 2 + name_length > terminal_width)
        {
            putchar('\n');
            position = 0;
        }

        /* Separate filenames on the same line with two spaces. */
        if (position > 0)
        {
            printf("  ");
            position += 2;
        }

        print_colorized_name(dir, names[i]);
        position += name_length;
    }

    putchar('\n');
}

/* Read directory entries and choose the requested display mode. */
void do_ls(const char *dir, enum DisplayMode mode)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory %s: ", dir);
        perror("");
        return;
    }

    char **names = NULL;
    size_t count = 0;
    size_t capacity = 0;

    /* Collect filenames into a dynamically allocated array. */
    while (1)
    {
        errno = 0;
        entry = readdir(dp);

        if (entry == NULL)
        {
            if (errno != 0)
                perror("readdir failed");

            break;
        }

        /* Skip hidden files, as in the existing version. */
        if (entry->d_name[0] == '.')
            continue;

        /* Increase array capacity when necessary. */
        if (count == capacity)
        {
            size_t new_capacity =
                (capacity == 0) ? 16 : capacity * 2;

            char **temp = realloc(
                names, new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc failed");

                for (size_t i = 0; i < count; i++)
                    free(names[i]);

                free(names);
                closedir(dp);
                return;
            }

            names = temp;
            capacity = new_capacity;
        }

        /* Allocate memory for the current filename. */
        size_t length = strlen(entry->d_name);
        names[count] = malloc(length + 1);

        if (names[count] == NULL)
        {
            perror("malloc failed");

            for (size_t i = 0; i < count; i++)
                free(names[i]);

            free(names);
            closedir(dp);
            return;
        }

        memcpy(names[count], entry->d_name, length + 1);
        count++;
    }

    closedir(dp);

    /* Sort filenames alphabetically before displaying them. */
    qsort(names, count, sizeof(char *), compare_names);

    /* Select the requested output mode. */
    if (mode == DISPLAY_LONG)
    {
        for (size_t i = 0; i < count; i++)
            print_long_entry(dir, names[i]);
    }
    else if (mode == DISPLAY_HORIZONTAL)
    {
        print_horizontal(dir, names, count);
    }
    else
    {
        print_columns(dir, names, count);
    }

    /* Release dynamically allocated memory. */
    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
}
