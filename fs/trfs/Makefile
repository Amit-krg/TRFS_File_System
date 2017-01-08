TRFS_VERSION="0.1"

EXTRA_CFLAGS += -DTRFS_VERSION=\"$(TRFS_VERSION)\"

obj-$(CONFIG_TRFS_FS) += trfs.o

trfs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
