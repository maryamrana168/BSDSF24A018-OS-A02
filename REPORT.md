
Question 1: What is the crucial difference between stat() and lstat()? In the context of the ls command, when is it more appropriate to use lstat()?

The crucial difference between stat() and lstat() is how they handle symbolic links.

stat() retrieves information about the file or directory that a symbolic link points to. It follows the symbolic link and reports the target's information.
lstat() retrieves information about the symbolic link itself without following it. It reports the link's own file type, permissions, and other metadata.

Both functions are declared in the <sys/stat.h> header file and store the retrieved metadata in a struct stat structure.

Example in C
#include <sys/stat.h>
#include <stdio.h>

int main(void)
{
    struct stat info;

    if (lstat("mylink", &info) == 0)
    {
        if (S_ISLNK(info.st_mode))
            printf("This is a symbolic link.\n");
    }

    return 0;
}

In this example, lstat() allows the program to identify whether mylink itself is a symbolic link.

Which is more appropriate for ls?

For a directory listing similar to ls -l, lstat() is often more appropriate when displaying each entry's own file type and metadata. For example, it allows the program to display a symbolic link as a link rather than displaying the target's type.

If stat() were used instead, a symbolic link pointing to a directory would appear to have the target directory's metadata. Using lstat() preserves the distinction between the link and its target.

Therefore, lstat() is useful for implementing a long listing that correctly identifies symbolic links. Note that the behavior of a real ls command can vary with options, and symbolic links that point to nonexistent targets require special handling.

Question 2: The st_mode field in struct stat contains both the file type and permission bits. Explain how bitwise operators and predefined macros can extract this information.

The st_mode field of struct stat is an integer containing two kinds of information:

File type: Indicates whether an entry is a regular file, directory, symbolic link, or another supported file type.
Permission bits: Indicate whether the owner, group, and others can read, write, or execute the file.

The <sys/stat.h> header provides predefined macros and constants for examining these bits.

1. Extracting the file type

Macros such as S_IFDIR, S_IFREG, and S_IFLNK represent file-type bit patterns. A bitwise AND (&) can test the relevant type bits:

if ((info.st_mode & S_IFMT) == S_IFDIR)
    printf("Directory\n");

else if ((info.st_mode & S_IFMT) == S_IFREG)
    printf("Regular file\n");

else if ((info.st_mode & S_IFMT) == S_IFLNK)
    printf("Symbolic link\n");

Here, S_IFMT is a mask for the file-type bits. Applying & isolates those bits, and comparing the result with a type constant identifies the type.

The more readable, standard approach is to use the predefined test macros:

if (S_ISDIR(info.st_mode))
    printf("Directory\n");

else if (S_ISREG(info.st_mode))
    printf("Regular file\n");

else if (S_ISLNK(info.st_mode))
    printf("Symbolic link\n");
2. Extracting permission bits

Permission constants include:

S_IRUSR: Owner has read permission.
S_IWUSR: Owner has write permission.
S_IXUSR: Owner has execute permission.
S_IRGRP, S_IWGRP, S_IXGRP: Read, write, and execute permissions for the group.
S_IROTH, S_IWOTH, S_IXOTH: Read, write, and execute permissions for others.

For example:

if (info.st_mode & S_IRUSR)
    printf("Owner can read\n");

if (info.st_mode & S_IWUSR)
    printf("Owner can write\n");

if (info.st_mode & S_IXUSR)
    printf("Owner can execute\n");

The bitwise AND (&) checks whether a particular permission bit is set. If the result is nonzero, that permission is enabled.

3. How ls -l displays permissions

A long listing represents permissions using ten characters, such as:

-rwxr-xr--

The first character indicates the file type (- for a regular file or d for a directory). The next nine characters represent the read, write, and execute permissions for the owner, group, and others.

A program can construct this string by checking each permission constant individually and placing r, w, or x in the appropriate position. If a permission is absent, it places -.

For example, -rwxr-xr-- means:

Owner: read, write, and execute.
Group: read and execute.
Others: read only.
Conclusion

The stat() and lstat() functions retrieve file metadata, while st_mode encodes file type and permissions. Bitwise operations and predefined macros let a C program extract this information and display it in the long listing format used by ls -l.
