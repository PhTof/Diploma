#include <sys/ioctl.h>	// ioctl()
#include <linux/fs.h>	// struct fsxattr && FS_XFLAG_*

#include "../headers/dax.h"

void dax_flag(int fd, int value) {
        struct fsxattr attr;

	(void) ioctl(fd, FS_IOC_FSGETXATTR, &attr);
	

	switch (value) {
		case 0: attr.fsx_xflags &= ~FS_XFLAG_DAX; break;
		case 1: attr.fsx_xflags |=  FS_XFLAG_DAX; break;
		default: return;
	}
        
	(void) ioctl(fd, FS_IOC_FSSETXATTR, &attr);
}

