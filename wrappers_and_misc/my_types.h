#ifndef MYTYPES
#define MYTYPES

#include <stdint.h>

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE
#define __LARGE64_FILES 

//typedef long long loff_t;


// ошибка сборки, если не little-endian
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#error "Only little-endian architectures are supported"
#endif

typedef uint64_t __le64;
typedef uint32_t __le32; // 32-битный little-endian
typedef uint16_t __le16; // 16-битный little-endian
typedef uint8_t __u8;  // 8-битный little-endian
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t u64;
typedef uint64_t size64_t;
typedef int64_t ssize64_t;
typedef int64_t off64_t;

#endif // !MYTYPES