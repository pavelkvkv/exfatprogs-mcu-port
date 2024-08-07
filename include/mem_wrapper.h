

#include <stdlib.h>
#include "FreeRTOS.h"
#include <string.h>
#include <wchar.h>
#include "log.h"

// void *w_malloc(size_t size);
// void *w_calloc(size_t num, size_t size);
// void w_free(void *ptr);


// Макрос для замены w_malloc
#define w_malloc(size) \
    ({ \
        void *ptr = pvPortMalloc(size); \
        logD("line: %d, malloc %d bytes, ptr = %p",  __LINE__, (int)size, ptr); \
        ptr; \
    })

// Макрос для замены w_calloc
#define w_calloc(num, size) \
    ({ \
        void *ptr = pvPortCalloc(num, size); \
        logD("line: %d, calloc %d bytes, ptr = %p",  __LINE__, (int)(num * size), ptr); \
        ptr; \
    })

// Макрос для замены w_free
#define w_free(ptr) \
    do { \
        logD("line: %d, free ptr = %p",  __LINE__, ptr); \
        vPortFree(ptr); \
    } while (0)



size_t w_mbstowcs(wchar_t *dest, const char *src, size_t n);
size_t w_wcrtomb(char *dest, wchar_t wc, mbstate_t *ps);
int print_utf16le(const char *utf16_string, size_t length);
void *my_memcpy(void *dest, const void *src, size_t n);

#pragma once
