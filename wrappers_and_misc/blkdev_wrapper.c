
#include "my_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "at32f435_437.h"
#include "sdcard_lowlevel_ops.h"
#include "ff.h"
#include "sdcard_main.h"

#include "log.h"
#include "blkdev_wrapper.h"
#define TAG "Fsck-wrapper"

// Статическая переменная для хранения файлового дескриптора
static int blkdev_fd		 = -1;
static off64_t position		 = 0; // Текущая позиция в файле (разделе)
static s64 filesystem_offset = 0; // Смещение раздела
static s64 blkdev_size		 = 0; // Размер раздела

static int mmc_read(u8 *buff, u64 addr, u32 count);
static int mmc_write(const u8 *buff, u64 addr, u32 count);

// Функция для открытия файла (аналог open)
int w_open(const char *pathname, int flags)
{
	logI("");
	if (blkdev_fd != -1)
	{
		// Если файл уже открыт
		errno = EBUSY;
		logW("File already open");
		return -1;
	}

	if (strcmp(pathname, "/sys") == 0)
	{
		logI("Processing /sys");
		blkdev_fd		  = 1;
		filesystem_offset = FS_OFFSET_SYS;
		blkdev_size		  = FS_SIZE_SYS;
	}
	else if (strcmp(pathname, "/dat") == 0)
	{
		logI("Processing /dat");
		blkdev_fd		  = 2;
		filesystem_offset = FS_OFFSET_DATA;
		blkdev_size		  = FS_SIZE_DATA;
	}
	else
	{
		logE("Unknown path: %s\n", pathname);
		errno = ENOENT;
		return -1;
	}

	if (blkdev_fd == -1)
	{
		// Ошибка при открытии файла
		return -1;
	}
	return blkdev_fd;
}

// Функция для записи в файл (аналог write)
ssize64_t w_write(int fd, const void *buf, size64_t count)
{
	logD("(%d, %p, %d)", fd, buf, count);
	ssize64_t result = w_pwrite(fd, buf, count, position);
	position += count;
	return result;
}

// Функция для чтения из файла (аналог read)
ssize64_t w_read(int fd, void *buf, size64_t count)
{
	logD("(%d, %p, %d)", fd, buf, count);
	ssize64_t result = w_pread(fd, buf, count, position);
	position += count;
	return result;
}

// Функция для перемещения указателя в файле (аналог lseek)
off64_t w_lseek(int fd, off64_t offset, int whence)
{
	_Static_assert(sizeof(off64_t) == 8, "sizeof(off64_t) != 8");

	UNUSED(fd);
	if (blkdev_fd == -1)
	{
		// Файл не открыт
		errno = EBADF;
		logW("File not opened");
		return -1;
	}
	// lseek(blkdev_fd, offset, whence);
	switch (whence)
	{
		case SEEK_SET:
			position = offset;
			break;
		case SEEK_CUR:
			position += offset;
			break;
		case SEEK_END:
			if (offset)
			{
				logE("Offset on SEEK_END is not supported");
				return 1;
			}
			position = blkdev_size;
			break;
		default:
			errno = EINVAL;
			logE("Invalid whence");
			return -1;
	}
	logI("result = %lld", (s64)position);
	return position;
}

// Функция для чтения из файла по смещению (аналог pread)
ssize64_t w_pread(int fd, void *buf, size64_t count, off64_t offset)
{
	logD("(%d, %p, %d, %lld)", fd, buf, count, (s64)offset);
	UNUSED(fd);
	if (blkdev_fd == -1)
	{
		// Файл не открыт
		errno = EBADF;
		logW("File not opened");
		return -1;
	}
	ssize64_t result = mmc_read((u8 *)buf, offset + filesystem_offset, count);
	if (result == -1 || result != count)
	{
		// Ошибка при чтении
		logE("Error reading, result = %d", result);
		return result;
	}
	return result;
}

// Функция для записи в файл по смещению (аналог pwrite)
ssize64_t w_pwrite(int fd, const void *buf, size64_t count, off64_t offset)
{
	logI("(%d, %p, %d, %lld)", fd, buf, count, (s64)offset);
	UNUSED(fd);
	if (blkdev_fd == -1)
	{
		// Файл не открыт
		errno = EBADF;
		logW("File not opened");
		return -1;
	}
	// pwrite(blkdev_fd, buf, count, offset);
	ssize64_t result = mmc_write((u8 *)buf, offset + filesystem_offset, count);
	if (result == -1 || result != count)
	{
		// Ошибка при записи
		logE("Error writing, result = %d", result);
		return result;
	}
	return result;
}

// Функция для закрытия файла (аналог close)
int w_close(int fd)
{
	UNUSED(fd);
	logI("");
	if (blkdev_fd == -1)
	{
		// Файл не открыт
		errno = EBADF;
		logW("File not opened");
		return -1;
	}

	blkdev_fd = -1; // Сброс дескриптора
	position  = 0;
	return 0;
}

int w_fsync(int fd)
{
	// UNUSED(fd);
	logD("");
	if (blkdev_fd == -1)
	{
		// Файл не открыт
		errno = EBADF;
		logW("File not opened");
		return -1;
	}
	return 0;
}

static int mmc_read(
	u8 *buff, /* Data buffer to store read data */
	u64 addr, /* Start address of byte to read */
	u32 count /* Number of bytes to read */)
{
	// Calculate initial sector and the offset within that sector
	int start_sector = addr / SD_MSC_BLOCK_SIZE; // block size 512
	int end_sector	 = (addr + count - 1) / SD_MSC_BLOCK_SIZE;
	int sector_count = end_sector - start_sector + 1;
	int result		 = 0;
	int count_ret = count;

	// Buffers for unaligned start and end sectors
	u8 *aligned_buf = (u8 *)pvPortMalloc(SD_MSC_BLOCK_SIZE);
	if (aligned_buf == NULL)
	{
		return (int)ENOMEM;
	}

	// Read unaligned start sector if needed
	if (addr % SD_MSC_BLOCK_SIZE != 0)
	{
		result = sdcard_msc_bread(aligned_buf, start_sector, 1);
		if (result != 0)
		{
			logW("Failed to read start sector");
			vPortFree(aligned_buf);
			return -1;
		}
		int offset		  = addr % SD_MSC_BLOCK_SIZE;
		int bytes_to_copy = SD_MSC_BLOCK_SIZE - offset;
		if (bytes_to_copy > count) bytes_to_copy = count;
		memcpy(buff, aligned_buf + offset, bytes_to_copy);
		buff += bytes_to_copy;
		addr += bytes_to_copy;
		count -= bytes_to_copy;
		start_sector++;
		sector_count--;
	}

	// Read aligned sectors in the middle
	if (sector_count > 0 && count >= SD_MSC_BLOCK_SIZE)
	{
		int aligned_sector_count = count / SD_MSC_BLOCK_SIZE;
		if ((u32)buff & 3)
		{ // Check alignment
			while (aligned_sector_count--)
			{
				result = sdcard_msc_bread(aligned_buf, start_sector++, 1);
				if (result != 0)
				{
					logW("Failed to read aligned sectors");
					vPortFree(aligned_buf);
					return -1;
				}
				memcpy(buff, aligned_buf, SD_MSC_BLOCK_SIZE);
				buff += SD_MSC_BLOCK_SIZE;
				count -= SD_MSC_BLOCK_SIZE;
			}
		}
		else
		{
			result = sdcard_msc_bread(buff, start_sector, aligned_sector_count);
			if (result != 0)
			{
				logW("Failed to read aligned sectors");
				vPortFree(aligned_buf);
				return -1;
			}
			int bytes_read = aligned_sector_count * SD_MSC_BLOCK_SIZE;
			buff += bytes_read;
			addr += bytes_read;
			count -= bytes_read;
			start_sector += aligned_sector_count;
			sector_count -= aligned_sector_count;
		}
	}

	// Read unaligned end sector if needed
	if (count > 0)
	{
		result = sdcard_msc_bread(aligned_buf, start_sector, 1);
		if (result != 0)
		{
			logW("Failed to read end sector");
			vPortFree(aligned_buf);
			return -1;
		}
		memcpy(buff, aligned_buf, count);
	}

	vPortFree(aligned_buf);
	return count_ret;
}

static int mmc_write(
	const u8 *buff, /* Data buffer to write data from */
	u64 addr,		/* Start address of byte to write */
	u32 count /* Number of bytes to write */)
{
	// Calculate initial sector and the offset within that sector
	int start_sector = addr / SD_MSC_BLOCK_SIZE; // block size 512
	int end_sector = (addr + count - 1) / SD_MSC_BLOCK_SIZE;
	int sector_count = end_sector - start_sector + 1;
	int result = 0;
	int count_ret = count;

	// Buffers for unaligned start and end sectors
	u8 *aligned_buf = (u8 *)pvPortMalloc(SD_MSC_BLOCK_SIZE);
	if (aligned_buf == NULL) {
		return (int)ENOMEM;
	}

	// Write unaligned start sector if needed
	if (addr % SD_MSC_BLOCK_SIZE != 0) {
		result = sdcard_msc_bread(aligned_buf, start_sector, 1);
		if (result != 0) {
			logW("Failed to read start sector for write");
			vPortFree(aligned_buf);
			return -1;
		}
		int offset = addr % SD_MSC_BLOCK_SIZE;
		int bytes_to_copy = SD_MSC_BLOCK_SIZE - offset;
		if (bytes_to_copy > count) bytes_to_copy = count;
		memcpy(aligned_buf + offset, buff, bytes_to_copy);
		result = sdcard_msc_bwrite(aligned_buf, start_sector, 1);
		if (result != 0) {
			logW("Failed to write start sector");
			vPortFree(aligned_buf);
			return -1;
		}
		buff += bytes_to_copy;
		addr += bytes_to_copy;
		count -= bytes_to_copy;
		start_sector++;
		sector_count--;
	}

	// Write aligned sectors in the middle
	if (sector_count > 0 && count >= SD_MSC_BLOCK_SIZE) {
		int aligned_sector_count = count / SD_MSC_BLOCK_SIZE;
		if ((u32)buff & 3) { // Check alignment
			while (aligned_sector_count--) {
				memcpy(aligned_buf, buff, SD_MSC_BLOCK_SIZE);
				result = sdcard_msc_bwrite(aligned_buf, start_sector++, 1);
				if (result != 0) {
					logW("Failed to write aligned sectors");
					vPortFree(aligned_buf);
					return -1;
				}
				buff += SD_MSC_BLOCK_SIZE;
				count -= SD_MSC_BLOCK_SIZE;
			}
		} else {
			result = sdcard_msc_bwrite(buff, start_sector, aligned_sector_count);
			if (result != 0) {
				logW("Failed to write aligned sectors");
				vPortFree(aligned_buf);
				return -1;
			}
			int bytes_written = aligned_sector_count * SD_MSC_BLOCK_SIZE;
			buff += bytes_written;
			addr += bytes_written;
			count -= bytes_written;
			start_sector += aligned_sector_count;
			sector_count -= aligned_sector_count;
		}
	}

	// Write unaligned end sector if needed
	if (count > 0) {
		result = sdcard_msc_bread(aligned_buf, start_sector, 1);
		if (result != 0) {
			logW("Failed to read end sector for write");
			vPortFree(aligned_buf);
			return -1;
		}
		memcpy(aligned_buf, buff, count);
		result = sdcard_msc_bwrite(aligned_buf, start_sector, 1);
		if (result != 0) {
			logW("Failed to write end sector");
			vPortFree(aligned_buf);
			return -1;
		}
	}

	vPortFree(aligned_buf);
	return count_ret;
}

