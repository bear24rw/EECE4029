#ifndef _VMM_H_
#define _VMM_H_

/* default pool size is (2^12)*(2^12) bytes */
#define DEFAULT_POOL_SIZE   16777216

#define MAJOR_NUM   100
#define DEVICE_NAME "vmm"

#define IOCTL_ALLOC             _IOWR(MAJOR_NUM, 0, int)
#define IOCTL_FREE              _IOWR(MAJOR_NUM, 2, int)
#define IOCTL_SET_IDX           _IOR(MAJOR_NUM, 3, int)
#define IOCTL_SET_READ_SIZE     _IOR(MAJOR_NUM, 4, int)
#define IOCTL_WRITE             _IOWR(MAJOR_NUM, 5, char *)
#define IOCTL_READ              _IOWR(MAJOR_NUM, 6, char *)

#endif
