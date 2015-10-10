#include "web_server.h"
static const struct {
  const char *extension;
  size_t ext_len;
  const char *mime_type;
} static_builtin_mime_types[] = {
  {".html", 5, "text/html"},
  {".htm", 4, "text/html"},
  {".shtm", 5, "text/html"},
  {".shtml", 6, "text/html"},
  {".css", 4, "text/css"},
  {".js",  3, "application/javascript"},
  {".ico", 4, "image/x-icon"},
  {".gif", 4, "image/gif"},
  {".jpg", 4, "image/jpeg"},
  {".jpeg", 5, "image/jpeg"},
  {".png", 4, "image/png"},
  {".svg", 4, "image/svg+xml"},
  {".txt", 4, "text/plain"},
  {".torrent", 8, "application/x-bittorrent"},
  {".wav", 4, "audio/x-wav"},
  {".mp3", 4, "audio/x-mp3"},
  {".mid", 4, "audio/mid"},
  {".m3u", 4, "audio/x-mpegurl"},
  {".ogg", 4, "application/ogg"},
  {".ram", 4, "audio/x-pn-realaudio"},
  {".xml", 4, "text/xml"},
  {".json",  5, "application/json"},
  {".xslt", 5, "application/xml"},
  {".xsl", 4, "application/xml"},
  {".ra",  3, "audio/x-pn-realaudio"},
  {".doc", 4, "application/msword"},
  {".exe", 4, "application/octet-stream"},
  {".zip", 4, "application/x-zip-compressed"},
  {".xls", 4, "application/excel"},
  {".tgz", 4, "application/x-tar-gz"},
  {".tar", 4, "application/x-tar"},
  {".gz",  3, "application/x-gunzip"},
  {".arj", 4, "application/x-arj-compressed"},
  {".rar", 4, "application/x-rar-compressed"},
  {".rtf", 4, "application/rtf"},
  {".pdf", 4, "application/pdf"},
  {".swf", 4, "application/x-shockwave-flash"},
  {".apk", 4, "application/vnd.android.package-archive"},
  {".mpg", 4, "video/mpeg"},
  {".webm", 5, "video/webm"},
  {".mpeg", 5, "video/mpeg"},
  {".mov", 4, "video/quicktime"},
  {".mp4", 4, "video/mp4"},
  {".m4v", 4, "video/x-m4v"},
  {".asf", 4, "video/x-ms-asf"},
  {".avi", 4, "video/x-msvideo"},
  {".bmp", 4, "image/bmp"},
  {".ttf", 4, "application/x-font-ttf"},
  {NULL,  0, NULL}
};
/**
* -1 出错， 0不是， 1是
*/
int isDir(const char *file_name)
{
	 file_stat_t st;
   int exists = stat(file_name, &st) == 0;
   if(!exists)
   {
   		return -1;
   	}
   int is_directory = S_ISDIR(st.st_mode);
   return is_directory;
}

int isFileExist(const char * savePath) {  
    if (!access(savePath, F_OK)) {  
        return 1;  
    } else {  
        return 0;  
    }  
}  

int64_t getFileSize(const char *file_name)
{
	 file_stat_t st;
   int exists = stat(file_name, &st) == 0;
   int64_t size = 0;
   if(!exists)
   {
   		return -1;
   	}
   size = st.st_size;
   return size;
}

size_t url_encode(const char *src, size_t s_len, char *dst, size_t dst_len) {
  static const char *dont_escape = "._-$,;~()";
  static const char *hex = "0123456789abcdef";
  size_t i = 0, j = 0;

  for (i = j = 0; dst_len > 0 && i < s_len && j + 2 < dst_len - 1; i++, j++) {
    if (isalnum(* (const unsigned char *) (src + i)) ||
        strchr(dont_escape, * (const unsigned char *) (src + i)) != NULL) {
      dst[j] = src[i];
    } else if (j + 3 < dst_len) {
      dst[j] = '%';
      dst[j + 1] = hex[(* (const unsigned char *) (src + i)) >> 4];
      dst[j + 2] = hex[(* (const unsigned char *) (src + i)) & 0xf];
      j += 2;
    }
  }

  dst[j] = '\0';
  return j;
}
int url_decode(const char *src, size_t src_len, char *dst,
                  size_t dst_len, int is_form_url_encoded) {
  size_t i, j = 0;
  int a, b;
#define HEXTOI(x) (isdigit(x) ? (x) - '0' : (x) - 'W')

  for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
    if (src[i] == '%' && i + 2 < src_len &&
        isxdigit(* (const unsigned char *) (src + i + 1)) &&
        isxdigit(* (const unsigned char *) (src + i + 2))) {
      a = tolower(* (const unsigned char *) (src + i + 1));
      b = tolower(* (const unsigned char *) (src + i + 2));
      dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
      i += 2;
    } else if (is_form_url_encoded && src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }

  dst[j] = '\0'; // Null-terminate the destination

  return i >= src_len ? j : -1;
}

static int get_var(const char *data, size_t data_len, const char *name,
                   char *dst, size_t dst_len) {
  int n=0;
  const char *p, *e = data + data_len, *s;
  size_t name_len;
  int i = 0, len = -1;

  if (dst == NULL || dst_len == 0) {
    len = -2;
  } else if (data == NULL || name == NULL || data_len == 0) {
    dst[0] = '\0';
  } else {
    name_len = strlen(name);
    dst[0] = '\0';

    // data is "var1=val1&var2=val2...". Find variable first
    for (p = data; p + name_len < e; p++) {
      if ((p == data || p[-1] == '&') && p[name_len] == '=' &&
          !strncasecmp(name, p, name_len)) {

        if (n != i++) continue;

        // Point p to variable value
        p += name_len + 1;

        // Point s to the end of the value
        s = (const char *) memchr(p, '&', (size_t)(e - p));
        if (s == NULL) {
          s = e;
        }
        assert(s >= p);

        // Decode variable into destination buffer
        len = url_decode(p, (size_t)(s - p), dst, dst_len, 1);

        // Redirect error code from -1 to -2 (destination buffer too small).
        if (len == -1) {
          len = -2;
        }
        break;
      }
    }
  }

  return len;
}

time_t getFileDate(const char *file_name)
{
	 file_stat_t st;
   int exists = stat(file_name, &st) == 0;
   int64_t size = 0;
   if(!exists)
   {
   		return -1;
   	}
   
   return st.st_mtime;
}


static int lowercase(const char *s) {
  return tolower(* (const unsigned char *) s);
}

static int mg_strcasecmp(const char *s1, const char *s2) {
  int diff;

  do {
    diff = lowercase(s1++) - lowercase(s2++);
  } while (diff == 0 && s1[-1] != '\0');

  return diff;
}

const char *mg_get_mime_type(const char *path, const char *default_mime_type) {
  const char *ext;
  size_t i, path_len;

  path_len = strlen(path);

  for (i = 0; static_builtin_mime_types[i].extension != NULL; i++) {
    ext = path + (path_len - static_builtin_mime_types[i].ext_len);
    if (path_len > static_builtin_mime_types[i].ext_len &&
        mg_strcasecmp(ext, static_builtin_mime_types[i].extension) == 0) { 
      return static_builtin_mime_types[i].mime_type;
    }
  }


  return default_mime_type;
}

static int configuration(int *port, char *path)
{
	
	int i;
	FILE *fp;
	char *p;
	char buf[50];
	fp = fopen("./config.ini", "r");
	
	if(fp == NULL)	{
		perror("fail to open config.ini");	
		return -1;
		
	}
	
	while(fgets(buf, 50, fp) != NULL)
	{
		if(buf[strlen(buf) -1] !='\n')
		{
			printf("error in config.ini\n");
			return -1;		
		}else{
			buf[strlen(buf) -1] = '\0';	
		}
		
		if(strstr(buf, "port") == buf)
		{
				if((p = strchr(buf, ':')) == NULL)
				{
					printf("config.ini expect ':'\n");
					return -1;
				}
				*port = atoi(p+2);
				if(*port <=0)
				{
						printf("error port\n");
						return -1;
				}
		}else if(strstr(buf, "root-path")  == buf)
		{
				if((p = strchr(buf, ':')) == NULL)
				{
					printf("config.ini expect ':'\n");
					return -1;
				}
				p = strchr(p,'/');
				strcpy(path, p);
			
		}else{
			printf("error in config.ini\n");	
			return -1;
		}

	}
	return 0;
}


int init(struct sockaddr_in *sin, int *lfd, int *port, char *path)
{
	int tfd;
	//configuration(port, path);
	*port = 10000;
	//strcpy(path, "/home/terry/test");
	strcpy(path, ROOT_PATH);
	bzero(sin, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	sin->sin_port = htons(*port);
	if((tfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
			perror("fail to creat socket");
			return -1;
	}
	
	if((bind(tfd, (struct sockaddr *)sin, sizeof(struct sockaddr_in))) == -1)
	{
		perror("fail to bind");
		return -1;
	}
	
	if((listen(tfd, 20)) == -1)
	{
			perror("fail to listen");
			return -1;
	}
	
	*lfd = tfd;
	return 0;
}


int get_path(int cfd, char *path, char *uri)
{
	char buf[MAX_LINE]={0};
	char tmp[MIN_LINE] = {0};
	if(read(cfd, buf, MAX_LINE) == -1)
	{
		return -1;
	}
	
	if(strstr(buf, "GET") != buf)
	{
		DEBUG_PRINT("wrong request\n");
		return -1;
	}
	printf("request:%s\n", buf);
	if((buf[4] == '/')&&(buf[5]) ==' ')
	{
			sprintf(tmp, "%s/index.html", path);
			if(isFileExist(tmp) == 1)
			{
				strcat(path, "/index.html");
			}
			strcpy(uri, "/");
			
	}else{
			strtok(&buf[4], " ");
			strcat(path, &buf[4]);
			strcpy(uri, &buf[4]);
	}
	printf("path:%s\nuri:%s\n", path, uri);
	return 0;
}


int error_page(int sock_fd)
{
	char err_str[1024];
	
	#ifdef DEBUG
	
		sprintf(err_str, "HTTP/1.1 404 %s\r\n", strerror(errno.h));	
	
	#else
		printf(err_str, "HTTP/1.1 404 not exist\r\n");
	#endif
	
	if(write(sock_fd, err_str, strlen(err_str)) == -1)
	{
		return -1;
	}
	
	if(write(sock_fd, "Content-Type: text/html\r\n\r\n", strlen("Content-Type: text/html\r\n\r\n")) == -1)
	{
			return -1;
	}
	if(write(sock_fd, "<html><body>the file dose not exsit</body></html>", strlen("<html><body>the file dose not exsit</body></html>"))== -1)
	{
			return -1;
	}
	
	return 0;
}



int write_page(int cfd, int fd, const char *path,const char  *offset, const char *downbyte)
{
	int n;
	char buf[MAX_LINE];
	const char *filetype = NULL;
	char size[20] = {0};
	int off_set = 0;
	int down_byte = 0;
	if(offset != NULL)
	{
		off_set = atoi(offset);
		off_set = off_set>0?off_set:0;
	}
	if(downbyte != NULL)
	{
			down_byte = atoi(downbyte);
			down_byte = down_byte>0?down_byte:0;
	}
	if(write(cfd, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n")) == -1)
	{
		return -1;
	}
	filetype = mg_get_mime_type(path, "text/plain;charset=utf-8");
	printf("file type is :%s\n", filetype);
	send_head(cfd, "Content-Type", filetype);
	send_head(cfd, "Connection", "keep-alive");
	if(down_byte > 0)
	{
		sprintf(size, "%d", down_byte);
	}else{
		sprintf(size, "%lld", getFileSize(path));
	}
	
	send_head(cfd, "Content-Length", size);
	send_head(cfd, "Accept-Ranges", "bytes");
	
	if(write(cfd, "\r\n", strlen("\r\n")) == -1)
		{
			return -1;
		}
	

	
	if(off_set > 0)
	{
		
  		if(lseek(fd, off_set,SEEK_SET) <0 || down_byte <0)
  		{
  			return -1;
  		}
  	
	}
	
	if(down_byte > 0)
  {
    	 while ((n = read(fd,  buf,MAX_LINE>down_byte?down_byte:MAX_LINE)) > 0) {
	    	
	      if(write(cfd, buf, n) == -1)
				{
					return -1;
				}	
		
	      down_byte = down_byte - n;
	    }
  }else{
  	
  	while((n = read(fd, buf, MAX_LINE))>0)
		{
			if(write(cfd, buf, n) == -1)
			{
				return -1;
			}	

		}
  
  }
    
    
	
	if(write(cfd, "\r\n", strlen("\r\n")) == -1)
		{
			return -1;
		}
		
	return 0;
	
}


int send_dir_list(int cfd,const char*dir, const char *uri)
{
		char buf[MAX_LINE*2]  = {0};
		struct dirent *dp;
		int is_dir = 0;
		char size[64]={0};
		char  mod[64] = {0};
		time_t t;
		int64_t fsize = 0;
		char path[MAX_LINE] ={0};
		char href[MAX_LINE]={0};
		char filepath[MIN_LINE] = {0};
		DIR *dirp;
		
		if(write(cfd, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n")) == -1)
		{
			return -1;
		}	
		
		send_head(cfd, "Connection", "close");
  	send_head(cfd, "Content-Type", "text/html; charset=utf-8");
  	if(write(cfd, "\r\n", strlen("\r\n")) == -1)
		{
			return -1;
		}	
  	//send_head(cfd, "Content-Type", "text/html");
  	sprintf(buf,  "<html><head><title>Index of %s</title>"
              "<style>th {text-align: left;}</style></head>"
              "<body><h1>Index of %s</h1><pre><table cellpadding=\"0\">"
              "<tr><th><a href=\"?nd\">Name</a></th>"
              "<th><a href=\"?dd\">Modified</a></th>"
              "<th><a href=\"?sd\">Size</a></th></tr>"
              "<tr><td colspan=\"3\"><hr></td></tr>",
              uri, uri);
     printf("%s\n", buf);         
    if(write(cfd, buf, strlen(buf)) == -1)
		{
			return -1;
		}	  
		
		if ((dirp = (opendir(dir))) == NULL) return 0;
			
		 while ((dp = readdir(dirp)) != NULL) {
    // Do not show current dir and hidden files
    		if (!strcmp(dp->d_name, ".") ||
        		!strcmp(dp->d_name, "..") ) {
        			
      			continue;
   		 }
   		 
   		 snprintf(path,MAX_LINE, "%s/%c/%s", dir, '/',  dp->d_name);
   		 is_dir = isDir(path);
   		 const char *slash = is_dir ? "/" : "";
   		 if(is_dir)
   		 {
   		 		snprintf(size, 64, "[DIRECTORY]");
   		 }else{
   		 		fsize = getFileSize(path);
   		 		if (fsize < 1024) {
     			 	 snprintf(size, sizeof(size), "%d", (int) fsize);
   				} else if (fsize < 0x100000) {
     				 snprintf(size, sizeof(size), "%.1fk", (double) fsize / 1024.0);
   				} else if (fsize < 0x40000000) {
     				 snprintf(size, sizeof(size), "%.1fM", (double) fsize / 1048576);
   				} else {
    				 snprintf(size, sizeof(size), "%.1fG", (double) fsize / 1073741824);
    			}
   		 	
   		 	}
   		 	
   		 	t = getFileDate(path);
   		  strftime(mod, sizeof(mod), "%d-%b-%Y %H:%M", localtime(&t));
   		  /*
   		  bzero(filepath, MIN_LINE);
   		  
   		  if(strcmp(uri, "/") == 0)
   		  {
   		  	  snprintf(filepath,MIN_LINE,"%s",  dp->d_name);
   		  }else{
   		  	  snprintf(filepath,MIN_LINE,"%s%s", uri, dp->d_name);
   		  }
   		
   		  printf("%s\n", filepath);
   		  url_encode(filepath, strlen(filepath), href, sizeof(href));
   		  */
   		  url_encode(dp->d_name, strlen(dp->d_name), href, sizeof(href));
   		  bzero(buf,MAX_LINE*2);
   		  snprintf(buf,MAX_LINE*2, "<tr><td><a href=\"%s%s\">%s%s</a></td>"
                  "<td>&nbsp;%s</td><td>&nbsp;&nbsp;%s</td></tr>\n",
                  href, slash, dp->d_name, slash, mod, size );
       
       printf("%s\n", buf);
   		if(write(cfd, buf, strlen(buf)) == -1)
			{
				return -1;
			}	 
    }       
   	if (write(cfd, "</body></html>", strlen("</body></html>")) == -1)
   	{
   		return -1;
   	}
   printf("</body></html>");
   return 0;
}


int send_head(int cfd, const char *type, const char*value)
{
		char tmp[MIN_LINE] = {0};
		sprintf(tmp, "%s: %s\r\n", type, value);
		printf("%s\n", tmp);
		if(write(cfd, tmp, strlen(tmp)) == -1)
		{
			return -1;
		}
		
}

int getPara(const char *data, char *file, char *offset, char *downbyte)
{
	if(strstr(data, "&") == NULL)
	{
		strcpy(file, data);
		return 0;
	}
	strncpy(file, data, strstr(data, "&")-data);
	get_var(data, strlen(data), "file_offset", offset, 20);
	get_var(data, strlen(data), "download_bytes", downbyte, 20);
	return 0;
}
