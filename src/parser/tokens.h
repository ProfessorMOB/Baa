#ifndef BAA_TOKENS_H
#define BAA_TOKENS_H

typedef enum
{
    // Special tokens
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    TOKEN_DOT, // Statement terminator '.'

    // Keywords
    TOKEN_FUNCTION, // دالة
    TOKEN_RETURN,   // إرجع
    TOKEN_IF,       // إذا
    TOKEN_ELSE,     // وإلا
    TOKEN_WHILE,    // طالما
    TOKEN_FOR,      // من_أجل
    TOKEN_DO,       // افعل
    TOKEN_SWITCH,   // اختر
    TOKEN_CASE,     // حالة
    TOKEN_BREAK,    // توقف
    TOKEN_CONTINUE, // استمر

    // Types
    TOKEN_INT,    // عدد_صحيح
    TOKEN_FLOAT,  // عدد_حقيقي
    TOKEN_CHAR,   // حرف
    TOKEN_VOID,   // فراغ
    TOKEN_STRUCT, // بنية
    TOKEN_UNION,  // اتحاد

    // Storage classes
    TOKEN_AUTO,     // تلقائي
    TOKEN_STATIC,   // ثابت
    TOKEN_EXTERN,   // خارجي
    TOKEN_REGISTER, // سجل

    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,

    // Operators
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_STAR,          // *
    TOKEN_SLASH,         // /
    TOKEN_PERCENT,       // %
    TOKEN_EQUAL,         // =
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_BANG_EQUAL,    // !=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_AND,           // &&
    TOKEN_OR,            // ||
    TOKEN_NOT,           // !

    // Delimiters
    TOKEN_LEFT_PAREN,    // (
    TOKEN_RIGHT_PAREN,   // )
    TOKEN_LEFT_BRACE,    // {
    TOKEN_RIGHT_BRACE,   // }
    TOKEN_LEFT_BRACKET,  // [
    TOKEN_RIGHT_BRACKET, // ]
    TOKEN_COMMA,         // ,
    TOKEN_SEMICOLON,     // ;

    // Preprocessor
    TOKEN_HASH,    // #
    TOKEN_INCLUDE, // تضمين#
    TOKEN_DEFINE,  // تعريف#
    TOKEN_IFDEF,   // إذا_عرف#
    TOKEN_ENDIF,   // نهاية_إذا#
} TokenType;

typedef struct
{
    TokenType type;
    const wchar_t *start;
    int length;
    int line;
    int column;
} Token;

#endif // BAA_TOKENS_H
