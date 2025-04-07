#include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/types/types.h"
#include "baa/operators/operators.h"
#include "baa/utils/utils.h"
#include "baa/lexer/lexer.h"
#include "baa/ast/literals.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdbool.h>

// Forward declarations
void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);
void baa_free_expression(BaaExpr* expr);
void baa_token_next(BaaParser *parser);

// Forward declaration of internal functions
static BaaExpr* parse_primary(BaaParser* parser);
static BaaExpr* parse_call_or_member(BaaParser* parser);
static BaaExpr* parse_unary(BaaParser* parser);
static BaaExpr* parse_factor(BaaParser* parser);
static BaaExpr* parse_term(BaaParser* parser);
static BaaExpr* parse_comparison(BaaParser* parser);
static BaaExpr* parse_equality(BaaParser* parser);
static BaaExpr* parse_logical_and(BaaParser* parser);
static BaaExpr* parse_logical_or(BaaParser* parser);
static BaaExpr* parse_assignment(BaaParser* parser);

// Forward declaration of the public function
BaaExpr* baa_parse_expression(BaaParser* parser);

// Forward declarations for expression freeing functions (assuming they exist elsewhere)
void baa_free_binary_expr_data(void* data);
void baa_free_unary_expr_data(void* data);
void baa_free_literal_expr_data(void* data);
void baa_free_variable_expr_data(void* data);
void baa_free_call_expr_data(void* data);
void baa_free_cast_expr_data(void* data);
void baa_free_assign_expr_data(void* data);
void baa_free_array_expr_data(void* data);
void baa_free_index_expr_data(void* data);
void baa_free_compound_assignment_expr_data(void* data);
void baa_free_inc_dec_expr_data(void* data);
void baa_free_grouping_expr_data(void* data);

// Implementation of the literal expression creation functions
static BaaExpr* baa_create_int_literal_expr(int value) {
    // Create the literal data
    BaaLiteralData* literal_data = baa_create_int_literal_data(value);
    if (!literal_data) return NULL;

    // Create the literal expression
    BaaExpr* expr = baa_create_literal_expr(literal_data);
    if (!expr) {
        baa_free_literal_data(literal_data);
        return NULL;
    }

    return expr;
}

static BaaExpr* baa_create_float_literal_expr(float value) {
    // Create the literal data
    BaaLiteralData* literal_data = baa_create_float_literal_data(value);
    if (!literal_data) return NULL;

    // Create the literal expression
    BaaExpr* expr = baa_create_literal_expr(literal_data);
    if (!expr) {
        baa_free_literal_data(literal_data);
        return NULL;
    }

    return expr;
}

static BaaExpr* baa_create_string_literal_expr(const wchar_t* value, size_t length) {
    // Create the literal data
    BaaLiteralData* literal_data = baa_create_string_literal_data(value, length);
    if (!literal_data) return NULL;

    // Create the literal expression
    BaaExpr* expr = baa_create_literal_expr(literal_data);
    if (!expr) {
        baa_free_literal_data(literal_data);
        return NULL;
    }

    return expr;
}

static BaaExpr* baa_create_bool_literal_expr(bool value) {
    // Create the literal data
    BaaLiteralData* literal_data = baa_create_bool_literal_data(value);
    if (!literal_data) return NULL;

    // Create the literal expression
    BaaExpr* expr = baa_create_literal_expr(literal_data);
    if (!expr) {
        baa_free_literal_data(literal_data);
        return NULL;
    }

    return expr;
}

static BaaExpr* baa_create_char_literal_expr(wchar_t value) {
    // Create the literal data
    BaaLiteralData* literal_data = baa_create_char_literal_data(value);
    if (!literal_data) return NULL;

    // Create the literal expression
    BaaExpr* expr = baa_create_literal_expr(literal_data);
    if (!expr) {
        baa_free_literal_data(literal_data);
        return NULL;
    }

    return expr;
}

static BaaExpr* baa_create_null_literal_expr(void) {
    // Create the literal data
    BaaLiteralData* literal_data = baa_create_null_literal_data();
    if (!literal_data) return NULL;

    // Create the literal expression
    BaaExpr* expr = baa_create_literal_expr(literal_data);
    if (!expr) {
        baa_free_literal_data(literal_data);
        return NULL;
    }

    return expr;
}

static BaaExpr* baa_create_identifier_expr(const wchar_t* name, size_t length) {
    // Make a copy of the identifier name
    wchar_t* name_copy = baa_strndup(name, length);
    if (!name_copy) return NULL;

    // Create the variable expression
    BaaExpr* expr = baa_create_variable_expr(name_copy, length);
    baa_free(name_copy); // baa_create_variable_expr makes its own copy

    return expr;
}

static BaaExpr* baa_create_call_expr_wrapper(BaaExpr* callee, BaaExpr** args, size_t arg_count) {
    return baa_create_call_expr(callee, args, arg_count, NULL, false);
}

static BaaExpr* baa_create_binary_expr_wrapper(BaaExpr* left, BaaExpr* right, BaaOperatorType op) {
    return baa_create_binary_expr(op, left, right);
}

static BaaExpr* baa_create_unary_expr_wrapper(BaaOperatorType op, BaaExpr* operand) {
    return baa_create_unary_expr(op, operand);
}

static BaaExpr* baa_create_member_access(BaaExpr* object, const wchar_t* name, size_t length) {
    // Create an identifier for the member name
    BaaExpr* member_name = baa_create_identifier_expr(name, length);
    if (!member_name) return NULL;

    // Create a binary expression for the member access
    BaaExpr* expr = baa_create_binary_expr_wrapper(object, member_name, BAA_OP_DOT);
    if (!expr) {
        baa_free_expr(member_name);
        return NULL;
    }

    return expr;
}

static BaaExpr* baa_create_array_access(BaaExpr* array, BaaExpr* index) {
    // Create a binary expression for the array access
    BaaExpr* expr = baa_create_binary_expr_wrapper(array, index, BAA_OP_SUBSCRIPT);
    if (!expr) return NULL;

    return expr;
}

static BaaExpr* baa_create_assignment(BaaExpr* target, BaaExpr* value) {
    // For now, we'll implement assignment as a binary expression with a special operator
    BaaExpr* expr = baa_create_binary_expr_wrapper(target, value, BAA_OP_ASSIGN);
    if (!expr) return NULL;

    return expr;
}

/**
 * Create a compound assignment expression
 */
static BaaExpr* baa_create_compound_assignment(BaaExpr* target, BaaExpr* value, BaaOperatorType op)
{
    // Validate input
    if (!target || !value) return NULL;

    // Use the specific creation function
    BaaExpr* expr = baa_create_compound_assignment_expr(target, value, op);
    return expr;
}

/**
 * Create an increment/decrement expression
 */
BaaExpr* baa_create_inc_dec_expr(BaaExpr* operand, BaaOperatorType op, bool is_prefix)
{
    // Use the specific creation function
    BaaExpr* expr = baa_create_inc_dec_expr(operand, op, is_prefix);
    return expr;
}

/**
 * Create a grouping expression
 */
static BaaExpr* baa_create_grouping(BaaExpr* expression)
{
    // Validate input
    if (!expression) return NULL;

    // Use the specific creation function
    BaaExpr* expr = baa_create_grouping_expr(expression);
    return expr;
}

/**
 * Parse a primary expression (literals, identifiers, grouping)
 */
static BaaExpr* parse_primary(BaaParser* parser)
{
    // Handle different token types for primary expressions
    switch (parser->current_token.type) {
        case BAA_TOKEN_INT_LIT:
        case BAA_TOKEN_FLOAT_LIT: {
            // Parse number literal
            const wchar_t* lexeme = parser->current_token.lexeme;
            size_t length = parser->current_token.length;

            // Check if it's an integer or float
            bool is_float = parser->current_token.type == BAA_TOKEN_FLOAT_LIT;

            BaaExpr* expr;
            if (is_float) {
                // Parse as float
                double value = wcstod(lexeme, NULL);
                expr = baa_create_float_literal_expr((float)value);
            } else {
                // Parse as integer
                long value = wcstol(lexeme, NULL, 10);
                expr = baa_create_int_literal_expr((int)value);
            }

            if (!expr) {
                baa_set_parser_error(parser, L"فشل في إنشاء تعبير رقمي");
                return NULL;
            }

            // Consume the number token
            baa_token_next(parser);
            return expr;
        }

        case BAA_TOKEN_BOOL_LIT: {
            // Parse boolean literal
            const wchar_t* lexeme = parser->current_token.lexeme;
            bool value = false;

            // Check if it's صحيح or خطأ
            if (wcsncmp(lexeme, L"صحيح", 4) == 0) {
                value = true;
            } else if (wcsncmp(lexeme, L"خطأ", 3) == 0) {
                value = false;
            }

            // Create the boolean literal expression
            BaaExpr* expr = baa_create_bool_literal_expr(value);
            if (!expr) {
                baa_set_parser_error(parser, L"فشل في إنشاء تعبير منطقي");
                return NULL;
            }

            // Consume the boolean token
            baa_token_next(parser);
            return expr;
        }

        case BAA_TOKEN_STRING_LIT: {
            // Parse string literal
            const wchar_t* value = parser->current_token.lexeme;
            size_t length = parser->current_token.length;

            // Create the string literal expression
            BaaExpr* expr = baa_create_string_literal_expr(value, length);
            if (!expr) {
                baa_set_parser_error(parser, L"فشل في إنشاء تعبير نصي");
                return NULL;
            }

            // Consume the string token
            baa_token_next(parser);
            return expr;
        }

        case BAA_TOKEN_IDENTIFIER: {
            // Parse identifier
            const wchar_t* name = parser->current_token.lexeme;
            size_t length = parser->current_token.length;

            // Create the identifier expression
            BaaExpr* expr = baa_create_identifier_expr(name, length);
            if (!expr) {
                baa_set_parser_error(parser, L"فشل في إنشاء معرف");
                return NULL;
            }

            // Consume the identifier token
            baa_token_next(parser);
            return expr;
        }

        case BAA_TOKEN_LPAREN: {
            // Parse grouping expression
            baa_token_next(parser);

            // Parse the expression inside the parentheses
            BaaExpr* expr = baa_parse_expression(parser);
            if (!expr) {
                return NULL;
            }

            // Expect closing parenthesis
            if (parser->current_token.type != BAA_TOKEN_RPAREN) {
                baa_unexpected_token_error(parser, L")");
                baa_free_expression(expr);
                return NULL;
            }

            // Consume the closing parenthesis
            baa_token_next(parser);

            // Create the grouping expression
            BaaExpr* grouping = baa_create_grouping(expr);
            if (!grouping) {
                baa_set_parser_error(parser, L"فشل في إنشاء تعبير مجموعة");
                baa_free_expression(expr);
                return NULL;
            }

            return grouping;
        }

        default:
            baa_set_parser_error(parser, L"توقعت تعبير");
            return NULL;
    }
}

/**
 * Parse a call expression or member access
 */
static BaaExpr* parse_call_or_member(BaaParser* parser)
{
    // Parse the primary expression
    BaaExpr* expr = parse_primary(parser);
    if (!expr) {
        return NULL;
    }

    // Handle call expressions and member access
    while (true) {
        if (parser->current_token.type == BAA_TOKEN_LPAREN) {
            // Function call
            // Consume the opening parenthesis
            baa_token_next(parser);

            // Parse arguments
            BaaExpr** arguments = NULL;
            size_t argument_count = 0;
            size_t argument_capacity = 0;

            // Parse arguments until we reach the closing parenthesis
            if (parser->current_token.type != BAA_TOKEN_RPAREN) {
                do {
                    // Parse an argument
                    BaaExpr* argument = baa_parse_expression(parser);
                    if (!argument) {
                        // Free already parsed arguments
                        for (size_t i = 0; i < argument_count; i++) {
                            baa_free_expression(arguments[i]);
                        }
                        free(arguments);
                        baa_free_expression(expr);
                        return NULL;
                    }

                    // Add argument to the list
                    if (argument_count >= argument_capacity) {
                        argument_capacity = argument_capacity == 0 ? 4 : argument_capacity * 2;
                        BaaExpr** new_arguments = realloc(arguments, argument_capacity * sizeof(BaaExpr*));
                        if (!new_arguments) {
                            baa_set_parser_error(parser, L"فشل في تخصيص الذاكرة للوسائط");
                            baa_free_expression(argument);
                            for (size_t i = 0; i < argument_count; i++) {
                                baa_free_expression(arguments[i]);
                            }
                            free(arguments);
                            baa_free_expression(expr);
                            return NULL;
                        }
                        arguments = new_arguments;
                    }

                    arguments[argument_count++] = argument;

                    // Check for comma
                    if (parser->current_token.type == BAA_TOKEN_COMMA) {
                        baa_token_next(parser);
                    } else {
                        break;
                    }
                } while (parser->current_token.type != BAA_TOKEN_RPAREN);
            }

            // Expect closing parenthesis
            if (parser->current_token.type != BAA_TOKEN_RPAREN) {
                baa_unexpected_token_error(parser, L")");
                for (size_t i = 0; i < argument_count; i++) {
                    baa_free_expression(arguments[i]);
                }
                free(arguments);
                baa_free_expression(expr);
                return NULL;
            }

            // Consume the closing parenthesis
            baa_token_next(parser);

            // Create the call expression
            BaaExpr* call = baa_create_call_expr_wrapper(expr, arguments, argument_count);
            if (!call) {
                baa_set_parser_error(parser, L"فشل في إنشاء نداء دالة");
                for (size_t i = 0; i < argument_count; i++) {
                    baa_free_expression(arguments[i]);
                }
                free(arguments);
                baa_free_expression(expr);
                return NULL;
            }

            expr = call;
        } else if (parser->current_token.type == BAA_TOKEN_DOT) {
            // Member access
            // Consume the dot token
            baa_token_next(parser);

            // Expect an identifier for the member name
            if (parser->current_token.type != BAA_TOKEN_IDENTIFIER) {
                baa_unexpected_token_error(parser, L"معرف");
                baa_free_expression(expr);
                return NULL;
            }

            // Get the member name
            const wchar_t* name = parser->current_token.lexeme;
            size_t length = parser->current_token.length;

            // Consume the identifier token
            baa_token_next(parser);

            // Create the member access expression
            BaaExpr* member = baa_create_member_access(expr, name, length);
            if (!member) {
                baa_set_parser_error(parser, L"فشل في إنشاء وصول للعضو");
                baa_free_expression(expr);
                return NULL;
            }

            expr = member;
        } else if (parser->current_token.type == BAA_TOKEN_LEFT_BRACKET) {
            // Array access
            // Consume the opening bracket
            baa_token_next(parser);

            // Parse the index expression
            BaaExpr* index = baa_parse_expression(parser);
            if (!index) {
                baa_free_expression(expr);
                return NULL;
            }

            // Expect closing bracket
            if (parser->current_token.type != BAA_TOKEN_RIGHT_BRACKET) {
                baa_unexpected_token_error(parser, L"]");
                baa_free_expression(index);
                baa_free_expression(expr);
                return NULL;
            }

            // Consume the closing bracket
            baa_token_next(parser);

            // Create the array access expression
            BaaExpr* array_access = baa_create_array_access(expr, index);
            if (!array_access) {
                baa_set_parser_error(parser, L"فشل في إنشاء وصول للمصفوفة");
                baa_free_expression(index);
                baa_free_expression(expr);
                return NULL;
            }

            expr = array_access;
        } else {
            break;
        }
    }

    return expr;
}

/**
 * Parse a unary expression
 */
static BaaExpr* parse_unary(BaaParser* parser)
{
    // Check for prefix increment and decrement operators
    if (parser->current_token.type == BAA_TOKEN_INCREMENT ||
        parser->current_token.type == BAA_TOKEN_DECREMENT) {

        // Get the operator type
        BaaOperatorType op_type = (parser->current_token.type == BAA_TOKEN_INCREMENT)
                                 ? BAA_OP_INC : BAA_OP_DEC;

        // Consume the operator token
        baa_token_next(parser);

        // Parse the operand (must be a valid assignment target)
        BaaExpr* operand = parse_unary(parser);
        if (!operand) {
            return NULL;
        }

        // Create the prefix increment/decrement expression
        BaaExpr* expr = baa_create_inc_dec_expr(operand, op_type, true);
        if (!expr) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير زيادة/نقصان");
            baa_free_expression(operand);
            return NULL;
        }

        return expr;
    }

    // Check for other unary operators
    if (parser->current_token.type == BAA_TOKEN_MINUS ||
        parser->current_token.type == BAA_TOKEN_BANG) {

        // Get the operator type
        BaaOperatorType op_type;
        if (parser->current_token.type == BAA_TOKEN_MINUS) {
            op_type = BAA_OP_SUB;  // Changed from BAA_OP_NEGATE
        } else {
            op_type = BAA_OP_NOT;  // This matches the operators.h definition
        }

        // Consume the operator token
        baa_token_next(parser);

        // Parse the operand
        BaaExpr* operand = parse_unary(parser);
        if (!operand) {
            return NULL;
        }

        // Create the unary expression
        BaaExpr* expr = baa_create_unary_expr_wrapper(op_type, operand);
        if (!expr) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير أحادي");
            baa_free_expression(operand);
            return NULL;
        }

        return expr;
    }

    // If not a unary operator, parse as a call expression
    BaaExpr* expr = parse_call_or_member(parser);

    // Check for postfix increment and decrement operators
    if (expr && (parser->current_token.type == BAA_TOKEN_INCREMENT ||
                parser->current_token.type == BAA_TOKEN_DECREMENT)) {

        // Get the operator type
        BaaOperatorType op_type = (parser->current_token.type == BAA_TOKEN_INCREMENT)
                                 ? BAA_OP_INC : BAA_OP_DEC;

        // Consume the operator token
        baa_token_next(parser);

        // Create the postfix increment/decrement expression
        BaaExpr* inc_dec_expr = baa_create_inc_dec_expr(expr, op_type, false);
        if (!inc_dec_expr) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير زيادة/نقصان");
            baa_free_expression(expr);
            return NULL;
        }

        return inc_dec_expr;
    }

    return expr;
}

/**
 * Parse a factor expression (multiplication, division, modulo)
 */
static BaaExpr* parse_factor(BaaParser* parser)
{
    // Parse the left operand
    BaaExpr* left = parse_unary(parser);
    if (!left) {
        return NULL;
    }

    // Check for factor operators
    while (parser->current_token.type == BAA_TOKEN_STAR ||
           parser->current_token.type == BAA_TOKEN_SLASH ||
           parser->current_token.type == BAA_TOKEN_PERCENT) {

        // Get the operator type
        BaaOperatorType op_type;
        if (parser->current_token.type == BAA_TOKEN_STAR) {
            op_type = BAA_OP_MUL;  // Changed from BAA_OP_MULTIPLY
        } else if (parser->current_token.type == BAA_TOKEN_SLASH) {
            op_type = BAA_OP_DIV;  // Changed from BAA_OP_DIVIDE
        } else {
            op_type = BAA_OP_MOD;  // Changed from BAA_OP_MODULO
        }

        // Consume the operator token
        baa_token_next(parser);

        // Parse the right operand
        BaaExpr* right = parse_unary(parser);
        if (!right) {
            baa_free_expression(left);
            return NULL;
        }

        // Create the binary expression
        BaaExpr* binary = baa_create_binary_expr_wrapper(left, right, op_type);
        if (!binary) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير ثنائي");
            baa_free_expression(left);
            baa_free_expression(right);
            return NULL;
        }

        // Update the left operand for the next iteration
        left = binary;
    }

    return left;
}

/**
 * Parse a term expression (addition, subtraction)
 */
static BaaExpr* parse_term(BaaParser* parser)
{
    // Parse the left operand
    BaaExpr* left = parse_factor(parser);
    if (!left) {
        return NULL;
    }

    // Check for term operators
    while (parser->current_token.type == BAA_TOKEN_PLUS ||
           parser->current_token.type == BAA_TOKEN_MINUS) {

        // Get the operator type
        BaaOperatorType op_type;
        if (parser->current_token.type == BAA_TOKEN_PLUS) {
            op_type = BAA_OP_ADD;  // Changed from BAA_OP_ADD
        } else {
            op_type = BAA_OP_SUB;  // Changed from BAA_OP_SUBTRACT
        }

        // Consume the operator token
        baa_token_next(parser);

        // Parse the right operand
        BaaExpr* right = parse_factor(parser);
        if (!right) {
            baa_free_expression(left);
            return NULL;
        }

        // Create the binary expression
        BaaExpr* binary = baa_create_binary_expr_wrapper(left, right, op_type);
        if (!binary) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير ثنائي");
            baa_free_expression(left);
            baa_free_expression(right);
            return NULL;
        }

        // Update the left operand for the next iteration
        left = binary;
    }

    return left;
}

/**
 * Parse a comparison expression (>, >=, <, <=)
 */
static BaaExpr* parse_comparison(BaaParser* parser)
{
    // Parse the left operand
    BaaExpr* left = parse_term(parser);
    if (!left) {
        return NULL;
    }

    // Check for comparison operators
    while (parser->current_token.type == BAA_TOKEN_GREATER ||
           parser->current_token.type == BAA_TOKEN_GREATER_EQUAL ||
           parser->current_token.type == BAA_TOKEN_LESS ||
           parser->current_token.type == BAA_TOKEN_LESS_EQUAL) {

        // Get the operator type
        BaaOperatorType op_type;
        if (parser->current_token.type == BAA_TOKEN_GREATER) {
            op_type = BAA_OP_GT;  // Changed from BAA_OP_GREATER
        } else if (parser->current_token.type == BAA_TOKEN_GREATER_EQUAL) {
            op_type = BAA_OP_GE;  // Changed from BAA_OP_GREATER_EQUAL
        } else if (parser->current_token.type == BAA_TOKEN_LESS) {
            op_type = BAA_OP_LT;  // Changed from BAA_OP_LESS
        } else {
            op_type = BAA_OP_LE;  // Changed from BAA_OP_LESS_EQUAL
        }

        // Consume the operator token
        baa_token_next(parser);

        // Parse the right operand
        BaaExpr* right = parse_term(parser);
        if (!right) {
            baa_free_expression(left);
            return NULL;
        }

        // Create the binary expression
        BaaExpr* binary = baa_create_binary_expr_wrapper(left, right, op_type);
        if (!binary) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير ثنائي");
            baa_free_expression(left);
            baa_free_expression(right);
            return NULL;
        }

        // Update the left operand for the next iteration
        left = binary;
    }

    return left;
}

/**
 * Parse an equality expression (==, !=)
 */
static BaaExpr* parse_equality(BaaParser* parser)
{
    // Parse the left operand
    BaaExpr* left = parse_comparison(parser);
    if (!left) {
        return NULL;
    }

    // Check for equality operators
    while (parser->current_token.type == BAA_TOKEN_EQUAL_EQUAL ||
           parser->current_token.type == BAA_TOKEN_BANG_EQUAL) {

        // Get the operator type
        BaaOperatorType op_type;
        if (parser->current_token.type == BAA_TOKEN_EQUAL_EQUAL) {
            op_type = BAA_OP_EQ;  // Changed from BAA_OP_EQUAL
        } else {
            op_type = BAA_OP_NE;  // Changed from BAA_OP_NOT_EQUAL
        }

        // Consume the operator token
        baa_token_next(parser);

        // Parse the right operand
        BaaExpr* right = parse_comparison(parser);
        if (!right) {
            baa_free_expression(left);
            return NULL;
        }

        // Create the binary expression
        BaaExpr* binary = baa_create_binary_expr_wrapper(left, right, op_type);
        if (!binary) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير ثنائي");
            baa_free_expression(left);
            baa_free_expression(right);
            return NULL;
        }

        // Update the left operand for the next iteration
        left = binary;
    }

    return left;
}

/**
 * Parse a logical AND expression
 */
static BaaExpr* parse_logical_and(BaaParser* parser)
{
    // Parse the left operand
    BaaExpr* left = parse_equality(parser);
    if (!left) {
        return NULL;
    }

    // Check for AND operator
    while (parser->current_token.type == BAA_TOKEN_AND) {
        // Consume the AND token
        baa_token_next(parser);

        // Parse the right operand
        BaaExpr* right = parse_equality(parser);
        if (!right) {
            baa_free_expression(left);
            return NULL;
        }

        // Create the logical expression
        BaaExpr* logical = baa_create_binary_expr_wrapper(left, right, BAA_OP_AND);
        if (!logical) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير منطقي");
            baa_free_expression(left);
            baa_free_expression(right);
            return NULL;
        }

        // Update the left operand for the next iteration
        left = logical;
    }

    return left;
}

/**
 * Parse a logical OR expression
 */
static BaaExpr* parse_logical_or(BaaParser* parser)
{
    // Parse the left operand
    BaaExpr* left = parse_logical_and(parser);
    if (!left) {
        return NULL;
    }

    // Check for OR operator
    while (parser->current_token.type == BAA_TOKEN_OR) {
        // Consume the OR token
        baa_token_next(parser);

        // Parse the right operand
        BaaExpr* right = parse_logical_and(parser);
        if (!right) {
            baa_free_expression(left);
            return NULL;
        }

        // Create the logical expression
        BaaExpr* logical = baa_create_binary_expr_wrapper(left, right, BAA_OP_OR);
        if (!logical) {
            baa_set_parser_error(parser, L"فشل في إنشاء تعبير منطقي");
            baa_free_expression(left);
            baa_free_expression(right);
            return NULL;
        }

        // Update the left operand for the next iteration
        left = logical;
    }

    return left;
}

/**
 * Parse an assignment expression
 */
static BaaExpr* parse_assignment(BaaParser* parser)
{
    // Parse the left-hand side (a logical OR expression)
    BaaExpr* left = parse_logical_or(parser);
    if (!left) {
        return NULL;
    }

    // Check for assignment operators
    bool is_assignment = false;
    BaaOperatorType op = BAA_OP_NONE;
    BaaTokenType token_type = parser->current_token.type;

    // Handle all types of assignment operators
    switch (token_type) {
        case BAA_TOKEN_ASSIGN:
            is_assignment = true;
            op = BAA_OP_ASSIGN;
            break;
        case BAA_TOKEN_PLUS_EQUAL:
            is_assignment = true;
            op = BAA_OP_ADD_ASSIGN;
            break;
        case BAA_TOKEN_MINUS_EQUAL:
            is_assignment = true;
            op = BAA_OP_SUB_ASSIGN;
            break;
        case BAA_TOKEN_STAR_EQUAL:
            is_assignment = true;
            op = BAA_OP_MUL_ASSIGN;
            break;
        case BAA_TOKEN_SLASH_EQUAL:
            is_assignment = true;
            op = BAA_OP_DIV_ASSIGN;
            break;
        case BAA_TOKEN_PERCENT_EQUAL:
            is_assignment = true;
            op = BAA_OP_MOD_ASSIGN;
            break;
        default:
            // Not an assignment operator
            break;
    }

    if (is_assignment) {
        // Consume the operator token
        baa_token_next(parser);

        // Parse the right-hand side (an assignment expression)
        BaaExpr* right = parse_assignment(parser);
        if (!right) {
            baa_free_expression(left);
            return NULL;
        }

        if (token_type == BAA_TOKEN_ASSIGN) {
            // Create a simple assignment expression
            BaaExpr* assignment = baa_create_assignment(left, right);
            if (!assignment) {
                baa_set_parser_error(parser, L"فشل في إنشاء تعبير تعيين");
                baa_free_expression(left);
                baa_free_expression(right);
                return NULL;
            }
            return assignment;
        } else {
            // Create a compound assignment expression
            BaaExpr* assignment = baa_create_compound_assignment(left, right, op);
            if (!assignment) {
                baa_set_parser_error(parser, L"فشل في إنشاء تعبير تعيين مركب");
                baa_free_expression(left);
                baa_free_expression(right);
                return NULL;
            }
            return assignment;
        }
    }

    // Not an assignment expression
    return left;
}

/**
 * Parse an expression
 */
BaaExpr* baa_parse_expression(BaaParser* parser)
{
    return parse_assignment(parser);
}

/**
 * Alias baa_free_expr as baa_free_expression for backward compatibility
 */
void baa_free_expression(BaaExpr* expr) {
    if (!expr) return;

    // Free the expression data based on its type
    if (expr->data) {
        switch (expr->kind) { // Use kind
            case BAA_EXPR_BINARY:
                baa_free_binary_expr_data(expr->data);
                break;
            case BAA_EXPR_UNARY:
                baa_free_unary_expr_data(expr->data);
                break;
            case BAA_EXPR_LITERAL:
                baa_free_literal_expr_data(expr->data);
                break;
            case BAA_EXPR_VARIABLE:
                baa_free_variable_expr_data(expr->data);
                break;
            case BAA_EXPR_CALL:
                baa_free_call_expr_data(expr->data);
                break;
            case BAA_EXPR_ARRAY:
                baa_free_array_expr_data(expr->data);
                break;
            case BAA_EXPR_COMPOUND_ASSIGN:
                baa_free_compound_assignment_expr_data(expr->data);
                break;
            case BAA_EXPR_INC_DEC:
                baa_free_inc_dec_expr_data(expr->data);
                break;
            case BAA_EXPR_GROUPING:
                baa_free_grouping_expr_data(expr->data);
                break;
            case BAA_EXPR_INDEX:
                baa_free_index_expr_data(expr->data);
                break;
            case BAA_EXPR_CAST:
                baa_free_cast_expr_data(expr->data);
                break;
            case BAA_EXPR_ASSIGN:
                baa_free_assign_expr_data(expr->data);
                break;
            default:
                // Handle unknown expression type or do nothing
                break;
        }
        baa_free(expr->data); // Free the data pointer itself
    }

    // Free the main expression structure
    // Note: Does not free the associated AST node (expr->ast_node)
    baa_free(expr);
}
