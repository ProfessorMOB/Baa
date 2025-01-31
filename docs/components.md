# B (باء) Compiler Components

## Lexical Analyzer

### Overview
The lexical analyzer (lexer) is responsible for reading source files and converting them into a stream of tokens. It handles UTF-16LE encoded files to support Arabic text and follows K&R C token patterns.

### Token Types
```c
typedef enum {
    // Keywords
    TOKEN_FUNCTION,    // دالة
    TOKEN_RETURN,      // إرجع
    TOKEN_IF,          // إذا
    TOKEN_ELSE,        // وإلا
    TOKEN_WHILE,       // طالما
    TOKEN_FOR,         // من_أجل
    TOKEN_STRUCT,      // بنية
    TOKEN_UNION,       // اتحاد
    TOKEN_TYPEDEF,     // نوع_مستخدم
    
    // Types
    TOKEN_INT,         // عدد_صحيح
    TOKEN_FLOAT,       // عدد_حقيقي
    TOKEN_CHAR,        // محرف
    TOKEN_VOID,        // فراغ
    
    // Operators
    TOKEN_PLUS,        // +
    TOKEN_MINUS,       // -
    TOKEN_MULTIPLY,    // *
    TOKEN_DIVIDE,      // /
    TOKEN_MODULO,      // %
    TOKEN_ASSIGN,      // =
    TOKEN_EQ,          // ==
    TOKEN_NEQ,         // !=
    TOKEN_LT,          // <
    TOKEN_GT,          // >
    TOKEN_LTE,         // <=
    TOKEN_GTE,         // >=
    TOKEN_AND,         // &&
    TOKEN_OR,          // ||
    TOKEN_NOT,         // !
    
    // Punctuation
    TOKEN_LPAREN,      // (
    TOKEN_RPAREN,      // )
    TOKEN_LBRACE,      // {
    TOKEN_RBRACE,      // }
    TOKEN_LBRACKET,    // [
    TOKEN_RBRACKET,    // ]
    TOKEN_SEMICOLON,   // ;
    TOKEN_COMMA,       // ,
    TOKEN_DOT,         // .
    
    // Literals
    TOKEN_NUMBER,      // Numeric literal
    TOKEN_STRING,      // String literal
    TOKEN_CHAR_LIT,    // Character literal
    TOKEN_IDENTIFIER,  // Identifier
    
    TOKEN_EOF,         // End of file
    TOKEN_ERROR        // Error token
} TokenType;
```

### Interface
```c
// Token structure
typedef struct {
    TokenType type;
    wchar_t *lexeme;
    int line;
    int column;
} Token;

// Lexer functions
Token *baa_next_token(void);
void baa_init_lexer(const wchar_t *source);
void baa_free_lexer(void);
```

## Parser

### Overview
The parser builds an Abstract Syntax Tree (AST) from the token stream, following K&R C grammar with Arabic syntax support.

### Grammar Rules
```ebnf
program     → declaration*
declaration → functionDecl | varDecl | structDecl | unionDecl | typedefDecl
functionDecl → "دالة" IDENTIFIER "(" parameters? ")" block
varDecl     → type IDENTIFIER ("=" expression)? ";"
structDecl  → "بنية" IDENTIFIER "{" declaration* "}"
unionDecl   → "اتحاد" IDENTIFIER "{" declaration* "}"
typedefDecl → "نوع_مستخدم" type IDENTIFIER ";"

statement   → exprStmt | ifStmt | whileStmt | returnStmt | block
block       → "{" declaration* statement* "}"
ifStmt      → "إذا" "(" expression ")" statement ("وإلا" statement)?
whileStmt   → "طالما" "(" expression ")" statement
returnStmt  → "إرجع" expression? ";"

expression  → assignment
assignment  → IDENTIFIER "=" expression | logical
logical     → equality ("&&" | "||") equality
equality    → comparison ("==" | "!=") comparison
comparison  → term ("<" | ">" | "<=" | ">=") term
term        → factor ("+" | "-") factor
factor      → unary ("*" | "/" | "%") unary
unary       → ("!" | "-") unary | primary
primary     → NUMBER | STRING | "true" | "false" | "(" expression ")"
```

## Type System

### Basic Types
The type system implements K&R C compatible types with Arabic names:

| Arabic Name | C Type | Description |
|------------|--------|-------------|
| عدد_صحيح   | int    | 32-bit integer |
| عدد_حقيقي  | float  | 32-bit float |
| محرف       | char   | 16-bit UTF-16 character |
| فراغ       | void   | No value type |

### Type Operations
```c
// Create a new type
Type* baa_create_type(TypeKind kind, uint32_t size, bool is_signed);

// Compare two types
bool baa_types_equal(Type* a, Type* b);

// Check type conversion
bool baa_can_convert(Type* from, Type* to);
```

### Type Safety
- Strong type checking
- Explicit conversion rules
- Error type for invalid operations
- UTF-16 support for Arabic text

## Operator System

### Basic Operators
| Category | Operators | Arabic Names |
|----------|-----------|--------------|
| Arithmetic | +, -, *, /, % | جمع, طرح, ضرب, قسمة, باقي |
| Comparison | ==, !=, <, >, <=, >= | يساوي, لا_يساوي, أصغر_من, أكبر_من, أصغر_أو_يساوي, أكبر_أو_يساوي |
| Assignment | = | تعيين |

### Operator Functions
```c
// Get operator by symbol
Operator* baa_get_operator(const char* symbol);

// Check types for binary operation
Type* baa_check_binary_op(Type* left, Type* right, OperatorKind op);

// Validate operator usage
bool baa_is_valid_operator(Type* left, Type* right, OperatorKind op);
```

### Type Rules
1. Arithmetic Operations:
   - Both operands must be numeric
   - Result is float if either operand is float
   - Modulo requires integer operands

2. Comparison Operations:
   - Both operands must be comparable
   - Result is always integer (0 or 1)
   - Numeric types can be compared

3. Assignment Operations:
   - Right operand must be convertible to left operand's type
   - Result type is left operand's type

## Symbol Table

### Symbol Types
```c
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
    SYMBOL_PARAMETER
} SymbolKind;

typedef struct Symbol {
    SymbolKind kind;
    wchar_t *name;
    Type *type;
    int scope_level;
    union {
        struct {
            int is_defined;
            Type *return_type;
            struct SymbolTable *parameters;
        } function;
        struct {
            int offset;
        } variable;
    } info;
} Symbol;
```

## Code Generator

### LLVM IR Generation
```c
// Generate LLVM IR for expressions
LLVMValueRef baa_gen_expression(Node *node);

// Generate LLVM IR for statements
void baa_gen_statement(Node *node);

// Generate LLVM IR for declarations
void baa_gen_declaration(Node *node);

// Set optimization level
void baa_set_optimization_level(int level);
```

## Error Handling

### Error Types
```c
typedef enum {
    ERROR_SYNTAX,
    ERROR_TYPE,
    ERROR_UNDEFINED,
    ERROR_REDEFINED,
    ERROR_INTERNAL
} ErrorType;

// Report an error
void baa_report_error(ErrorType type, const wchar_t *message, int line, int column);

// Get error count
int baa_get_error_count(void);
```

## Build System

### CMake Configuration
```cmake
# Component libraries
add_library(baa_lexer
    src/lexer/lexer.c
    src/lexer/token.c
)

add_library(baa_parser
    src/parser/parser.c
    src/parser/ast.c
)

add_library(baa_types
    src/types/type.c
    src/types/check.c
)

add_library(baa_codegen
    src/codegen/gen.c
    src/codegen/optimize.c
)

# Main executable
add_executable(Baa src/main.c)
target_link_libraries(Baa
    baa_lexer
    baa_parser
    baa_types
    baa_codegen
)
