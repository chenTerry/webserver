#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <assert.h>

#define MAX_LINE 1024
#define BUG  1
#define MIN_LINE 256

#ifdef BUG
#define DEBUG_PRINT(str); printf(str);
#endif 

#ifndef S_ISDIR
#define S_ISDIR(x) ((x) & _S_IFDIR)
#endif

typedef struct stat file_stat_t;

extern int init(struct sockaddr_in * sin , int *lfd, int *port, char *path);
extern int error_page(int sock_fd);
extern int get_path(int cfd, char *path, char *uri);
extern int write_page(int cfd, int fd, const char *path,const char  *offset, const char *downbyte);

extern int isDir(const char *path);
extern int send_head(int cfd, const char *type, const char*value);
extern int64_t getFileSize(const char *file_name);
extern size_t url_encode(const char *src, size_t s_len, char *dst, size_t dst_len);

//#define ROOT_PATH  "/home/terry/test"
#define ROOT_PATH  "/card"

#endif
