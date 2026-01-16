#ifndef DATA_ADAPTER_H
#define DATA_ADAPTER_H

#include "asf_parser.h"
#include "database.h"

// ============================================================================
// Адаптер данных: преобразование бизнес-структур <-> ASF AST
// ============================================================================

// ...

// Конвертация структур автосервиса в AST
DataNode* technical_maintenance_to_asf(const technical_maintenance* records, int count);
DataNode* database_to_asf(const data_base* db, const char* username);

// Конвертация AST в структуры автосервиса
technical_maintenance* asf_to_technical_maintenance(const DataNode* node, int* count);
data_base* asf_to_database(const DataNode* root);

// Создание метаданных для файла
DataNode* create_metadata(const char* username, int record_count);

// Поиск значения по ключу в объекте (возвращает именно значение, а не пару)
DataNode* find_node_by_key(DataNode* object, const char* key);
const DataNode* find_node_by_key_const(const DataNode* object, const char* key);

#endif // DATA_ADAPTER_H
