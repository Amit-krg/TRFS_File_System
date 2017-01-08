#ifndef _OPERATIONS_H_
#define _OPERATIONS_H_


/*file operation 2 byte*/
#define OP_OPEN			0x01
#define OP_CLOSE		0x02
#define	OP_READ			0x04
#define	OP_WRITE		0x08
#define	OP_MKDIR		0x10
#define	OP_RMDIR		0x20
#define OP_LINK			0x40
#define OP_UNLINK		0x80
#define	OP_DELETE		0x100
#define	OP_RENAMEDIR	0x200

/*other operations*/
#define	OP_LS		0x1000
#define	OP_STATFS	0x2000
#define OP_ALL		0xFFFFFFFF


#define FEATURES 1
#define THREAD	2
#define EXTRA_FEATURES 3
/*PREAD=1, PWRITE=2,PEXEC=3, UNKNOWN=4*/
enum permission{
PREAD=1,		
PWRITE,
PEXEC,
UNKNOWN
};

/*
	<4B: record ID>
	<2B: record size that follows in bytes>
	<1B: record type>
	<4B: open flags>
	<4B: permission mode>
	<2B: length of pathname>
	<variable length: null-terminated pathname>
	<4B: return value or errno>
*/
typedef struct {
	int record_size;
	int record_id;
	unsigned long fp;
	unsigned short rsize;  
	int rtype;
	unsigned short flag;
	unsigned int perm; 
	unsigned short pathlength;
	int err;
	//char oldname[1024];
	char pname[1];
	//char newname[1];
	}record;

void structToBuffer(void *,record *);
typedef struct
{
	unsigned int data;
}bitmap;
#endif