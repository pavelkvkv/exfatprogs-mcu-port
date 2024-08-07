/***
 * @file mem_wrapper.c
 * @author Pavel kv
 * @date 2024-06-00
 */

#include <stdlib.h>
#include "FreeRTOS.h"
#include "mem_wrapper.h"
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include "log.h"

/***
 * @brief Convert a multibyte string to a wide character string
 *
 * @param dest Pointer to the destination buffer
 * @param src Pointer to the source string
 * @param n Maximum number of wide characters to convert
 */
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

/***
 * @brief Convert a wide character string to a multibyte string
 *
 * @param dest Pointer to the destination buffer
 * @param src Pointer to the source string
 * @param n Maximum number of multibyte characters to convert
 */
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

