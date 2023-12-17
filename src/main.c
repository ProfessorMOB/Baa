#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

long file_size(FILE *file)
{
    if (!file)
    {
        /* code */
        return 0;
    }
    fpos_t orginal = 0;
    if (fgetpos(file, &orginal) != 0)
    {
        /* code */
        printf("لا يمكن الوصول لموقع الملف : %! \n", errno) return 0;
    }
    //
    fseek(file, 0, SEEK_END);
    long out = ftell(file);
    if (fsetpos(file, &orginal) != 0)
    {
        /* code */
        printf("لا يمكن الوصول لموقع الملف : %! \n", errno) return 0;
    }
    return out;
}
/// @brief Gets File Content
/// @param path
/// @return File Content In Char
char *File_Content(char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        printf("لا يمكن قراءة الملف في المسار %s", path);
        return NULL;
    }
    fpos_t size = file_size(file);
}
/// @brief Prints Usage Of Arguments
/// @param argv Argument Char
void print_argUsage(char **argv)
{
    printf("الإستخدام: %s <الملف_المراد_تجميعه>\n", argv[0]);
}
/// @brief The Main Method - or Starting Method
/// @param argc Numbers Of Arguments
/// @param argv Argument Char
/// @return 0 for Success Or 1 for Failure
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        print_argUsage(argv);
        exit(0);
    }

    return 0;
}