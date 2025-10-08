/*
* Programming Assignment 04: lsv1.6.0
* Version 1.6.0 - Recursive Listing (-R)
* Usage:
*       $ lsv1.6.0
*       $ lsv1.6.0 -l
*       $ lsv1.6.0 -x
*       $ lsv1.6.0 -R
*       $ lsv1.6.0 -lR /home
*       $ lsv1.6.0 -xR /etc/
*
* Feature 7:
* - Adds recursive directory listing using -R flag
* - Displays subdirectory contents by calling do_ls() recursively
* - Handles proper path construction (dirname/entryname)
* - Works with colorized output and all display modes
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>  // for strcasecmp
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h> // For terminal width

extern int errno;
extern int optind;

/* ===============================================
   ANSI Color Codes (Feature 6)
   =============================================== */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

void do_ls(const char *dir, int mode, int recursive);
void do_ls_long(const char *dir, int recursive);
void print_in_columns(char **filenames, int count, const char *dir, int recursive);
void print_in_columns_horizontal(char **filenames, int count, const char *dir, int recursive);
int compare_filenames(const void *a, const void *b);
void handle_recursive_subdirs(char **filenames, int count, const char *dir, int recursive);

/* ===============================================
   Helper Function: Print filename with color
   =============================================== */

void print_colored(const char *name, mode_t st_mode)
{
    if (S_ISDIR(st_mode))
        printf(COLOR_BLUE "%s" COLOR_RESET, name);
    else if (S_ISLNK(st_mode))
        printf(COLOR_PINK "%s" COLOR_RESET, name);
    else if (st_mode & S_IXUSR || st_mode & S_IXGRP || st_mode & S_IXOTH)
        printf(COLOR_GREEN "%s" COLOR_RESET, name);
    else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
        printf(COLOR_RED "%s" COLOR_RESET, name);
    else if (S_ISCHR(st_mode) || S_ISBLK(st_mode) || S_ISFIFO(st_mode) || S_ISSOCK(st_mode))
        printf(COLOR_REVERSE "%s" COLOR_RESET, name);
    else
        printf("%s", name);
}

int main(int argc, char *argv[])
{
    int opt;
    int long_listing = 0;
    int horizontal_display = 0;
    int recursive_flag = 0;

    // Parse -l, -x, and -R flags
    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        if (opt == 'l') {
            long_listing = 1;
        } else if (opt == 'x') {
            horizontal_display = 1;
        } else if (opt == 'R') {
            recursive_flag = 1;
        }
    }

    if (optind == argc) {
        // No directories given, use current directory
        if (long_listing)
            do_ls_long(".", recursive_flag);
        else
            do_ls(".", horizontal_display, recursive_flag);
    } else {
        // Directories provided
        for (int i = optind; i < argc; i++) {
            printf("%s:\n", argv[i]);
            if (long_listing)
                do_ls_long(argv[i], recursive_flag);
            else
                do_ls(argv[i], horizontal_display, recursive_flag);
            puts("");
        }
    }

    return 0;
}

/* ===============================================
   Default or Horizontal Listing (Based on -x)
   =============================================== */
void do_ls(const char *dir, int horizontal, int recursive)
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

        int len = strlen(entry->d_name);
        if (len > max_len)
            max_len = len;

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

    // Step 2: Sort filenames alphabetically using qsort()
    qsort(filenames, count, sizeof(char *), compare_filenames);

    // Step 3: Print filenames based on display mode
    if (horizontal)
        print_in_columns_horizontal(filenames, count, dir, recursive);
    else
        print_in_columns(filenames, count, dir, recursive);

    // Step 4: Recursively list subdirectories (if -R enabled)
    if (recursive)
        handle_recursive_subdirs(filenames, count, dir, recursive);

    // Step 5: Free memory
    for (int i = 0; i < count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

/* ===============================================
   Helper Function: Handle Recursive Subdirectories
   =============================================== */
void handle_recursive_subdirs(char **filenames, int count, const char *dir, int recursive)
{
    struct stat st;
    char path[1024];

    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, filenames[i]);
        if (lstat(path, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode)) {
            if (strcmp(filenames[i], ".") == 0 || strcmp(filenames[i], "..") == 0)
                continue;

            printf("\n%s:\n", path);
            do_ls(path, 0, recursive);
        }
    }
}

/* ===============================================
   Helper Function: Default Down-then-Across Display
   =============================================== */
void print_in_columns(char **filenames, int count, const char *dir, int recursive)
{
	(void)recursive;  // Prevent unused parameter warning
    struct winsize w;
    int terminal_width = 80; // fallback

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        terminal_width = w.ws_col;
    }

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

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < columns; c++) {
            int idx = c * rows + r;
            if (idx < count) {
                char path[1024];
                snprintf(path, sizeof(path), "%s/%s", dir, filenames[idx]);
                struct stat st;
                if (lstat(path, &st) == 0)
                    print_colored(filenames[idx], st.st_mode);
                else
                    printf("%s", filenames[idx]);
                int pad = col_width - strlen(filenames[idx]);
                for (int s = 0; s < pad; s++) printf(" ");
            }
        }
        printf("\n");
    }
}

/* ===============================================
   Helper Function: Horizontal (Left-to-Right) Display
   =============================================== */
void print_in_columns_horizontal(char **filenames, int count, const char *dir, int recursive)
{	
	(void)recursive; // Prevent unused parameter warning
    struct winsize w;
    int terminal_width = 80; // fallback

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        terminal_width = w.ws_col;
    }

    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len)
            max_len = len;
    }

    int spacing = 2;
    int col_width = max_len + spacing;
    int current_width = 0;

    for (int i = 0; i < count; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir, filenames[i]);
        struct stat st;
        if (lstat(path, &st) == 0)
            print_colored(filenames[i], st.st_mode);
        else
            printf("%s", filenames[i]);

        int next_width = current_width + col_width;
        if (next_width > terminal_width) {
            printf("\n");
            current_width = 0;
        } else {
            int pad = col_width - strlen(filenames[i]);
            for (int s = 0; s < pad; s++) printf(" ");
            current_width += col_width;
        }
    }
    printf("\n");
}

/* ===============================================
   Comparison Function for qsort()
   =============================================== */
int compare_filenames(const void *a, const void *b)
{
    const char *fileA = *(const char **)a;
    const char *fileB = *(const char **)b;
    return strcasecmp(fileA, fileB);
}

/* ===============================================
   Long Listing Mode (Unchanged from v1.1.0)
   =============================================== */
void do_ls_long(const char *dir, int recursive)
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

        char ftype = '?';
        if (S_ISREG(st.st_mode)) ftype = '-';
        else if (S_ISDIR(st.st_mode)) ftype = 'd';
        else if (S_ISLNK(st.st_mode)) ftype = 'l';
        else if (S_ISCHR(st.st_mode)) ftype = 'c';
        else if (S_ISBLK(st.st_mode)) ftype = 'b';
        else if (S_ISFIFO(st.st_mode)) ftype = 'p';
        else if (S_ISSOCK(st.st_mode)) ftype = 's';

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

        nlink_t links = st.st_nlink;
        struct passwd *pwd = getpwuid(st.st_uid);
        struct group  *grp = getgrgid(st.st_gid);
        off_t size = st.st_size;
        char *mtime = ctime(&st.st_mtime);
        mtime[strlen(mtime)-1] = '\0';

        printf("%c%s %lu %s %s %5ld %s ", ftype, perms, links,
               pwd ? pwd->pw_name : "unknown",
               grp ? grp->gr_name : "unknown",
               size, mtime);

        print_colored(entry->d_name, st.st_mode);
        printf("\n");
    }

    if (errno != 0) {
        perror("readdir failed");
    }

    closedir(dp);

    // Recursive listing for -R
    if (recursive) {
        dp = opendir(dir);
        if (dp == NULL) return;

        while ((entry = readdir(dp)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
            struct stat st;
            if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                printf("\n%s:\n", path);
                do_ls_long(path, recursive);
            }
        }
        closedir(dp);
    }
}

