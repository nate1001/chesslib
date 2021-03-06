#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cstring.h"
#include "calloc.h"

static const char* empty_string = "";

void chess_string_init(ChessString* string)
{
    string->size = 0;
}

void chess_string_init_assign(ChessString* string, const char* s)
{
    chess_string_init_assign_size(string, s, strlen(s));
}

void chess_string_init_assign_size(ChessString* string, const char* s, size_t n)
{
    char* buf;
    assert(s != NULL);
    if (n == 0)
    {
        string->size = 0;
        string->data = empty_string;
        return;
    }
    string->size = n;
    buf = (char*)chess_alloc(n + 1);
    strncpy(buf, s, n);
    buf[n] = '\0';
    string->data = buf;
}

void chess_string_cleanup(ChessString* string)
{
    if (string->size > 0)
        chess_free((char*)string->data);
}

size_t chess_string_size(const ChessString* string)
{
    return string->size;
}

const char* chess_string_data(const ChessString* string)
{
    return (string->size > 0) ? string->data : empty_string;
}

void chess_string_clear(ChessString* string)
{
    chess_string_cleanup(string);
    chess_string_init(string);
}

void chess_string_assign(ChessString* string, const char* s)
{
    chess_string_cleanup(string);
    chess_string_init_assign(string, s);
}

void chess_string_assign_size(ChessString* string, const char* s, size_t n)
{
    chess_string_cleanup(string);
    chess_string_init_assign_size(string, s, n);
}
