#ifndef BAA_LEXER_H
#define BAA_LEXER_H

#include <stdio.h>
#include <wchar.h>
#include <stdbool.h>

// Token types
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_COMMENT,
    // Keywords
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_DO,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQUALS
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    wchar_t *lexeme;
    int line;
    int column;
    size_t length;
} Token;

// Lexer state
typedef struct {
    const wchar_t *source;
    size_t position;
    size_t line;
    size_t column;
    Token current;
    bool had_error;
} Lexer;

/**
 * @brief Get the size of a file.
 *
 * This function calculates the size of the given file by seeking to the end
 * and using ftell to determine the position.
 *
 * @param file A pointer to the FILE object.
 * @return The size of the file in bytes, or 0 if the file is NULL or an error occurs.
 */
long baa_file_size(FILE *file);

/**
 * @brief Read the content of a file.
 *
 * This function opens a file in read mode and reads its content into a wide character string.
 *
 * @param path The path to the file.
 * @return A pointer to the wide character string containing the file content, or NULL if the file cannot be opened.
 */
wchar_t *baa_file_content(const wchar_t *path);

/**
 * @brief Initialize a new lexer instance.
 *
 * @param source The source code to tokenize.
 * @return A pointer to the initialized Lexer structure.
 */
Lexer *baa_lexer_init(const wchar_t *source);

/**
 * @brief Free resources used by the lexer.
 *
 * @param lexer The lexer instance to free.
 */
void baa_lexer_free(Lexer *lexer);

/**
 * @brief Get the next token from the source.
 *
 * @param lexer The lexer instance.
 * @return The next token.
 */
Token baa_lexer_next_token(Lexer *lexer);

/**
 * @brief Check if the lexer encountered any errors.
 *
 * @param lexer The lexer instance.
 * @return true if there were errors, false otherwise.
 */
bool baa_lexer_had_error(const Lexer *lexer);

#endif /* BAA_LEXER_H */
