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
    if (day < 1 || day > 31) return 0;
    if (month < 1 || month > 12) return 0;
    if (year < 2000 || year > 2100) return 0;
    
    return 1;
}

// Расчет контрольной суммы
unsigned int calculate_checksum(struct data_base* system) {
    unsigned int checksum = 0;
    unsigned char* data = (unsigned char*)system->records;
    size_t data_size = system->size * sizeof(technical_maintenance);
    
    for (size_t i = 0; i < data_size; i++) {
        checksum = (checksum << 3) ^ data[i] ^ (checksum >> 29);
    }
    
    // Добавляем размер в контрольную сумму
    checksum ^= system->size;
    
    return checksum;
}

// Валидация заголовка файла
int validate_file_header(struct file_header* header) {
    if (header->magic != FILE_MAGIC) {
        printf("Ошибка: Неверный формат файла (магическое число)\n");
        return 0;
    }
    
    if (header->version != FILE_VERSION) {
        printf("Ошибка: Несовместимая версия файла (%u, ожидается %u)\n", 
               header->version, FILE_VERSION);
        return 0;
    }
    
    if (header->record_count > 100000) { // Разумный лимит
        printf("Ошибка: Некорректное количество записей: %u\n", header->record_count);
        return 0;
    }
    
    return 1;
}

// Сохранение в бинарный файл с заголовком
void save_to_file(struct data_base* system, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Ошибка открытия файла для записи: %s\n", filename);
        return;
    }
    
    // Создаем заголовок файла
    file_header header;
    header.magic = FILE_MAGIC;
    header.version = FILE_VERSION;
    header.record_count = system->size;
    header.checksum = calculate_checksum(system);
    
    // Записываем заголовок
    if (fwrite(&header, sizeof(file_header), 1, file) != 1) {
        printf("Ошибка записи заголовка файла\n");
        fclose(file);
        return;
    }
    
    // Записываем данные
    for (int i = 0; i < system->size; i++) {
        if (fwrite(&system->records[i], sizeof(technical_maintenance), 1, file) != 1) {
            printf("Ошибка записи записи %d\n", i);
            break;
        }
    }
    
    fclose(file);
    printf("Данные сохранены в файл: %s\n", filename);
    printf("  Записей: %d, Контрольная сумма: 0x%08X\n", system->size, header.checksum);
}

// Загрузка из бинарного файла с проверкой заголовка
void load_from_file(struct data_base* system, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Файл не найден, начинаем с пустой базы: %s\n", filename);
        return;
    }
    
    // Читаем заголовок
    file_header header;
    if (fread(&header, sizeof(file_header), 1, file) != 1) {
        printf("Ошибка чтения заголовка файла\n");
        fclose(file);
        return;
    }
    
    // Проверяем заголовок
    if (!validate_file_header(&header)) {
        fclose(file);
        return;
    }
    
    // Подготавливаем память
    if (header.record_count > system->capacity) {
        system->capacity = header.record_count;
        system->records = realloc(system->records, system->capacity * sizeof(technical_maintenance));
    }
    
    // Читаем записи
    int records_read = 0;
    for (int i = 0; i < header.record_count; i++) {
        if (fread(&system->records[i], sizeof(technical_maintenance), 1, file) != 1) {
            printf("Ошибка чтения записи %d\n", i);
            break;
        }
        records_read++;
    }
    
    system->size = records_read;
    fclose(file);
    
    // Проверяем контрольную сумму
    unsigned int actual_checksum = calculate_checksum(system);
    if (actual_checksum != header.checksum) {
        printf("Предупреждение: Контрольная сумма не совпадает!\n");
        printf("  Ожидалось: 0x%08X, Получено: 0x%08X\n", header.checksum, actual_checksum);
        printf("  Возможно повреждение данных\n");
    } else {
        printf("Данные загружены из файла: %s\n", filename);
        printf("  Записей: %d, Контрольная сумма: 0x%08X ✓\n", system->size, actual_checksum);
    }
}

// Полная очистка базы данных
void clear_database(struct data_base* system) {
    free(system->records);
    system->records = malloc(10 * sizeof(technical_maintenance));
    system->size = 0;
    system->capacity = 10;
    printf("База данных полностью очищена!\n");
}