#ifndef TREPLAY_H_
#define TREPLAY_H_
#pragma pack(1)
struct tfile_read {
        int id;
        unsigned short size;
        unsigned short type;
        int open_flags;
        int perm;
        unsigned short path_len;
	unsigned short path_len_old;
        int retval;
        //struct file *file;
        int count;	
        int offset;
	int bufsize;
        char path[1];
        };
        
#endif

