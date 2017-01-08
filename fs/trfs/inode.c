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
#include <linux/dcache.h>
#include <linux/limits.h>
#include "treplay.h"
#include "queue.h"
#define TEST_PAGE_SIZE 20



static int trfs_create(struct inode *dir, struct dentry *dentry,
			 umode_t mode, bool want_excl)
{
	int len=0 , total_len=0 , err;	
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	struct tfile_read *r=NULL;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_create(d_inode(lower_parent_dentry), lower_dentry, mode,
			 want_excl);
	if (err)
		goto out;
	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	if(TRFS_SB(dentry->d_sb)->bitmap & 0x00000004)
	{
	len=strlen(dentry_path_raw(dentry, tempbuf,PAGE_SIZE));
        total_len = (sizeof(struct tfile_read)+len+1);
       	r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(dir->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(dir->i_sb)->count));
        r->size=total_len -6;
        r->type=3;
        r->open_flags=0;
        r->perm=mode;
        r->path_len=len+1;
	r->path_len_old=0;
        r->retval=err;
        r->offset=0;
        r->count=0;
        r->bufsize=0;
       	strncpy(r->path,dentry_path_raw(dentry, tempbuf,PAGE_SIZE), r->path_len);
        strcat(r->path,"\0");
        task(dir->i_sb, r, total_len,NULL,0);

	if (tempbuf)
	    kfree(tempbuf);
	}
	return err;
}

static int trfs_link(struct dentry *old_dentry, struct inode *dir,
		       struct dentry *new_dentry)
{	
	int err;
	int len1,len2,total_len;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	struct tfile_read *r=NULL;
	struct dentry *lower_old_dentry;
	struct dentry *lower_new_dentry;
	struct dentry *lower_dir_dentry;
	u64 file_size_save;
	struct path lower_old_path, lower_new_path;
	len1=strlen(dentry_path_raw(new_dentry, tempbuf,PAGE_SIZE));
	len2=strlen(dentry_path_raw(old_dentry, tempbuf,PAGE_SIZE));
	file_size_save = i_size_read(d_inode(old_dentry));
	trfs_get_lower_path(old_dentry, &lower_old_path);
	trfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_dir_dentry = lock_parent(lower_new_dentry);

	err  = vfs_link(lower_old_dentry, d_inode(lower_dir_dentry),
		       lower_new_dentry, NULL);
	if (err || !d_inode(lower_new_dentry))
		goto out;
	err = trfs_interpose(new_dentry, dir->i_sb, &lower_new_path);
	len1=strlen(dentry_path_raw(new_dentry, tempbuf,PAGE_SIZE));
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, d_inode(lower_new_dentry));
	fsstack_copy_inode_size(dir, d_inode(lower_new_dentry));
	set_nlink(d_inode(old_dentry),
		  trfs_lower_inode(d_inode(old_dentry))->i_nlink);
	i_size_write(d_inode(new_dentry), file_size_save);
out:
	unlock_dir(lower_dir_dentry);
	trfs_put_lower_path(old_dentry, &lower_old_path);
	trfs_put_lower_path(new_dentry, &lower_new_path);
	if(TRFS_SB(old_dentry->d_sb)->bitmap & 0x00000080)
	{
        total_len = (sizeof(struct tfile_read)+len1+len2+1);
        r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(dir->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(dir->i_sb)->count));
        r->size=total_len -6;
        r->type=9;
        r->open_flags=0;
        r->perm=0;
        r->path_len=len1;
	r->path_len_old=len2+1;
        r->retval=err;
        r->offset=0;
        r->count=0;
	memset(r->path, '\0', len1+len2+1); 
       	strncpy(r->path,dentry_path_raw(new_dentry, tempbuf,PAGE_SIZE),r->path_len);
       	strcat(r->path,dentry_path_raw(old_dentry, tempbuf,PAGE_SIZE));
       	task(dir->i_sb, r, total_len,NULL,0);

	if (tempbuf)
 	   kfree(tempbuf);
	}
	return err;
}

static int trfs_unlink(struct inode *dir, struct dentry *dentry)
{
	int len=0,total_len=0;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	struct tfile_read *r=NULL;

	int err;
	struct dentry *lower_dentry;
	struct inode *lower_dir_inode = trfs_lower_inode(dir);
	struct dentry *lower_dir_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	dget(lower_dentry);
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_unlink(lower_dir_inode, lower_dentry, NULL);

	/*
	 * Note: unlinking on top of NFS can cause silly-renamed files.
	 * Trying to delete such files results in EBUSY from NFS
	 * below.  Silly-renamed files will get deleted by NFS later on, so
	 * we just need to detect them here and treat such EBUSY errors as
	 * if the upper file was successfully deleted.
	 */
	if (err == -EBUSY && lower_dentry->d_flags & DCACHE_NFSFS_RENAMED)
		err = 0;
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, lower_dir_inode);
	fsstack_copy_inode_size(dir, lower_dir_inode);
	set_nlink(d_inode(dentry),
		  trfs_lower_inode(d_inode(dentry))->i_nlink);
	d_inode(dentry)->i_ctime = dir->i_ctime;
	d_drop(dentry); /* this is needed, else LTP fails (VFS won't do it) */
out:
	unlock_dir(lower_dir_dentry);
	dput(lower_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	if(TRFS_SB(dentry->d_sb)->bitmap & 0x00000100)
	{
	len=strlen(dentry_path_raw(dentry, tempbuf,PAGE_SIZE));
        total_len = (sizeof(struct tfile_read)+len+1);
        r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(dir->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(dir->i_sb)->count));
        r->size=total_len -6;
        r->type=3;
        r->open_flags=0;
        r->perm=0;
        r->path_len=len+1;
	r->path_len_old=0;
        r->retval=err;
        r->offset=0;
        r->count=0;
        r->bufsize=0;
       	strncpy(r->path,dentry_path_raw(dentry, tempbuf,PAGE_SIZE), r->path_len);
        strcat(r->path,"\0");
        task(dir->i_sb, r, total_len,NULL,0);

	if (tempbuf)
	    kfree(tempbuf);
	}	

	return err;
}

static int trfs_symlink(struct inode *dir, struct dentry *dentry,
			  const char *symname)
{
	
	int err;
	int len1,len2,total_len;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	struct tfile_read *r=NULL;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;
	len1=strlen(dentry_path_raw(dentry, tempbuf,PAGE_SIZE));
	len2=strlen(symname);

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_symlink(d_inode(lower_parent_dentry), lower_dentry, symname);
	if (err)
		goto out;
	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	if(TRFS_SB(dentry->d_sb)->bitmap & 0x00000200)
	{
	total_len = (sizeof(struct tfile_read)+len1+len2+1);
        r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(dir->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(dir->i_sb)->count));
        r->size=total_len -6;
        r->type=7;
        r->open_flags=0;
        r->perm=0;
        r->path_len=len1;
	r->path_len_old=len2+1;
        r->retval=err;
        r->offset=0;
        r->count=0;
       memset(r->path, '\0', len1+len2+1); 
       	strncpy(r->path,dentry_path_raw(dentry, tempbuf,PAGE_SIZE),r->path_len);
       	task(dir->i_sb, r, total_len,NULL,0);

	if (tempbuf)
 	   kfree(tempbuf);
	}

	return err;
}

static int trfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int len=0,total_len=0;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	struct tfile_read *r=NULL;
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;
//	struct tfile_read *rec_mkdir;
/*	char *buf;
        buf=(char *) kmalloc(TEST_PAGE_SIZE,GFP_KERNEL);
        memset(buf,'\0', TEST_PAGE_SIZE);
*/

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_mkdir(d_inode(lower_parent_dentry), lower_dentry, mode);
	if (err)
		goto out;

	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;

	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));
	/* update number of links on parent directory */
	set_nlink(dir, trfs_lower_inode(dir)->i_nlink);

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	if(TRFS_SB(dentry->d_sb)->bitmap & 0x00000008)
	{
	len=strlen(dentry_path_raw(dentry, tempbuf,PAGE_SIZE));
        total_len = (sizeof(struct tfile_read)+len+1);
        r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(dir->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(dir->i_sb)->count));
        r->size=total_len -6;
        r->type=4;
        r->open_flags=0;
        r->perm=mode;
        r->path_len=len+1;
	r->path_len_old=0;
        r->retval=err;
        r->offset=0;
        r->count=0;
       	r->bufsize=0; 
       	strncpy(r->path,dentry_path_raw(dentry, tempbuf,PAGE_SIZE), r->path_len);
        strcat(r->path,"\0");
	task(dir->i_sb, r, total_len ,NULL ,0);
	if (tempbuf)
 	   kfree(tempbuf);
	}
	return err;
}

static int trfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	int len=0,total_len=0;
	char *tempbuf=(char *)kmalloc(PAGE_SIZE, GFP_KERNEL);
	struct dentry *lower_dentry;
	struct tfile_read *r=NULL;
	struct dentry *lower_dir_dentry;
	int err;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_dir_dentry = lock_parent(lower_dentry);

	err = vfs_rmdir(d_inode(lower_dir_dentry), lower_dentry);
	if (err)
		goto out;

	d_drop(dentry);	/* drop our dentry on success (why not VFS's job?) */
	if (d_inode(dentry))
		clear_nlink(d_inode(dentry));
	fsstack_copy_attr_times(dir, d_inode(lower_dir_dentry));
	fsstack_copy_inode_size(dir, d_inode(lower_dir_dentry));
	set_nlink(dir, d_inode(lower_dir_dentry)->i_nlink);

out:
	unlock_dir(lower_dir_dentry);
	trfs_put_lower_path(dentry, &lower_path);

	if(TRFS_SB(dentry->d_sb)->bitmap & 0x00000040)
	{
	len=strlen(dentry_path_raw(dentry, tempbuf,PAGE_SIZE));
        total_len = (sizeof(struct tfile_read)+len+1);
        r = kmalloc(total_len, GFP_KERNEL);
        atomic_inc(&(TRFS_SB(dir->i_sb)->count));
        r->id =  atomic_read(&(TRFS_SB(dir->i_sb)->count));
        r->size=total_len -6;
        r->type=8;
        r->open_flags=0;
        r->perm=0;
        r->path_len=len+1;
	r->path_len_old=0;
        r->retval=err;
        r->offset=0;
        r->count=0;
        r->bufsize=0;
       	strncpy(r->path,dentry_path_raw(dentry, tempbuf,PAGE_SIZE), r->path_len);
        strcat(r->path,"\0");
        task(dir->i_sb, r, total_len,NULL,0);
	if (tempbuf)
   	 kfree(tempbuf);

	}
	return err;
}

static int trfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
			dev_t dev)
{
	int err;
	struct dentry *lower_dentry;
	struct dentry *lower_parent_dentry = NULL;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_parent_dentry = lock_parent(lower_dentry);

	err = vfs_mknod(d_inode(lower_parent_dentry), lower_dentry, mode, dev);
	if (err)
		goto out;

	err = trfs_interpose(dentry, dir->i_sb, &lower_path);
	if (err)
		goto out;
	fsstack_copy_attr_times(dir, trfs_lower_inode(dir));
	fsstack_copy_inode_size(dir, d_inode(lower_parent_dentry));

out:
	unlock_dir(lower_parent_dentry);
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

/*
 * The locking rules in trfs_rename are complex.  We could use a simpler
 * superblock-level name-space lock for renames and copy-ups.
 */
static int trfs_rename(struct inode *old_dir, struct dentry *old_dentry,
			 struct inode *new_dir, struct dentry *new_dentry)
{
	int err = 0;
	struct dentry *lower_old_dentry = NULL;
	struct dentry *lower_new_dentry = NULL;
	struct dentry *lower_old_dir_dentry = NULL;
	struct dentry *lower_new_dir_dentry = NULL;
	struct dentry *trap = NULL;
	struct path lower_old_path, lower_new_path;

	trfs_get_lower_path(old_dentry, &lower_old_path);
	trfs_get_lower_path(new_dentry, &lower_new_path);
	lower_old_dentry = lower_old_path.dentry;
	lower_new_dentry = lower_new_path.dentry;
	lower_old_dir_dentry = dget_parent(lower_old_dentry);
	lower_new_dir_dentry = dget_parent(lower_new_dentry);

	trap = lock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	/* source should not be ancestor of target */
	if (trap == lower_old_dentry) {
		err = -EINVAL;
		goto out;
	}
	/* target should not be ancestor of source */
	if (trap == lower_new_dentry) {
		err = -ENOTEMPTY;
		goto out;
	}

	err = vfs_rename(d_inode(lower_old_dir_dentry), lower_old_dentry,
			 d_inode(lower_new_dir_dentry), lower_new_dentry,
			 NULL, 0);
	if (err)
		goto out;

	fsstack_copy_attr_all(new_dir, d_inode(lower_new_dir_dentry));
	fsstack_copy_inode_size(new_dir, d_inode(lower_new_dir_dentry));
	if (new_dir != old_dir) {
		fsstack_copy_attr_all(old_dir,
				      d_inode(lower_old_dir_dentry));
		fsstack_copy_inode_size(old_dir,
					d_inode(lower_old_dir_dentry));
	}

out:
	unlock_rename(lower_old_dir_dentry, lower_new_dir_dentry);
	dput(lower_old_dir_dentry);
	dput(lower_new_dir_dentry);
	trfs_put_lower_path(old_dentry, &lower_old_path);
	trfs_put_lower_path(new_dentry, &lower_new_path);
	return err;
}

static int trfs_readlink(struct dentry *dentry, char __user *buf, int bufsiz)
{
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op ||
	    !d_inode(lower_dentry)->i_op->readlink) {
		err = -EINVAL;
		goto out;
	}

	err = d_inode(lower_dentry)->i_op->readlink(lower_dentry,
						    buf, bufsiz);
	if (err < 0)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry), d_inode(lower_dentry));

out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static const char *trfs_get_link(struct dentry *dentry, struct inode *inode,
				   struct delayed_call *done)
{
	char *buf;
	int len = PAGE_SIZE, err;
	mm_segment_t old_fs;

	if (!dentry)
		return ERR_PTR(-ECHILD);

	/* This is freed by the put_link method assuming a successful call. */
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		buf = ERR_PTR(-ENOMEM);
		return buf;
	}

	/* read the symlink, and then we will follow it */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	err = trfs_readlink(dentry, buf, len);
	set_fs(old_fs);
	if (err < 0) {
		kfree(buf);
		buf = ERR_PTR(err);
	} else {
		buf[err] = '\0';
	}
	set_delayed_call(done, kfree_link, buf);
	return buf;
}

static int trfs_permission(struct inode *inode, int mask)
{
	struct inode *lower_inode;
	int err;

	lower_inode = trfs_lower_inode(inode);
	err = inode_permission(lower_inode, mask);
//	printk("inside inode perm \n");
	return err;
}

static int trfs_setattr(struct dentry *dentry, struct iattr *ia)
{
	int err;
	struct dentry *lower_dentry;
	struct inode *inode;
	struct inode *lower_inode;
	struct path lower_path;
	struct iattr lower_ia;

	inode = d_inode(dentry);

	/*
	 * Check if user has permission to change inode.  We don't check if
	 * this user can change the lower inode: that should happen when
	 * calling notify_change on the lower inode.
	 */
	err = inode_change_ok(inode, ia);
	if (err)
		goto out_err;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	lower_inode = trfs_lower_inode(inode);

	/* prepare our own lower struct iattr (with the lower file) */
	memcpy(&lower_ia, ia, sizeof(lower_ia));
	if (ia->ia_valid & ATTR_FILE)
		lower_ia.ia_file = trfs_lower_file(ia->ia_file);

	/*
	 * If shrinking, first truncate upper level to cancel writing dirty
	 * pages beyond the new eof; and also if its' maxbytes is more
	 * limiting (fail with -EFBIG before making any change to the lower
	 * level).  There is no need to vmtruncate the upper level
	 * afterwards in the other cases: we fsstack_copy_inode_size from
	 * the lower level.
	 */
	if (ia->ia_valid & ATTR_SIZE) {
		err = inode_newsize_ok(inode, ia->ia_size);
		if (err)
			goto out;
		truncate_setsize(inode, ia->ia_size);
	}

	/*
	 * mode change is for clearing setuid/setgid bits. Allow lower fs
	 * to interpret this in its own way.
	 */
	if (lower_ia.ia_valid & (ATTR_KILL_SUID | ATTR_KILL_SGID))
		lower_ia.ia_valid &= ~ATTR_MODE;

	/* notify the (possibly copied-up) lower inode */
	/*
	 * Note: we use d_inode(lower_dentry), because lower_inode may be
	 * unlinked (no inode->i_sb and i_ino==0.  This happens if someone
	 * tries to open(), unlink(), then ftruncate() a file.
	 */
	inode_lock(d_inode(lower_dentry));
	err = notify_change(lower_dentry, &lower_ia, /* note: lower_ia */
			    NULL);
	inode_unlock(d_inode(lower_dentry));
	if (err)
		goto out;

	/* get attributes from the lower inode */
	fsstack_copy_attr_all(inode, lower_inode);
	/*
	 * Not running fsstack_copy_inode_size(inode, lower_inode), because
	 * VFS should update our inode size, and notify_change on
	 * lower_inode should update its size.
	 */

out:
	trfs_put_lower_path(dentry, &lower_path);
out_err:
	return err;
}

static int trfs_getattr(struct vfsmount *mnt, struct dentry *dentry,
			  struct kstat *stat)
{
	int err;
	struct kstat lower_stat;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_getattr(&lower_path, &lower_stat);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
	generic_fillattr(d_inode(dentry), stat);
	stat->blocks = lower_stat.blocks;
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int
trfs_setxattr(struct dentry *dentry, const char *name, const void *value,
		size_t size, int flags)
{
	int err; struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->setxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_setxattr(lower_dentry, name, value, size, flags);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static ssize_t
trfs_getxattr(struct dentry *dentry, const char *name, void *buffer,
		size_t size)
{
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->getxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_getxattr(lower_dentry, name, buffer, size);
	if (err)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry),
				d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static ssize_t
trfs_listxattr(struct dentry *dentry, char *buffer, size_t buffer_size)
{
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op->listxattr) {
		err = -EOPNOTSUPP;
		goto out;
	}
	err = vfs_listxattr(lower_dentry, buffer, buffer_size);
	if (err)
		goto out;
	fsstack_copy_attr_atime(d_inode(dentry),
				d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}

static int
trfs_removexattr(struct dentry *dentry, const char *name)
{
	int err;
	struct dentry *lower_dentry;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!d_inode(lower_dentry)->i_op ||
	    !d_inode(lower_dentry)->i_op->removexattr) {
		err = -EINVAL;
		goto out;
	}
	err = vfs_removexattr(lower_dentry, name);
	if (err)
		goto out;
	fsstack_copy_attr_all(d_inode(dentry),
			      d_inode(lower_path.dentry));
out:
	trfs_put_lower_path(dentry, &lower_path);
	return err;
}
const struct inode_operations trfs_symlink_iops = {
	.readlink	= trfs_readlink,
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.get_link	= trfs_get_link,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};

const struct inode_operations trfs_dir_iops = {
	.create		= trfs_create,
	.lookup		= trfs_lookup,
	.link		= trfs_link,
	.unlink		= trfs_unlink,
	.symlink	= trfs_symlink,
	.mkdir		= trfs_mkdir,
	.rmdir		= trfs_rmdir,
	.mknod		= trfs_mknod,
	.rename		= trfs_rename,
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};

const struct inode_operations trfs_main_iops = {
	.permission	= trfs_permission,
	.setattr	= trfs_setattr,
	.getattr	= trfs_getattr,
	.setxattr	= trfs_setxattr,
	.getxattr	= trfs_getxattr,
	.listxattr	= trfs_listxattr,
	.removexattr	= trfs_removexattr,
};
