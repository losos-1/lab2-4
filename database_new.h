#ifndef DATABASE_NEW_H
#define DATABASE_NEW_H

#include "database.h"
#include "asf_parser.h"

// ============================================================================
// НОВЫЕ ФУНКЦИИ ДЛЯ РАБОТЫ С ASF ФОРМАТОМ
// ============================================================================

// Сохранение в ASF формат (замена save_to_file)
void save_to_asf(struct data_base* system, const char* filename, const char* username);

// Загрузка из ASF формата (замена load_from_file)
void load_from_asf(struct data_base* system, const char* filename);

// Тестовая функция для проверки парсера
void test_asf_parser(void);

#endif // DATABASE_NEW_H