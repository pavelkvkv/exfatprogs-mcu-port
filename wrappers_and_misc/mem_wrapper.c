#include <stdlib.h>
#include "FreeRTOS.h"
#include "mem_wrapper.h"
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include "log.h"

// /**
//  * Обертка для xPortMalloc из FreeRTOS.
//  *
//  * @param size Размер памяти, которую необходимо выделить.
//  * @return Указатель на выделенную память или NULL, если память не могла быть выделена.
//  */
// void *w_malloc(size_t size)
// {
// 	void *ptr = pvPortMalloc(size);
//     logI("%d bytes, ptr = %p\n", size, ptr);
// }

// /**
//  * Обертка для xPortCalloc из FreeRTOS.
//  *
//  * @param num Количество элементов.
//  * @param size Размер одного элемента.
//  * @return Указатель на выделенную и обнулённую память или NULL, если память не могла быть выделена.
//  */
// void *w_calloc(size_t num, size_t size)
// {
// 	void *ptr = pvPortCalloc(num , size);
//     logI("%d bytes, ptr = %p\n", num * size, ptr);
// 	return ptr;
// }

// /**
//  * Обертка для vPortFree из FreeRTOS.
//  *
//  * @param ptr Указатель на память, которую необходимо освободить.
//  */
// void w_free(void *ptr)
// {
//     logI("%p\n", ptr);
// 	vPortFree(ptr);
// }

/**
 * Обертка для xPortRealloc из FreeRTOS.
 * Данная функция обычно отсутствует в FreeRTOS, поэтому ее необходимо реализовать вручную.
 *
 * @param ptr Указатель на ранее выделенную память или NULL.
 * @param size Новый размер памяти.
 * @return Указатель на выделенную память или NULL, если память не могла быть выделена.
 */
// void* w_realloc(void* ptr, size_t size) {
//     if (ptr == NULL) {
//         return pvPortMalloc(size);
//     }

//     if (size == 0) {
//         vPortFree(ptr);
//         return NULL;
//     }

//     // Выделяем новую память
//     void* new_ptr = pvPortMalloc(size);
//     if (new_ptr == NULL) {
//         return NULL;
//     }

//     // Переносим данные в новую область памяти
//     // (Учтите, что на практике это может потребовать уведомления о размере старого блока)
//     size_t old_size = malloc_usable_size(ptr); // malloc_usable_size не является частью стандарта C
//     if (old_size > size) {
//         old_size = size;
//     }
//     memcpy(new_ptr, ptr, old_size);

//     // Освобождаем старую память
//     vPortFree(ptr);

//     return new_ptr;
// }

size_t w_mbstowcs(wchar_t *dest, const char *src, size_t n)
{
    size_t i = 0;
    size_t len = 0;

    while (*src)
    {
        uint32_t codepoint = 0;
        size_t seq_len = 0;

        // Определение длины последовательности UTF-8 и извлечение кода символа
        if ((*src & 0x80) == 0)
        {
            // 1-байтная последовательность (ASCII)
            codepoint = *src++;
            seq_len = 1;
        }
        else if ((*src & 0xE0) == 0xC0)
        {
            // 2-байтная последовательность
            codepoint = (*src++ & 0x1F) << 6;
            if ((*src & 0xC0) != 0x80)
                return (size_t)-1;
            codepoint |= (*src++ & 0x3F);
            seq_len = 2;
        }
        else if ((*src & 0xF0) == 0xE0)
        {
            // 3-байтная последовательность
            codepoint = (*src++ & 0x0F) << 12;
            if ((*src & 0xC0) != 0x80)
                return (size_t)-1;
            codepoint |= (*src++ & 0x3F) << 6;
            if ((*src & 0xC0) != 0x80)
                return (size_t)-1;
            codepoint |= (*src++ & 0x3F);
            seq_len = 3;
        }
        else if ((*src & 0xF8) == 0xF0)
        {
            // 4-байтная последовательность
            codepoint = (*src++ & 0x07) << 18;
            if ((*src & 0xC0) != 0x80)
                return (size_t)-1;
            codepoint |= (*src++ & 0x3F) << 12;
            if ((*src & 0xC0) != 0x80)
                return (size_t)-1;
            codepoint |= (*src++ & 0x3F) << 6;
            if ((*src & 0xC0) != 0x80)
                return (size_t)-1;
            codepoint |= (*src++ & 0x3F);
            seq_len = 4;
        }
        else
        {
            return (size_t)-1;
        }

        // Если код символа больше 0xFFFF, он требует использования пары суррогатов UTF-16
        if (codepoint >= 0x10000)
        {
            if (dest && len + 2 >= n)
                return (size_t)-1;

            codepoint -= 0x10000;
            if (dest)
            {
                dest[len++] = (wchar_t)(0xD800 | (codepoint >> 10));
                dest[len++] = (wchar_t)(0xDC00 | (codepoint & 0x3FF));
            }
            else
            {
                len += 2;
            }
        }
        else
        {
            if (dest && len + 1 >= n)
                return (size_t)-1;

            if (dest)
            {
                dest[len++] = (wchar_t)codepoint;
            }
            else
            {
                len++;
            }
        }
    }

    // Добавление нулевого символа в конец строки, если есть место
    if (dest && len < n)
    {
        dest[len] = L'\0';
    }

    return len;
}

size_t w_wcrtomb(char *dest, wchar_t wc, mbstate_t *ps)
{
    if (dest == NULL)
    {
        return 1; // Возвращает 1, так как функция не записывает ничего
    }

    if (wc < 0x80)
    {
        // 1-байтная последовательность (ASCII)
        dest[0] = (char)wc;
        return 1;
    }
    else if (wc < 0x800)
    {
        // 2-байтная последовательность
        dest[0] = (char)(0xC0 | (wc >> 6));
        dest[1] = (char)(0x80 | (wc & 0x3F));
        return 2;
    }
    else if (wc >= 0xD800 && wc <= 0xDFFF)
    {
        // Неверный символ: суррогатная пара не может быть закодирована в UTF-8
        errno = EILSEQ;
        return (size_t)-1;
    }
    else if (wc < 0x10000)
    {
        // 3-байтная последовательность
        dest[0] = (char)(0xE0 | (wc >> 12));
        dest[1] = (char)(0x80 | ((wc >> 6) & 0x3F));
        dest[2] = (char)(0x80 | (wc & 0x3F));
        return 3;
    }
    else if (wc < 0x110000)
    {
        // 4-байтная последовательность
        dest[0] = (char)(0xF0 | (wc >> 18));
        dest[1] = (char)(0x80 | ((wc >> 12) & 0x3F));
        dest[2] = (char)(0x80 | ((wc >> 6) & 0x3F));
        dest[3] = (char)(0x80 | (wc & 0x3F));
        return 4;
    }
    else
    {
        // Неверный символ: за пределами допустимого диапазона UTF-8
        errno = EILSEQ;
        return (size_t)-1;
    }
}

int print_utf16le(const char *utf16_string, size_t length)
{
    // Пробегаем по массиву символов и выводим их побайтово
    for (size_t i = 0; i < length * 2; i += 2)
    {
        // Проверка на нулевой символ, как конец строки
        if (utf16_string[i] == 0 && utf16_string[i + 1] == 0)
        {
            break;
        }
        fwrite(&utf16_string[i], sizeof(char), 2, stdout);
    }

    return 0;
}

/**
 * @brief Копирование памяти.
 *
 * Функция копирует n байт из области памяти src в область памяти dest.
 *
 * @param[out] dest Указатель на место, куда будут скопированы данные.
 * @param[in] src Указатель на источник данных.
 * @param[in] n Количество байт для копирования.
 * @return Указатель на dest.
 */
void *my_memcpy(void *dest, const void *src, size_t n)
{
    // Преобразуем указатели в байтовые указатели для побайтового копирования
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    // Копируем n байт из src в dest
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }

    return dest;
}