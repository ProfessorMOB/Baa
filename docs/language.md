# لغة باء - مواصفات اللغة

# B (باء) Language Specification

## 1. Basic Syntax Examples

*(Note: See subsequent sections for detailed definitions)*

### 1.1 Preprocessor Directives

Baa supports a preprocessor step that handles directives starting with `#` before lexical analysis.

*   **Include:** `#تضمين` is used to include the content of other files. - *[Implemented]*
    *   `#تضمين "مسار/ملف/نسبي.ب"` : Includes a file relative to the current file's path.
    *   `#تضمين <مكتبة_قياسية>` : Includes a file found in standard library include paths.

```baa
#تضمين "my_definitions.b" // Include a local file
#تضمين <standard_io>      // Include a standard library
```
*   **Macros:** `#تعريف`, `#الغاء_تعريف` - Basic parameterless define/undef implemented. *[Implemented]*
*   **Conditional Compilation:** `#إذا_عرف`, `#إذا_لم_يعرف`, `#إلا`, `#نهاية_إذا` - Basic support implemented. *[Implemented]*

### 1.2 Statement Termination

Statements are terminated with a dot (`.`) instead of a semicolon.

```baa
اطبع("مرحبا").    // Correct
اطبع("مرحبا");    // Incorrect
```

### 1.3 Function Declaration Example

*(See Section 4.2 for full details)*
Functions are declared using the `دالة` keyword.



```baa
// [Implemented] Main function (entry point)
دالة رئيسية() {
    // Function body
}

// [Implemented] Function with integer parameter and return value
// Note: Return type syntax `-> type` might need verification against parser implementation.
دالة مربع(عدد_صحيح س) -> عدد_صحيح {
    إرجع س * س.
}
```

### 1.4 Variable Declaration Example

*(See Section 4.1 for full details)*
Variables are currently declared using the C-style syntax.

```baa
// [Implemented] Declare an integer named 'counter'
عدد_صحيح counter.

// [Implemented] Declare and initialize a float named 'price'
عدد_حقيقي price = 10.5.
```

### 1.5 Control Flow Example

*(See Section 6 for full details)*
Control structures use Arabic keywords and dot termination.

```baa
// If statement
إذا (س > 0) {
    اطبع("موجب").
} وإلا {
    اطبع("سالب").
}

// While loop
طالما (س < 10) {
    اطبع(س).
    س = س + 1.
}
```

### 1.6 Operator Examples

*(See Section 5 for full details)*

```baa
// Increment and decrement
متغير++.    // Postfix increment
--متغير.    // Prefix decrement

// Compound assignment
متغير += 5.  // Add and assign
```

## 2. Lexical Structure

This section describes the low-level building blocks of Baa programs **after** preprocessing. Baa source files are expected to be encoded in **UTF-16LE**.

### 2.1 Comments

Baa supports standard C/C++ style comments:

*   **Single-line:** Begins with `//` and continues to the end of the line. The lexer skips these. - *[Implemented]*
*   **Multi-line:** Begins with `/*` and ends with `*/`. These comments can span multiple lines. The lexer skips these. - *[Implemented]*
*   **Preprocessor Directives:** Lines starting with `#` (e.g., `#تضمين`, `#تعريف`) are handled entirely by the preprocessor before lexical analysis. They are not treated as comments by the lexer. - *[Implemented for various directives]*

```baa
// هذا تعليق سطر واحد.
/*
  هذا تعليق
  متعدد الأسطر.
*/
// # نمط التعليق القديم. (تمت إزالته، المعالج المسبق يتعامل مع #)
```

### 2.2 Identifiers

Identifiers are names used for variables, functions, types, etc.

*   **Allowed Characters:** Arabic letters (Unicode ranges 0x0600-0x06FF, FB50-FDFF, FE70-FEFF), English letters (`a-z`, `A-Z`), Arabic-Indic digits (`٠-٩`), ASCII digits (`0-9`), and the underscore (`_`).
*   **Starting Character:** Must begin with a letter (Arabic or English) or an underscore.
*   **Case Sensitivity:** Identifiers are case-sensitive.
*   **Reserved Words:** Keywords (see Section 2.3) cannot be used as identifiers.

```baa
// أمثلة صحيحة (Valid Examples)
متغير الإسم_الأول.
متغير _temporaryValue.
متغير قيمة_رقمية1.
متغير القيمة٢.

// أمثلة خاطئة (Invalid Examples)
// متغير 1stValue. // Cannot start with a digit
// متغير قيمة-خاصة. // Hyphen not allowed
// متغير إذا. // Keyword cannot be an identifier
```

### 2.3 Keywords

Keywords are reserved words with special meaning in the Baa language and cannot be used as identifiers.

*(Based on `lexer.h` and `language.md`)*
*   **Declarations:** `دالة`, `ثابت` (`const`), `خارجي` (`extern`) - *[Implemented/Partial]*
    *   *Planned:* `متغير` (`var`), `نوع_مستخدم` (`typedef`), `ثابت` (`static` storage), `حجم` (`sizeof`)
*   **Control Flow:** `إذا`, `وإلا`, `طالما`, `إرجع`, `توقف` (`break`), `أكمل` (`continue`) - *[Implemented]*
    *   *Partial/Planned:* `من_أجل` (`for`), `افعل` (`do`), `اختر` (`switch`), `حالة` (`case`)
*   **Types:** `عدد_صحيح`, `عدد_حقيقي`, `حرف`, `منطقي`, `فراغ` - *[Implemented]*
*   **Boolean Literals:** `صحيح`, `خطأ` - *[Implemented]*

*(Note: Standard library function names like `اطبع` are technically identifiers, not keywords)*

### 2.4 Literals

Literals represent fixed values in the source code.

*   **Integer Literals (`عدد_صحيح`):** Represent whole numbers. The lexer tokenizes these as `BAA_TOKEN_INT_LIT`.
    *   **Decimal:** Sequences of Western digits (`0`-`9`) and/or Arabic-Indic digits (`٠`-`٩` / U+0660-U+0669).
        - Examples: `123`, `٠`, `٤٢`, `1٠2` - *[Implemented]*
    *   **Hexadecimal:** Must start with `0x` or `0X`, followed by hexadecimal digits (`0`-`9`, `a`-`f`, `A`-`F`).
        - Examples: `0xFF`, `0x1a`, `0XDEADBEEF` - *[Implemented]*
    *   **Binary:** Must start with `0b` or `0B`, followed by binary digits (`0` or `1`).
        - Examples: `0b1010`, `0B11001` - *[Implemented]*
    *   **Underscores for Readability:** Single underscores (`_`) can be used as separators within the digits of any integer literal type. They cannot be consecutive, at the very beginning of the digit sequence (immediately after a prefix like `0x_`), or at the end of the number.
        - Examples: `1_000_000`, `0xAB_CD`, `0b10_10`, `١_٢٣٤` - *[Implemented]*
*   **Floating-Point Literals (`عدد_حقيقي`):** Represent numbers with a fractional part or in scientific notation. The lexer tokenizes these as `BAA_TOKEN_FLOAT_LIT`.
    *   **Decimal Representation:** Consist of an integer part, a decimal point, and a fractional part. Digits can be Western or Arabic-Indic.
        - The decimal point can be a period `.` (U+002E) or an Arabic Decimal Separator `٫` (U+066B).
        - Examples: `3.14`, `0.5`, `١٢٫٣٤`, `٠.٥`, `123.0` - *[Implemented]*
        - Note: Literals like `.5` or `123.` are tokenized by the lexer as separate tokens (e.g., `DOT`, `INT_LIT`); their validity as floats is a parser concern.
    *   **Scientific Notation:** Introduced by `e` or `E`, followed by an optional sign (`+` or `-`), and then one or more decimal digits (Western or Arabic-Indic). Can be applied to numbers with or without a decimal point.
        - Examples: `1.23e4`, `5E-2`, `42e+0`, `1e10`, `٣٫١٤E-٠٢` - *[Implemented]*
    *   **Underscores for Readability:** Single underscores (`_`) can be used as separators in the integer, fractional, and exponent parts of float literals, with the same restrictions as for integers.
        - Examples: `1_234.567_890`, `3.141_592e+1_0` - *[Implemented]*
*   **Boolean Literals (`منطقي`):**
    *   `صحيح` (true) - *[Implemented]*
    *   `خطأ` (false) - *[Implemented]*
*   **Character Literals (`حرف`):** Represent single characters enclosed in single quotes (`'`).
    *   `'a'`, `'أ'`, `'#'`, `'١'` - *[Implemented]*
    *   Escape Sequences:
        *   `'\n'` (newline), `'\t'` (tab), `'\\'` (backslash), `'\''` (single quote) - *[Implemented]*
        *   `'\"'` (double quote) - *[Implemented]*
        *   `'\r'` (carriage return), `'\0'` (null char) - *[Implemented]*
        *   `'\uXXXX'` (Unicode escape, where XXXX are four hex digits) - *[Implemented]*
*   **String Literals (`نص` - inferred type):** Represent sequences of characters enclosed in double quotes (`"`). Uses UTF-16LE encoding internally.
    *   `"مرحباً"` - *[Implemented]*
    *   `"Hello, World!"` - *[Implemented]*
    *   Escape Sequences: Similar to characters: `\n`, `\t`, `\"`, `\\`, `\r`, `\0`, `\uXXXX` are implemented. - *[Implemented]*
    *   *Planned:* Multiline/raw strings.

### 2.5 Operators

Operators perform operations on values. See Section 5 for a detailed list and precedence.

### 2.6 Separators / Delimiters

Symbols used to structure code: `( ) { } [ ] , . :`
*   **`.` (Dot):** Terminates statements. - *[Implemented]*

## 3. Types

Baa has a static type system based on C, with Arabic names for built-in types.

### 3.1 Basic Types

| Arabic Name | English Equiv. | Description            | Size   | Status      |
| ----------- | -------------- | ---------------------- | ------ | ----------- |
| `عدد_صحيح`  | `int`          | Signed Integer         | 32-bit | Implemented |
| `عدد_حقيقي` | `float`        | Floating-point         | 32-bit | Implemented |
| `حرف`       | `char`         | Character (`wchar_t`)  | 16-bit | Implemented |
| `منطقي`     | `bool`         | Boolean (`صحيح`/`خطأ`) | 8-bit? | Implemented |
| `فراغ`      | `void`         | Represents no value    | N/A    | Implemented |

*(Internal representation sizes based on `types.c`)*

### 3.2 Derived Types

| Arabic Name | English Equiv. | Description                    | Status  | Notes                                     |
| ----------- | -------------- | ------------------------------ | ------- | ----------------------------------------- |
| `مصفوفة`    | array          | Ordered collection of elements | Partial | Basic AST/Type support exists.            |
| `مؤشر`      | pointer        | Address of a variable          | Planned | Requires memory model & operator support. |
| `بنية`      | struct         | Collection of named members    | Planned |                                           |
| `اتحاد`     | union          | Shared memory for members      | Planned |                                           |

### 3.3 Type Compatibility & Conversion

*(Based on current `types.c` implementation)*

*   **General Principle:** Operations (like assignment, arithmetic) require operands of compatible types.
*   **Implicit Conversions Allowed:**
    *   `عدد_صحيح` -> `عدد_حقيقي` (Integer to Float)
    *   `حرف` -> `عدد_صحيح` (Character to Integer)
*   **Boolean Conversion:** In conditional contexts (like `if`, `while`), integer or float values might be implicitly converted (0 is false, non-zero is true). `منطقي` type is preferred. *(Needs verification)*.
*   **Operators:** Require compatible operands (see Section 5). Results have defined types (e.g., comparison yields `منطقي`).
*   **Explicit Casts:** Syntax and implementation for explicit type casting are *[Planned]*.

*[Status: Type definitions exist. Enforcement via semantic analysis is currently limited.]*

## 4. Declarations

Declarations introduce new names (identifiers) into a scope.

### 4.1 Variable Declarations

Variables store values that can potentially change.

*   **Syntax:** `type identifier ('=' initializer_expression)? '.'` - *[Implemented]*
*   **Initialization:** Optional. If omitted, default value depends on scope (e.g., zero/null for globals/static, potentially uninitialized for locals - *needs clarification*).
*   **Constants:** Use the `ثابت` keyword before the type to declare a constant whose value cannot be changed after initialization. `ثابت type identifier = initializer_expression '.'` - *[Partial - Keyword parsed, semantic enforcement needed]*.

```baa
عدد_صحيح counter.             // Variable, likely default initialized
عدد_حقيقي price = 10.5.        // Variable, initialized
ثابت منطقي FLAG = صحيح.       // Constant, must be initialized
// price = 11.0.               // OK
// FLAG = خطأ.                // Error: Cannot assign to constant
```

*   **Planned:** `متغير` keyword for alternative declaration styles (see Section 1.4 example).

### 4.2 Function Declarations

Functions define reusable blocks of code.

*   **Syntax:** `دالة identifier '(' parameter_list? ')' ('->' return_type)? '{' statement* '}'` - *[Implemented]*
*   **Parameters (`parameter_list`):** Comma-separated list of `type identifier`. `( )` for no parameters.
*   **Return Type:** Specified after `->`. If omitted, defaults to `فراغ` (void). Functions returning non-void *must* use `إرجع` with a compatible value on all execution paths. *(Flow analysis check is partial)*.
*   **Entry Point:** The program must contain a function named `رئيسية` with no parameters and typically returning `عدد_صحيح`. `دالة رئيسية() -> عدد_صحيح { ... إرجع 0. }`

```baa
// Function with no parameters, implicit void return
دالة print_hello() {
    اطبع("مرحبا!").
}

// Function with parameters and explicit return type
دالة add(عدد_صحيح a, عدد_صحيح b) -> عدد_صحيح {
    إرجع a + b.
}
```

*   **Planned/Partial:** Optional parameters, rest parameters, named arguments (AST support exists, parsing/analysis status unclear).

### 4.3 Other Declarations (Planned)

Support for `typedef` (`نوع_مستخدم`), `struct` (`بنية`), `union` (`اتحاد`), pointers (`مؤشر`), and `enum` is planned for closer C compatibility.

## 5. Operator Precedence and Associativity

The following table summarizes operator precedence (highest first) and associativity. This determines the order of evaluation in complex expressions (e.g., `a + b * c` is evaluated as `a + (b * c)` because `*` has higher precedence than `+`).

| Precedence Level  | Operators          | Associativity | Notes                    | Status                  |
| ----------------- | ------------------ | ------------- | ------------------------ | ----------------------- |
| Highest (Primary) | `()` `[]` `. `     | Left-to-right | Grouping, Index, Member  | Implemented/Partial(.)  |
| (Postfix)         | `++` `--`          | Left-to-right | Postfix Inc/Dec          | Implemented             |
| (Unary)           | `++` `--`          | Right-to-left | Prefix Inc/Dec           | Implemented             |
|                   | `+` `-` (unary)    | Right-to-left | Unary Plus/Minus         | Implemented             |
|                   | `!` `~`            | Right-to-left | Logical NOT, Bitwise NOT | Implemented/Partial (~) |
| (Factor)          | `*` `/` `%`        | Left-to-right | Multiplicative           | Implemented             |
| (Term)            | `+` `-` (binary)   | Left-to-right | Additive                 | Implemented             |
| (Shift)           | `<<` `>>`          | Left-to-right | Bitwise Shift            | Partial                 |
| (Comparison)      | `<` `>` `<=` `>=`  | Left-to-right | Relational               | Implemented             |
| (Equality)        | `==` `!=`          | Left-to-right | Equality                 | Implemented             |
| (Bitwise AND)     | `&`                | Left-to-right | Bitwise AND              | Partial                 |
| (Bitwise XOR)     | `^`                | Left-to-right | Bitwise XOR              | Partial                 |
| (Bitwise OR)      | `                  | `             | Left-to-right            | Bitwise OR              | Partial    |
| (Logical AND)     | `&&`               | Left-to-right | Logical AND              | Implemented             |
| (Logical OR)      | `                  |               | `                        | Left-to-right           | Logical OR | Implemented |
| Lowest(Assign)    | `=` `+=` `-=` etc. | Right-to-left | Assignment               | Implemented             |

*(Status indicates parsing support based on `expression_parser.c` and `operators.h`. Semantic validation status for operators varies - see Section 3.3 and `c_comparison.md`)*.

*(Note: Member access `.` and pointer dereference `*`/address-of `&` depend on planned struct/pointer features).*

## 6. Statements

Statements are the units of execution. They are terminated by a dot (`.`).

### 6.1 Expression Statements

An expression followed by a dot is a statement. The expression is evaluated for its side effects.
```baa
counter++.             // Increment counter
add(5, 3).             // Call function (result discarded)
x = y + 1.             // Assignment
```

### 6.2 Block Statements (Compound Statements)

A sequence of zero or more statements enclosed in curly braces `{}`. Defines a block scope.
```baa
{
    عدد_صحيح temp = x.
    x = y.
    y = temp.
}
```

### 6.3 Conditional Statements (`if`/`else`)

Executes statements based on a condition.
*   **Syntax:** `إذا '(' expression ')' statement_or_block ( وإلا statement_or_block )?` - *[Implemented]*
*   **Condition:** The `expression` is evaluated. It should result in a `منطقي` value (or be convertible to one).
```baa
إذا (x > 0) {
    اطبع("Positive").
}

إذا (y == 0)
    اطبع("Zero").
وإلا {
    اطبع("Non-zero").
}
```

### 6.4 Iteration Statements (Loops)

#### 6.4.1 `while` Loop

Repeats a statement/block as long as a condition is true.
*   **Syntax:** `طالما '(' expression ')' statement_or_block` - *[Implemented]*
*   **Condition:** Evaluated *before* each iteration.
```baa
عدد_صحيح i = 0.
طالما (i < 5) {
    اطبع(i).
    i++.
}
```

#### 6.4.2 `for` Loop

Provides initialization, condition, and post-iteration expressions for controlled looping.
*   **Syntax:** `من_أجل '(' init_expr? ';' condition_expr? ';' incr_expr? ')' statement_or_block` - *[Implemented - Note: Uses semicolons ';', not dots '.' as separators inside parentheses]*.
*   **Components:**
    *   `init_expr`: Evaluated once before the loop.
    *   `condition_expr`: Evaluated before each iteration. Loop continues if true.
    *   `incr_expr`: Evaluated after each iteration.
```baa
// Example - Note the use of semicolons inside the parentheses
من_أجل (عدد_صحيح i = 0; i < 10; i++) {
    اطبع(i).
}
```

#### 6.4.3 `do-while` Loop (Planned)

Repeats a statement/block, evaluating the condition *after* the first iteration.
*   **Syntax:** `افعل statement_or_block طالما '(' expression ')' '.'` - *[Partial - Keyword exists, AST/Parsing status unclear]*.

### 6.5 Jump Statements

Transfer control flow unconditionally.

#### 6.5.1 `return` Statement

Exits the current function, optionally returning a value.
*   **Syntax:** `إرجع (expression)? '.'` - *[Implemented]*
*   **Value:** If the function returns non-void, the `expression` type must be compatible with the declared return type.

#### 6.5.2 `break` Statement

Exits the innermost enclosing loop (`while`, `for`, `do`) or `switch` statement.
*   **Syntax:** `توقف '.'` - *[Implemented]*

#### 6.5.3 `continue` Statement

Skips the remainder of the current iteration of the innermost enclosing loop (`while`, `for`, `do`) and proceeds to the next iteration (evaluating the condition/increment).
*   **Syntax:** `أكمل '.'` - *[Implemented]*

#### 6.5.4 `goto` Statement (Planned)

Unconditional jump to a labeled statement within the same function.
*   **Syntax:** `اذهب identifier '.'` and `identifier ':' statement` - *[Planned]*.

### 6.6 `switch` Statement

Selects a block of code to execute based on the value of an expression.
*   **Syntax:** `اختر '(' expression ')' '{' case_group* '}'` - *[Implemented]*.
*   **`case_group`:** Consists of one or more `حالة constant_expression ':'` labels or a `افتراضي ':'` (default) label, followed by statements. Execution falls through cases unless `توقف` is used.
```baa
// Example - Syntax/Semantics need verification
اختر (day) {
    حالة 1:
        اطبع("Monday").
        توقف.
    حالة 2:
        اطبع("Tuesday").
        توقف.
    افتراضي:
        اطبع("Other day").
}

```

## 7. Scope Rules

Baa uses lexical scoping, similar to C. The scope determines the visibility of identifiers (variables, functions, etc.).

*   **Global Scope:** Declarations made outside any function are generally visible from their point of declaration to the end of the source file. *[Note: Visibility across multiple files via `خارجي` is planned but likely requires linker/codegen support].*
*   **Function Scope:** Parameters and variables declared directly inside a function body (outside any nested blocks) are visible from their point of declaration to the end of the function.
*   **Block Scope:** Variables declared inside a block (code enclosed in `{...}`, e.g., within an `if`, `while`, or compound statement) are visible only from their point of declaration to the end of that block (including nested blocks).
*   **Shadowing:** A declaration in an inner scope can *shadow* (hide) a declaration with the same name from an outer scope. The inner declaration takes precedence within its scope.

```baa
عدد_صحيح global_var = 10. // Global scope

دالة test(عدد_صحيح param) { // Function scope starts
    عدد_صحيح func_var = param + global_var. // Can access global and param

    إذا (func_var > 20) { // Block scope starts
        عدد_صحيح block_var = 5.
        عدد_صحيح param = 0. // Shadows the function parameter 'param'
        اطبع(block_var + param). // Accesses block_var and the *shadowing* param (0)
    } // Block scope ends

    // اطبع(block_var). // Error: block_var not visible here

    اطبع(param). // Accesses the *original* function parameter 'param'
} // Function scope ends

```
*[Status: Basic block scoping is likely handled by the parser structure. Full enforcement and symbol table management via `scope.h` happens during semantic analysis, which is currently underdeveloped.]*

## 8. Program Structure

### 8.1 Basic Program Structure

Every executable program must have a main function (`دالة رئيسية`) which serves as the entry point.

```baa
// مثال برنامج بسيط بلغة باء
دالة رئيسية() -> عدد_صحيح {
    اطبع("مرحباً بالعالم!\").
    إرجع 0. // Conventionally return 0 on success
}
```

### 8.2 File Extension

Baa source code files typically use the `.ب` extension.

```
برنامج.ب
my_module.ب
```

## 9. Naming Conventions

(These are conventions, not strictly enforced by the compiler beyond identifier rules)

*   Use clear and descriptive names in Arabic or English.
*   Follow consistent casing (e.g., `snake_case` or `camelCase`).
*   Avoid overly short or ambiguous names.

```
```
