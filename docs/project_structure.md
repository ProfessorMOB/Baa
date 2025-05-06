# هيكل مشروع لغة باء (Baa Project Structure)

## المجلدات الرئيسية (Main Directories)

```
baa/
├── include/                 # ملفات الرأس العامة - Public headers
│   └── baa/
│       ├── ast/           # بنية الشجرة النحوية
│       │   ├── expressions.h  # تعريفات التعبيرات
│       │   ├── statements.h   # تعريفات الجمل
│       │   ├── function.h     # تعريفات الدوال
│       │   └── ast.h         # تعريفات عامة
│       ├── lexer/         # المحلل اللفظي
│       │   ├── lexer.h            # الواجهة الرئيسية للمحلل اللفظي (Public API)
│       │   ├── token_scanners.h   # تعريفات دوال مسح الرموز (Token scanning functions)
│       │   └── lexer_char_utils.h # أدوات مساعدة للتعامل مع الحروف (Character utilities)
│       ├── parser/        # المحلل النحوي
│       │   ├── parser.h      # واجهة المحلل النحوي
│       │   ├── expression_parser.h # محلل التعبيرات
│       │   └── statement_parser.h  # محلل الجمل
│       ├── preprocessor/  # المعالج المسبق
│       │   └── preprocessor.h # واجهة المعالج المسبق
│       ├── operators.h     # العمليات والأسبقية
│       └── types.h         # نظام الأنواع
│       └── compiler.h      # واجهة المترجم الرئيسي
│
├── src/                    # الشيفرة المصدرية - Source code
│   ├── ast/               # تنفيذ الشجرة النحوية
│   │   ├── expressions.c  # تنفيذ تعبيرات الشجرة
│   │   ├── statements.c   # تنفيذ جمل الشجرة
│   │   ├── function.c     # تنفيذ دوال الشجرة
│   │   └── ast.c         # تنفيذ عام للشجرة
│   ├── lexer/             # تنفيذ المحلل اللفظي
│   │   ├── lexer.c              # المنطق الرئيسي للمحلل اللفظي (Core lexer logic)
│   │   ├── token_scanners.c     # تنفيذ دوال مسح الرموز (Implementation of token scanners)
│   │   ├── lexer_char_utils.c   # تنفيذ أدوات الحروف (Implementation of char utilities)
│   │   └── number_parser.c      # محلل الأرقام (لتحويل النص إلى قيمة عددية - Number string to value parser)
│   ├── parser/            # تنفيذ المحلل النحوي
│   │   ├── parser.c      # تنفيذ رئيسي
│   │   ├── expression_parser.c # تنفيذ معالجة التعبيرات
│   │   └── statement_parser.c  # تنفيذ معالجة الجمل
│   ├── preprocessor/      # تنفيذ المعالج المسبق
│   │   └── preprocessor.c  # تنفيذ رئيسي للمعالج
│   ├── operators/         # تنفيذ العمليات
│   └── types/             # تنفيذ نظام الأنواع
│   ├── compiler.c         # منطق الترجمة الرئيسي
│   └── main.c             # نقطة الدخول الرئيسية للتطبيق
│
├── tests/                  # الاختبارات - Tests
│   ├── unit/              # اختبارات الوحدات
│   │   ├── ast/          # اختبارات الشجرة النحوية
│   │   ├── lexer/        # اختبارات المحلل اللفظي
│   │   ├── operators/    # اختبارات العمليات
│   │   └── parser/       # اختبارات المحلل النحوي
│   │
│   └── integration/       # اختبارات التكامل
│       └── examples/      # أمثلة للاختبار
│
├── docs/                   # التوثيق - Documentation
│   ├── PREPROCESSOR_ROADMAP.md # خارطة طريق المعالج المسبق
│   ├── ar/               # التوثيق بالعربية
│   │   ├── grammar.md    # قواعد اللغة
│   │   └── tutorial.md   # دليل التعلم
│   │
│   └── en/               # English documentation
│       ├── grammar.md    # Language grammar
│       └── tutorial.md   # Tutorial
│
├── examples/               # أمثلة - Examples
│   ├── basic/            # أمثلة أساسية
│   └── advanced/         # أمثلة متقدمة
│
└── tools/                 # الأدوات المساعدة - Tools
    ├── formatter/        # منسق الشيفرة
    └── debugger/         # أداة التنقيح
```

## ملفات المشروع الرئيسية (Main Project Files)

```
baa/
├── CMakeLists.txt         # ملف بناء المشروع الرئيسي
├── README.md              # معلومات المشروع
├── CHANGELOG.md           # سجل التغييرات
├── LICENSE                # رخصة المشروع
└── .editorconfig         # إعدادات المحرر
```

## تنظيم الشيفرة (Code Organization)

### المكتبات (Libraries)
- `libbaa_ast`: مكتبة الشجرة النحوية - تتضمن كل أنواع التعبيرات والجمل والوظائف المعززة لمعالم الدوال
- `libbaa_lexer`: مكتبة المحلل اللفظي - تتضمن دعم القيم المنطقية والعمليات المركبة والتعليقات
- `libbaa_preprocessor`: مكتبة المعالج المسبق - تدعم معالجة التوجيهات (#تضمين، #تعريف)
- `libbaa_operators`: مكتبة العمليات - تتضمن الزيادة/النقصان وعمليات التعيين المركبة
- `libbaa_parser`: مكتبة المحلل النحوي - مبنية على طريقة النزول المتكرر وتدعم كل أنواع التعبيرات
- `libbaa_types`: مكتبة الأنواع - تتضمن النوع المنطقي ودعم المصفوفات

### الاختبارات (Tests)
- اختبارات الوحدات لكل مكتبة
- اختبارات التكامل للنظام كامل
- اختبارات الأداء

### التوثيق (Documentation)
- توثيق API بالعربية والإنجليزية
- أمثلة مع الشرح
- دليل المساهمة في المشروع

## نمط التسمية (Naming Conventions)

### الملفات (Files)
- ملفات المصدر: `.c`
- ملفات الرأس: `.h`
- ملفات باء: `.ب`
- ملفات الاختبار: `_test.c`

### التسمية في الشيفرة (Code Naming)
- الدوال: `baa_*`
- الأنواع: `Baa*`
- الثوابت: `BAA_*`
- المتغيرات العامة: `g_*`

## إرشادات التطوير (Development Guidelines)

1. كل تغيير يجب أن يكون له اختبار
2. التوثيق بالعربية والإنجليزية
3. الالتزام بمعايير تنسيق الشيفرة
4. تحديث سجل التغييرات
5. مراجعة الشيفرة قبل الدمج
