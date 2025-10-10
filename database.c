#include "database.h"

// создание динамического массива
void init_system(struct data_base* system, int capacity) {
    system->records = malloc(capacity * sizeof(technical_maintenance));
    system->size = 0;
    system->capacity = capacity;
}

// добавление записи
void add_item(struct data_base* system, struct technical_maintenance record) {
    if (system->size >= system->capacity) {
        system->capacity *= 2;
        system->records = realloc(system->records, system->capacity * sizeof(struct technical_maintenance));
    }
    system->records[system->size++] = record;
}

// удаление записи
void delete_item(struct data_base* system, int index) {
    if (index >= 0 && index < system->size) {
        // смещаем элементы
        for (int i = index; i < system->size - 1; i++) {
            system->records[i] = system->records[i + 1];
            // Обновляем ID
            system->records[i].id = i + 1;
        }
        system->size--;
    }
}

// изменение элемента
void modify_item(struct data_base* system, int index, struct technical_maintenance new_item) {
    if (index >= 0 && index < system->size) {
        system->records[index] = new_item;
    }
}

// вывод всех элементов
void display_items(struct data_base* system) {
    for (int i = 0; i < system->size; i++) {
        printf("Запись %d:\n", i + 1);
        printf("ID: %d\n", system->records[i].id);
        printf("Дата: %s\n", system->records[i].date);
        printf("Тип работы: %s\n", system->records[i].type_work);
        printf("Пробег: %d\n", system->records[i].mileage);
        printf("Стоимость: %.2f\n\n", system->records[i].price);
    }
}

// освобождение памяти
void free_system(struct data_base* system) {
    free(system->records);
    system->size = 0;
    system->capacity = 0;
}
//проверка корректного ввода даты
int validate_date(const char* date) {
    if (strlen(date) != 10) return 0;
    if (date[2] != '.' || date[5] != '.') return 0;
    
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (date[i] < '0' || date[i] > '9') return 0;
    }
    return 1;
}
//автовыставка цен за работу
void autoprice(technical_maintenance* record) {
    if (strcmp(record->type_work, "замена масла") == 0 || 
        strcmp(record->type_work, "Замена масла") == 0) {
        record->price = 2100;
    } else if (strcmp(record->type_work, "осмотр ТС") == 0 || 
               strcmp(record->type_work, "Осмотр ТС") == 0) {
        record->price = 999.99;
    } else if (strcmp(record->type_work, "замена фильтра") == 0 || 
               strcmp(record->type_work, "Замена фильтра") == 0) {
        record->price = 14999.90;
    } else if (strcmp(record->type_work, "покраска кузова") == 0 || 
               strcmp(record->type_work, "Покраска кузова") == 0) {
        record->price = 33500.90;
    } else if (strcmp(record->type_work, "ремонт двигателя") == 0 || 
               strcmp(record->type_work, "Ремонт двигателя") == 0) {
        record->price = 150000.90;
    } else if (strcmp(record->type_work, "полное ТО") == 0 || 
               strcmp(record->type_work, "Полное ТО") == 0) {
        record->price = 8999.90;
    } else {
        printf("│ Введите стоимость: ");
        scanf("%f", &record->price);
    }
}
// Сохранение в бинарный файл
void load_from_file(struct data_base* system, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Файл не найден, начинаем с пустой базы.\n");
        return;
    }
    
    // Читаем количество записей
    int size;
    fread(&size, sizeof(int), 1, file);
    
    // Увеличиваем емкость если нужно
    if (size > system->capacity) {
        system->capacity = size;
        system->records = realloc(system->records, system->capacity * sizeof(technical_maintenance));
    }
    
    // Читаем все записи
    for (int i = 0; i < size; i++) {
        fread(&system->records[i], sizeof(technical_maintenance), 1, file);
    }
    
    system->size = size;
    fclose(file);
    printf("Данные загружены из файла: %s. Записей: %d\n", filename, size);
}

// Загрузка из бинарного файла
void load_from_file(struct data_base* system, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Файл не найден, начинаем с пустой базы.\n");
        return;
    }
    
    // Читаем количество записей
    int size;
    fread(&size, sizeof(int), 1, file);
    
    // Увеличиваем емкость если нужно
    if (size > system->capacity) {
        system->capacity = size;
        system->records = realloc(system->records, system->capacity * sizeof(technical_maintenance));
    }
    
    // Читаем все записи
    for (int i = 0; i < size; i++) {
        fread(&system->records[i], sizeof(technical_maintenance), 1, file);
    }
    
    system->size = size;
    fclose(file);
    printf("Данные загружены из файла: %s\n", filename);
}