#include "web_server.h"

int main(int argc, char *argv[])
{
	struct sockaddr_in sin, cin;
	socklen_t len = sizeof(cin);
	int lfd, cfd, fd;
	pid_t pid;
	int sock_opt = 1;
	int port;
	int is_dir = 0;
	char path[MAX_LINE] = {0};
	char uri[MIN_LINE] = {0};
	struct stat statbuf;
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	
	printf("initializing ...\n");
	
	if(init(&sin, &lfd, &port, path) == -1){
		
		DEBUG_PRINT("perror during initializing\n");
		exit(1);	
	}
	
	while(1){
		
		DEBUG_PRINT("waiting connecting ...\n");
		cfd = accept(lfd, (struct sockaddr *)&cin, &len);
		
		if(cfd == -1)
		{
			perror("fail to accept");
			exit(1);
		}
		pid = fork();	
		if(pid < 0)
		{
			perror("fail to fork");
			exit(1);
		}else if(pid == 0)
		{
				close(lfd);
				bzero(uri, MIN_LINE);
				if(get_path(cfd, path, uri) == -1)
				{
					 DEBUG_PRINT("error during geting filepath\n");
					 exit(1);
				}
				char file[MIN_LINE] = {0};
				char offset[20] = {0};
				char downbyte[20] = {0};
				getPara(path, file, offset, downbyte);
				printf("file:%s\toffset:%s\tdownbyte:%s\n", file, offset, downbyte);
				printf("request file:[%s]\n", file);
				if(fstat(cfd, &statbuf) < 0)
				{
					perror("fail to get file status");
					exit(1);
				}
				/*
				if(!S_ISREG(statbuf.st_mode)){
					if(error_page(cfd) == -1)
					{
							DEBUG_PRINT("error during writing error_page\n");
							close(fd);
							exit(1);
					}	
					close(cfd);
					exit(0);
					
				}
				
				
				if(statbuf.st_mode & S_IXOTH)
				{            
					dup2(cfd, STDOUT_FILENO);
					if(execl(path, path,NULL) == -1)
					{
						perror("fail to exec");
						exit(1);
					}
				}
				*/
				
				is_dir = isDir(file);
				printf("%s is path [%d]\n", file,is_dir);
				if(is_dir == 1)
				{
						send_dir_list(cfd, file, uri);
						
						close(cfd);
						exit(0);
				}
			
			
				if((fd = open(file, O_RDONLY)) < 0)
				{
					if(error_page(cfd) == -1)
					{
							DEBUG_PRINT("error during writeing error-page\n");
							close(cfd);
							exit(1);
					}
					close(cfd);
					exit(0);
				}
				if(write_page(cfd, fd, file, offset, downbyte) == -1)
				{
					DEBUG_PRINT("error during writing page\n");
					exit(1);
				}
				close(fd);
				close(cfd);
				exit(0);
		}else{
			close(cfd);
		}
		
	}
	return 0;

}
