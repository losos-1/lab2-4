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
// освобождение памяти
void free_system(struct data_base* system) {
    free(system->records);
    system->size = 0;
    system->capacity = 0;
}
//проверка коректного ввода пробега
int validate_mileage(int mileage){
    if (mileage >= 0){
        return 1;
    }else{
        return 0;
    }
}
//проверка корректного ввода даты
int validate_date(const char* date) {
    if (strlen(date) != 10) return 0;
    if (date[2] != '.' || date[5] != '.') return 0;  
    int day, month, year;
    if (sscanf(date, "%2d.%2d.%4d", &day, &month, &year) != 3) return 0;   
    // Проверка корректности даты
    if (day < 1 || day > 31) return 0;
    if (month < 1 || month > 12) return 0;
    if (year < 2000 || year > 2100) return 0;
    
    return 1;
}

// Сохранение в бинарный файл
void save_to_file(struct data_base* system, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Ошибка открытия файла для записи!\n");
        return;
    }
    
    // Записываем количество записей
    fwrite(&system->size, sizeof(int), 1, file);
    
    // Записываем все записи
    for (int i = 0; i < system->size; i++) {
        fwrite(&system->records[i], sizeof(technical_maintenance), 1, file);
    }
    
    fclose(file);
    printf("Данные сохранены в файл: %s. Записей: %d\n", filename, system->size);
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