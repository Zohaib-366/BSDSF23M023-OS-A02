/*
* Programming Assignment 03: lsv1.3.0
* Version 1.3.0 - Horizontal Column Display (-x)
* Usage:
*       $ lsv1.3.0
*       $ lsv1.3.0 -l
*       $ lsv1.3.0 -x
*       $ lsv1.3.0 /home
*       $ lsv1.3.0 -l /home/kali/ /etc/
*       $ lsv1.3.0 -x /home/kali/
*
* Feature 4:
* - Adds a new horizontal (row-major) column layout with -x flag
* - Default (no flag) still prints in “down then across” format
* - -l prints long listing
* - -x prints files left to right, wrapping by terminal width
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h> // For terminal width

extern int errno;
extern int optind;

void do_ls(const char *dir, int mode);
void do_ls_long(const char *dir);
void print_in_columns(char **filenames, int count);
void print_in_columns_horizontal(char **filenames, int count);

int main(int argc, char *argv[])
{
    int opt;
    int long_listing = 0;
    int horizontal_display = 0;

    // Parse -l and -x flags
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        if (opt == 'l') {
            long_listing = 1;
        } else if (opt == 'x') {
            horizontal_display = 1;
        }
    }

    if (optind == argc) {
        // No directories given, use current directory
        if (long_listing)
            do_ls_long(".");
        else
            do_ls(".", horizontal_display);
    } else {
        // Directories provided
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            if (long_listing)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i], horizontal_display);
            puts("");
        }
    }

    return 0;
}

/* ===============================================
   Default or Horizontal Listing (Based on -x)
   =============================================== */
void do_ls(const char *dir, int horizontal)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    // Step 1: Gather all filenames dynamically
    char **filenames = NULL;
    int count = 0;
    int max_len = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        // Track longest filename
        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;

        // Store filename in dynamic array
        filenames = realloc(filenames, sizeof(char *) * (count + 1));
        filenames[count] = strdup(entry->d_name);
        count++;
    }

    if (errno != 0) {
        perror("readdir failed");
    }
    closedir(dp);

    if (count == 0) {
        free(filenames);
        return;
    }

    // Step 3: Print filenames based on display mode
    if (horizontal)
        print_in_columns_horizontal(filenames, count);
    else
        print_in_columns(filenames, count);

    // Step 4: Free memory
    for (int i = 0; i < count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

/* ===============================================
   Helper Function: Default Down-then-Across Display
   =============================================== */
void print_in_columns(char **filenames, int count)
{
    struct winsize w;
    int terminal_width = 80; // fallback

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        terminal_width = w.ws_col;
    }

    // Find longest filename
    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len)
            max_len = len;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    int columns = terminal_width / col_width;
    if (columns < 1)
        columns = 1;

    int rows = (count + columns - 1) / columns;

    // Down-then-across printing
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < columns; c++) {
            int idx = c * rows + r;
            if (idx < count)
                printf("%-*s", col_width, filenames[idx]);
        }
        printf("\n");
    }
}

/* ===============================================
   Helper Function: Horizontal (Left-to-Right) Display
   =============================================== */
void print_in_columns_horizontal(char **filenames, int count)
{
    struct winsize w;
    int terminal_width = 80; // fallback

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        terminal_width = w.ws_col;
    }

    // Find longest filename
    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len)
            max_len = len;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    int current_width = 0;

    // Print files left to right, wrapping when full
    for (int i = 0; i < count; i++) {
        int next_width = current_width + col_width;
        if (next_width > terminal_width) {
            printf("\n");
            current_width = 0;
        }
        printf("%-*s", col_width, filenames[i]);
        current_width += col_width;
    }
    printf("\n");
}

/* ===============================================
   Long Listing Mode (Unchanged from v1.1.0)
   =============================================== */
void do_ls_long(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        struct stat st;
        if (lstat(path, &st) == -1) {
            perror("lstat failed");
            continue;
        }

        // File type
        char ftype = '?';
        if (S_ISREG(st.st_mode)) ftype = '-';
        else if (S_ISDIR(st.st_mode)) ftype = 'd';
        else if (S_ISLNK(st.st_mode)) ftype = 'l';
        else if (S_ISCHR(st.st_mode)) ftype = 'c';
        else if (S_ISBLK(st.st_mode)) ftype = 'b';
        else if (S_ISFIFO(st.st_mode)) ftype = 'p';
        else if (S_ISSOCK(st.st_mode)) ftype = 's';

        // Permissions
        char perms[10];
        perms[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perms[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perms[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        perms[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perms[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perms[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        perms[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
        perms[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perms[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        perms[9] = '\0';

        // Links, owner, group, size, modification time
        nlink_t links = st.st_nlink;
        struct passwd *pwd = getpwuid(st.st_uid);
        struct group  *grp = getgrgid(st.st_gid);
        off_t size = st.st_size;
        char *mtime = ctime(&st.st_mtime);
        mtime[strlen(mtime)-1] = '\0'; // remove newline

        // Print long listing format
        printf("%c%s %lu %s %s %5ld %s %s\n",
               ftype, perms, links,
               pwd ? pwd->pw_name : "unknown",
               grp ? grp->gr_name : "unknown",
               size, mtime, entry->d_name);
    }

    if (errno != 0) {
        perror("readdir failed");
    }

    closedir(dp);
}

