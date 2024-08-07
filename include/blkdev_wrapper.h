
#include "my_types.h"

#define FS_OFFSET_SYS  (65536ULL * 512)
#define FS_OFFSET_DATA (589824ULL * 512)

#define FS_END_SYS	   (FS_OFFSET_DATA - 1) // последний байт раздела sys
#define FS_SIZE_SYS	   (524288ULL * 512)		// размер раздела sys (как даёт линукс)

#define FS_END_DATA	   (get_sdcard_size() - 1)			  // последний байт раздела data
#define FS_SIZE_DATA   (FS_END_DATA + 1 - FS_OFFSET_DATA) // размер раздела data (как даёт линукс)

int w_open(const char *pathname, int flags);

ssize64_t w_write(int fd, const void *buf, size64_t count);

ssize64_t w_read(int fd, void *buf, size64_t count);

off64_t w_lseek(int fd, off64_t offset, int whence);

ssize64_t w_pread(int fd, void *buf, size64_t count, off64_t offset);

ssize64_t w_pwrite(int fd, const void *buf, size64_t count, off64_t offset);

int w_close(int fd);

int w_fsync(int fd);

#pragma once
