#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "chess.h"
#include "cstring.h"
#include "pgn-tokenizer.h"

static void token_init_simple(ChessPgnToken* token, ChessPgnTokenType type)
{
    token->type = type;
}

static void token_init_symbol(ChessPgnToken* token, const char* s, size_t n)
{
    token->type = CHESS_PGN_TOKEN_SYMBOL;
    chess_string_init_assign_size(&token->data.string, s, n);
}

static void token_init_string(ChessPgnToken* token, const char* s, size_t n)
{
    char* buf = malloc(n + 1);
    size_t i, b = 0;

    for (i = 0; i < n; i++)
    {
        if (s[i] == '\\' && (s[i + 1] == '\\' || s[i + 1] == '"'))
            i++;
        buf[b++] = s[i];
    }
    token->type = CHESS_PGN_TOKEN_STRING;
    chess_string_init_assign_size(&token->data.string, buf, b);
    free(buf);
}

static ChessBoolean token_init_number(ChessPgnToken* token, const char* s, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        if (!isdigit(s[i]))
            return CHESS_FALSE;
    token->type = CHESS_PGN_TOKEN_NUMBER;
    token->data.number = strtol(s, NULL, 10);
    return CHESS_TRUE;
}

static void pgn_token_cleanup(ChessPgnToken* token)
{
    switch (token->type)
    {
        case CHESS_PGN_TOKEN_SYMBOL:
        case CHESS_PGN_TOKEN_STRING:
            chess_string_cleanup(&token->data.string);
            break;
        default:
            break;
    }
}

static const char* string_token_end(const char* s)
{
    /* Eat everything, but check for escape chars */
    while (*s)
    {
        if (*s == '"')
            break;

        if (*s == '\\' && *(s + 1) == '"')
            s++;

        s++;
    }
    return s;
}

static const char* symbol_token_end(const char* s)
{
    while (*s && (isalnum(*s) || strchr("_+#=:-", *s)))
        ++s;
    return s;
}

static void skip_token(ChessPgnTokenizer* tokenizer)
{
    ChessPgnToken* token = tokenizer->last;
    size_t size;
    const char *t, *s;

    pgn_token_cleanup(tokenizer->last);
    tokenizer->last = tokenizer->next;
    tokenizer->next = token;

    while (isspace(tokenizer->text[tokenizer->index]))
        tokenizer->index++;

    t = &tokenizer->text[tokenizer->index];

    if (!strncmp(t, "1-0", 3))
    {
        token_init_simple(token, CHESS_PGN_TOKEN_ONE_ZERO);
        tokenizer->index += 3;
        return;
    }
    if (!strncmp(t, "0-1", 3))
    {
        token_init_simple(token, CHESS_PGN_TOKEN_ZERO_ONE);
        tokenizer->index += 3;
        return;
    }
    if (!strncmp(t, "1/2-1/2", 7))
    {
        token_init_simple(token, CHESS_PGN_TOKEN_HALF_HALF);
        tokenizer->index += 7;
        return;
    }

    if (*t == '"')
    {
        /* String token */
        s = string_token_end(t + 1);
        size = s - t - 1;
        token_init_string(token, t + 1, size);
        tokenizer->index += size + 2;
        return;
    }

    if (isalnum(*t))
    {
        /* Symbol or integer token */
        s = symbol_token_end(t + 1);
        size = s - t;
        if (!token_init_number(token, t, size))
            token_init_symbol(token, t, size);
        tokenizer->index += size;
        return;
    }

    switch (*t)
    {
        case '\0':
            token_init_simple(token, CHESS_PGN_TOKEN_EOF);
            return;
        case '(':
            token_init_simple(token, CHESS_PGN_TOKEN_L_PARENTHESIS);
            tokenizer->index++;
            return;
        case ')':
            token_init_simple(token, CHESS_PGN_TOKEN_R_PARENTHESIS);
            tokenizer->index++;
            return;
        case '[':
            token_init_simple(token, CHESS_PGN_TOKEN_L_BRACKET);
            tokenizer->index++;
            return;
        case ']':
            token_init_simple(token, CHESS_PGN_TOKEN_R_BRACKET);
            tokenizer->index++;
            return;
        case '{':
            token_init_simple(token, CHESS_PGN_TOKEN_L_BRACE);
            tokenizer->index++;
            return;
        case '}':
            token_init_simple(token, CHESS_PGN_TOKEN_R_BRACE);
            tokenizer->index++;
            return;
        case '*':
            token_init_simple(token, CHESS_PGN_TOKEN_ASTERISK);
            tokenizer->index++;
            return;
        case '.':
            token_init_simple(token, CHESS_PGN_TOKEN_PERIOD);
            tokenizer->index++;
            return;
        default:
            break;
    }
}

ChessPgnTokenizer* chess_pgn_tokenizer_new(const char* text)
{
    ChessPgnTokenizer* tokenizer = malloc(sizeof(ChessPgnTokenizer));
    memset(tokenizer, 0, sizeof(ChessPgnTokenizer));
    tokenizer->text = strdup(text);
    tokenizer->last = &tokenizer->tokens[0];
    tokenizer->next = &tokenizer->tokens[1];
    skip_token(tokenizer);
    return tokenizer;
}

const ChessPgnToken* chess_pgn_tokenizer_peek(const ChessPgnTokenizer* tokenizer)
{
    return tokenizer->next;
}

const ChessPgnToken* chess_pgn_tokenizer_next(ChessPgnTokenizer* tokenizer)
{
    skip_token(tokenizer);
    return tokenizer->last;
}

void chess_pgn_tokenizer_destroy(ChessPgnTokenizer* tokenizer)
{
    pgn_token_cleanup(tokenizer->last);
    pgn_token_cleanup(tokenizer->next);
    free((char*)tokenizer->text);
    free(tokenizer);
}