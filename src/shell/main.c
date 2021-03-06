#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

#include "../game.h"
#include "../generate.h"
#include "../parse.h"
#include "../print.h"
#include "../fen.h"
#include "../pgn.h"

static char* read_line(const char* prompt)
{
    char* buf, *c;
    size_t buf_size = 0;
    size_t chunk_size = 128;

    buf_size = chunk_size;
    buf = malloc(buf_size);

    fputs(prompt, stdout);

    c = buf;
    while ((c = fgets(c, chunk_size, stdin)))
    {
        c = strchr(buf, '\0');
        if (c > buf && *(c - 1) == '\n')
        {
            *(c - 1) = 0;
            return buf;
        }

        buf_size += chunk_size;
        buf = realloc(buf, buf_size);
    }
    return buf;
}

static ChessBoolean parse_line(char* s, char** cmd, char** args)
{
    while (isspace(*s))
        s++;
    *cmd = s;
    if (!*s)
        return CHESS_FALSE;

    while (*s && !isspace(*s))
        s++;

    *s++ = 0; /* Null terminate the cmd */

    /* Parse args */
    while (*s && isspace(*s))
        s++;
    *args = s;
    return CHESS_TRUE;
}

static void list_moves(const ChessGameIterator* iter)
{
    const ChessPosition* position = chess_game_iterator_position(iter);
    ChessMoveGenerator generator;
    ChessMove move;
    char buf[16];

    chess_move_generator_init(&generator, position);
    while ((move = chess_move_generator_next(&generator)))
    {
        chess_print_move_san(move, position, buf);
        printf("%s ", buf);
    }
    putchar('\n');
}

static void game_moves(const ChessGame* game)
{
    ChessFileWriter writer;
    chess_file_writer_init(&writer, stdout);
    chess_print_game_moves(game, (ChessWriter*)&writer);
    chess_file_writer_cleanup(&writer);
    putchar('\n');
}

static void print_board(const ChessGameIterator* iter)
{
    const ChessPosition* position = chess_game_iterator_position(iter);
    char buf[1024];
    chess_print_position(position, buf);
    fputs(buf, stdout);
}

static void load_fen(ChessGame* game, const char* fen)
{
    ChessGameIterator* iter;
    chess_game_reset_fen(game, fen);
    iter = chess_game_get_iterator(game);
    print_board(iter);
    chess_game_iterator_destroy(iter);
}

static void save_pgn(const ChessGame* game)
{
    ChessFileWriter writer;
    chess_file_writer_init(&writer, stdout);
    chess_pgn_save(game, (ChessWriter*)&writer);
    chess_file_writer_cleanup(&writer);
    putchar('\n');
}

/*
static void load_pgn(ChessGame* game, const char* filename)
{
    FILE* file;
    ChessFileReader reader;
    ChessPgnLoader loader;
    ChessPgnLoadResult result;
    int count = 0;

    file = fopen(filename, "r");
    if (file == NULL)
    {
        puts("Unable to open file.");
        return;
    }

    chess_file_reader_init(&reader, file);
    chess_pgn_loader_init(&loader, (ChessReader*)&reader);
    do
    {
        chess_game_reset(game);
        result = chess_pgn_loader_next(&loader, game);
        printf("%d. Loaded with result: %d\n", ++count, result);
    } while (result != CHESS_PGN_LOAD_EOF);
    chess_pgn_loader_cleanup(&loader);
    chess_file_reader_cleanup(&reader);
}
*/

static void undo_move(ChessGameIterator* iter)
{
    if (chess_game_iterator_ply(iter) == 0)
    {
        puts("Error: no move to undo");
    }
    else
    {
        chess_game_iterator_step_back(iter);
        print_board(iter);
    }
}

static void handle_move(ChessGameIterator* iter, const char* cmd)
{
    const ChessPosition* position = chess_game_iterator_position(iter);
    ChessMove move = 0;
    ChessParseMoveResult result = chess_parse_move(cmd, position, &move);
    ChessResult game_result;
    char buf[10];

    if (result == CHESS_PARSE_MOVE_ERROR)
    {
        printf("Error (unknown command): %s\n", cmd);
    }
    else if (result == CHESS_PARSE_MOVE_ILLEGAL)
    {
        printf("Error (illegal move): %s\n", cmd);
    }
    else if (result == CHESS_PARSE_MOVE_AMBIGUOUS)
    {
        printf("Error (ambiguous move): %s\n", cmd);
    }
    else
    {
        chess_print_move_san(move, position, buf);
        puts(buf);

        chess_game_iterator_append_move(iter, move);
        print_board(iter);

        game_result = chess_game_iterator_check_result(iter);
        if (game_result != CHESS_RESULT_NONE)
        {
            chess_print_result(game_result, buf);
            puts(buf);
        }
    }
}

static void set_event(ChessGame* game, const char* arg)
{
    if (strlen(arg) == 0)
        puts(chess_game_event(game));
    else
        chess_game_set_event(game, arg);
}

static void set_site(ChessGame* game, const char* arg)
{
    if (strlen(arg) == 0)
        puts(chess_game_site(game));
    else
        chess_game_set_site(game, arg);
}

static void set_date(ChessGame* game, const char* arg)
{
    if (strlen(arg) == 0)
        puts(chess_game_date(game));
    else
        chess_game_set_date(game, arg);
}

static void set_round(ChessGame* game, const char* arg)
{
    if (strlen(arg) == 0)
        puts(chess_game_round(game));
    else
        chess_game_set_round(game, arg);
}

static void set_white(ChessGame* game, const char* arg)
{
    if (strlen(arg) == 0)
        puts(chess_game_white(game));
    else
        chess_game_set_white(game, arg);
}

static void set_black(ChessGame* game, const char* arg)
{
    if (strlen(arg) == 0)
        puts(chess_game_black(game));
    else
        chess_game_set_black(game, arg);
}

static void set_result(ChessGame* game, const char* arg)
{
    ChessResult result;
    char buf[10];

    if (strlen(arg) == 0)
    {
        result = chess_game_result(game);
        if (result == CHESS_RESULT_NONE)
            result = CHESS_RESULT_IN_PROGRESS;
        chess_print_result(result, buf);
        puts(buf);
    }
    else
    {
        if (!strcasecmp(arg, "white"))
        {
            chess_game_set_result(game, CHESS_RESULT_WHITE_WINS);
            game_moves(game);
        }
        else if (!strcasecmp(arg, "black"))
        {
            chess_game_set_result(game, CHESS_RESULT_BLACK_WINS);
            game_moves(game);
        }
        else if (!strcasecmp(arg, "draw"))
        {
            chess_game_set_result(game, CHESS_RESULT_DRAW);
            game_moves(game);
        }
        else if (!strcasecmp(arg, "*"))
        {
            chess_game_set_result(game, CHESS_RESULT_NONE);
            game_moves(game);
        }
        else
        {
            printf("Error (unknown result): %s\n", arg);
        }
    }

}

int main (int argc, const char* argv[])
{
    char *line, *cmd, *args;
    ChessGame* game;
    ChessGameIterator* iter;
    int quit = 0;

    chess_generate_init();

    game = chess_game_new();
    iter = chess_game_get_iterator(game);
    print_board(iter);

    line = 0;

    for (;;)
    {
        if (line)
            free(line);

        if (quit)
            break;

        line = read_line("> ");
        if (!parse_line(line, &cmd, &args))
            continue;

        if (!strcmp(cmd, "quit") || !strcmp(cmd, "q"))
        {
            quit = 1;
        }
        else if (!strcmp(cmd, "new"))
        {
            chess_game_iterator_destroy(iter);
            chess_game_reset(game);
            iter = chess_game_get_iterator(game);
            print_board(iter);
        }
        else if (!strcmp(cmd, "fen"))
        {
            load_fen(game, args);
        }
        else if (!strcmp(cmd, "pgn"))
        {
            save_pgn(game);
        }
        else if (!strcmp(cmd, "ls"))
        {
            list_moves(iter);
        }
        else if (!strcmp(cmd, "moves"))
        {
            game_moves(game);
        }
        else if (!strcmp(cmd, "bd"))
        {
            print_board(iter);
        }
        else if (!strcmp(cmd, "undo"))
        {
            undo_move(iter);
        }
        else if (!strcmp(cmd, "event"))
        {
            set_event(game, args);
        }
        else if (!strcmp(cmd, "site"))
        {
            set_site(game, args);
        }
        else if (!strcmp(cmd, "date"))
        {
            set_date(game, args);
        }
        else if (!strcmp(cmd, "round"))
        {
            set_round(game, args);
        }
        else if (!strcmp(cmd, "white"))
        {
            set_white(game, args);
        }
        else if (!strcmp(cmd, "black"))
        {
            set_black(game, args);
        }
        else if (!strcmp(cmd, "result"))
        {
            set_result(game, args);
        }
        else
        {
            handle_move(iter, cmd);
        }
    }

    chess_game_iterator_destroy(iter);
    chess_game_destroy(game);

    return 0;
}
