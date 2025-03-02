#include "baa/ast/expressions.h"
#include "baa/ast/literals.h"
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>

// Generic expression creation helper
static BaaExpr* baa_create_expr(BaaExprKind kind) {
    BaaExpr* expr = (BaaExpr*)baa_malloc(sizeof(BaaExpr));
    if (!expr) return NULL;
    
    expr->kind = kind;
    expr->type = NULL;
    expr->data = NULL;
    expr->ast_node = NULL;
    
    return expr;
}

BaaExpr* baa_create_literal_expr(BaaLiteralData* literal_data) {
    if (!literal_data) return NULL;
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_LITERAL);
    if (!expr) return NULL;
    
    // Create a copy of the literal data to ensure proper ownership
    BaaLiteralData* copied_data = baa_copy_literal_data(literal_data);
    if (!copied_data) {
        baa_free(expr);
        return NULL;
    }
    
    expr->data = copied_data;
    expr->type = baa_get_type_for_literal(copied_data);
    
    return expr;
}

BaaExpr* baa_create_variable_expr(const wchar_t* name, size_t name_length) {
    if (!name) return NULL;
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_VARIABLE);
    if (!expr) return NULL;
    
    BaaVariableExpr* var_expr = (BaaVariableExpr*)baa_malloc(sizeof(BaaVariableExpr));
    if (!var_expr) {
        baa_free(expr);
        return NULL;
    }
    
    // Use our custom string duplication function for proper Unicode handling
    var_expr->name = baa_strndup(name, name_length);
    if (!var_expr->name) {
        baa_free(var_expr);
        baa_free(expr);
        return NULL;
    }
    
    var_expr->name_length = name_length;
    
    expr->data = var_expr;
    // Note: type will be determined during semantic analysis
    
    return expr;
}

BaaExpr* baa_create_binary_expr(BaaOperatorType op, BaaExpr* left, BaaExpr* right) {
    if (!left || !right) return NULL;
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_BINARY);
    if (!expr) return NULL;
    
    BaaBinaryExpr* bin_expr = (BaaBinaryExpr*)baa_malloc(sizeof(BaaBinaryExpr));
    if (!bin_expr) {
        baa_free(expr);
        return NULL;
    }
    
    bin_expr->left = left;
    bin_expr->right = right;
    bin_expr->op = op;
    
    expr->data = bin_expr;
    
    // Type will be determined based on operands and operator during type checking
    // This is simplified here but would use a function like:
    // expr->type = baa_get_binary_op_result_type(left->type, right->type, op);
    
    return expr;
}

BaaExpr* baa_create_unary_expr(BaaOperatorType op, BaaExpr* operand) {
    if (!operand) return NULL;
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_UNARY);
    if (!expr) return NULL;
    
    BaaUnaryExpr* unary_expr = (BaaUnaryExpr*)baa_malloc(sizeof(BaaUnaryExpr));
    if (!unary_expr) {
        baa_free(expr);
        return NULL;
    }
    
    unary_expr->operand = operand;
    unary_expr->op = op;
    
    expr->data = unary_expr;
    
    // Type will be determined based on operand and operator during type checking
    // This is simplified here but would use a function like:
    // expr->type = baa_get_unary_op_result_type(operand->type, op);
    
    return expr;
}

BaaExpr* baa_create_call_expr(BaaExpr* callee, BaaExpr** arguments, size_t argument_count, 
                          wchar_t** named_args, bool is_method_call) {
    if (!callee) return NULL;
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_CALL);
    if (!expr) return NULL;
    
    BaaCallExpr* call_expr = (BaaCallExpr*)baa_malloc(sizeof(BaaCallExpr));
    if (!call_expr) {
        baa_free(expr);
        return NULL;
    }
    
    call_expr->callee = callee;
    call_expr->argument_count = argument_count;
    call_expr->arguments = NULL;
    call_expr->named_args = NULL;
    call_expr->is_method_call = is_method_call;
    
    // Allocate and copy the arguments array if there are any arguments
    if (argument_count > 0) {
        call_expr->arguments = (BaaExpr**)baa_malloc(sizeof(BaaExpr*) * argument_count);
        if (!call_expr->arguments) {
            baa_free(call_expr);
            baa_free(expr);
            return NULL;
        }
        
        for (size_t i = 0; i < argument_count; i++) {
            call_expr->arguments[i] = arguments[i];
        }
        
        // Handle named arguments if provided
        if (named_args) {
            call_expr->named_args = (wchar_t**)baa_malloc(sizeof(wchar_t*) * argument_count);
            if (!call_expr->named_args) {
                baa_free(call_expr->arguments);
                baa_free(call_expr);
                baa_free(expr);
                return NULL;
            }
            
            for (size_t i = 0; i < argument_count; i++) {
                if (named_args[i]) {
                    call_expr->named_args[i] = baa_wcs_dup(named_args[i]);
                    if (!call_expr->named_args[i]) {
                        // Free previously allocated names
                        for (size_t j = 0; j < i; j++) {
                            if (call_expr->named_args[j]) {
                                baa_free(call_expr->named_args[j]);
                            }
                        }
                        baa_free(call_expr->named_args);
                        baa_free(call_expr->arguments);
                        baa_free(call_expr);
                        baa_free(expr);
                        return NULL;
                    }
                } else {
                    call_expr->named_args[i] = NULL; // Positional argument
                }
            }
        }
    }
    
    expr->data = call_expr;
    
    // Type will be determined based on callee during type checking
    // Function will need to be resolved to determine return type
    
    return expr;
}

BaaExpr* baa_create_cast_expr(BaaExpr* operand, BaaType* target_type) {
    if (!operand || !target_type) return NULL;
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_CAST);
    if (!expr) return NULL;
    
    BaaCastExpr* cast_expr = (BaaCastExpr*)baa_malloc(sizeof(BaaCastExpr));
    if (!cast_expr) {
        baa_free(expr);
        return NULL;
    }
    
    cast_expr->operand = operand;
    cast_expr->target_type = target_type;
    
    expr->data = cast_expr;
    expr->type = target_type;  // Type of a cast is the target type
    
    return expr;
}

BaaExpr* baa_create_assign_expr(BaaExpr* target, BaaExpr* value) {
    if (!target || !value) return NULL;
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_ASSIGN);
    if (!expr) return NULL;
    
    BaaAssignExpr* assign_expr = (BaaAssignExpr*)baa_malloc(sizeof(BaaAssignExpr));
    if (!assign_expr) {
        baa_free(expr);
        return NULL;
    }
    
    assign_expr->target = target;
    assign_expr->value = value;
    
    expr->data = assign_expr;
    
    // Type of an assignment expression is the type of the value
    expr->type = value->type;
    
    return expr;
}

BaaExpr* baa_create_array_expr(BaaExpr** elements, size_t element_count, BaaType* element_type) {
    // Validate arguments
    if ((element_count > 0 && !elements) || !element_type) {
        return NULL;
    }
    
    // Validate array elements if provided
    if (element_count > 0 && !baa_validate_array_elements(elements, element_count, element_type)) {
        return NULL;
    }
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_ARRAY);
    if (!expr) return NULL;
    
    BaaArrayExpr* array_expr = (BaaArrayExpr*)baa_malloc(sizeof(BaaArrayExpr));
    if (!array_expr) {
        baa_free(expr);
        return NULL;
    }
    
    array_expr->element_count = element_count;
    array_expr->element_type = element_type;
    array_expr->elements = NULL;
    
    // Copy the elements array if there are any elements
    if (element_count > 0) {
        array_expr->elements = (BaaExpr**)baa_malloc(sizeof(BaaExpr*) * element_count);
        if (!array_expr->elements) {
            baa_free(array_expr);
            baa_free(expr);
            return NULL;
        }
        
        // Copy the elements
        for (size_t i = 0; i < element_count; i++) {
            array_expr->elements[i] = elements[i];
        }
    }
    
    expr->data = array_expr;
    
    // Create an array type for this expression
    expr->type = baa_create_array_type(element_type, element_count);
    
    return expr;
}

BaaExpr* baa_create_index_expr(BaaExpr* array, BaaExpr* index) {
    // Validate arguments
    if (!array || !index) {
        return NULL;
    }
    
    // Validate the array and index expressions
    if (!baa_validate_index_expr(array, index)) {
        return NULL;
    }
    
    BaaExpr* expr = baa_create_expr(BAA_EXPR_INDEX);
    if (!expr) return NULL;
    
    BaaIndexExpr* index_expr = (BaaIndexExpr*)baa_malloc(sizeof(BaaIndexExpr));
    if (!index_expr) {
        baa_free(expr);
        return NULL;
    }
    
    index_expr->array = array;
    index_expr->index = index;
    
    expr->data = index_expr;
    
    // The type of an indexing expression is the element type of the array
    if (array->type && array->type->kind == BAA_TYPE_ARRAY) {
        expr->type = array->type->element_type;
    }
    
    return expr;
}

// Expression type checking and validation
BaaType* baa_get_expr_type(BaaExpr* expr) {
    if (!expr) return NULL;
    
    return expr->type;
}

bool baa_validate_expr_type(BaaExpr* expr, BaaType* expected_type) {
    if (!expr || !expected_type) return false;
    
    // This is a simplified implementation
    // A real implementation would check type compatibility rules
    
    return baa_types_equal(expr->type, expected_type);
}

bool baa_validate_array_elements(BaaExpr** elements, size_t element_count, BaaType* element_type) {
    if (!elements || element_count == 0 || !element_type) return false;
    
    // Check that all elements are compatible with the element type
    for (size_t i = 0; i < element_count; i++) {
        if (!elements[i]) return false;
        
        // Get the element type and validate it
        BaaType* elem_expr_type = baa_get_expr_type(elements[i]);
        if (!elem_expr_type) return false;
        
        // Check type compatibility
        if (!baa_can_convert(elem_expr_type, element_type)) {
            return false;
        }
    }
    
    return true;
}

bool baa_validate_index_expr(BaaExpr* array, BaaExpr* index) {
    if (!array || !index) return false;
    
    // Get the array type
    BaaType* array_type = baa_get_expr_type(array);
    if (!array_type || array_type->kind != BAA_TYPE_ARRAY) {
        return false;
    }
    
    // Get the index type
    BaaType* index_type = baa_get_expr_type(index);
    if (!index_type) return false;
    
    // The index must be an integer type
    return index_type->kind == BAA_TYPE_INT;
}

void baa_free_expr(BaaExpr* expr) {
    if (!expr) return;
    
    // Free the specific expression data based on the kind
    switch (expr->kind) {
        case BAA_EXPR_LITERAL:
            if (expr->data) {
                baa_free_literal_data((BaaLiteralData*)expr->data);
            }
            break;
            
        case BAA_EXPR_VARIABLE:
            if (expr->data) {
                BaaVariableExpr* var_expr = (BaaVariableExpr*)expr->data;
                if (var_expr->name) {
                    baa_free((void*)var_expr->name);
                }
                baa_free(var_expr);
            }
            break;
            
        case BAA_EXPR_BINARY:
            if (expr->data) {
                BaaBinaryExpr* bin_expr = (BaaBinaryExpr*)expr->data;
                if (bin_expr->left) {
                    baa_free_expr(bin_expr->left);
                }
                if (bin_expr->right) {
                    baa_free_expr(bin_expr->right);
                }
                baa_free(bin_expr);
            }
            break;
            
        case BAA_EXPR_UNARY:
            if (expr->data) {
                BaaUnaryExpr* unary_expr = (BaaUnaryExpr*)expr->data;
                if (unary_expr->operand) {
                    baa_free_expr(unary_expr->operand);
                }
                baa_free(unary_expr);
            }
            break;
            
        case BAA_EXPR_CALL:
            if (expr->data) {
                BaaCallExpr* call_expr = (BaaCallExpr*)expr->data;
                if (call_expr->callee) {
                    baa_free_expr(call_expr->callee);
                }
                
                if (call_expr->arguments) {
                    for (size_t i = 0; i < call_expr->argument_count; i++) {
                        if (call_expr->arguments[i]) {
                            baa_free_expr(call_expr->arguments[i]);
                        }
                    }
                    baa_free(call_expr->arguments);
                }
                
                if (call_expr->named_args) {
                    for (size_t i = 0; i < call_expr->argument_count; i++) {
                        if (call_expr->named_args[i]) {
                            baa_free(call_expr->named_args[i]);
                        }
                    }
                    baa_free(call_expr->named_args);
                }
                
                baa_free(call_expr);
            }
            break;
            
        case BAA_EXPR_CAST:
            if (expr->data) {
                BaaCastExpr* cast_expr = (BaaCastExpr*)expr->data;
                if (cast_expr->operand) {
                    baa_free_expr(cast_expr->operand);
                }
                baa_free(cast_expr);
            }
            break;
            
        case BAA_EXPR_ASSIGN:
            if (expr->data) {
                BaaAssignExpr* assign_expr = (BaaAssignExpr*)expr->data;
                if (assign_expr->target) {
                    baa_free_expr(assign_expr->target);
                }
                if (assign_expr->value) {
                    baa_free_expr(assign_expr->value);
                }
                baa_free(assign_expr);
            }
            break;
            
        case BAA_EXPR_ARRAY:
            if (expr->data) {
                BaaArrayExpr* array_expr = (BaaArrayExpr*)expr->data;
                if (array_expr->elements) {
                    for (size_t i = 0; i < array_expr->element_count; i++) {
                        if (array_expr->elements[i]) {
                            baa_free_expr(array_expr->elements[i]);
                        }
                    }
                    baa_free(array_expr->elements);
                }
                baa_free(array_expr);
            }
            break;
            
        case BAA_EXPR_INDEX:
            if (expr->data) {
                BaaIndexExpr* index_expr = (BaaIndexExpr*)expr->data;
                if (index_expr->array) {
                    baa_free_expr(index_expr->array);
                }
                if (index_expr->index) {
                    baa_free_expr(index_expr->index);
                }
                baa_free(index_expr);
            }
            break;
    }
    
    // Free the AST node if one was assigned
    if (expr->ast_node) {
        baa_free_node(expr->ast_node);
    }
    
    // Free the expression itself
    baa_free(expr);
}
