#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "asf_parser.h"

// ============================================================================
// Валидация AST и бизнес-схемы
// ============================================================================

// Общая структурная валидация AST (не привязана к предметной области)
// Возвращает 1 если корректно, иначе 0 и заполняет error (если != NULL).
int asf_validate_node(const DataNode* node, char* error, size_t error_cap);

// Валидация ожидаемой структуры файла автосервиса:
//   metadata: object
//   records: array of object
int asf_validate_database_ast(const DataNode* root, char* error, size_t error_cap);

#endif // VALIDATOR_H
