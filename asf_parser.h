#ifndef ASF_PARSER_H
#define ASF_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// ============================================================================
// ASF (AutoService Format)
// Пользовательский текстовый формат данных.
// Минимум по ТЗ: комментарии, пары ключ-значение, типы (string/number/bool/null),
// массивы, объекты, необязательные запятые/пробелы/переносы.
// ============================================================================

// ---------------------------- Tokenizer ------------------------------------

typedef enum {
    TOKEN_STRING,     // "text"
    TOKEN_INTEGER,    // 42, -15, 0xFF
    TOKEN_FLOAT,      // 3.14, -1.0e3
    TOKEN_BOOLEAN,    // true, false
    TOKEN_NULL,       // null

    TOKEN_IDENTIFIER, // name, record_count
    TOKEN_KEYWORD,    // object, array

    TOKEN_EQUALS,     // =
    TOKEN_COLON,      // :
    TOKEN_COMMA,      // ,

    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LBRACKET,   // [
    TOKEN_RBRACKET,   // ]

    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct Token {
    TokenType type;
    char* lexeme;           // владение у списка токенов
    int line;
    int column;
    struct Token* next;
} Token;

// ------------------------------ AST ----------------------------------------

typedef enum {
    NODE_OBJECT,
    NODE_ARRAY,
    NODE_STRING,
    NODE_INTEGER,
    NODE_FLOAT,
    NODE_BOOLEAN,
    NODE_NULL,
    NODE_KEY_VALUE
} NodeType;

typedef struct DataNode {
    NodeType type;

    // Для NODE_KEY_VALUE
    char* key;

    union {
        char* string_value;
        long int_value;
        double float_value;
        int bool_value;

        struct {
            struct DataNode** items;
            int count;
            int capacity;
        } array;

        struct {
            struct DataNode** pairs; // элементы должны быть NODE_KEY_VALUE
            int count;
            int capacity;
        } object;

        struct DataNode* child; // для NODE_KEY_VALUE: значение
    } value;
} DataNode;

// --------------------------- Public API ------------------------------------

// Парсинг
DataNode* asf_parse_file(const char* filename);
DataNode* asf_parse_string(const char* text);

// Сериализация
char* asf_serialize_node(const DataNode* node, int pretty);
int asf_save_file(const char* filename, const DataNode* node, int pretty);

// Память / отладка
void asf_free_node(DataNode* node);
void asf_print_node(const DataNode* node, int indent);

// Фабрики узлов (для адаптера/валидатора)
DataNode* asf_node_create(NodeType type);
DataNode* asf_node_string(const char* value);
DataNode* asf_node_integer(long value);
DataNode* asf_node_float(double value);
DataNode* asf_node_boolean(int value);
DataNode* asf_node_null(void);
DataNode* asf_pair_create(const char* key, DataNode* child);
int asf_array_add(DataNode* array_node, DataNode* item);
int asf_object_put(DataNode* object_node, const char* key, DataNode* child);
const DataNode* asf_object_get(const DataNode* object_node, const char* key);

// Вспомогательное
const char* asf_token_type_to_string(TokenType type);
const char* asf_node_type_to_string(NodeType type);

#endif // ASF_PARSER_H
