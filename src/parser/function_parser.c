#include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/types/types.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Forward declarations for functions defined in other files
extern BaaExpr* baa_parse_expression(BaaParser* parser);
extern BaaType* baa_parse_type_annotation(BaaParser* parser);
extern void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
extern void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);

// Forward declarations for functions defined in this file
BaaStmt* baa_parse_function_declaration(BaaParser* parser);
BaaStmt* baa_parse_function_definition(BaaParser* parser);
BaaStmt* baa_parse_function_call(BaaParser* parser);
BaaStmt* baa_parse_return_statement(BaaParser* parser);

// Function implementations for statement creation
BaaStmt* baa_create_function_declaration(const wchar_t* name, size_t name_len,
                                       BaaParameter** parameters, size_t param_count,
                                       BaaType* return_type, BaaBlock* body) {
    return baa_create_func_decl_stmt(name, name_len, parameters, param_count, return_type, body);
}

BaaStmt* baa_create_function_definition(const wchar_t* name, size_t name_len,
                                      BaaParameter** parameters, size_t param_count,
                                      BaaType* return_type, BaaBlock* body) {
    return baa_create_func_def_stmt(name, name_len, parameters, param_count, return_type, body);
}

BaaStmt* baa_create_function_call(const wchar_t* name, size_t name_len,
                                BaaExpr** arguments, size_t arg_count) {
    return baa_create_func_call_stmt(name, name_len, arguments, arg_count);
}

BaaStmt* baa_create_return_statement(BaaExpr* value) {
    return baa_create_return_stmt(value);
}

// ... rest of the file ...
