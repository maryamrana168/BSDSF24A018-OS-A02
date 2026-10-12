
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
int compare_names(const void *a, const void *b);
void do_ls(const char *dir, int long_format);
void print_permissions(mode_t mode, char permissions[11]);
void print_long_entry(const char *dir, const char *name);
void print_columns(char **names, size_t count);

int main(int argc, char *argv[])
{
    int long_format = 0;
    int directory_count = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0)
            long_format = 1;
        else
            directory_count++;
    }

    if (directory_count == 0)
    {
        do_ls(".", long_format);
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            if (strcmp(argv[i], "-l") == 0)
                continue;

            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], long_format);
            puts("");
        }
    }

    return 0;
}

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



int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char **)a;
    const char *name_b = *(const char **)b;

    return strcmp(name_a, name_b);
}

void print_long_entry(const char *dir, const char *name)
{
    char path[4096];
    struct stat file_info;
    char permissions[11];
    char time_text[32];

    snprintf(path, sizeof(path), "%s/%s", dir, name);

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

    printf("%s %2lu %-8s %-8s %8lld %s %s\n",
           permissions,
           (unsigned long)file_info.st_nlink,
           owner ? owner->pw_name : "?",
           group ? group->gr_name : "?",
           (long long)file_info.st_size,
           time_text,
           name);
}


void print_columns(char **names, size_t count)
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

    /* Detect terminal width using ioctl(). */
    struct winsize window;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 &&
        window.ws_col > 0)
    {
        terminal_width = window.ws_col;
    }

    /* Calculate how many columns can fit. */
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
                if (col == columns - 1 ||
                    index + rows >= count)
                {
                    printf("%s", names[index]);
                }
                else
                {
                    printf("%-*s",
                           (int)column_width, names[index]);
                }
            }
        }

        putchar('\n');
    }
}

void do_ls(const char *dir, int long_format)
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

        if (count == capacity)
        {
            size_t new_capacity = (capacity == 0) ? 16 : capacity * 2;

            char **temp = realloc(names,
                                  new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc failed");

                for (size_t i = 0; i < count; i++)
                    free(names[i]);

                free(names);
                closedir(dp);
                qsort(names, count, sizeof(char *), compare_names);
                return;
            }


            names = temp;
            capacity = new_capacity;
        }

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

    /* Preserve the existing long-listing functionality. */
    if (long_format)
    {
        for (size_t i = 0; i < count; i++)
            print_long_entry(dir, names[i]);
    }
    else
    {
        print_columns(names, count);
    }

    /* Release dynamically allocated memory. */
    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
}
