#ifndef BAA_DIAGNOSTICS_H
#define BAA_DIAGNOSTICS_H

#include "baa/ast/ast.h"
#include <stdbool.h>
#include <wchar.h>

// Diagnostic severity levels
typedef enum {
    BAA_DIAG_ERROR,    // Fatal error that prevents compilation
    BAA_DIAG_WARNING,  // Warning that doesn't prevent compilation
    BAA_DIAG_NOTE,     // Informational note
    BAA_DIAG_HINT      // Suggestion for fixing an issue
} BaaDiagnosticSeverity;

// Diagnostic categories
typedef enum {
    BAA_DIAG_SYNTAX,      // Syntax errors
    BAA_DIAG_TYPE,        // Type errors
    BAA_DIAG_SEMANTIC,    // Semantic errors
    BAA_DIAG_FLOW,        // Control flow errors
    BAA_DIAG_MEMORY,      // Memory-related errors
    BAA_DIAG_SYSTEM       // System/IO errors
} BaaDiagnosticCategory;

// Diagnostic message
typedef struct {
    BaaDiagnosticSeverity severity;
    BaaDiagnosticCategory category;
    BaaSourceLocation location;
    const wchar_t* message;
    const wchar_t* source_line;
    size_t column_start;
    size_t column_end;
    const wchar_t* fix_hint;
} BaaDiagnostic;

// Diagnostic context
typedef struct {
    BaaDiagnostic** diagnostics;
    size_t count;
    size_t capacity;
    bool had_error;
    const wchar_t* source_file;
} BaaDiagnosticContext;

// Context management
BaaDiagnosticContext* baa_create_diagnostic_context(const wchar_t* source_file);
void baa_free_diagnostic_context(BaaDiagnosticContext* context);

// Diagnostic reporting
void baa_report_diagnostic(BaaDiagnosticContext* context,
                         BaaDiagnosticSeverity severity,
                         BaaDiagnosticCategory category,
                         BaaSourceLocation location,
                         const wchar_t* message,
                         const wchar_t* fix_hint);

void baa_report_error(BaaDiagnosticContext* context,
                    BaaDiagnosticCategory category,
                    BaaSourceLocation location,
                    const wchar_t* message);

void baa_report_warning(BaaDiagnosticContext* context,
                      BaaDiagnosticCategory category,
                      BaaSourceLocation location,
                      const wchar_t* message);

// Diagnostic formatting
void baa_format_diagnostic(const BaaDiagnostic* diagnostic, wchar_t* buffer, size_t buffer_size);
void baa_print_diagnostic(const BaaDiagnostic* diagnostic);
void baa_print_all_diagnostics(const BaaDiagnosticContext* context);

// Source code helpers
const wchar_t* baa_get_source_line(const wchar_t* source_file, size_t line_number);
void baa_print_source_snippet(const wchar_t* source_line, size_t column_start, size_t column_end);

#endif /* BAA_DIAGNOSTICS_H */
