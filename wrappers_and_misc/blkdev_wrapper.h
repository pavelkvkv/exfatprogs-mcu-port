/***
 * @file blkdev_wrapper.h
 * @author Pavel kv
 * @date 2024-06-00
 */

#ifndef BLKDEV_WRAPPER_H
#define BLKDEV_WRAPPER_H

#include "my_types.h" 
/*
    Hardcode offsets for sys and data partitions.
    Now sdcard has 2 partitions: sys (256 MiB) and data, all with 512 KiB clusters.
*/
#define FS_OFFSET_SYS  (65536ULL * 512)
#define FS_OFFSET_DATA (589824ULL * 512)

#define FS_END_SYS	   (FS_OFFSET_DATA - 1)     // last byte of sys partition
#define FS_SIZE_SYS	   (524288ULL * 512)		// size of sys partition (as linux says)

#define FS_END_DATA	   (get_sdcard_size() - 1)			  // last byte of data partition
#define FS_SIZE_DATA   (FS_END_DATA + 1 - FS_OFFSET_DATA) // size of data partition

// Opens a file (wrapper for open)
int w_open(const char *pathname, int flags);

// Closes a file (wrapper for close)
ssize64_t w_write(int fd, const void *buf, size64_t count);

// Reads from a file (wrapper for read)
ssize64_t w_read(int fd, void *buf, size64_t count);

// Seeks in a file (wrapper for lseek)
off64_t w_lseek(int fd, off64_t offset, int whence);

// Reads from a file (wrapper for pread)
ssize64_t w_pread(int fd, void *buf, size64_t count, off64_t offset);

// Writes to a file (wrapper for pwrite)
ssize64_t w_pwrite(int fd, const void *buf, size64_t count, off64_t offset);

// Closes a file (wrapper for close)
int w_close(int fd);

// Syncs a file (wrapper for fsync)
int w_fsync(int fd);

#endif // BLKDEV_WRAPPER_H
