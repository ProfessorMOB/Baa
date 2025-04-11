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

// Function implementation
BaaStmt* baa_parse_function(BaaParser* parser) {
    // Parse function name
    if (parser->current.type != BAA_TOKEN_IDENTIFIER) {
        baa_unexpected_token_error(parser, L"معرف الدالة");
        return NULL;
    }
    const wchar_t* name = parser->current.value;
    baa_token_next(parser);

    // Parse parameters
    if (parser->current.type != BAA_TOKEN_LEFT_PAREN) {
        baa_unexpected_token_error(parser, L"(");
        return NULL;
    }
    baa_token_next(parser);

    BaaParameter** params = NULL;
    size_t param_count = 0;
    size_t param_capacity = 0;

    while (parser->current.type != BAA_TOKEN_RIGHT_PAREN) {
        if (parser->current.type != BAA_TOKEN_IDENTIFIER) {
            baa_unexpected_token_error(parser, L"معرف الوسيط");
            goto cleanup;
        }
        const wchar_t* param_name = parser->current.value;
        baa_token_next(parser);

        if (parser->current.type != BAA_TOKEN_COLON) {
            baa_unexpected_token_error(parser, L":");
            goto cleanup;
        }
        baa_token_next(parser);

        BaaType* param_type = baa_parse_type(parser);
        if (!param_type) {
            baa_set_parser_error(parser, L"فشل في تحليل نوع الوسيط");
            goto cleanup;
        }

        // Create parameter
        BaaParameter* param = baa_create_parameter(param_name, param_type);
        if (!param) {
            baa_set_parser_error(parser, L"فشل في إنشاء وسيط");
            baa_free_type(param_type);
            goto cleanup;
        }

        // Add parameter to array
        if (param_count >= param_capacity) {
            size_t new_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
            BaaParameter** new_params = baa_realloc(params, new_capacity * sizeof(BaaParameter*));
            if (!new_params) {
                baa_free_parameter(param);
                baa_set_parser_error(parser, L"فشل في تخصيص الذاكرة");
                goto cleanup;
            }
            params = new_params;
            param_capacity = new_capacity;
        }
        params[param_count++] = param;

        if (parser->current.type == BAA_TOKEN_COMMA) {
            baa_token_next(parser);
        } else if (parser->current.type != BAA_TOKEN_RIGHT_PAREN) {
            baa_unexpected_token_error(parser, L", أو )");
            goto cleanup;
        }
    }
    baa_token_next(parser);

    // Parse return type
    BaaType* return_type = NULL;
    if (parser->current.type == BAA_TOKEN_ARROW) {
        baa_token_next(parser);
        return_type = baa_parse_type(parser);
        if (!return_type) {
            baa_set_parser_error(parser, L"فشل في تحليل نوع الإرجاع");
            goto cleanup;
        }
    }

    // Parse function body
    if (parser->current.type != BAA_TOKEN_LEFT_BRACE) {
        baa_unexpected_token_error(parser, L"{");
        goto cleanup;
    }
    baa_token_next(parser);

    BaaStmt* body = baa_parse_block(parser);
    if (!body) {
        baa_set_parser_error(parser, L"فشل في تحليل جسم الدالة");
        goto cleanup;
    }

    // Create function declaration
    BaaStmt* func_decl = baa_create_function_declaration(name, params, param_count, return_type, body);
    if (!func_decl) {
        baa_set_parser_error(parser, L"فشل في إنشاء تصريح الدالة");
        goto cleanup;
    }

    return func_decl;

cleanup:
    // Clean up parameters
    for (size_t i = 0; i < param_count; i++) {
        baa_free_parameter(params[i]);
    }
    baa_free(params);
    baa_free_type(return_type);
    return NULL;
}

// ... rest of the file ...
