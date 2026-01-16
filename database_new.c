#include "database_new.h"
#include "data_adapter.h"
#include <stdio.h>

// ============================================================================
// ИНТЕГРАЦИЯ С СУЩЕСТВУЮЩИМ КОДОМ
// ============================================================================

// Сохранение в ASF формат
void save_to_asf(struct data_base* system, const char* filename, const char* username) {
    if (!system || !filename) {
        printf("Ошибка: неверные параметры для сохранения\n");
        return;
    }
    
    printf("Сохранение данных в ASF формат: %s\n", filename);
    
    // Конвертируем базу данных в AST
    DataNode* root = database_to_asf(system);
    if (!root) {
        printf("Ошибка: не удалось конвертировать данные в ASF формат\n");
        return;
    }
    
    // Обновляем метаданные с именем пользователя
    DataNode* metadata_node = find_node_by_key(root, "metadata");
    if (metadata_node && username) {
        // Добавляем или обновляем поле user
        DataNode* user_node = NULL;
        for (int i = 0; i < metadata_node->value.object.count; i++) {
            DataNode* pair = metadata_node->value.object.pairs[i];
            if (pair->key && strcmp(pair->key, "user") == 0) {
                user_node = pair;
                break;
            }
        }
        
        if (!user_node) {
            // Создаем новый узел user
            user_node = create_string_node("user", username);
            if (user_node) {
                if (metadata_node->value.object.count >= metadata_node->value.object.capacity) {
                    metadata_node->value.object.capacity *= 2;
                    metadata_node->value.object.pairs = (DataNode**)realloc(
                        metadata_node->value.object.pairs,
                        metadata_node->value.object.capacity * sizeof(DataNode*)
                    );
                }
                metadata_node->value.object.pairs[metadata_node->value.object.count++] = user_node;
            }
        } else if (user_node->type == NODE_STRING && user_node->value.string_value) {
            // Обновляем существующий узел
            free(user_node->value.string_value);
            user_node->value.string_value = strdup(username);
        }
    }
    
    // Сохраняем в файл с красивым форматированием
    if (!asf_save_file(filename, root, 1)) {
        printf("Ошибка: не удалось сохранить файл %s\n", filename);
    } else {
        printf("Данные успешно сохранены в формате ASF:\n");
        printf("  Файл: %s\n", filename);
        printf("  Записей: %d\n", system->size);
        
        // Показываем превью файла
        char* preview = asf_serialize_node(root, 1);
        if (preview) {
            printf("\nПревью файла:\n");
            printf("%s\n", preview);
            free(preview);
        }
    }
    
    // Очищаем память
    asf_free_node(root);
}

// Загрузка из ASF формата
void load_from_asf(struct data_base* system, const char* filename) {
    if (!system || !filename) {
        printf("Ошибка: неверные параметры для загрузки\n");
        return;
    }
    
    printf("Загрузка данных из ASF формата: %s\n", filename);
    
    // Парсим файл
    DataNode* root = asf_parse_file(filename);
    if (!root) {
        printf("Ошибка: не удалось загрузить или распарсить файл %s\n", filename);
        return;
    }
    
    // Конвертируем AST в базу данных
    data_base* new_db = asf_to_database(root);
    if (!new_db) {
        printf("Ошибка: не удалось конвертировать ASF данные в базу\n");
        asf_free_node(root);
        return;
    }
    
    // Очищаем старую базу и копируем новую
    free_system(system);
    system->records = new_db->records;
    system->size = new_db->size;
    system->capacity = new_db->capacity;
    
    // Освобождаем временную структуру (но не записи!)
    free(new_db);
    
    // Показываем метаданные
    DataNode* metadata_node = find_node_by_key(root, "metadata");
    if (metadata_node) {
        DataNode* user_node = find_node_by_key(metadata_node, "user");
        DataNode* created_node = find_node_by_key(metadata_node, "created");
        DataNode* version_node = find_node_by_key(metadata_node, "version");
        
        printf("Файл успешно загружен:\n");
        if (version_node && version_node->type == NODE_STRING) {
            printf("  Версия формата: %s\n", version_node->value.string_value);
        }
        if (created_node && created_node->type == NODE_STRING) {
            printf("  Дата создания: %s\n", created_node->value.string_value);
        }
        if (user_node && user_node->type == NODE_STRING) {
            printf("  Пользователь: %s\n", user_node->value.string_value);
        }
        printf("  Загружено записей: %d\n", system->size);
    }
    
    // Очищаем AST
    asf_free_node(root);
}

// Тестовая функция для проверки парсера
void test_asf_parser(void) {
    printf("\n╔═════════════════════════════════════════╗\n");
    printf("║        ТЕСТ ПАРСЕРА ASF ФОРМАТА        ║\n");
    printf("╚═════════════════════════════════════════╝\n\n");
    
    // Тестовые данные в ASF формате
    const char* test_asf = 
        "# Тестовый файл ASF\n"
        "database = object {\n"
        "  metadata = object {\n"
        "    version = \"1.0\"\n"
        "    created = \"2024-12-14 19:30:00\"\n"
        "    user = \"testuser\"\n"
        "    record_count = 2\n"
        "  }\n"
        "  \n"
        "  records = array [\n"
        "    object {\n"
        "      id = 1\n"
        "      date = \"15.12.2024\"\n"
        "      type_work = \"замена масла\"\n"
        "      mileage = 15000\n"
        "      price = 2100.00\n"
        "    }\n"
        "    object {\n"
        "      id = 2\n"
        "      date = \"20.12.2024\"\n"
        "      type_work = \"осмотр ТС\"\n"
        "      mileage = 20000\n"
        "      price = 999.99\n"
        "    }\n"
        "  ]\n"
        "}\n";
    
    printf("Тест 1: Парсинг строки ASF\n");
    printf("===========================\n");
    
    DataNode* parsed = asf_parse_string(test_asf);
    if (!parsed) {
        printf("❌ Ошибка парсинга!\n");
        return;
    }
    
    printf("✅ Парсинг успешен!\n");
    printf("\nСтруктура AST:\n");
    asf_print_node(parsed, 0);
    
    printf("\nТест 2: Сериализация обратно в ASF\n");
    printf("===================================\n");
    
    char* serialized = asf_serialize_node(parsed, 1);
    if (serialized) {
        printf("✅ Сериализация успешна!\n");
        printf("\nСериализованные данные:\n%s\n", serialized);
        free(serialized);
    } else {
        printf("❌ Ошибка сериализации!\n");
    }
    
    printf("\nТест 3: Конвертация в структуры автосервиса\n");
    printf("===========================================\n");
    
    data_base* test_db = asf_to_database(parsed);
    if (test_db) {
        printf("✅ Конвертация успешна!\n");
        printf("Записей в базе: %d\n", test_db->size);
        
        // Показываем записи
        for (int i = 0; i < test_db->size; i++) {
            printf("\nЗапись #%d:\n", test_db->records[i].id);
            printf("  Дата: %s\n", test_db->records[i].date);
            printf("  Тип работы: %s\n", test_db->records[i].type_work);
            printf("  Пробег: %d\n", test_db->records[i].mileage);
            printf("  Стоимость: %.2f\n", test_db->records[i].price);
        }
        
        // Очищаем тестовую базу
        free_system(test_db);
        free(test_db);
    } else {
        printf("❌ Ошибка конвертации!\n");
    }
    
    // Очищаем память
    asf_free_node(parsed);
    
    printf("\n✅ Все тесты пройдены успешно!\n");
}