#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void do_ls(const char *dir, int long_format);
void print_permissions(mode_t mode, char permissions[11]);
void print_long_entry(const char *dir, const char *name);

int main(int argc, char *argv[])
{
    int long_format = 0;
    int directory_count = 0;

    /* First, check whether the -l option was supplied. */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0)
            long_format = 1;
        else
            directory_count++;
    }

    /* If no directory was supplied, list the current directory. */
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

            printf("Directory listing of %s :\n", argv[i]);
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

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_format)
            print_long_entry(dir, entry->d_name);
        else
            printf("%s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}
