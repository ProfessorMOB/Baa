#ifndef BAA_LEXER_H
#define BAA_LEXER_H

#include <stdio.h>
#include <wchar.h>

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

#endif /* BAA_LEXER_H */
