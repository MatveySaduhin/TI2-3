#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_TABLE_SIZE 4096 // Максимальный размер таблицы
#define BYTE_SIZE 8         // Размер байта

// Структура для хранения таблицы строк
typedef struct {
    char *entry;
} Dictionary;

// Инициализация таблицы строк
void initializeTable(Dictionary *table, int *nextCode) {
    for (int i = 0; i < 256; i++) {
        table[i].entry = (char *)malloc(2);
        table[i].entry[0] = (char)i;
        table[i].entry[1] = '\0';
    }
    *nextCode = 258; // ClearCode = 256, CodeEndOfInformation = 257
}

// Найти строку в таблице
int findInTable(Dictionary *table, int tableSize, const char *str) {
    for (int i = 0; i < tableSize; i++) {
        if (table[i].entry && strcmp(table[i].entry, str) == 0) {
            return i;
        }
    }
    return -1;
}

// Добавить новую строку в таблицу
void addToTable(Dictionary *table, int *nextCode, const char *str) {
    if (*nextCode < MAX_TABLE_SIZE) {
        table[*nextCode].entry = strdup(str);
        (*nextCode)++;
    }
}

// Сжать данные
void compress(const char *input, const char *output) {
    FILE *in = fopen(input, "rb");
    FILE *out = fopen(output, "wb");

    if (!in || !out) {
        fprintf(stderr, "Error opening files.\n");
        exit(EXIT_FAILURE);
    }

    Dictionary table[MAX_TABLE_SIZE];
    int nextCode;
    initializeTable(table, &nextCode);

    char currentStr[256] = "";
    char buffer[2] = "";
    int code;

    fputc(256, out); // Записываем ClearCode

    while (fread(buffer, 1, 1, in) > 0) {
        strcat(currentStr, buffer);

        if (findInTable(table, nextCode, currentStr) == -1) {
            code = findInTable(table, nextCode, currentStr - strlen(buffer));
            fwrite(&code, sizeof(uint16_t), 1, out);
            addToTable(table, &nextCode, currentStr);
            strcpy(currentStr, buffer);
        }
    }

    // Последний код
    if (strlen(currentStr) > 0) {
        code = findInTable(table, nextCode, currentStr);
        fwrite(&code, sizeof(uint16_t), 1, out);
    }

    fputc(257, out); // Записываем CodeEndOfInformation

    fclose(in);
    fclose(out);
}

// Разархивация данных
void decompress(const char *input, const char *output) {
    FILE *in = fopen(input, "rb");
    FILE *out = fopen(output, "wb");

    if (!in || !out) {
        fprintf(stderr, "Error opening files.\n");
        exit(EXIT_FAILURE);
    }

    Dictionary table[MAX_TABLE_SIZE];
    int nextCode;
    initializeTable(table, &nextCode);

    int oldCode, newCode;
    char *str;

    fread(&oldCode, sizeof(uint16_t), 1, in);
    if (oldCode == 256) { // ClearCode
        initializeTable(table, &nextCode);
        fread(&oldCode, sizeof(uint16_t), 1, in);
    }

    str = table[oldCode].entry;
    fwrite(str, 1, strlen(str), out);

    while (fread(&newCode, sizeof(uint16_t), 1, in) > 0) {
        if (newCode == 257) { // CodeEndOfInformation
            break;
        }

        if (newCode < nextCode) {
            str = table[newCode].entry;
        } else {
            str = table[oldCode].entry;
            char temp[256];
            strcpy(temp, str);
            strcat(temp, str);
            str = strdup(temp);
        }

        fwrite(str, 1, strlen(str), out);

        char temp[256];
        strcpy(temp, table[oldCode].entry);
        strncat(temp, str, 1);
        addToTable(table, &nextCode, temp);

        oldCode = newCode;
    }

    fclose(in);
    fclose(out);
}

int main() {
    compress("input.txt", "output.lzw");
    decompress("output.lzw", "output.txt");
    return 0;
}
