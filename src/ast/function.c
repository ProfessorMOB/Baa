#include "baa/ast/ast.h"
#include "baa/ast/expressions.h" // Added for baa_free_expr
#include "baa/ast/statements.h"  // Added for baa_free_block
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>

// Create a new function
BaaFunction* baa_create_function(const wchar_t* name, size_t name_length) {
    if (!name) return NULL;

    BaaFunction* function = (BaaFunction*)baa_malloc(sizeof(BaaFunction));
    if (!function) return NULL;

    // Create a copy of the name with proper allocation
    function->name = baa_strndup(name, name_length); // Use correct function name from utils.h
    if (!function->name) {
        baa_free(function);
        return NULL;
    }

    function->name_length = name_length;
    function->return_type = NULL;
    function->parameters = NULL;
    function->parameter_count = 0;
    function->parameter_capacity = 0;
    function->body = NULL;
    function->is_variadic = false;
    function->is_extern = false;
    function->is_method = false;
    function->module_name = NULL;
    function->ast_node = NULL;
    function->documentation = NULL;

    return function;
}

// Create a regular parameter
BaaParameter* baa_create_parameter(const wchar_t* name, size_t name_length,
                                  BaaType* type, bool is_mutable) {
    if (!name || !type) return NULL;

    BaaParameter* parameter = (BaaParameter*)baa_malloc(sizeof(BaaParameter));
    if (!parameter) return NULL;

    // Create a copy of the name with proper allocation
    parameter->name = baa_strndup(name, name_length); // Use correct function name from utils.h
    if (!parameter->name) {
        baa_free(parameter);
        return NULL;
    }

    parameter->name_length = name_length;
    parameter->type = type;
    parameter->is_mutable = is_mutable;
    parameter->is_optional = false;
    parameter->default_value = NULL;
    parameter->is_rest = false;

    return parameter;
}

// Create an optional parameter with a default value
BaaParameter* baa_create_optional_parameter(const wchar_t* name, size_t name_length,
                                           BaaType* type, bool is_mutable,
                                           BaaExpr* default_value) {
    BaaParameter* parameter = baa_create_parameter(name, name_length, type, is_mutable);
    if (!parameter) return NULL;

    parameter->is_optional = true;
    parameter->default_value = default_value;

    return parameter;
}

// Create a rest parameter (e.g., ...args)
BaaParameter* baa_create_rest_parameter(const wchar_t* name, size_t name_length,
                                       BaaType* element_type, bool is_mutable) {
    BaaParameter* parameter = baa_create_parameter(name, name_length, element_type, is_mutable);
    if (!parameter) return NULL;

    parameter->is_rest = true;

    return parameter;
}

// Add a parameter to a function
bool baa_add_parameter_to_function(BaaFunction* function, BaaParameter* parameter) {
    if (!function || !parameter) return false;

    // Check if we need to allocate or resize the parameters array (array of POINTERS)
    if (function->parameter_count >= function->parameter_capacity) {
        size_t new_capacity = function->parameter_capacity == 0 ? 4 : function->parameter_capacity * 2;
        // Reallocate for an array of BaaParameter POINTERS
        BaaParameter** new_parameters = (BaaParameter**)baa_realloc(
            function->parameters,
            new_capacity * sizeof(BaaParameter*) // Size of pointer
        );

        if (!new_parameters) return false;

        function->parameters = new_parameters;
        function->parameter_capacity = new_capacity;
    }

    // Add the parameter POINTER to the function's array
    function->parameters[function->parameter_count] = parameter; // Assign the pointer directly (Corrected)
    function->parameter_count++;

    // Do NOT free the parameter here, the function now owns the pointer.
    // baa_free(parameter);

    return true;
}

// Validate function signature
bool baa_validate_function_signature(BaaFunction* function) {
    if (!function) return false;

    // Check if return type is specified
    if (!function->return_type) return false;

    // Check parameter ordering:
    // 1. Regular parameters must come first
    // 2. Optional parameters must come after regular parameters
    // 3. Rest parameter must be last, if present

    bool found_optional = false;
    bool found_rest = false;

    for (size_t i = 0; i < function->parameter_count; i++) {
        // Get the pointer from the array
        BaaParameter* param = function->parameters[i];

        // Rest parameter found
        if (param->is_rest) {
            // Can only have one rest parameter, and it must be last
            if (found_rest || i != function->parameter_count - 1) {
                return false;
            }
            found_rest = true;
        }
        // Optional parameter found
        else if (param->is_optional) {
            found_optional = true;
        }
        // Regular parameter found after optional parameter
        else if (found_optional) {
            // Regular parameters cannot come after optional ones
            return false;
        }
    }

    return true;
}

// Free a parameter's resources
void baa_free_parameter(BaaParameter* parameter) {
    if (!parameter) return;

    if (parameter->name) {
        baa_free(parameter->name);
    }

    // Note: We don't free parameter->type since type ownership is managed elsewhere

    // Free default value expression if present
    if (parameter->default_value) {
        baa_free_expr(parameter->default_value);
    }
}

// Free all function resources
void baa_free_function(BaaFunction* function) {
    if (!function) return;

    if (function->name) {
        baa_free(function->name);
    }

    if (function->parameters) {
        for (size_t i = 0; i < function->parameter_count; i++) {
            // Free the parameter pointed to by the array element
            baa_free_parameter(function->parameters[i]);
        }
        // Free the array of pointers itself
        baa_free(function->parameters);
    }

    if (function->body) {
        baa_free_block(function->body);
    }

    if (function->module_name) {
        baa_free(function->module_name);
    }

    if (function->documentation) {
        baa_free(function->documentation);
    }

    // Note: We don't free function->ast_node since ast node ownership is managed elsewhere
    // Note: We don't free function->return_type since type ownership is managed elsewhere

    baa_free(function);
}
