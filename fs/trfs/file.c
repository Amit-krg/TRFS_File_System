/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "trfs.h"
#include "treplay.h"
#include "queue.h"
#define MAJOR_NUM 200
#define IOCTL_BITMAP_SET_MAP _IOW(MAJOR_NUM,0,char *)
#define IOCTL_BITMAP_GET_MAP _IOR(MAJOR_NUM+1,0,char *)

static ssize_t trfs_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{	
	int len=0,total_len=0,ofst=*ppos,err,ret;
	struct tfile_read *r = NULL;
	char *tfile_buf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);	

	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	memset(tfile_buf,'\0',PAGE_SIZE);
	lower_file = trfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0)
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
	ret=copy_from_user(tfile_buf, buf, strlen(buf));
	if (ret<0){
		printk("copy from user failed\n");
		
	}
        if(TRFS_SB(file->f_path.dentry->d_sb)->bitmap & 0x00000001)
	{
	len=strlen(dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE));
	//len=strlen(file->f_path.dentry->d_iname);
        total_len = (sizeof(struct tfile_read)+len+1);
        r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(file->f_inode->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(file->f_inode->i_sb)->count));
        r->size=total_len -6;
        r->type=1;
        r->open_flags=file->f_flags;
        r->perm=file->f_path.dentry->d_inode->i_mode;
        r->path_len=len+1;
        r->retval=err;
        r->offset=ofst;
        r->count=count;
       	r->bufsize= strlen(tfile_buf);
       	strncpy(r->path,dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE), r->path_len);
        //strncpy(r->path,file->f_path.dentry->d_iname, r->path_len);
        strcat(r->path,"\0");

	if (tempbuf)
  	  kfree(tempbuf);
	printk("REACHING HERE !!!\n");
 	task(file->f_inode->i_sb, r, total_len,tfile_buf,r->bufsize);
   /*	out:
	  if(tfile_buf)
	     kfree(tfile_buf); */
	}
	return err;
}

static ssize_t trfs_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{	
	int len=0,total_len=0,ofst=*ppos,err,ret;
	struct tfile_read *r = NULL;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);	
	char *tfile_buf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	//int bufsize=strlen(buf);
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	
	memset(tfile_buf,'\0',PAGE_SIZE);
	lower_file = trfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
		fsstack_copy_inode_size(d_inode(dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(dentry),
					file_inode(lower_file));
	}
	ret=copy_from_user(tfile_buf, buf, strlen(buf));
	if (ret<0){
		printk("copy from user failed\n");
		
	}
	if(TRFS_SB(file->f_path.dentry->d_sb)->bitmap & 0x00000010)
	{
	//len=strlen(file->f_path.dentry->d_iname);
	len=strlen(dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE));
        total_len = (sizeof(struct tfile_read)+len+1);
        r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(file->f_inode->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(file->f_inode->i_sb)->count));
        r->size=total_len -6;
        r->type=5;
        r->open_flags=file->f_flags;
        r->perm=file->f_path.dentry->d_inode->i_mode;
        r->path_len=len+1;
        r->retval=err;
        r->offset=ofst;
        r->count=count;
        r->bufsize= count;
        //strncpy(r->path,file->f_path.dentry->d_iname, r->path_len);
       	strncpy(r->path,dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE), r->path_len);
        strcat(r->path,"\0");
        printk("size of struct =%lu\n",sizeof(struct tfile_read));
        printk("total_len=%d\n",total_len);
        printk("sizeof r =%lu\n",sizeof(r));
        printk("strlen=%d\n",len);
        printk("strlen container= %zu \n",strlen(r->path));
	printk("after call offset=%d\n",r->offset);
        printk("!!!!!!!!!!!!!!buffer= %s\n", buf);
        printk("!!!cOUNT =%zu", count);
        printk("id =%d,\n size=%hu\n,type=%hu,open_flags=%d\n,perm=%d \n,path_len=%hu \n,retval=%d \n,path=%s\n",
         (r->id),
        (r->size),
        (r->type),
        (r->open_flags),
        (r->perm),
        (r->path_len),
        (r->retval),
        (r->path));
	
 	task(file->f_inode->i_sb, r, total_len,tfile_buf,r->bufsize);
	}
	return err;
}

static int trfs_readdir(struct file *file, struct dir_context *ctx)
{
	int err;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = trfs_lower_file(file);
	err = iterate_dir(lower_file, ctx);
	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
	printk("file.c readdir \n");
	return err;
}
static long trfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	long err = 0;
	int ret=0;
	struct file *lower_file;
	int *bitmap_arg;	
	int bitmap=0xffffffff;
	bitmap_arg=&bitmap;	
	switch(cmd)
	{
		case IOCTL_BITMAP_SET_MAP:
		{
			ret=copy_from_user(bitmap_arg,(int *)arg,sizeof(int));
			TRFS_SB(file->f_path.dentry->d_sb)->bitmap=bitmap;	
			break;
		}
		case IOCTL_BITMAP_GET_MAP:
		{
			bitmap=TRFS_SB(file->f_path.dentry->d_sb)->bitmap;
			ret=copy_to_user((int *)arg,bitmap_arg,sizeof(int));
			break;
		}
		default:
		{
			err=-ENOTTY;
			lower_file = trfs_lower_file(file);	
			/* XXX: use vfs_ioctl if/when VFS exports it */
			if (!lower_file || !lower_file->f_op)
				goto out;
			if (lower_file->f_op->unlocked_ioctl)
				err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

			/* some ioctls can change inode attributes (EXT2_IOC_SETFLAGS) */
			if (!err)
				fsstack_copy_attr_all(file_inode(file),
			file_inode(lower_file));
			break;
		}
	}
out:
	return err;
}

#ifdef CONFIG_BITMAP
static long trfs_bitmap_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	printk("In bitamp hello \n");
	lower_file = trfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->bitmap_ioctl)
		err = lower_file->f_op->bitmap_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif


#ifdef CONFIG_COMPAT
static long trfs_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = trfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int trfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappings won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = trfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		printk(KERN_ERR "trfs: lower file system does not "
		       "support writeable mmap\n");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!TRFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			printk(KERN_ERR "trfs: lower mmap failed %d\n", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	vma->vm_ops = &trfs_vm_ops;

	file->f_mapping->a_ops = &trfs_aops; /* set our aops */
	if (!TRFS_F(file)->lower_vm_ops) /* save for our ->fault */
		TRFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	printk("mmap file.c \n");
	return err;
}

static int trfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	int len=0,total_len=0 ;
	struct tfile_read *r;
	struct file *lower_file = NULL;
	struct path lower_path;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	/* don't open unhashed/deleted files */
	if (d_unhashed(file->f_path.dentry)) {
		err = -ENOENT;
		goto out_err;
	}

	file->private_data =
		kzalloc(sizeof(struct trfs_file_info), GFP_KERNEL);
	if (!TRFS_F(file)) {
		err = -ENOMEM;
		goto out_err;
	}

	/* open lower object and link trfs's file struct to lower's */
	trfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
	path_put(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		lower_file = trfs_lower_file(file);
		if (lower_file) {
			trfs_set_lower_file(file, NULL);
			fput(lower_file); /* fput calls dput for lower_dentry */
		}
	} else {
		trfs_set_lower_file(file, lower_file);
	}

	if (err)
		kfree(TRFS_F(file));
	else
		fsstack_copy_attr_all(inode, trfs_lower_inode(inode));
out_err:
	
	if(TRFS_SB(file->f_path.dentry->d_sb)->bitmap & 0x00000002)
	{
		len=strlen(dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE));
        	total_len = (sizeof(struct tfile_read)+len+1);
        	r = kmalloc(total_len, GFP_KERNEL);
       	 	atomic_inc(&(TRFS_SB(file->f_inode->i_sb)->count));
        	r->id =  atomic_read(&(TRFS_SB(file->f_inode->i_sb)->count));
        	r->size=total_len -6;
       		 r->type=2;
        	r->open_flags=file->f_flags;
        	r->perm=file->f_path.dentry->d_inode->i_mode;
        	r->path_len=len+1;
		r->path_len_old=0;
        	r->retval=err;
        	r->offset=0;
        	r->count=0;
       		r->bufsize= 0;
       		strncpy(r->path,dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE), r->path_len);
        	strcat(r->path,"\0");
        	task(file->f_inode->i_sb, r, total_len,NULL,0);          
        
	}
		if (tempbuf)
  			kfree(tempbuf);

	return err;
}

static int trfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		err = lower_file->f_op->flush(lower_file, id);
	}
	printk("in file.c flush \n");
	return err;
}

/* release all lower object references & free the file info structure */
static int trfs_file_release(struct inode *inode, struct file *file)
{      

	int len=0,total_len=0 ;
        struct tfile_read *r;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);

	struct file *lower_file;

	lower_file = trfs_lower_file(file);
	if (lower_file) {
		trfs_set_lower_file(file, NULL);
		fput(lower_file);
	}

	kfree(TRFS_F(file));
	printk("in file.c file_release \n");
	if(TRFS_SB(file->f_path.dentry->d_sb)->bitmap & 0x00000020)
	{	len=strlen(dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE));
                total_len = (sizeof(struct tfile_read)+len+1);
                r = kmalloc(total_len, GFP_KERNEL);
                atomic_inc(&(TRFS_SB(file->f_inode->i_sb)->count));
                r->id =  atomic_read(&(TRFS_SB(file->f_inode->i_sb)->count));
                r->size=total_len -6;
                r->type=6;
                r->open_flags=file->f_flags;
                r->perm=file->f_path.dentry->d_inode->i_mode;
                r->path_len=len+1;
                r->path_len_old=0;
                r->retval=0;
                r->offset=0;
                r->count=0;
                r->bufsize= 0;
                strncpy(r->path,dentry_path_raw(file->f_path.dentry, tempbuf,PAGE_SIZE), r->path_len);
                strcat(r->path,"\0");
                printk("size of struct =%lu\n",sizeof(struct tfile_read));
                printk("total_len=%d\n",total_len);
                printk("sizeof r =%lu\n",sizeof(r));
                printk("strlen=%d\n",len);
                printk("strlen container= %zu \n",strlen(r->path));
                task(file->f_inode->i_sb, r, total_len,NULL,0);


                printk("id =%d,\n size=%hu\n,type=%hu,open_flags=%d\n,perm=%d \n,path_len=%hu \n,retval=%d \n,path=%s\n",
                 (r->id),
                (r->size),
                (r->type),
                (r->open_flags),
                (r->perm),
                (r->path_len),
                (r->retval),
                (r->path));
        if (tempbuf)
          kfree(tempbuf);


	}
	return 0;
}

static int trfs_fsync(struct file *file, loff_t start, loff_t end,
			int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;
	lower_file = trfs_lower_file(file);
	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	trfs_put_lower_path(dentry, &lower_path);
out:
	printk("in file.c fsysnc \n");
	return err;
}

static int trfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = trfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);
	printk("in file.c fasync \n");
	return err;
}

/*
 * Wrapfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t trfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;

	lower_file = trfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	printk("in file.c file_llseek \n");
	return err;
}

/*
 * Wrapfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
trfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode atime as needed */
	if (err >= 0 || err == -EIOCBQUEUED)
		fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
					file_inode(lower_file));
out:
	printk("in file.c read_iter \n ");
	return err;
}

/*
 * Wrapfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
trfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = trfs_lower_file(file);
	if (!lower_file->f_op->write_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode times/sizes as needed */
	if (err >= 0 || err == -EIOCBQUEUED) {
		fsstack_copy_inode_size(d_inode(file->f_path.dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(file->f_path.dentry),
					file_inode(lower_file));
	}
out:	
	printk("in file.c trfs_write_iter \n");
	return err;
}

const struct file_operations trfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= trfs_read,
	.write		= trfs_write,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.mmap		= trfs_mmap,
	.open		= trfs_open,
	.flush		= trfs_flush,
	.release	= trfs_file_release,
	.fsync		= trfs_fsync,
	.fasync		= trfs_fasync,
	.read_iter	= trfs_read_iter,
	.write_iter	= trfs_write_iter,
};

/* trimmed directory options */
const struct file_operations trfs_dir_fops = {
	.llseek		= trfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= trfs_readdir,
	.unlocked_ioctl	= trfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= trfs_compat_ioctl,
#endif
	.open		= trfs_open,
	.release	= trfs_file_release,
	.flush		= trfs_flush,
	.fsync		= trfs_fsync,
	.fasync		= trfs_fasync,
};
