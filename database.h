#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_MAGIC 0x4D41494E  // "MAIN" 

#define FILE_VERSION 1

typedef struct technical_maintenance {
    int id;
    char date[11]; // формата xx.xx.xxxx
    char type_work[100]; // тип работы
    int mileage; // пробег
    float price; // стоимость
} technical_maintenance;

// структура для динамического массива
typedef struct data_base {
    technical_maintenance* records;
    int size;  // кол-во элементов
    int capacity; // макс вместимость
} data_base;

// Структура заголовка файла
typedef struct file_header {
    unsigned int magic;        // Магическое число
    unsigned int version;      // Версия формата
    unsigned int record_count; // Количество записей
    unsigned int checksum;     // Контрольная сумма
} file_header;

// Прототипы функций
void init_system(struct data_base* system, int capacity);
void add_item(struct data_base* system, struct technical_maintenance record);
void delete_item(struct data_base* system, int index);
void modify_item(struct data_base* system, int index, struct technical_maintenance new_item);
void display_items(struct data_base* system);
void free_system(struct data_base* system);
void autoprice(technical_maintenance* record);
int  validate_date(const char* date);
int validate_mileage(int mileage); 
void save_to_file(struct data_base* system, const char* filename);
void load_from_file(struct data_base* system, const char* filename);
void clear_database(struct data_base* system);

// Новые функции для работы с заголовком
unsigned int calculate_checksum(struct data_base* system);
int validate_file_header(struct file_header* header);

#endif