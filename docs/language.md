# B (باء) Programming Language Specification

## Overview

B (باء) is a programming language designed to support Arabic syntax while maintaining K&R C compatibility. It allows developers to write code using Arabic keywords and identifiers while following established C programming patterns.

## Language Elements

### Basic Types
| Arabic      | English | Description           | Size    |
|-------------|---------|----------------------|---------|
| عدد_صحيح    | int     | Integer              | 32-bit  |
| عدد_حقيقي   | float   | Floating-point       | 32-bit  |
| محرف        | char    | Character            | 16-bit  |
| فراغ        | void    | No type             | -       |

### Derived Types
| Arabic      | English  | Description         | Example                    |
|-------------|----------|---------------------|----------------------------|
| مصفوفة      | array    | Array type         | عدد_صحيح مصفوفة[10]       |
| مؤشر        | pointer  | Pointer type       | عدد_صحيح* مؤشر            |
| بنية        | struct   | Structure type     | بنية نقطة { عدد_صحيح س؛ }  |
| اتحاد       | union    | Union type         | اتحاد متغير { عدد_صحيح أ؛ }|

### Keywords

#### Control Flow
| Arabic      | English  | Description           |
|-------------|----------|-----------------------|
| إذا         | if       | Conditional statement |
| وإلا        | else     | Else clause          |
| طالما       | while    | While loop           |
| من_أجل      | for      | For loop             |
| افعل        | do       | Do-while loop        |
| اختر        | switch   | Switch statement     |
| حالة        | case     | Case label           |
| توقف        | break    | Break statement      |
| استمر       | continue | Continue statement   |
| إرجع        | return   | Return statement     |

#### Declarations
| Arabic      | English  | Description           |
|-------------|----------|-----------------------|
| دالة        | function | Function declaration  |
| ثابت        | const    | Constant declaration  |
| خارجي       | extern   | External declaration  |
| ثابت        | static   | Static declaration    |
| نوع_مستخدم  | typedef  | Type definition      |
| حجم         | sizeof   | Size operator        |

### Operators

#### Arithmetic Operators
```
+    Addition
-    Subtraction
*    Multiplication
/    Division
%    Modulo
++   Increment
--   Decrement
```

#### Comparison Operators
```
==   Equal to
!=   Not equal to
<    Less than
>    Greater than
<=   Less than or equal
>=   Greater than or equal
```

#### Logical Operators
```
&&   Logical AND
||   Logical OR
!    Logical NOT
```

#### Bitwise Operators
```
&    Bitwise AND
|    Bitwise OR
^    Bitwise XOR
<<   Left shift
>>   Right shift
~    Bitwise NOT
```

#### Assignment Operators
```
=    Simple assignment
+=   Add and assign
-=   Subtract and assign
*=   Multiply and assign
/=   Divide and assign
%=   Modulo and assign
&=   Bitwise AND and assign
|=   Bitwise OR and assign
^=   Bitwise XOR and assign
<<=  Left shift and assign
>>=  Right shift and assign
```

### Function Declaration
```c
دالة اسم_الدالة(المعاملات) {
    // جسم الدالة
    إرجع القيمة؛
}
```

### Variable Declaration
```c
عدد_صحيح متغير؛             // Integer variable
عدد_حقيقي* مؤشر؛           // Float pointer
محرف مصفوفة[50]؛          // Character array
```

### Structure Definition
```c
بنية نقطة {
    عدد_صحيح س؛
    عدد_صحيح ص؛
}؛

بنية نقطة نقطة1؛         // Structure variable
```

### Control Flow Examples

#### If Statement
```c
إذا (شرط) {
    // عبارات
} وإلا {
    // عبارات
}
```

#### While Loop
```c
طالما (شرط) {
    // عبارات
}
```

#### For Loop
```c
من_أجل (عدد_صحيح i = 0؛ i < 10؛ i++) {
    // عبارات
}
```

#### Switch Statement
```c
اختر (متغير) {
    حالة 1:
        // عبارات
        توقف؛
    حالة 2:
        // عبارات
        توقف؛
    تلقائي:
        // عبارات
}
```

### Preprocessor Directives
```c
تضمين# "ملف.ه"           // #include
تعريف# ثابت 100         // #define
إذا_عرف# اسم            // #ifdef
نهاية_إذا#              // #endif
```

## Memory Model

### Storage Classes
- تلقائي (auto): Default for local variables
- ثابت (static): Persistent storage
- خارجي (extern): External linkage
- سجل (register): Hint for register storage

### Scope Rules
1. Block scope
2. File scope
3. Function scope
4. Prototype scope

### Type Qualifiers
- ثابت (const): Value cannot be modified
- متطاير (volatile): Value may change externally
- مقيد (restrict): Pointer optimization hint

## Standard Library

### Input/Output Functions
```c
اطبع(نص)؛              // printf equivalent
اقرأ(متغير)؛           // scanf equivalent
افتح_ملف(اسم)؛         // fopen equivalent
اغلق_ملف(ملف)؛         // fclose equivalent
```

### String Functions
```c
نسخ_نص(هدف، مصدر)؛     // strcpy equivalent
طول_نص(نص)؛           // strlen equivalent
مقارنة_نص(نص1، نص2)؛   // strcmp equivalent
```

### Memory Functions
```c
حجز_ذاكرة(حجم)؛        // malloc equivalent
حرر_ذاكرة(مؤشر)؛       // free equivalent
نسخ_ذاكرة(هدف، مصدر)؛  // memcpy equivalent
```

## Implementation Notes

### File Format
- UTF-16LE encoding required
- `.baa` file extension
- Optional BOM marker

### Compilation Process
1. Lexical analysis
2. Preprocessing
3. Parsing
4. Type checking
5. Code generation
6. Optimization
7. Linking

### Error Handling
- Compile-time error messages in Arabic
- Runtime error messages in Arabic
- Warning system for potential issues
- Error recovery mechanisms

## Future Extensions

### Planned Features
1. Enhanced type safety
2. Better error messages
3. Modern memory management
4. Development tools integration
