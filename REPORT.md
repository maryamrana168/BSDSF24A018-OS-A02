
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

## Feature 3: Column Display (Down Then Across)

### Question 1: Explain the logic for displaying filenames down then across. Why is a single loop insufficient?

The program first collects all filenames into a dynamically allocated array and determines the longest filename. It calculates the available terminal width using `ioctl()` and `TIOCGWINSZ`, then calculates the number of columns and rows.

The filename index is calculated using:

`index = row + column * rows`

This allows the program to print filenames vertically down each column before moving to the next column. A simple loop that prints each filename followed by a newline would display only one filename per line and would not produce the required column layout.

### Question 2: What is the purpose of ioctl()? What are the limitations of using a fixed width of 80 columns as a fallback?

The `ioctl()` system call is used with `TIOCGWINSZ` to obtain the terminal window dimensions. The number of terminal columns is stored in `ws_col`. This helps the program determine how many filename columns can fit on the screen.

If terminal-size detection fails or the output is not connected to a suitable terminal, the program uses 80 columns as a fallback. This is a reasonable default, but it may not match the actual terminal width. Consequently, output may be too wide or may leave unused space.

## Feature 4: Horizontal Column Display (`-x`)

### Q1. Compare the implementation complexity of down-then-across and horizontal display. Which requires more pre-calculation, and why?

The down-then-across display requires more pre-calculation. It must find the longest filename, determine the terminal width, calculate the number of columns that fit, calculate the required number of rows, and use an index formula to print entries column by column.

The horizontal display is simpler. It processes filenames from left to right while tracking the current line position. Before printing the next filename, it checks whether the filename and its spacing will exceed the terminal width. If so, it starts a new line.

Therefore, the down-then-across algorithm requires more layout calculations, whereas the horizontal algorithm mainly needs to track the current output position.

### Q2. How are the `-l`, `-x`, and default display modes managed?

The program defines an enumeration called `DisplayMode` with three values: `DISPLAY_VERTICAL`, `DISPLAY_LONG`, and `DISPLAY_HORIZONTAL`.

The `getopt()` function processes the command-line options. The `-l` option selects `DISPLAY_LONG`, while `-x` selects `DISPLAY_HORIZONTAL`. If neither option is provided, the program uses `DISPLAY_VERTICAL` by default.

The selected mode is passed to `do_ls()`. After collecting and sorting the filenames, `do_ls()` uses conditional statements to call `print_long_entry()` for long listings, `print_horizontal()` for horizontal display, or `print_columns()` for the default vertical display.

This design separates argument processing, directory reading, and output formatting, making the program easier to maintain and extend.

## Feature 5: Alphabetical Sort (v1.4.0)

### Q1. Why is it necessary to read all directory entries into memory before sorting them? What are the potential drawbacks for directories containing millions of files?

The `readdir()` function reads directory entries one at a time. To sort all filenames alphabetically using `qsort()`, the program first collects the filenames into a dynamically allocated array. Once all entries have been collected, `qsort()` sorts the array before the display function prints the results.

For directories containing millions of files, storing every filename and its pointer can consume a large amount of memory. Memory allocation may fail, and collecting and sorting a large number of entries can take considerable time. An alternative for very large collections is external sorting, which processes data in smaller portions using secondary storage.

### Q2. Explain the purpose and signature of the comparison function required by `qsort()`. How does it work, and why must it take `const void *` arguments?

The comparison function tells `qsort()` how to compare two array elements. Its signature is:

`int compare_names(const void *a, const void *b)`

The `const void *` parameters allow the generic `qsort()` function to accept a comparison function for different data types. The `const` qualifier indicates that the comparison function should not modify the elements being compared.

Because the array contains pointers to filenames, the function converts the arguments to pointers to filename-pointer elements and dereferences them to obtain the strings. It then uses `strcmp()` to compare the filenames.

The function returns a negative value if the first filename comes before the second, zero if the filenames are equal, and a positive value if the first comes after the second. `qsort()` uses these results to arrange the filenames alphabetically.
## Report Questions — Feature 6: Colorized Output (v1.5.0)

### 1. How do ANSI escape codes work to produce color in a standard Linux terminal? Show the specific code sequence for printing text in green.

ANSI escape codes are special character sequences used to control text formatting in a terminal. They can change the foreground color, background color, and text style. The escape character is represented in C by `\033` (octal 33), followed by `[` and a formatting code.

For example, the following C statement prints text in green:

```c
printf("\033[0;32mThis text is green\033[0m\n");
```

Explanation:

* `\033` represents the escape character that begins the ANSI sequence.
* `[0;32m` selects normal text formatting and green foreground color.
* `This text is green` is the text displayed in green.
* `\033[0m` resets the terminal formatting to its default settings.
* `\n` moves the cursor to the next line.

In Feature 6, these escape codes are defined using macros such as `COLOR_GREEN`, `COLOR_BLUE`, `COLOR_RED`, and `COLOR_RESET`. The program prints the appropriate color code before each filename and resets the formatting afterward. This makes it easier to distinguish directories, executables, archives, symbolic links, and special files.

### 2. To color an executable file, which permission bits in `st_mode` must be checked to determine whether it is executable by the owner, group, or others?

The `st_mode` field of the `struct stat` structure contains information about a file's type and permission bits. Linux provides three execute-permission constants:

* `S_IXUSR`: Indicates that the file's owner has execute permission.
* `S_IXGRP`: Indicates that members of the file's group have execute permission.
* `S_IXOTH`: Indicates that other users have execute permission.

To determine whether any of these execute bits is set, the program can use the bitwise AND (`&`) operator:

```c
if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
{
    printf("\033[0;32m%s\033[0m", filename);
}
```

Here, the bitwise OR (`|`) combines the three permission masks, and the bitwise AND (`&`) checks whether at least one corresponding bit is set. If the condition is true, the filename is printed in green.

**Note:** This check identifies files with at least one execute permission bit set. It does not guarantee that the current user can successfully execute the file, because actual execution also depends on the user's permissions, directory access, filesystem settings, and other security restrictions.


## Report Questions — Feature 7: Recursive Listing (v1.6.0)

### 1. In a recursive function, what is a base case? In the context of recursive `ls`, what stops the recursion from continuing forever?

A **base case** is a condition that stops a recursive function from making further recursive calls. Without a suitable stopping condition, recursion could continue indefinitely or eventually exhaust the program's resources.

In the recursive `ls` implementation, recursion proceeds only when the `-R` option is enabled and an entry is identified as a directory using `S_ISDIR(file_info.st_mode)`. The function does not recurse into ordinary files or symbolic links. It also skips entries beginning with a dot, including `.` and `..`.

Eventually, a directory contains no eligible subdirectories. In that case, no further recursive calls are made, and the function returns. This provides the natural stopping condition for the traversal.

### 2. Why is it essential to construct a full path before making a recursive call? What would happen if we simply called `do_ls("subdir")` from within `do_ls("parent_dir")`?

Constructing the full path is essential because a relative filename such as `subdir` is resolved relative to the program's current working directory, not automatically relative to the directory currently being listed.

For example, if the program is listing `parent_dir` and finds a directory named `subdir`, the correct path is:

```text
parent_dir/subdir
```

The program can construct this path using:

```c
snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
```

It should then pass the full path to the recursive call:

```c
do_ls(path, mode, recursive_flag);
```

If the program instead calls `do_ls("subdir", mode, recursive_flag)`, it may try to open a directory named `subdir` in the current working directory. This can result in an error or cause it to list the wrong directory.

Using the full path ensures that every recursive call visits the intended nested directory, regardless of the program's current working directory.
