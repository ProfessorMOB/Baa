#include "baa/ast/statements.h"
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations
static void baa_free_case_stmt(BaaCaseStmt* case_stmt);
static void baa_free_default_stmt(BaaDefaultStmt* default_stmt);

// Generic statement creation helper
static BaaStmt* baa_create_stmt(BaaStmtKind kind) {
    BaaStmt* stmt = (BaaStmt*)baa_malloc(sizeof(BaaStmt));
    if (!stmt) return NULL;

    stmt->kind = kind;
    stmt->ast_node = NULL;
    stmt->data = NULL;

    return stmt;
}

// Block creation
BaaBlock* baa_create_block(void) {
    BaaBlock* block = (BaaBlock*)baa_malloc(sizeof(BaaBlock));
    if (!block) return NULL;

    block->statements = NULL;
    block->count = 0;
    block->capacity = 0;

    return block;
}

// Statement creation functions
BaaStmt* baa_create_if_stmt(BaaExpr* condition, BaaBlock* if_body, BaaBlock* else_body) {
    if (!condition || !if_body) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_IF);
    if (!stmt) return NULL;

    BaaIfStmt* if_stmt = (BaaIfStmt*)baa_malloc(sizeof(BaaIfStmt));
    if (!if_stmt) {
        baa_free(stmt);
        return NULL;
    }

    if_stmt->condition = condition;
    if_stmt->if_body = if_body;
    if_stmt->else_body = else_body;  // Can be NULL

    stmt->data = if_stmt;

    return stmt;
}

BaaStmt* baa_create_while_stmt(BaaExpr* condition, BaaBlock* body) {
    if (!condition || !body) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_WHILE);
    if (!stmt) return NULL;

    BaaWhileStmt* while_stmt = (BaaWhileStmt*)baa_malloc(sizeof(BaaWhileStmt));
    if (!while_stmt) {
        baa_free(stmt);
        return NULL;
    }

    while_stmt->condition = condition;
    while_stmt->body = body;

    stmt->data = while_stmt;

    return stmt;
}

BaaStmt* baa_create_for_stmt(BaaStmt* initializer, BaaExpr* condition, BaaExpr* increment, BaaBlock* body) {
    if (!body) return NULL;
    // Note: initializer, condition, and increment can be NULL in certain cases

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_FOR);
    if (!stmt) return NULL;

    BaaForStmt* for_stmt = (BaaForStmt*)baa_malloc(sizeof(BaaForStmt));
    if (!for_stmt) {
        baa_free(stmt);
        return NULL;
    }

    for_stmt->initializer = initializer;  // Can be NULL
    for_stmt->condition = condition;      // Can be NULL (infinite loop)
    for_stmt->increment = increment;      // Can be NULL
    for_stmt->body = body;

    stmt->data = for_stmt;

    return stmt;
}

BaaStmt* baa_create_return_stmt(BaaExpr* value) {
    BaaStmt* stmt = baa_create_stmt(BAA_STMT_RETURN);
    if (!stmt) return NULL;

    BaaReturnStmt* return_stmt = (BaaReturnStmt*)baa_malloc(sizeof(BaaReturnStmt));
    if (!return_stmt) {
        baa_free(stmt);
        return NULL;
    }

    return_stmt->value = value;  // Can be NULL for void returns

    stmt->data = return_stmt;

    return stmt;
}

BaaStmt* baa_create_expr_stmt(BaaExpr* expr) {
    if (!expr) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_EXPR);
    if (!stmt) return NULL;

    BaaExprStmt* expr_stmt = (BaaExprStmt*)baa_malloc(sizeof(BaaExprStmt));
    if (!expr_stmt) {
        baa_free(stmt);
        return NULL;
    }

    expr_stmt->expr = expr;

    stmt->data = expr_stmt;

    return stmt;
}

BaaStmt* baa_create_block_stmt(void) {
    BaaStmt* stmt = baa_create_stmt(BAA_STMT_BLOCK);
    if (!stmt) return NULL;

    BaaBlock* block = baa_create_block();
    if (!block) {
        baa_free(stmt);
        return NULL;
    }

    stmt->data = block;

    return stmt;
}

BaaStmt* baa_create_var_decl_stmt(const wchar_t* name, size_t name_length, BaaType* type, BaaExpr* initializer) {
    if (!name || !type) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_VAR_DECL);
    if (!stmt) return NULL;

    BaaVarDeclStmt* var_decl = (BaaVarDeclStmt*)baa_malloc(sizeof(BaaVarDeclStmt));
    if (!var_decl) {
        baa_free(stmt);
        return NULL;
    }

    // Duplicate the name string
    wchar_t* name_copy = baa_strndup(name, name_length);
    if (!name_copy) {
        baa_free(var_decl);
        baa_free(stmt);
        return NULL;
    }

    var_decl->name = name_copy;
    var_decl->name_length = name_length;
    var_decl->type = type;
    var_decl->initializer = initializer;  // Can be NULL

    stmt->data = var_decl;

    return stmt;
}

BaaStmt* baa_create_switch_stmt(BaaExpr* expression) {
    if (!expression) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_SWITCH);
    if (!stmt) return NULL;

    BaaSwitchStmt* switch_stmt = (BaaSwitchStmt*)baa_malloc(sizeof(BaaSwitchStmt));
    if (!switch_stmt) {
        baa_free(stmt);
        return NULL;
    }

    switch_stmt->expression = expression;
    switch_stmt->cases = NULL;
    switch_stmt->case_count = 0;
    switch_stmt->default_case = NULL;

    stmt->data = switch_stmt;

    return stmt;
}

BaaStmt* baa_create_case_stmt(BaaExpr* value, BaaBlock* body, bool has_fallthrough) {
    if (!value || !body) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_CASE);
    if (!stmt) return NULL;

    BaaCaseStmt* case_stmt = (BaaCaseStmt*)baa_malloc(sizeof(BaaCaseStmt));
    if (!case_stmt) {
        baa_free(stmt);
        return NULL;
    }

    case_stmt->value = value;
    case_stmt->body = body;
    case_stmt->has_fallthrough = has_fallthrough;

    stmt->data = case_stmt;

    return stmt;
}

BaaStmt* baa_create_default_stmt(BaaBlock* body) {
    if (!body) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_DEFAULT);
    if (!stmt) return NULL;

    BaaDefaultStmt* default_stmt = (BaaDefaultStmt*)baa_malloc(sizeof(BaaDefaultStmt));
    if (!default_stmt) {
        baa_free(stmt);
        return NULL;
    }

    default_stmt->body = body;

    stmt->data = default_stmt;

    return stmt;
}

BaaStmt* baa_create_break_stmt(int loop_depth, bool is_switch_break) {
    // Validate the parameters before proceeding
    if (!baa_validate_break_stmt(loop_depth, is_switch_break)) {
        return NULL;
    }

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_BREAK);
    if (!stmt) return NULL;

    BaaBreakStmt* break_stmt = (BaaBreakStmt*)baa_malloc(sizeof(BaaBreakStmt));
    if (!break_stmt) {
        baa_free(stmt);
        return NULL;
    }

    break_stmt->loop_depth = loop_depth;
    break_stmt->is_switch_break = is_switch_break;

    stmt->data = break_stmt;

    return stmt;
}

BaaStmt* baa_create_continue_stmt(int loop_depth) {
    // Validate the parameters before proceeding
    if (!baa_validate_continue_stmt(loop_depth)) {
        return NULL;
    }

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_CONTINUE);
    if (!stmt) return NULL;

    BaaContinueStmt* continue_stmt = (BaaContinueStmt*)baa_malloc(sizeof(BaaContinueStmt));
    if (!continue_stmt) {
        baa_free(stmt);
        return NULL;
    }

    continue_stmt->loop_depth = loop_depth;

    stmt->data = continue_stmt;

    return stmt;
}

BaaStmt* baa_create_import_stmt(const wchar_t* path, const wchar_t* alias) {
    if (!path) return NULL;

    BaaStmt* stmt = baa_create_stmt(BAA_STMT_IMPORT);
    if (!stmt) return NULL;

    BaaImportStmt* import_data = (BaaImportStmt*)baa_malloc(sizeof(BaaImportStmt));
    if (!import_data) {
        baa_free(stmt);
        return NULL;
    }

    // Duplicate path and alias
    import_data->path = baa_strdup(path);
    import_data->alias = alias ? baa_strdup(alias) : NULL;

    if (!import_data->path || (alias && !import_data->alias)) {
        // Handle allocation failure during string duplication
        baa_free((void*)import_data->path);
        baa_free((void*)import_data->alias);
        baa_free(import_data);
        baa_free(stmt);
        return NULL;
    }

    stmt->data = import_data;

    return stmt;
}


bool baa_add_case_to_switch(BaaStmt* switch_stmt, BaaStmt* case_stmt) {
    if (!switch_stmt || !case_stmt || switch_stmt->kind != BAA_STMT_SWITCH ||
        (case_stmt->kind != BAA_STMT_CASE && case_stmt->kind != BAA_STMT_DEFAULT)) {
        return false;
    }

    BaaSwitchStmt* swstmt = (BaaSwitchStmt*)switch_stmt->data;

    // If it's a default case, just store it
    if (case_stmt->kind == BAA_STMT_DEFAULT) {
        if (swstmt->default_case) {
            // Cannot have multiple default cases
            return false;
        }
        swstmt->default_case = (BaaDefaultStmt*)case_stmt->data;
        return true;
    }

    // It's a regular case
    // Allocate or reallocate the cases array
    if (swstmt->case_count == 0) {
        swstmt->cases = (BaaCaseStmt**)baa_malloc(sizeof(BaaCaseStmt*));
        if (!swstmt->cases) return false;
    } else {
        BaaCaseStmt** new_cases = (BaaCaseStmt**)baa_realloc(
            swstmt->cases,
            (swstmt->case_count + 1) * sizeof(BaaCaseStmt*)
        );
        if (!new_cases) return false;
        swstmt->cases = new_cases;
    }

    // Add the case to the array
    swstmt->cases[swstmt->case_count] = (BaaCaseStmt*)case_stmt->data;
    swstmt->case_count++;

    return true;
}

bool baa_set_default_case(BaaStmt* switch_stmt, BaaStmt* default_stmt) {
    if (!switch_stmt || !default_stmt || switch_stmt->kind != BAA_STMT_SWITCH ||
        default_stmt->kind != BAA_STMT_DEFAULT) {
        return false;
    }

    BaaSwitchStmt* swstmt = (BaaSwitchStmt*)switch_stmt->data;

    if (swstmt->default_case) {
        // Cannot have multiple default cases
        return false;
    }

    swstmt->default_case = (BaaDefaultStmt*)default_stmt->data;

    return true;
}

// Block management
bool baa_add_stmt_to_block(BaaBlock* block, BaaStmt* stmt) {
    if (!block || !stmt) return false;

    // Check if we need to grow the array
    if (block->count >= block->capacity) {
        size_t new_capacity = block->capacity == 0 ? 4 : block->capacity * 2;
        BaaStmt** new_statements = (BaaStmt**)baa_realloc(block->statements,
                                                         sizeof(BaaStmt*) * new_capacity);
        if (!new_statements) return false;

        block->statements = new_statements;
        block->capacity = new_capacity;
    }

    // Add the statement
    block->statements[block->count++] = stmt;
    return true;
}

// Statement validation
bool baa_validate_condition(BaaExpr* condition) {
    if (!condition) return false;

    // TODO: Check type compatibility and ensure it's a boolean-compatible expression
    return true;
}

bool baa_validate_return_value(BaaExpr* value, BaaType* expected_type) {
    if (!value) {
        // If no return value but we expect one
        return expected_type == NULL || expected_type->kind == BAA_TYPE_VOID;
    }

    // TODO: Check type compatibility
    return true;
}

bool baa_validate_var_decl(const wchar_t* name, size_t name_length, BaaType* type, BaaExpr* initializer) {
    if (!name || name_length == 0) return false;

    // TODO: Check variable name validity, type compatibility with initializer, etc.
    return true;
}

bool baa_validate_for_stmt(BaaStmt* initializer, BaaExpr* condition, BaaExpr* increment) {
    // Initializer is optional (NULL is valid for this argument)
    // If provided, it must be a valid declaration or expression
    if (initializer && initializer->kind != BAA_STMT_VAR_DECL && initializer->kind != BAA_STMT_EXPR) {
        return false;
    }

    // If condition is provided, validate it
    if (condition && !baa_validate_condition(condition)) {
        return false;
    }

    // Increment is optional (NULL is valid)

    return true;
}

bool baa_validate_switch_expr(BaaExpr* expression) {
    if (!expression) return false;

    // TODO: Check that the expression is of a type that can be switched on
    // (typically integral types, enums, or other comparable types)

    return true;
}

bool baa_validate_case_value(BaaExpr* value, BaaExpr* switch_expr) {
    if (!value || !switch_expr) return false;

    // TODO: Check that:
    // 1. Case values are constant expressions
    // 2. Their types are compatible with the switch expression
    // 3. Case values are unique across the switch

    return true;
}

bool baa_validate_break_stmt(int loop_depth, bool is_switch_break) {
    // Loop depth must be at least 1
    if (loop_depth < 1) return false;

    // TODO: In a real implementation, we would need to check that:
    // 1. There is actually a loop or switch to break out of
    // 2. The loop_depth does not exceed the actual loop nesting level
    // 3. If is_switch_break is true, we are actually in a switch statement

    // For now, just do a basic validation
    return true;
}

bool baa_validate_continue_stmt(int loop_depth) {
    // Loop depth must be at least 1
    if (loop_depth < 1) return false;

    // TODO: In a real implementation, we would need to check that:
    // 1. There is actually a loop to continue
    // 2. The loop_depth does not exceed the actual loop nesting level

    // For now, just do a basic validation
    return true;
}

// Memory management
void baa_free_block(BaaBlock* block) {
    if (!block) return;

    if (block->statements) {
        for (size_t i = 0; i < block->count; i++) {
            baa_free_stmt(block->statements[i]);
        }
        baa_free(block->statements);
    }

    baa_free(block);
}

void baa_free_stmt(BaaStmt* stmt) {
    if (!stmt) return;

    // Free the statement data based on kind
    switch (stmt->kind) {
        case BAA_STMT_IF: {
            BaaIfStmt* if_stmt = (BaaIfStmt*)stmt->data;
            if (if_stmt) {
                baa_free_expr(if_stmt->condition);
                baa_free_block(if_stmt->if_body);
                baa_free_block(if_stmt->else_body);
                baa_free(if_stmt);
            }
            break;
        }

        case BAA_STMT_WHILE: {
            BaaWhileStmt* while_stmt = (BaaWhileStmt*)stmt->data;
            if (while_stmt) {
                baa_free_expr(while_stmt->condition);
                baa_free_block(while_stmt->body);
                baa_free(while_stmt);
            }
            break;
        }

        case BAA_STMT_FOR: {
            BaaForStmt* for_stmt = (BaaForStmt*)stmt->data;
            if (for_stmt) {
                baa_free_stmt(for_stmt->initializer);
                baa_free_expr(for_stmt->condition);
                baa_free_expr(for_stmt->increment);
                baa_free_block(for_stmt->body);
                baa_free(for_stmt);
            }
            break;
        }

        case BAA_STMT_RETURN: {
            BaaReturnStmt* return_stmt = (BaaReturnStmt*)stmt->data;
            if (return_stmt) {
                baa_free_expr(return_stmt->value);
                baa_free(return_stmt);
            }
            break;
        }

        case BAA_STMT_EXPR: {
            BaaExprStmt* expr_stmt = (BaaExprStmt*)stmt->data;
            if (expr_stmt) {
                baa_free_expr(expr_stmt->expr);
                baa_free(expr_stmt);
            }
            break;
        }

        case BAA_STMT_BLOCK: {
            BaaBlock* block = (BaaBlock*)stmt->data;
            if (block) {
                baa_free_block(block);
            }
            break;
        }

        case BAA_STMT_VAR_DECL: {
            BaaVarDeclStmt* var_decl = (BaaVarDeclStmt*)stmt->data;
            if (var_decl) {
                baa_free((void*)var_decl->name);
                baa_free_expr(var_decl->initializer);
                baa_free(var_decl);
            }
            break;
        }

        case BAA_STMT_SWITCH: {
            BaaSwitchStmt* switch_stmt_data = (BaaSwitchStmt*)stmt->data; // Use different name
            if (switch_stmt_data) {
                baa_free_expr(switch_stmt_data->expression);
                for (size_t i = 0; i < switch_stmt_data->case_count; i++) {
                    // Assuming cases store the full BaaStmt, not just BaaCaseStmt data
                    // If cases store BaaCaseStmt*, need a specific free function
                     baa_free_stmt((BaaStmt*)switch_stmt_data->cases[i]); // Adjust if needed
                }
                baa_free(switch_stmt_data->cases);
                // Assuming default_case stores the full BaaStmt
                baa_free_stmt((BaaStmt*)switch_stmt_data->default_case); // Adjust if needed
                baa_free(switch_stmt_data);
            }
            break;
        }
        // Add case for BAA_STMT_IMPORT
        case BAA_STMT_IMPORT: {
            BaaImportStmt* import_data = (BaaImportStmt*)stmt->data;
            if (import_data) {
                baa_free((void*)import_data->path);
                baa_free((void*)import_data->alias);
                baa_free(import_data);
            }
            break;
        }
        // Add cases for BREAK and CONTINUE if they allocate data
        case BAA_STMT_BREAK: {
             BaaBreakStmt* break_data = (BaaBreakStmt*)stmt->data;
             if (break_data) baa_free(break_data);
             break;
        }
        case BAA_STMT_CONTINUE: {
             BaaContinueStmt* continue_data = (BaaContinueStmt*)stmt->data;
             if (continue_data) baa_free(continue_data);
             break;
        }
        // Add cases for CASE and DEFAULT if they allocate data beyond the block
         case BAA_STMT_CASE: {
             BaaCaseStmt* case_data = (BaaCaseStmt*)stmt->data;
             if (case_data) {
                 baa_free_expr(case_data->value);
                 baa_free_block(case_data->body); // Body is freed here
                 baa_free(case_data);
             }
             break;
         }
         case BAA_STMT_DEFAULT: {
             BaaDefaultStmt* default_data = (BaaDefaultStmt*)stmt->data;
             if (default_data) {
                 baa_free_block(default_data->body); // Body is freed here
                 baa_free(default_data);
             }
             break;
         }

    }

    // Free the statement itself
    baa_free(stmt);
}

void baa_free_case_stmt(BaaCaseStmt* case_stmt) {
    if (!case_stmt) return;

    baa_free_expr(case_stmt->value);
    baa_free_block(case_stmt->body);
    baa_free(case_stmt);
}

void baa_free_default_stmt(BaaDefaultStmt* default_stmt) {
    if (!default_stmt) return;

    baa_free_block(default_stmt->body);
    baa_free(default_stmt);
}
