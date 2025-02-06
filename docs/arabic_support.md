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
| إرجع | return | Return statement |
| بنية | struct | Structure definition |
| اتحاد | union | Union definition |

### 2. Types (الأنواع)
| Arabic | C Type | Size | Description |
|--------|---------|------|-------------|
| عدد_صحيح | int | 32-bit | Integer type |
| عدد_حقيقي | float | 32-bit | Floating-point type |
| حرف | char | 8-bit | Character type (UTF-8) |
| فراغ | void | - | No type |
| مصفوفة | array | varies | Array type |
| مؤشر | pointer | varies | Pointer type |

### 3. Operators (العمليات)
| Category | Arabic | Symbol | Description |
|----------|---------|--------|-------------|
| Arithmetic | جمع | + | Addition |
| | طرح | - | Subtraction |
| | ضرب | * | Multiplication |
| | قسمة | / | Division |
| | باقي | % | Modulo |
| Comparison | يساوي | == | Equal to |
| | لا_يساوي | != | Not equal to |
| | أكبر_من | > | Greater than |
| | أصغر_من | < | Less than |
| | أكبر_أو_يساوي | >= | Greater than or equal |
| | أصغر_أو_يساوي | <= | Less than or equal |
| Logical | و | && | Logical AND |
| | أو | \|\| | Logical OR |
| | ليس | ! | Logical NOT |

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
- UTF-8 encoding for source files
- Support for Arabic comments and strings
- Proper handling of RTL text

## Error Messages (رسائل الخطأ)
The compiler provides error messages in both Arabic and English:

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
