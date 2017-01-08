#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#define PAGE_SIZE 4096
#define n_flag 0x01
#define s_flag 0x02
#include "treplay.h"

/*printing records info*/
void print_buf(struct tfile_read *buf)
{
	printf("id =%d,\n size=%hu\n,type=%hu, \n,path_len=%hu \n,retval=%d \n", (buf->id), (buf->size),(buf->type),(buf->path_len),(buf->retval));
}
/*replay the records based on flags*/
int syscall_replay(int fd,struct tfile_read *buf,char *path_old,u_int replay_mode,char *b)
{
	int ret,err=0;
	switch(buf->type)
	{
		case 1:
		{
			
			int tempfd, bytes=0;
			char *log_buffer = (char *)malloc(PAGE_SIZE); /*initialize buffer*/
			memset(log_buffer,'\0',PAGE_SIZE);
			char *replay_buffer = (char *)malloc(PAGE_SIZE);
			memset(replay_buffer,'\0',PAGE_SIZE);
			printf("read system call\n");
			read(fd,log_buffer,buf->bufsize); /*read buffer */
			if(!(replay_mode & n_flag))
			{
				printf("Replaying \n");
				tempfd=open(b,buf->perm);
				lseek(tempfd,buf->offset,SEEK_CUR);
				bytes=read(tempfd,replay_buffer,buf->bufsize);
				
			}
			if(replay_mode & s_flag) /* sflag check */
			{
				if (bytes != buf->retval)
				{							
					printf("Deviation occured\t retval=%d \tbytes=%d\n",buf->retval,bytes);
					err=-1;
					goto out; /* break if deviation occured*/
				}	
			}
			
			printf("bytes read from replay buffer=%d \n",bytes);
			if(log_buffer)
				free(log_buffer);
			if(replay_buffer)
				free(replay_buffer);
			break;
		}
		case 2:
		{
			printf("open system call \n");
			printf("open path : %s\n",b);
			printf("open flags : %d\n",buf->open_flags);
			if(!(replay_mode & n_flag))
			{	ret=open(b,buf->open_flags);
				printf("open returned %d\n",ret);
			}
			if(replay_mode & s_flag)
			{	if ( buf->retval>=0)				
				{
					if(ret<0)
					{
						 printf("Deviation occured while replaying\n");
        	                                err=-1;
                	                        goto out;
					}				
				}
				if(buf->retval<0)
				{
					if(buf->retval!=ret)
					{
						 printf("Deviation occured while replaying\n");
		                                 err=-1;
                	                        goto out;
					}
				}
			}
			break;
		}
		case 3:
		{
			printf("create system call\n");
			printf("path create %s\n",b);
			if(!(replay_mode & n_flag ))
			{	ret=creat(b,buf->perm);
				printf("file created with return value %d\n",ret);
			}
			if(replay_mode & s_flag)
			{	 if ( buf->retval>=0)
                                {
                                        if(ret<0)
                                        {
                                                 printf("Deviation occured while replaying\n");
                                                err=-1;
                                                goto out;
                                        }
                                }
                                if(buf->retval<0)
                                {
                                        if(buf->retval!=ret)
                                        {
                                                 printf("Deviation occured while replaying\n");
                                                 err=-1;
                                                goto out;
                                        }
                                }

			}
			break;
		}
		case 4:
		{
			printf("mkdir sys call \n");
			if(!(replay_mode & n_flag))
			{	ret=mkdir(b,buf->perm);
				printf("directory created with return value %d\n",ret);
			}
			if(replay_mode & s_flag)
			{	if (buf->retval!=ret)
				{	printf("Deviation occured while replaying\n");
					err=-1;
					goto out;
				}
			}
			break;
		}
		case 5:
		{
			int writefd,bytes=0;
			char *log_buffer = (char *)malloc(PAGE_SIZE);
			char *replay_buffer = (char *)malloc(PAGE_SIZE);
			read(fd,log_buffer,buf->bufsize);
			if(!(replay_mode & n_flag))
			{
				writefd=open(b,buf->open_flags);
				lseek(writefd,buf->offset,SEEK_CUR);			
				bytes=write(writefd,log_buffer,buf->bufsize);
			}
			if(replay_mode & s_flag)
			{	
				if (bytes!=buf->retval){
					printf("deviation occured \t retval=%d \treplayed_bytes=%d\n",buf->retval,bytes);
					err=-1;
					goto out;
				}	
			}
			printf("write system call \n");
			printf("bytes read from replay buffer=%d \n",bytes);
			if(log_buffer)
				free(log_buffer);
			if(replay_buffer)
				free(replay_buffer);

			break;	
		}
		case 6:
		{	
			int closefd;
			printf("close system call \n");
			if(!(replay_mode & n_flag))
				closefd=open(b,buf->open_flags);
				ret=close(closefd);
			if(replay_mode & s_flag)
			{	if (buf->retval!=ret)
				{	printf("Deviation occured while replaying\n");
					err=-1;
					goto out;
				}
			}
			break;
		}
		case 7:
		{
			printf("symlink system call \n");
			if(!(replay_mode & n_flag))
			{	ret=symlink(path_old,b);
				printf("link returned with error value %d\n",ret);
			}
			if(replay_mode & s_flag)
			{	if (buf->retval!=ret)
				{	
					printf("Deviation occured while replaying\n");
					err=-1;
					goto out;
				}
			}
			break;			
		}
		case 8:
		{
			printf("rmdir syscall \n");
			if(!(replay_mode & n_flag))
			{	ret=rmdir(b);			
				printf("directory removed with retrun value %d\n",ret);
			}
			if(replay_mode & s_flag)
			{	if (buf->retval!=ret)
				{	printf("Deviation occured while replaying\n");
					err=-1;
					goto out;
				}
			}
			break;
		}
		case 9:
		{
			printf("link sys call \n");	
			if(!(replay_mode & n_flag))
			{	ret=link(path_old,b);
				printf("link returned with error value %d\n",ret);
			}
			if(replay_mode & s_flag)
			{	if (buf->retval!=ret)
				{	
					printf("Deviation occured while replaying\n");
					err=-1;
					goto out;
				}
			}
			break;
		}
		case 10:
		{
			printf("unlink sys call \n");
			if(!(replay_mode & n_flag))
			{	ret=unlink(b);
				printf("link returned with error value %d\n",ret);
			}
			if(replay_mode & s_flag)
			{	if (buf->retval!=ret)
				{	
					printf("Deviation occured while replaying\n");
					err=-1;
					goto out;
				}
			}
			break;

		}
	}
out:
	return err;


}

int main(int argc, char *argv[])
{
        extern char *optarg;
        extern int optind;
	int c;
	int err,errno;
	int fd;
	int bytes_read=0;    
	struct tfile_read *buf=NULL;
	struct tfile_read *rec=NULL;
	char *path_old=NULL;
	char *o_path=NULL;
	char * filename=NULL;
        printf("start flags \n");
	u_int replay_mode;
        printf("before flags\n");
	buf=(struct tfile_read *)malloc(PAGE_SIZE);
	/*allocation failed */
	
	if (!buf)
        {
                printf("[user]: malloc FAILED");
                errno = -ENOMEM;
                goto out;
        }
	rec=(struct tfile_read *)malloc(sizeof(struct tfile_read));	
	if (!rec)
        {
                printf("[user]: malloc FAILED");
                errno = -ENOMEM;
                goto out;
        }
	memset(buf,'\0',PAGE_SIZE);
	memset(rec,'\0',sizeof(struct tfile_read));
	/*parsing command line options*/
	while ((c = getopt(argc, argv, "ns")) != -1)
        {
                switch (c)
                {
                        case 'n':
                                replay_mode|=n_flag;
                                break;
                        case 's':
                                replay_mode|=s_flag;
                                break;
			case '?':
				err=-1;
				goto out;
				
                }
        }
	
	filename=argv[optind];
	fd=open(filename,O_RDWR);
	if(fd==-1)
	{
		printf("[user]: FIle could not be opened\n");
		goto out;
	}
	
	if(fd>=0)
	{
		do{	/* read records*/				
			bytes_read=read(fd,buf,sizeof(struct tfile_read)-1);
			if (bytes_read < sizeof(struct tfile_read)-1)
				break;		
			print_buf(buf);			
			read(fd,buf->path,buf->path_len);
			char *b=buf->path+1;
				
			/*handling 2nd path of link*/	
			if( buf->type==9)
				{
					path_old=(char *)malloc(buf->path_len_old);
					lseek(fd,1,SEEK_CUR);
					read(fd ,path_old , buf->path_len_old);
					o_path=path_old+1;
					printf("old path= %s\n",path_old);
					printf("char o_path =%s\n",o_path);
					lseek(fd,-1,SEEK_CUR);
	
				}

			/*handling 2nd path of symlink */
			if(buf->type==7)
			{
				printf("symlink case \n");
				path_old=(char *)malloc(buf->path_len_old);
				read(fd ,path_old , buf->path_len_old);
				o_path=path_old+1;
				printf("old path= %s\n",path_old);
				//lseek(fd,-1,SEEK_CUR);
			}
			
	
			printf("path = %s\n",b);
			lseek(fd,1,SEEK_CUR);
			err=syscall_replay(fd,buf,path_old,replay_mode,b);
			if (err==-1)
				goto out;
			memset(buf->path,'\0',buf->path_len); /*resseting the path*/
	
		}while(1); /* exit from parsing*/ 
	}	
out:/*freeing buffers*/
	if(buf)
		free(buf);
	if(rec)
		free(rec);
	exit(err);
	
}
