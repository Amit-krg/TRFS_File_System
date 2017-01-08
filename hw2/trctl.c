#include "treplay.h"


#define all 0xffffffff
#define none 0
//#define IOCTL_DEVICE "/mnt/trfs"
#define SET 1
#define GET 0

int main(int argc, char **argv){

// void * mypointer;
 unsigned long bitmap;
 char *ioctl_device;
 int err,l,fd,type,help;
 err=0;
 l=0;
 help=0;
 bitmap=all;
 ioctl_device=malloc(1024*sizeof(char));
 #if EXTRA_CREDIT
 	if(argc <2)
 	{
 		err=-1;
 		printf("Input Should have an valid argument.Terimating..... \n");
 		help=1;
 	}
 	else
 	{
 		if(argc==2)
			{
				if(argv[1][0]!='/')// && argv[1][1]!='m' && argv[1][2]!='n' 
				//&& argv[1][3]!='t')
				{
					printf("Please input the proper path,e.g./mnt/trfs \n");
					err=-1;
					goto out;
					
				}
				else	
				 { 
					strcpy(ioctl_device,argv[1]);
					type=GET;
					printf("%s",ioctl_device);
				}
			}
		else
		{
			count=argc;

		}

 	}
 #else
 if(argc<2 || argc>3)
	{
		err=-1;
		printf("Please enter valid arguments. Terminating...\n");
		help=1;
	}	
 else
	{
		if(argc==2)
			{
				if(argv[1][0]!='/')// && argv[1][1]!='m' && argv[1][2]!='n' 
				//&& argv[1][3]!='t')
				{
					printf("Please input the proper path,e.g./mnt/trfs \n");
					err=-1;
					goto out;
					
				}
				else	
				 { 
					strcpy(ioctl_device,argv[1]);
					type=GET;
				}
				
			}
			if(argc==3)
			{
				strcpy(ioctl_device,argv[2]);
				switch(argv[1][0])
				{
					case 'a':
						if(argv[1][1]=='l' && argv[1][2]=='l')
						{
							bitmap&= all;
							type=SET;
						}
						else
						{
							err=-1;
							printf("Invalid Argument, it should be all. Terminating ....\n");
							goto out;
						}
						break;
					case 'n':
						if(argv[1][1]=='o' && argv[1][2]=='n' && argv[1][3]== 'e')
						{
							bitmap &= none;
							type=SET;
						}
						else
						{
							err=-1;
							printf("Invalid Argument, it should be none. Terminating ....\n");
							goto out;
						}
						break;
					case '0':					//case of 0XNN
						 l=strlen(argv[1]);
						 if(l<5 && l>2)
						{
							if(l==3)
								bitmap=argv[1][2]-'0';
							else
								bitmap=16*(argv[1][2]-'0')+argv[1][3]-'0';
								type=SET;
							if(bitmap>136)
								{
									printf("Not supported\n");	
									 help=1;
								}
						}
						else
						{
							err=-1;
							printf("Invalid Argument, it should be in the form of 0xNN where N is decimal number(1-8). Terminating ....\n");
							help=1
						}
						break;
					default:
						err=-1;
						printf("invalid argument, option should be in all, none and 0xNN \n");
						 help=1;
				}
			}
		}
	
#endif	

 if(help)
 {

  printf("USAGE:\n./treplay [OPTIONS] [INPUT] \nDESCRIPTION\n\tOPTIONS\n\t\tIt can in the form of"
  	 	"\"all\" , \"none\" or \"0xNN\"\n	INPUT\n\t Input the name of the trace file\n\n");
  printf("Following Syscall tracing is supported by the program:...\n\n" );
  printf("0x01: Track File Open Operation \n"); 
  printf("0x02: Track File Close Operation \n");
  printf("0x04: Treck File read Operation \n");
  printf("0x08: Track File Write Operation \n");
  printf("0x10: Track MKDIR Operation \n");
  printf("0x20: Track RMDIR Operation \n");
 // printf("0x40: Track Link Operation \n");
  printf("0x80: Track UNLINK Operation \n");
  printf("You can choose any combination of above to trace those particular set of operations\n");
  goto out;
}



 fd=open(ioctl_device,0644);
 if(fd<0)
 {
	printf("failed to open %s \n",ioctl_device);
	err=-1;
	goto out;
 }
 if(type==SET)
 {

 	ioctl(fd, TRFS_BITMAPSET, &bitmap);
	
  }
 if(type==GET)
 {
	err=ioctl(fd,TRFS_BITMAPGET,&bitmap);
	printf("Current BITMAP:%04lX\n",bitmap);
 }
  close(fd);
  
    out:
	if(err<0)
		return -1;
	else	
		return 0;

}