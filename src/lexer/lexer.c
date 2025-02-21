#include "baa/lexer.h"
#include "parser/tokens.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Array of keywords and their corresponding token types
static struct
{
    const wchar_t *keyword;
    TokenType token;
} keywords[] = {
    {L"دالة", TOKEN_FUNCTION},
    {L"إرجع", TOKEN_RETURN},
    {L"إذا", TOKEN_IF},
    {L"إلا", TOKEN_ELSE},
    {L"طالما", TOKEN_WHILE},
    {L"لأجل", TOKEN_FOR},
    {L"افعل", TOKEN_DO},
    {L"اختر", TOKEN_SWITCH},
    {L"حالة", TOKEN_CASE},
    {L"توقف", TOKEN_BREAK},
    {L"استمر", TOKEN_CONTINUE}};

long baa_file_size(FILE *file)
{
    if (!file)
    {
        return 0;
    }
    fpos_t original = 0;
    if (fgetpos(file, &original) != 0)
    {
        printf("fgetpos() فشلت دالة: %i \n", errno);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long out = ftell(file);
    if (fsetpos(file, &original) != 0)
    {
        printf("fsetpos() فشلت دالة: %i \n", errno);
    }
    return out;
}

wchar_t *baa_file_content(const wchar_t *path)
{
    FILE *file;
    errno_t err = _wfopen_s(&file, path, L"rb, ccs=UTF-16LE");
    if (err != 0)
    {
        printf("لا يمكن فتح الملف\n");
        return NULL;
    }

    // Skip BOM if present
    wchar_t bom;
    if (fread(&bom, sizeof(wchar_t), 1, file) == 1)
    {
        if (bom != 0xFEFF)
        {
            fseek(file, 0, SEEK_SET);
        }
    }

    long file_size = baa_file_size(file);
    size_t char_count = (file_size / sizeof(wchar_t)) + 1; // +1 for null terminator
    wchar_t *contents = malloc(char_count * sizeof(wchar_t));

    if (!contents)
    {
        fclose(file);
        return NULL;
    }

    size_t chars_read = fread(contents, sizeof(wchar_t), char_count - 1, file);
    contents[chars_read] = L'\0';

    fclose(file);
    return contents;
}

// New keyword initialization
void initialize_keywords()
{
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i)
    {
        // Initialize each keyword here
        // This will be used by the lexer to recognize keywords
        printf("Initialized keyword: %ls\n", keywords[i].keyword);
    }
}
