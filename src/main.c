#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * \brief Get the size of a file.
 *
 * This function calculates the size of the given file by seeking to the end
 * and using ftell to determine the position.
 *
 * \param file A pointer to the FILE object.
 * \return The size of the file in bytes, or 0 if the file is NULL or an error occurs.
 */
long file_size(FILE *file) {
    if (!file) {
        return 0;
    }
    fpos_t orginal = 0;
    if (fgetpos(file, &orginal) != 0) {
        printf("fgetpos() فشلت دالة: %i \n", errno);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long out = ftell(file);
    if (fsetpos(file, &orginal) != 0) {
        printf("fsetpos() فشلت دالة: %i \n", errno);
    }
    return out;
}

/**
 * \brief Read the content of a file.
 *
 * This function opens a file in read mode and reads its content into a wide character string.
 *
 * \param path The path to the file.
 * \return A pointer to the wide character string containing the file content, or NULL if the file cannot be opened.
 */
wchar_t *file_content(wchar_t *path) {
    FILE *file = _wfopen(path, L"r");
    if (!file) {
        printf("لا يمكن فتح الملف\n");
        return NULL;
    }

    long size = file_size(file);
    wchar_t *contents = malloc(size + 1);
    wchar_t *write_it = contents;
    size_t bytes_read = 0;
    while (bytes_read < size) {
        bytes_read += fread(write_it, /*sizeof(wchar_t)*/ 1, size - bytes_read, file);
        write_it += bytes_read;
    }

    if (bytes_read != size) {
        printf("فشلت عملية القراءة\n");
        free(contents);
        return NULL;
    }
    contents[size] = L'\0';
    return contents;
}

/**
 * \brief Print usage information.
 *
 * This function prints the correct usage of the program to the console.
 *
 * \param argv The argument vector containing the program name and arguments.
 */
void print_usage(wchar_t **argv) {
    printf("الاستخدام خاطئ\n");
    printf("الرجاء استخدام الأمر بالشكل الصحيح\n");
    printf("%s \n<المسار_إلي_الملف_المراد_تجميعه>", argv[0]);
}

/**
 * \brief The main entry point of the program.
 *
 * This function checks the number of command-line arguments and prints usage information if the arguments are insufficient.
 *
 * \param argc The argument count.
 * \param argv The argument vector.
 * \return An integer representing the exit status of the program.
 */
int main(int argc, wchar_t **argv) {
    if (argc < 2) {
        print_usage(argv);
        exit(0);
    }
    wchar_t *path = argv[1];
    wchar_t *content = file_content(path);
    printf("content of %s:\n---\n\"%s\"\n---\n", path, content);
    free(content);
    return 0;
}