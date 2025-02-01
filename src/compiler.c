#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int compile_baa_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        printf("Processing line: %s", line);
    }

    fclose(file);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename.ب>\n", argv[0]);
        return 1;
    }

    return compile_baa_file(argv[1]);
}
