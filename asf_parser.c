#include "asf_parser.h"

#include <stdarg.h>

// ============================================================================
// Internal utilities
// ============================================================================

static char* asf_strdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n + 1);
    return out;
}

static char* asf_strndup(const char* s, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static int asf_is_ident_start(char c) {
    unsigned char uc = (unsigned char)c;
    return isalpha(uc) || c == '_';
}

static int asf_is_ident_part(char c) {
    unsigned char uc = (unsigned char)c;
    return isalnum(uc) || c == '_';
}

static int asf_key_needs_quotes(const char* key) {
    if (!key || !key[0]) return 1;
    if (!asf_is_ident_start(key[0])) return 1;
    for (const char* p = key + 1; *p; ++p) {
        if (!asf_is_ident_part(*p)) return 1;
    }
    return 0;
}

// ============================================================================
// Tokenizer
// ============================================================================

typedef struct {
    const char* src;
    size_t pos;
    int line;
    int col;
    int has_error;
    char error[256];

    Token* head;
    Token* tail;
} Lexer;

static char lx_peek(const Lexer* lx) {
    return lx->src[lx->pos];
}

static char lx_peek2(const Lexer* lx) {
    char c = lx->src[lx->pos];
    if (c == '\0') return '\0';
    return lx->src[lx->pos + 1];
}

static char lx_advance(Lexer* lx) {
    char c = lx->src[lx->pos];
    if (c == '\0') return c;
    lx->pos++;
    if (c == '\n') {
        lx->line++;
        lx->col = 1;
    } else {
        lx->col++;
    }
    return c;
}

static int lx_match(Lexer* lx, char expected) {
    if (lx_peek(lx) != expected) return 0;
    lx_advance(lx);
    return 1;
}

static void lx_error(Lexer* lx, const char* fmt, ...) {
    if (lx->has_error) return;
    lx->has_error = 1;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(lx->error, sizeof(lx->error), fmt, ap);
    va_end(ap);
}

static Token* token_new(TokenType t, const char* lexeme, int line, int col) {
    Token* tok = (Token*)calloc(1, sizeof(Token));
    if (!tok) return NULL;
    tok->type = t;
    tok->line = line;
    tok->column = col;
    tok->lexeme = lexeme ? asf_strdup(lexeme) : NULL;
    tok->next = NULL;
    return tok;
}

static Token* token_new_span(TokenType t, const char* start, size_t len, int line, int col) {
    Token* tok = (Token*)calloc(1, sizeof(Token));
    if (!tok) return NULL;
    tok->type = t;
    tok->line = line;
    tok->column = col;
    tok->lexeme = start ? asf_strndup(start, len) : NULL;
    tok->next = NULL;
    return tok;
}

static void lx_push(Lexer* lx, Token* tok) {
    if (!tok) {
        lx_error(lx, "Недостаточно памяти при создании токена");
        return;
    }
    if (!lx->head) {
        lx->head = lx->tail = tok;
    } else {
        lx->tail->next = tok;
        lx->tail = tok;
    }
}

static void lx_skip_ws_comments(Lexer* lx) {
    for (;;) {
        // whitespace
        while (isspace((unsigned char)lx_peek(lx))) {
            lx_advance(lx);
        }

        // # comment
        if (lx_peek(lx) == '#') {
            while (lx_peek(lx) != '\0' && lx_peek(lx) != '\n') {
                lx_advance(lx);
            }
            continue;
        }

        // // comment
        if (lx_peek(lx) == '/' && lx_peek2(lx) == '/') {
            lx_advance(lx);
            lx_advance(lx);
            while (lx_peek(lx) != '\0' && lx_peek(lx) != '\n') {
                lx_advance(lx);
            }
            continue;
        }

        // /* comment */
        if (lx_peek(lx) == '/' && lx_peek2(lx) == '*') {
            int start_line = lx->line;
            int start_col = lx->col;
            lx_advance(lx);
            lx_advance(lx);
            while (lx_peek(lx) != '\0') {
                if (lx_peek(lx) == '*' && lx_peek2(lx) == '/') {
                    lx_advance(lx);
                    lx_advance(lx);
                    break;
                }
                lx_advance(lx);
            }
            if (lx_peek(lx) == '\0') {
                lx_error(lx, "Незакрытый комментарий /* */ (строка %d, колонка %d)", start_line, start_col);
            }
            continue;
        }

        break;
    }
}

static Token* lx_read_string(Lexer* lx) {
    int line = lx->line;
    int col = lx->col;

    // consume opening quote
    lx_advance(lx);

    size_t cap = 128;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) {
        lx_error(lx, "Недостаточно памяти для строки");
        return token_new(TOKEN_ERROR, NULL, line, col);
    }

    while (1) {
        char c = lx_peek(lx);
        if (c == '\0') {
            free(buf);
            lx_error(lx, "Незавершенная строка (строка %d, колонка %d)", line, col);
            return token_new(TOKEN_ERROR, NULL, line, col);
        }
        if (c == '"') {
            lx_advance(lx);
            break;
        }
        if (c == '\\') {
            lx_advance(lx);
            char e = lx_peek(lx);
            if (e == '\0') {
                free(buf);
                lx_error(lx, "Незавершенная escape-последовательность (строка %d, колонка %d)", lx->line, lx->col);
                return token_new(TOKEN_ERROR, NULL, line, col);
            }
            lx_advance(lx);
            switch (e) {
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'u':
                    // Unicode не требуется по ТЗ; ставим маркер
                    c = '?';
                    // пропускаем 4 hex, если есть
                    for (int i = 0; i < 4; i++) {
                        if (isxdigit((unsigned char)lx_peek(lx))) lx_advance(lx);
                    }
                    break;
                default:
                    c = e;
                    break;
            }
        } else {
            lx_advance(lx);
        }

        if (len + 1 >= cap) {
            cap *= 2;
            char* nb = (char*)realloc(buf, cap);
            if (!nb) {
                free(buf);
                lx_error(lx, "Недостаточно памяти для строки");
                return token_new(TOKEN_ERROR, NULL, line, col);
            }
            buf = nb;
        }
        buf[len++] = c;
    }

    buf[len] = '\0';
    Token* tok = token_new(TOKEN_STRING, buf, line, col);
    free(buf);
    return tok;
}

static int lx_is_num_start(const Lexer* lx) {
    char c = lx_peek(lx);
    char d = lx_peek2(lx);
    if (isdigit((unsigned char)c)) return 1;
    if ((c == '+' || c == '-') && (isdigit((unsigned char)d) || (d == '.' && isdigit((unsigned char)lx->src[lx->pos + 2])))) return 1;
    if (c == '.' && isdigit((unsigned char)d)) return 1;
    return 0;
}

static Token* lx_read_number(Lexer* lx) {
    int line = lx->line;
    int col = lx->col;

    size_t start = lx->pos;

    // optional sign
    if (lx_peek(lx) == '+' || lx_peek(lx) == '-') lx_advance(lx);

    // hex integer: 0x...
    if (lx_peek(lx) == '0' && (lx_peek2(lx) == 'x' || lx_peek2(lx) == 'X')) {
        lx_advance(lx);
        lx_advance(lx);
        int digits = 0;
        while (isxdigit((unsigned char)lx_peek(lx))) {
            lx_advance(lx);
            digits++;
        }
        if (digits == 0) {
            lx_error(lx, "Некорректное hex-число (строка %d, колонка %d)", line, col);
            return token_new(TOKEN_ERROR, NULL, line, col);
        }
        return token_new_span(TOKEN_INTEGER, lx->src + start, lx->pos - start, line, col);
    }

    int saw_digit = 0;
    while (isdigit((unsigned char)lx_peek(lx))) {
        saw_digit = 1;
        lx_advance(lx);
    }

    int is_float = 0;

    if (lx_peek(lx) == '.') {
        is_float = 1;
        lx_advance(lx);
        while (isdigit((unsigned char)lx_peek(lx))) {
            saw_digit = 1;
            lx_advance(lx);
        }
    }

    // exponent
    if (lx_peek(lx) == 'e' || lx_peek(lx) == 'E') {
        is_float = 1;
        lx_advance(lx);
        if (lx_peek(lx) == '+' || lx_peek(lx) == '-') lx_advance(lx);
        int exp_digits = 0;
        while (isdigit((unsigned char)lx_peek(lx))) {
            exp_digits++;
            lx_advance(lx);
        }
        if (exp_digits == 0) {
            lx_error(lx, "Некорректная экспонента (строка %d, колонка %d)", line, col);
            return token_new(TOKEN_ERROR, NULL, line, col);
        }
    }

    if (!saw_digit) {
        lx_error(lx, "Некорректное число (строка %d, колонка %d)", line, col);
        return token_new(TOKEN_ERROR, NULL, line, col);
    }

    return token_new_span(is_float ? TOKEN_FLOAT : TOKEN_INTEGER, lx->src + start, lx->pos - start, line, col);
}

static Token* lx_read_identifier(Lexer* lx) {
    int line = lx->line;
    int col = lx->col;

    size_t start = lx->pos;
    lx_advance(lx);
    while (asf_is_ident_part(lx_peek(lx))) {
        lx_advance(lx);
    }

    size_t len = lx->pos - start;
    char* ident = asf_strndup(lx->src + start, len);
    if (!ident) {
        lx_error(lx, "Недостаточно памяти для идентификатора");
        return token_new(TOKEN_ERROR, NULL, line, col);
    }

    TokenType t = TOKEN_IDENTIFIER;
    if (strcmp(ident, "true") == 0 || strcmp(ident, "false") == 0) {
        t = TOKEN_BOOLEAN;
    } else if (strcmp(ident, "null") == 0) {
        t = TOKEN_NULL;
    } else if (strcmp(ident, "object") == 0 || strcmp(ident, "array") == 0) {
        t = TOKEN_KEYWORD;
    }

    Token* tok = token_new(t, ident, line, col);
    free(ident);
    return tok;
}

static Token* tokenize(const char* src, char* errbuf, size_t errcap) {
    Lexer lx;
    memset(&lx, 0, sizeof(lx));
    lx.src = src ? src : "";
    lx.pos = 0;
    lx.line = 1;
    lx.col = 1;

    for (;;) {
        lx_skip_ws_comments(&lx);
        if (lx.has_error) break;

        int line = lx.line;
        int col = lx.col;
        char c = lx_peek(&lx);

        if (c == '\0') {
            lx_push(&lx, token_new(TOKEN_EOF, "", line, col));
            break;
        }

        if (c == '"') {
            lx_push(&lx, lx_read_string(&lx));
            continue;
        }

        if (asf_is_ident_start(c)) {
            lx_push(&lx, lx_read_identifier(&lx));
            continue;
        }

        if (lx_is_num_start(&lx)) {
            lx_push(&lx, lx_read_number(&lx));
            continue;
        }

        // punctuation
        switch (c) {
            case '=': lx_advance(&lx); lx_push(&lx, token_new(TOKEN_EQUALS, "=", line, col)); break;
            case ':': lx_advance(&lx); lx_push(&lx, token_new(TOKEN_COLON, ":", line, col)); break;
            case ',': lx_advance(&lx); lx_push(&lx, token_new(TOKEN_COMMA, ",", line, col)); break;
            case '{': lx_advance(&lx); lx_push(&lx, token_new(TOKEN_LBRACE, "{", line, col)); break;
            case '}': lx_advance(&lx); lx_push(&lx, token_new(TOKEN_RBRACE, "}", line, col)); break;
            case '[': lx_advance(&lx); lx_push(&lx, token_new(TOKEN_LBRACKET, "[", line, col)); break;
            case ']': lx_advance(&lx); lx_push(&lx, token_new(TOKEN_RBRACKET, "]", line, col)); break;
            default:
                lx_error(&lx, "Неожиданный символ '%c' (строка %d, колонка %d)", c, line, col);
                lx_push(&lx, token_new(TOKEN_ERROR, NULL, line, col));
                break;
        }

        if (lx.has_error) break;
    }

    if (lx.has_error) {
        if (errbuf && errcap) {
            snprintf(errbuf, errcap, "%s", lx.error);
        }
    }

    return lx.head;
}

static void free_tokens(Token* t) {
    while (t) {
        Token* next = t->next;
        free(t->lexeme);
        free(t);
        t = next;
    }
}

// ============================================================================
// AST factories / helpers
// ============================================================================

DataNode* asf_node_create(NodeType type) {
    DataNode* n = (DataNode*)calloc(1, sizeof(DataNode));
    if (!n) return NULL;
    n->type = type;
    switch (type) {
        case NODE_ARRAY:
            n->value.array.capacity = 8;
            n->value.array.count = 0;
            n->value.array.items = (DataNode**)calloc((size_t)n->value.array.capacity, sizeof(DataNode*));
            if (!n->value.array.items) { free(n); return NULL; }
            break;
        case NODE_OBJECT:
            n->value.object.capacity = 8;
            n->value.object.count = 0;
            n->value.object.pairs = (DataNode**)calloc((size_t)n->value.object.capacity, sizeof(DataNode*));
            if (!n->value.object.pairs) { free(n); return NULL; }
            break;
        default:
            break;
    }
    return n;
}

DataNode* asf_node_string(const char* value) {
    DataNode* n = asf_node_create(NODE_STRING);
    if (!n) return NULL;
    n->value.string_value = asf_strdup(value ? value : "");
    if (!n->value.string_value) { free(n); return NULL; }
    return n;
}

DataNode* asf_node_integer(long value) {
    DataNode* n = asf_node_create(NODE_INTEGER);
    if (!n) return NULL;
    n->value.int_value = value;
    return n;
}

DataNode* asf_node_float(double value) {
    DataNode* n = asf_node_create(NODE_FLOAT);
    if (!n) return NULL;
    n->value.float_value = value;
    return n;
}

DataNode* asf_node_boolean(int value) {
    DataNode* n = asf_node_create(NODE_BOOLEAN);
    if (!n) return NULL;
    n->value.bool_value = value ? 1 : 0;
    return n;
}

DataNode* asf_node_null(void) {
    return asf_node_create(NODE_NULL);
}

DataNode* asf_pair_create(const char* key, DataNode* child) {
    if (!key || !child) return NULL;
    DataNode* n = asf_node_create(NODE_KEY_VALUE);
    if (!n) return NULL;
    n->key = asf_strdup(key);
    if (!n->key) { free(n); return NULL; }
    n->value.child = child;
    return n;
}

int asf_array_add(DataNode* array_node, DataNode* item) {
    if (!array_node || array_node->type != NODE_ARRAY || !item) return 0;
    if (array_node->value.array.count >= array_node->value.array.capacity) {
        int newcap = array_node->value.array.capacity * 2;
        DataNode** ni = (DataNode**)realloc(array_node->value.array.items, (size_t)newcap * sizeof(DataNode*));
        if (!ni) return 0;
        array_node->value.array.items = ni;
        array_node->value.array.capacity = newcap;
    }
    array_node->value.array.items[array_node->value.array.count++] = item;
    return 1;
}

int asf_object_put(DataNode* object_node, const char* key, DataNode* child) {
    if (!object_node || object_node->type != NODE_OBJECT || !key || !child) return 0;

    // replace if exists
    for (int i = 0; i < object_node->value.object.count; i++) {
        DataNode* pair = object_node->value.object.pairs[i];
        if (pair && pair->type == NODE_KEY_VALUE && pair->key && strcmp(pair->key, key) == 0) {
            asf_free_node(pair->value.child);
            pair->value.child = child;
            return 1;
        }
    }

    if (object_node->value.object.count >= object_node->value.object.capacity) {
        int newcap = object_node->value.object.capacity * 2;
        DataNode** np = (DataNode**)realloc(object_node->value.object.pairs, (size_t)newcap * sizeof(DataNode*));
        if (!np) return 0;
        object_node->value.object.pairs = np;
        object_node->value.object.capacity = newcap;
    }

    DataNode* pair = asf_pair_create(key, child);
    if (!pair) return 0;
    object_node->value.object.pairs[object_node->value.object.count++] = pair;
    return 1;
}

const DataNode* asf_object_get(const DataNode* object_node, const char* key) {
    if (!object_node || object_node->type != NODE_OBJECT || !key) return NULL;
    for (int i = 0; i < object_node->value.object.count; i++) {
        DataNode* pair = object_node->value.object.pairs[i];
        if (pair && pair->type == NODE_KEY_VALUE && pair->key && strcmp(pair->key, key) == 0) {
            return pair->value.child;
        }
    }
    return NULL;
}

// ============================================================================
// Parser
// ============================================================================

typedef struct {
    Token* cur;
    int has_error;
    char error[256];
} Parser;

static void ps_error(Parser* ps, const char* fmt, ...) {
    if (ps->has_error) return;
    ps->has_error = 1;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(ps->error, sizeof(ps->error), fmt, ap);
    va_end(ap);
}

static int ps_check(Parser* ps, TokenType t) {
    return ps->cur && ps->cur->type == t;
}

static Token* ps_advance(Parser* ps) {
    Token* prev = ps->cur;
    if (ps->cur) ps->cur = ps->cur->next;
    return prev;
}

static int ps_match(Parser* ps, TokenType t) {
    if (!ps_check(ps, t)) return 0;
    ps_advance(ps);
    return 1;
}

static int ps_expect(Parser* ps, TokenType t, const char* what) {
    if (ps_check(ps, t)) {
        ps_advance(ps);
        return 1;
    }
    int line = ps->cur ? ps->cur->line : -1;
    int col = ps->cur ? ps->cur->column : -1;
    ps_error(ps, "Ожидалось %s (строка %d, колонка %d)", what, line, col);
    return 0;
}

static DataNode* parse_value(Parser* ps);

static DataNode* parse_array(Parser* ps) {
    // optional keyword already consumed by caller
    if (!ps_expect(ps, TOKEN_LBRACKET, "'['")) return NULL;

    DataNode* arr = asf_node_create(NODE_ARRAY);
    if (!arr) { ps_error(ps, "Недостаточно памяти для массива"); return NULL; }

    // empty array
    if (ps_match(ps, TOKEN_RBRACKET)) return arr;

    while (!ps->has_error) {
        DataNode* v = parse_value(ps);
        if (!v) { asf_free_node(arr); return NULL; }
        if (!asf_array_add(arr, v)) {
            asf_free_node(v);
            asf_free_node(arr);
            ps_error(ps, "Недостаточно памяти при добавлении элемента массива");
            return NULL;
        }

        // optional comma
        ps_match(ps, TOKEN_COMMA);

        if (ps_match(ps, TOKEN_RBRACKET)) break;
        // Allow newline/space separation without commas: continue until ']'
        if (ps_check(ps, TOKEN_EOF)) {
            ps_error(ps, "Неожиданный конец файла: ожидался ']' для массива");
            break;
        }
    }

    if (ps->has_error) {
        asf_free_node(arr);
        return NULL;
    }

    return arr;
}

static int token_is_key(Token* t) {
    return t && (t->type == TOKEN_IDENTIFIER || t->type == TOKEN_STRING);
}

static DataNode* parse_object(Parser* ps) {
    // optional keyword already consumed by caller
    if (!ps_expect(ps, TOKEN_LBRACE, "'{'")) return NULL;

    DataNode* obj = asf_node_create(NODE_OBJECT);
    if (!obj) { ps_error(ps, "Недостаточно памяти для объекта"); return NULL; }

    // empty object
    if (ps_match(ps, TOKEN_RBRACE)) return obj;

    while (!ps->has_error) {
        if (!token_is_key(ps->cur)) {
            int line = ps->cur ? ps->cur->line : -1;
            int col = ps->cur ? ps->cur->column : -1;
            ps_error(ps, "Ожидался ключ объекта (строка/идентификатор) (строка %d, колонка %d)", line, col);
            break;
        }

        const char* key = ps->cur->lexeme ? ps->cur->lexeme : "";
        ps_advance(ps);

        if (!(ps_match(ps, TOKEN_EQUALS) || ps_match(ps, TOKEN_COLON))) {
            int line = ps->cur ? ps->cur->line : -1;
            int col = ps->cur ? ps->cur->column : -1;
            ps_error(ps, "Ожидался разделитель '=' или ':' после ключа (строка %d, колонка %d)", line, col);
            break;
        }

        DataNode* val = parse_value(ps);
        if (!val) { asf_free_node(obj); return NULL; }

        if (!asf_object_put(obj, key, val)) {
            asf_free_node(val);
            asf_free_node(obj);
            ps_error(ps, "Недостаточно памяти при добавлении пары ключ-значение");
            return NULL;
        }

        // optional comma
        ps_match(ps, TOKEN_COMMA);

        if (ps_match(ps, TOKEN_RBRACE)) break;
        if (ps_check(ps, TOKEN_EOF)) {
            ps_error(ps, "Неожиданный конец файла: ожидался '}' для объекта");
            break;
        }
    }

    if (ps->has_error) {
        asf_free_node(obj);
        return NULL;
    }

    return obj;
}

static DataNode* parse_value(Parser* ps) {
    if (!ps->cur) {
        ps_error(ps, "Неожиданный конец ввода");
        return NULL;
    }

    switch (ps->cur->type) {
        case TOKEN_STRING: {
            const char* s = ps->cur->lexeme ? ps->cur->lexeme : "";
            ps_advance(ps);
            return asf_node_string(s);
        }
        case TOKEN_INTEGER: {
            const char* s = ps->cur->lexeme ? ps->cur->lexeme : "0";
            ps_advance(ps);
            errno = 0;
            char* end = NULL;
            long v = strtol(s, &end, 0);
            if (errno != 0 || !end || *end != '\0') {
                ps_error(ps, "Некорректное целое число: %s", s);
                return NULL;
            }
            return asf_node_integer(v);
        }
        case TOKEN_FLOAT: {
            const char* s = ps->cur->lexeme ? ps->cur->lexeme : "0";
            ps_advance(ps);
            errno = 0;
            char* end = NULL;
            double v = strtod(s, &end);
            if (errno != 0 || !end || *end != '\0') {
                ps_error(ps, "Некорректное вещественное число: %s", s);
                return NULL;
            }
            return asf_node_float(v);
        }
        case TOKEN_BOOLEAN: {
            const char* s = ps->cur->lexeme ? ps->cur->lexeme : "false";
            ps_advance(ps);
            return asf_node_boolean(strcmp(s, "true") == 0);
        }
        case TOKEN_NULL:
            ps_advance(ps);
            return asf_node_null();

        case TOKEN_KEYWORD: {
            const char* kw = ps->cur->lexeme ? ps->cur->lexeme : "";
            ps_advance(ps);
            if (strcmp(kw, "object") == 0) return parse_object(ps);
            if (strcmp(kw, "array") == 0) return parse_array(ps);
            ps_error(ps, "Неизвестное ключевое слово: %s", kw);
            return NULL;
        }

        case TOKEN_LBRACE:
            return parse_object(ps);
        case TOKEN_LBRACKET:
            return parse_array(ps);

        default: {
            int line = ps->cur ? ps->cur->line : -1;
            int col = ps->cur ? ps->cur->column : -1;
            ps_error(ps, "Неожиданный токен %s при разборе значения (строка %d, колонка %d)",
                     asf_token_type_to_string(ps->cur->type), line, col);
            return NULL;
        }
    }
}

static DataNode* parse_root(Parser* ps) {
    // allow keyword object/array as root
    if (ps_check(ps, TOKEN_KEYWORD)) {
        const char* kw = ps->cur->lexeme ? ps->cur->lexeme : "";
        if (strcmp(kw, "object") == 0) {
            ps_advance(ps);
            return parse_object(ps);
        }
        if (strcmp(kw, "array") == 0) {
            ps_advance(ps);
            return parse_array(ps);
        }
    }

    if (ps_check(ps, TOKEN_LBRACE)) return parse_object(ps);
    if (ps_check(ps, TOKEN_LBRACKET)) return parse_array(ps);

    // otherwise: implicit object of top-level pairs
    DataNode* root = asf_node_create(NODE_OBJECT);
    if (!root) { ps_error(ps, "Недостаточно памяти для корневого объекта"); return NULL; }

    while (!ps->has_error && !ps_check(ps, TOKEN_EOF)) {
        if (!token_is_key(ps->cur)) {
            int line = ps->cur ? ps->cur->line : -1;
            int col = ps->cur ? ps->cur->column : -1;
            ps_error(ps, "Ожидалась пара ключ-значение на верхнем уровне (строка %d, колонка %d)", line, col);
            break;
        }

        const char* key = ps->cur->lexeme ? ps->cur->lexeme : "";
        ps_advance(ps);

        if (!(ps_match(ps, TOKEN_EQUALS) || ps_match(ps, TOKEN_COLON))) {
            int line = ps->cur ? ps->cur->line : -1;
            int col = ps->cur ? ps->cur->column : -1;
            ps_error(ps, "Ожидался разделитель '=' или ':' после ключа (строка %d, колонка %d)", line, col);
            break;
        }

        DataNode* val = parse_value(ps);
        if (!val) { asf_free_node(root); return NULL; }

        if (!asf_object_put(root, key, val)) {
            asf_free_node(val);
            asf_free_node(root);
            ps_error(ps, "Недостаточно памяти при добавлении пары ключ-значение");
            return NULL;
        }

        // optional comma between pairs
        ps_match(ps, TOKEN_COMMA);
    }

    if (ps->has_error) {
        asf_free_node(root);
        return NULL;
    }

    return root;
}

// ============================================================================
// Public parsing API
// ============================================================================

DataNode* asf_parse_string(const char* text) {
    char err[256];
    err[0] = '\0';

    Token* tokens = tokenize(text ? text : "", err, sizeof(err));
    if (!tokens) {
        fprintf(stderr, "Ошибка токенизации: %s\n", err[0] ? err : "unknown");
        return NULL;
    }

    Parser ps;
    memset(&ps, 0, sizeof(ps));
    ps.cur = tokens;

    DataNode* root = parse_root(&ps);
    if (!root) {
        fprintf(stderr, "Ошибка парсинга: %s\n", ps.error[0] ? ps.error : (err[0] ? err : "unknown"));
        free_tokens(tokens);
        return NULL;
    }

    if (!ps_check(&ps, TOKEN_EOF)) {
        // extra tokens
        int line = ps.cur ? ps.cur->line : -1;
        int col = ps.cur ? ps.cur->column : -1;
        fprintf(stderr, "Предупреждение: лишние данные после корневого узла (строка %d, колонка %d)\n", line, col);
    }

    free_tokens(tokens);
    return root;
}

DataNode* asf_parse_file(const char* filename) {
    if (!filename) return NULL;

    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Ошибка открытия файла %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "Ошибка позиционирования в файле %s\n", filename);
        return NULL;
    }

    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        fprintf(stderr, "Ошибка определения размера файла %s\n", filename);
        return NULL;
    }
    rewind(f);

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "Недостаточно памяти для чтения %s\n", filename);
        return NULL;
    }

    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';

    DataNode* root = asf_parse_string(buf);
    free(buf);
    return root;
}

// ============================================================================
// Free / debug
// ============================================================================

void asf_free_node(DataNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_STRING:
            free(node->value.string_value);
            break;
        case NODE_ARRAY:
            for (int i = 0; i < node->value.array.count; i++) {
                asf_free_node(node->value.array.items[i]);
            }
            free(node->value.array.items);
            break;
        case NODE_OBJECT:
            for (int i = 0; i < node->value.object.count; i++) {
                asf_free_node(node->value.object.pairs[i]);
            }
            free(node->value.object.pairs);
            break;
        case NODE_KEY_VALUE:
            free(node->key);
            asf_free_node(node->value.child);
            break;
        default:
            break;
    }

    free(node);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void asf_print_node(const DataNode* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }

    print_indent(indent);
    printf("%s", asf_node_type_to_string(node->type));

    switch (node->type) {
        case NODE_STRING:
            printf(": \"%s\"\n", node->value.string_value ? node->value.string_value : "");
            break;
        case NODE_INTEGER:
            printf(": %ld\n", node->value.int_value);
            break;
        case NODE_FLOAT:
            printf(": %g\n", node->value.float_value);
            break;
        case NODE_BOOLEAN:
            printf(": %s\n", node->value.bool_value ? "true" : "false");
            break;
        case NODE_NULL:
            printf(": null\n");
            break;
        case NODE_ARRAY:
            printf(" (%d items)\n", node->value.array.count);
            for (int i = 0; i < node->value.array.count; i++) {
                asf_print_node(node->value.array.items[i], indent + 1);
            }
            break;
        case NODE_OBJECT:
            printf(" (%d pairs)\n", node->value.object.count);
            for (int i = 0; i < node->value.object.count; i++) {
                DataNode* pair = node->value.object.pairs[i];
                if (!pair || pair->type != NODE_KEY_VALUE) continue;
                print_indent(indent + 1);
                printf("%s =\n", pair->key ? pair->key : "(no-key)");
                asf_print_node(pair->value.child, indent + 2);
            }
            break;
        case NODE_KEY_VALUE:
            printf(" (key=%s)\n", node->key ? node->key : "(no-key)");
            asf_print_node(node->value.child, indent + 1);
            break;
        default:
            printf("\n");
            break;
    }
}

// ============================================================================
// String helpers
// ============================================================================

const char* asf_token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_STRING: return "STRING";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_BOOLEAN: return "BOOLEAN";
        case TOKEN_NULL: return "NULL";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_EQUALS: return "EQUALS";
        case TOKEN_COLON: return "COLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "?";
    }
}

const char* asf_node_type_to_string(NodeType type) {
    switch (type) {
        case NODE_OBJECT: return "OBJECT";
        case NODE_ARRAY: return "ARRAY";
        case NODE_STRING: return "STRING";
        case NODE_INTEGER: return "INTEGER";
        case NODE_FLOAT: return "FLOAT";
        case NODE_BOOLEAN: return "BOOLEAN";
        case NODE_NULL: return "NULL";
        case NODE_KEY_VALUE: return "KEY_VALUE";
        default: return "?";
    }
}

