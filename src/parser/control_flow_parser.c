#include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/types/types.h"
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Forward declarations for functions defined in other files
extern BaaExpr* baa_parse_expression(BaaParser* parser);
extern BaaType* baa_parse_type_annotation(BaaParser* parser);
extern void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
extern void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);

// Forward declarations for functions defined in this file
BaaStmt* baa_parse_if_statement(BaaParser* parser);
BaaStmt* baa_parse_while_statement(BaaParser* parser);
BaaStmt* baa_parse_for_statement(BaaParser* parser);
BaaStmt* baa_parse_switch_statement(BaaParser* parser);
BaaStmt* baa_parse_case_statement(BaaParser* parser);
BaaStmt* baa_parse_default_statement(BaaParser* parser);

// Function implementations for statement creation
BaaStmt* baa_create_if_statement(BaaExpr* condition, BaaBlock* then_block, BaaBlock* else_block) {
    return baa_create_if_stmt(condition, then_block, else_block);
}

BaaStmt* baa_create_while_statement(BaaExpr* condition, BaaBlock* body) {
    return baa_create_while_stmt(condition, body);
}

BaaStmt* baa_create_for_statement(BaaStmt* init, BaaExpr* condition, BaaExpr* increment, BaaBlock* body) {
    return baa_create_for_stmt(init, condition, increment, body);
}

// Updated: Now only takes the expression. Cases/default are added later by the parser.
BaaStmt* baa_create_switch_statement(BaaExpr* condition) {
    return baa_create_switch_stmt(condition);
    // Note: The caller (parser) is now responsible for using
    // baa_add_case_to_switch and baa_set_default_case
}

// Updated: Now takes the has_fallthrough flag
BaaStmt* baa_create_case_statement(BaaExpr* value, BaaBlock* body, bool has_fallthrough) {
    return baa_create_case_stmt(value, body, has_fallthrough);
}

BaaStmt* baa_create_default_statement(BaaBlock* body) {
    return baa_create_default_stmt(body);
}

// ... rest of the file ...
