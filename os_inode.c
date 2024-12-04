#include "os_inode.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

void pr_path(const char *fname) {
    printf("Path: ");
    if (fname[0] == '/') {
        printf("%s\n\n", fname);
        return;
    }
    else if (fname[0] == '.' && (fname[1] == '/' || fname[1] == '.')) {
        char *p = realpath(fname, NULL);
        if (!p) {
            if (errno == EACCES) {
                puts("Permission denied\n");
                return;
            }
            else return;
        }
        printf("%s\n\n", p);
        free(p);
        return;
    }
    size_t psize = (4096 + (strlen(fname) + 1)) * sizeof(char);
    char *path = malloc(psize);
    if (!path) {
        perror("Malloc");
        exit(errno);
    }
    path = getcwd(path, psize);
    if (path == NULL) {
        if (errno == EACCES) puts("Permission denied\n");
        else if (errno == ERANGE) {
            do {
                psize *= 2;
                path = realloc(path, psize);
                if (!path) {
                    perror("Realloc");
                    exit(errno);
                }
                path = getcwd(path, psize);
            } while (path == NULL && errno == ERANGE);
        }
        else {
            perror("Getcwd");
            exit(errno);
        }
    }
    strcat(path, "/");
    strcat(path, fname);
    printf("%s\n\n", path);
    free(path);
}

void pr_inode(ino_t ino) {
    printf("Inode:\t\t\t%ld\n", ino);
}

void pr_home(dev_t dev) {
    printf("Home:\t\t\t%ld\t\t", dev);
    char dev_ver[128], buf[256];
    if (sprintf(dev_ver, "%d:%d", (int)dev >> 8, (int)dev & 0xff) < 0) {
        printf("\n");
        return;
    }
    FILE *fp = fopen("/proc/self/mountinfo", "r");
    if (!fp) {
        printf("\n");
        return;
    }
    char *tok, *loc;
    while (fgets(buf, sizeof(buf), fp)) {
        tok = strtok(buf, "-");
        loc = strstr(tok, dev_ver);
        if (loc) {
            for (int i = 0; i < 2; i++) {
                tok = strtok(NULL, " -:");
            }
            break;
        }
    }
    fclose(fp);
    printf("%s\n", (loc) ? tok : " ");
}

void pr_uid(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    printf("UID:\t\t\t%u\t\t%s\n", uid, (pw) ? pw->pw_name : " "); 
}

void pr_gid(gid_t gid) {
    struct group *gr = getgrgid(gid);
    printf("GID:\t\t\t%u\t\t%s\n", gid, gr->gr_name);
}

void pr_owners(uid_t uid, gid_t gid) {
    pr_uid(uid);
    pr_gid(gid);
}

char* ret_ftype(mode_t mode) {
    switch (mode & __S_IFMT) {
        case __S_IFDIR: return "Directory";
        case __S_IFBLK: return "Block device";
        case __S_IFCHR: return "Char. device";
        case __S_IFREG: return "Regular file";
        case __S_IFIFO: return "Pipe/FIFO";
        case __S_IFSOCK: return "Socket";
        case __S_IFLNK: return "Symbolic link";
        default: return "Unknown";
    }
}

void pr_type(struct stat *fstat) {
    printf("Type:\t\t\t%s\n", ret_ftype(fstat->st_mode));
    if (S_ISBLK(fstat->st_mode) || S_ISCHR(fstat->st_mode)) {
        printf("Dev. ID:\t\t%ld\n", fstat->st_rdev);
    }
}

void get_entcount(const char *fname) {
    printf("# of entries:\t\t");
    int count = 0;
    DIR *d = opendir(fname);
    if (!d) {
        if (errno == EACCES) {
            printf("Permission denied\n");
            return;
        }
        else if (errno == ELOOP) {
            printf("Unknown (symlink loop)\n");
            return;
        }
        else {
            perror("");
            return;
        }
    } 
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
        if (ent->d_type == DT_REG || ent->d_type == DT_DIR || ent->d_type == DT_LNK)
            count++;
    }
    printf("%d\n", count);
}

void pr_blksize(size_t blksize) {
    printf("Block size:\t\t%ld\n", blksize);
}

void pr_permissions(mode_t mode) {
    printf("Permissions:\t\t");
    int bits = mode;
    for (int i = 0; i < 3; i++) {
        bits = mode;
        bits &= 0x1C0 >> (3 * i);
        bits = bits << (3 * i);
        for (int k = 0; k < 3; k++) {
            if ((bits & 0x100) == 0x100) {
                switch (k) {
                    case 0:
                        printf("r");
                        break;
                    case 1:
                        printf("w");
                        break;
                    case 2:
                        printf("x");
                        break;
                    default: break;
                }
            }
            else printf("-");
            bits = bits << 1;
        }
    }
    printf("\n");
}

void pr_size(size_t size) {
    int mb = 1024 * 1024;
    printf("Size:\t\t\t");

    if (size >= 1024 && size < mb)
        printf("%.2fK\n", (float)size / 1024);
    else if (size >= mb && size < (mb * 1024))
        printf("%.2fM\n", (float)size / mb);
    else if (size > (mb * 1024))
        printf("%.2fG\n", (float)size / (mb * 1024));
    else
        printf("%lu\n", size);
}

void print_inode_info(const char *fname, struct stat *fstat) {
    pr_path(fname);
    pr_inode(fstat->st_ino);
    pr_home(fstat->st_dev);
    pr_owners(fstat->st_uid, fstat->st_gid); 
    pr_type(fstat);
    if (S_ISDIR(fstat->st_mode))
        get_entcount(fname);
    pr_permissions(fstat->st_mode);
    pr_size(fstat->st_size);
}