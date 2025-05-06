# دعم اللغة العربية (Arabic Language Support)

## Overview

Baa (باء) provides comprehensive support for Arabic programming through:

- Full UTF-8 encoding support
- Right-to-left (RTL) text handling
- Arabic identifiers and keywords
- Bilingual error messages
- Arabic documentation

## Language Features (ميزات اللغة)

### 1. Keywords (الكلمات المفتاحية)

| Arabic | English | Description |
|--------|---------|-------------|
| دالة | function | Function declaration |
| إذا | if | Conditional statement |
| وإلا | else | Alternative branch |
| طالما | while | While loop |
| من_أجل | for | For loop |
| افعل | do | Do-while loop |
| اختر | switch | Switch statement |
| حالة | case | Case label |
| توقف | break | Break statement |
| استمر | continue| Continue statement |
| إرجع | return | Return statement |
| بنية | struct | Structure definition |
| متغير | var | Variable keyword |
| ثابت | const | Constant keyword |
| اتحاد | union | Union definition |

### 2. Types (الأنواع)

| Arabic      | English | Description           | Size    |
|-------------|---------|----------------------|---------|
| عدد_صحيح    | int     | Integer              | 32-bit  |
| عدد_حقيقي   | float   | Floating-point       | 32-bit  |
| حرف        | char    | Character            | 16-bit  | (Lexer uses wchar_t)
| فراغ        | void    | No type             | -       |
| منطقي       | bool    | Boolean              | 8-bit   |
| مصفوفة      | array    | Array type         | -       |
| مؤشر        | pointer  | Pointer type       | -       |
| بنية        | struct   | Structure type     | -       |
| اتحاد       | union    | Union type         | -       |

### 3. Operators (العمليات)

| Category | Arabic | Symbol | Description |
|----------|---------|--------|-------------|
| Arithmetic | جمع | + | Addition |
| | طرح | - | Subtraction |
| | ضرب | * | Multiplication |
| | قسمة | / | Division |
| | باقي | % | Modulo |
| | زيادة | ++ | Increment |
| | إنقاص | -- | Decrement |
| Compound | جمع_وتعيين | += | Add and assign |
| | طرح_وتعيين | -= | Subtract and assign |
| | ضرب_وتعيين | *= | Multiply and assign |
| | قسمة_وتعيين | /= | Divide and assign |
| | باقي_وتعيين | %= | Modulo and assign |
| Comparison | يساوي | == | Equal to |
| | لا_يساوي | != | Not equal to |
| | أكبر_من | > | Greater than |
| | أصغر_من | < | Less than |
| | أكبر_أو_يساوي | >= | Greater than or equal |
| | أصغر_أو_يساوي | <= | Less than or equal |
| Logical | و | && | Logical AND |
| | أو | \|\| | Logical OR |
| | ليس | ! | Logical NOT |

### 4. Boolean Literals (القيم المنطقية)

| Arabic | English | Description |
|--------|---------|-------------|
| صحيح | true | Boolean true value |
| خطأ | false | Boolean false value |

### 5. Numeric Literals (القيم العددية)

Baa supports Arabic in numeric literals extensively:
- **Arabic-Indic Digits (الأرقام الهندية):** Digits `٠` through `٩` (U+0660-U+0669) can be used wherever Western digits (`0`-`9`) are used, for both integers and floats.
  - Examples: `متغير س = ١٢٣.` (s = 123), `متغير ص = ٣٫١٤.` (p = 3.14 using Arabic decimal separator)
- **Arabic Decimal Separator (الفاصلة العشرية العربية):** The character `٫` (U+066B) is recognized as a decimal separator in floating-point numbers, in addition to the period (`.`).
  - Example: `عدد_حقيقي pi = ٣٫١٤١٥٩.`
- **Underscores with Arabic Numerals (الشرطة السفلية مع الأرقام العربية):** Underscores can be used as separators for readability with Arabic-Indic digits as well.
  - Example: `عدد_صحيح كبير = ١_٠٠٠_٠٠٠.` (one million)

### 6. Character Literals (القيم الحرفية)

Character literals are enclosed in single quotes and support basic escape sequences:

```baa
متغير حرف1 : حرف = 'ب'.
متغير حرف2 : حرف = '\t'. // Tab character
```

### 7. Function Parameters (معاملات الدالة)

| Feature | Description |
|---------|-------------|
| Optional Parameters | Parameters with default values |
| Rest Parameters | Variable number of parameters (...args) |
| Named Arguments | Arguments specified by parameter name |
| Method/Function Distinction | Functions that belong to an object/class |

## Code Examples (أمثلة برمجية)

### 1. Hello World

```baa
دالة رئيسية() {
    اطبع("مرحباً بالعالم!").
    إرجع 0.
}
```

### 2. Function with Parameters

```baa
دالة جمع_الأرقام(عدد_صحيح أ، عدد_صحيح ب) {
    إرجع أ + ب.
}
```

### 3. Conditional Statement

```baa
إذا (عمر >= 18) {
    اطبع("بالغ").
} وإلا {
    اطبع("قاصر").
}
```

### 4. Loop Example

```baa
عدد_صحيح مجموع = 0.
من_أجل (عدد_صحيح i = 1; i <= 10; i++) {
    مجموع += i.
}
```

## File Support (دعم الملفات)

### 1. File Extensions

- `.ب` - Primary Arabic source file extension
- `.baa` - Alternative source file extension

### 2. File Encoding

- UTF-16LE encoding for source files (implemented)
- Support for Arabic comments and strings (implemented)
- Enhanced RTL text support (planned)

## Error Messages (رسائل الخطأ)

The compiler will provide error messages in both Arabic and English (planned feature):

```
خطأ: متغير غير معرف 'س'
Error: Undefined variable 's'

خطأ: نوع غير متوافق في التعيين
Error: Incompatible type in assignment
```

## Development Tools (أدوات التطوير)

### 1. Editor Support

- RTL text rendering
- Arabic syntax highlighting
- Auto-completion for Arabic keywords
- Error messages in Arabic

### 2. Debugging

- Arabic variable names in debugger
- Arabic call stack
- Arabic watch expressions

## Best Practices (أفضل الممارسات)

### 1. Naming Conventions

- Use meaningful Arabic names
- Follow Arabic grammar rules
- Be consistent with naming style
- Use underscores for compound names

### 2. Code Style

- Maintain consistent text direction
- Use clear Arabic comments
- Follow standard indentation
- Group related declarations

## Future Enhancements (التحسينات المستقبلية)

1. Enhanced RTL support in editors
2. More Arabic standard library functions
3. Arabic documentation generator
4. Improved error messages in Arabic
5. Arabic code formatting tools

## Version Support (دعم الإصدارات)

- Version 0.1.7: Basic Arabic support
- Version 0.1.8: Enhanced Arabic features
  - Improved error messages
  - Better RTL handling
  - More Arabic keywords
  - Extended documentation
