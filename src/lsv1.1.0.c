/*
* Programming Assignment 02: lsv1.1.0
* Version 1.1.0 - Complete Long Listing Format
* Usage:
*       $ lsv1.0.0
*       $ lsv1.0.0 -l
*       $ lsv1.0.0 /home
*       $ lsv1.0.0 -l /home/kali/ /etc/
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

extern int errno;
extern int optind;

void do_ls(const char *dir);
void do_ls_long(const char *dir);

int main(int argc, char *argv[])
{
    int opt;
    int long_listing = 0;

    // Parse -l flag
    while ((opt = getopt(argc, argv, "l")) != -1) {
        if (opt == 'l') {
            long_listing = 1;
        }
    }

    if (optind == argc) {
        // No directories given, use current directory
        if (long_listing)
            do_ls_long(".");
        else
            do_ls(".");
    } else {
        // Directories provided
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            if (long_listing)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i]);
            puts("");
        }
    }

    return 0;
}

void do_ls(const char *dir)
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
        printf("%s\n", entry->d_name);
    }

    if (errno != 0) {
        perror("readdir failed");
    }

    closedir(dp);
}

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

