#include <linux/workqueue.h>
#include <linux/slab.h>
#include "treplay.h"
#include "trfs.h"

struct my_work_t
{
	struct work_struct my_work;
	int total_len;
	struct tfile_read *record;
	struct super_block *sb;
	char *tfile_buf;
	int buflen;
};
typedef struct my_work_t my_work_t;

static inline  int read_file(struct super_block *sb, struct tfile_read *r, int total_len, char *tfile_buf, int buflen){
	
	struct file *filp=NULL;
	int bytes_write=0;
	mm_segment_t oldfs;	
	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(TRFS_SB(sb)->tfile, O_CREAT|O_WRONLY | O_APPEND, 0);
	bytes_write = vfs_write(filp, (char *) r, total_len, &filp->f_pos);
      	if (bytes_write < 0) {
          printk(KERN_ALERT"[output file ] : vfs_write FAILED");

        }
	if (tfile_buf)
  	  bytes_write = vfs_write(filp, tfile_buf, buflen, &filp->f_pos);
	if (bytes_write < 0) {
          printk(KERN_ALERT"[output file ] : vfs_write FAILED");
        }
	
	filp_close(filp, NULL);
	set_fs(oldfs);
	if (tfile_buf)
    	  kfree(tfile_buf);
	if (r)
	  kfree(r);
	return bytes_write;
}
static inline void ework_handler(struct work_struct *pwork)
{
	my_work_t *temp;
	temp = container_of(pwork,my_work_t,my_work);
	read_file(temp->sb,temp->record,temp->total_len,temp->tfile_buf,temp->buflen);
	if(temp)
	{	
		kfree(temp);
	}
}

static inline void task(struct super_block *sb, struct tfile_read *record, int total_len, char *tfile_buf, int buflen)
{
	my_work_t *work;
	work = (my_work_t *) kmalloc(sizeof(my_work_t), GFP_KERNEL);   	   	
	work->record=record;
	work->sb=sb;
	work->total_len=total_len;
	work->tfile_buf=tfile_buf;
	work->buflen=buflen;

	INIT_WORK( &(work->my_work), ework_handler );
 	if (eWq)
    	{
        	printk("Push!! my work into the eWq--Queue Work..\r\n");
	        queue_work(eWq, &(work->my_work) );
    	}
	
}

extern int eworkqueue_init(void);
extern void eworkqueue_exit(void);

