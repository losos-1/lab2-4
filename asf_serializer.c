#include "asf_parser.h"

#include <stdarg.h>

// ============================================================================
// String builder
// ============================================================================

typedef struct {
    char* buf;
    size_t len;
    size_t cap;
} StrBuf;

static int sb_reserve(StrBuf* sb, size_t add) {
    if (!sb) return 0;
    size_t need = sb->len + add + 1;
    if (need <= sb->cap) return 1;
    size_t newcap = sb->cap ? sb->cap : 1024;
    while (newcap < need) newcap *= 2;
    char* nb = (char*)realloc(sb->buf, newcap);
    if (!nb) return 0;
    sb->buf = nb;
    sb->cap = newcap;
    return 1;
}

static int sb_append_n(StrBuf* sb, const char* s, size_t n) {
    if (!sb || !s) return 0;
    if (!sb_reserve(sb, n)) return 0;
    memcpy(sb->buf + sb->len, s, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
    return 1;
}

static int sb_append(StrBuf* sb, const char* s) {
    return sb_append_n(sb, s, s ? strlen(s) : 0);
}

static int sb_append_ch(StrBuf* sb, char c) {
    if (!sb_reserve(sb, 1)) return 0;
    sb->buf[sb->len++] = c;
    sb->buf[sb->len] = '\0';
    return 1;
}

static int sb_append_fmt(StrBuf* sb, const char* fmt, ...) {
    if (!sb || !fmt) return 0;
    va_list ap;
    va_start(ap, fmt);
    char tmp[256];
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    if (n < 0) return 0;
    if ((size_t)n < sizeof(tmp)) return sb_append_n(sb, tmp, (size_t)n);

    // large
    char* big = (char*)malloc((size_t)n + 1);
    if (!big) return 0;
    va_start(ap, fmt);
    vsnprintf(big, (size_t)n + 1, fmt, ap);
    va_end(ap);
    int ok = sb_append_n(sb, big, (size_t)n);
    free(big);
    return ok;
}

// ============================================================================
// Escaping / formatting helpers
// ============================================================================

static int is_ident_start(char c) {
    unsigned char uc = (unsigned char)c;
    return isalpha(uc) || c == '_';
}

static int is_ident_part(char c) {
    unsigned char uc = (unsigned char)c;
    return isalnum(uc) || c == '_';
}

static int key_needs_quotes(const char* key) {
    if (!key || !key[0]) return 1;
    if (!is_ident_start(key[0])) return 1;
    for (const char* p = key + 1; *p; ++p) {
        if (!is_ident_part(*p)) return 1;
    }
    return 0;
}

static int sb_append_escaped_string(StrBuf* sb, const char* s) {
    if (!sb_append_ch(sb, '"')) return 0;
    if (!s) s = "";
    for (const char* p = s; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        switch (c) {
            case '"': if (!sb_append(sb, "\\\"")) return 0; break;
            case '\\': if (!sb_append(sb, "\\\\")) return 0; break;
            case '\b': if (!sb_append(sb, "\\b")) return 0; break;
            case '\f': if (!sb_append(sb, "\\f")) return 0; break;
            case '\n': if (!sb_append(sb, "\\n")) return 0; break;
            case '\r': if (!sb_append(sb, "\\r")) return 0; break;
            case '\t': if (!sb_append(sb, "\\t")) return 0; break;
            default:
                if (c < 0x20) {
                    // control -> \u00XX
                    if (!sb_append_fmt(sb, "\\u%04x", (unsigned int)c)) return 0;
                } else {
                    if (!sb_append_ch(sb, (char)c)) return 0;
                }
                break;
        }
    }
    return sb_append_ch(sb, '"');
}

static int sb_indent(StrBuf* sb, int indent) {
    for (int i = 0; i < indent; ++i) {
        if (!sb_append(sb, "  ")) return 0;
    }
    return 1;
}

// ============================================================================
// Serialization
// ============================================================================

static int ser_value(const DataNode* node, StrBuf* sb, int pretty, int indent);

static int ser_object_braced(const DataNode* node, StrBuf* sb, int pretty, int indent) {
    if (!sb_append_ch(sb, '{')) return 0;

    if (pretty) {
        if (node->value.object.count > 0) {
            if (!sb_append_ch(sb, '\n')) return 0;
        }
        for (int i = 0; i < node->value.object.count; ++i) {
            const DataNode* pair = node->value.object.pairs[i];
            if (!pair || pair->type != NODE_KEY_VALUE || !pair->key || !pair->value.child) continue;

            if (!sb_indent(sb, indent + 1)) return 0;

            if (key_needs_quotes(pair->key)) {
                if (!sb_append_escaped_string(sb, pair->key)) return 0;
            } else {
                if (!sb_append(sb, pair->key)) return 0;
            }

            if (!sb_append(sb, " = ")) return 0;
            if (!ser_value(pair->value.child, sb, pretty, indent + 1)) return 0;

            if (i < node->value.object.count - 1) {
                // запятая опциональна; в pretty режиме опускаем
            }
            if (!sb_append_ch(sb, '\n')) return 0;
        }
        if (node->value.object.count > 0) {
            if (!sb_indent(sb, indent)) return 0;
        }
        return sb_append_ch(sb, '}');
    }

    // compact
    if (node->value.object.count > 0) {
        if (!sb_append_ch(sb, ' ')) return 0;
    }
    for (int i = 0; i < node->value.object.count; ++i) {
        const DataNode* pair = node->value.object.pairs[i];
        if (!pair || pair->type != NODE_KEY_VALUE || !pair->key || !pair->value.child) continue;

        if (i > 0) {
            if (!sb_append(sb, ", ")) return 0;
        }

        if (key_needs_quotes(pair->key)) {
            if (!sb_append_escaped_string(sb, pair->key)) return 0;
        } else {
            if (!sb_append(sb, pair->key)) return 0;
        }
        if (!sb_append(sb, " = ")) return 0;
        if (!ser_value(pair->value.child, sb, pretty, indent)) return 0;
    }
    if (node->value.object.count > 0) {
        if (!sb_append_ch(sb, ' ')) return 0;
    }
    return sb_append_ch(sb, '}');
}

static int ser_array(const DataNode* node, StrBuf* sb, int pretty, int indent) {
    if (!sb_append_ch(sb, '[')) return 0;

    if (pretty) {
        if (node->value.array.count > 0) {
            if (!sb_append_ch(sb, '\n')) return 0;
        }
        for (int i = 0; i < node->value.array.count; ++i) {
            const DataNode* item = node->value.array.items[i];
            if (!item) continue;
            if (!sb_indent(sb, indent + 1)) return 0;
            if (!ser_value(item, sb, pretty, indent + 1)) return 0;
            if (i < node->value.array.count - 1) {
                // запятая опциональна; в pretty режиме опускаем
            }
            if (!sb_append_ch(sb, '\n')) return 0;
        }
        if (node->value.array.count > 0) {
            if (!sb_indent(sb, indent)) return 0;
        }
        return sb_append_ch(sb, ']');
    }

    // compact
    for (int i = 0; i < node->value.array.count; ++i) {
        const DataNode* item = node->value.array.items[i];
        if (!item) continue;
        if (i > 0) {
            if (!sb_append(sb, ", ")) return 0;
        }
        if (!ser_value(item, sb, pretty, indent)) return 0;
    }
    return sb_append_ch(sb, ']');
}

static int ser_value(const DataNode* node, StrBuf* sb, int pretty, int indent) {
    if (!node) return sb_append(sb, "null");

    switch (node->type) {
        case NODE_STRING:
            return sb_append_escaped_string(sb, node->value.string_value);
        case NODE_INTEGER:
            return sb_append_fmt(sb, "%ld", node->value.int_value);
        case NODE_FLOAT:
            // сохраняем точность, без принудительных 2 знаков
            return sb_append_fmt(sb, "%.17g", node->value.float_value);
        case NODE_BOOLEAN:
            return sb_append(sb, node->value.bool_value ? "true" : "false");
        case NODE_NULL:
            return sb_append(sb, "null");
        case NODE_ARRAY:
            return ser_array(node, sb, pretty, indent);
        case NODE_OBJECT:
            return ser_object_braced(node, sb, pretty, indent);
        case NODE_KEY_VALUE:
            // как значение выводим child
            return node->value.child ? ser_value(node->value.child, sb, pretty, indent) : sb_append(sb, "null");
        default:
            return sb_append(sb, "null");
    }
}

// ============================================================================
// Public API
// ============================================================================

char* asf_serialize_node(const DataNode* node, int pretty) {
    if (!node) return NULL;

    StrBuf sb;
    memset(&sb, 0, sizeof(sb));

    // Корневой объект выводим как список пар (без внешних фигурных скобок)
    if (node->type == NODE_OBJECT) {
        if (pretty) {
            for (int i = 0; i < node->value.object.count; ++i) {
                const DataNode* pair = node->value.object.pairs[i];
                if (!pair || pair->type != NODE_KEY_VALUE || !pair->key || !pair->value.child) continue;

                if (!sb_indent(&sb, 0)) { free(sb.buf); return NULL; }

                if (key_needs_quotes(pair->key)) {
                    if (!sb_append_escaped_string(&sb, pair->key)) { free(sb.buf); return NULL; }
                } else {
                    if (!sb_append(&sb, pair->key)) { free(sb.buf); return NULL; }
                }

                if (!sb_append(&sb, " = ")) { free(sb.buf); return NULL; }
                if (!ser_value(pair->value.child, &sb, pretty, 0)) { free(sb.buf); return NULL; }

                if (!sb_append_ch(&sb, '\n')) { free(sb.buf); return NULL; }
            }
        } else {
            for (int i = 0; i < node->value.object.count; ++i) {
                const DataNode* pair = node->value.object.pairs[i];
                if (!pair || pair->type != NODE_KEY_VALUE || !pair->key || !pair->value.child) continue;

                if (i > 0) {
                    if (!sb_append(&sb, "; ")) { free(sb.buf); return NULL; }
                }

                if (key_needs_quotes(pair->key)) {
                    if (!sb_append_escaped_string(&sb, pair->key)) { free(sb.buf); return NULL; }
                } else {
                    if (!sb_append(&sb, pair->key)) { free(sb.buf); return NULL; }
                }

                if (!sb_append(&sb, " = ")) { free(sb.buf); return NULL; }
                if (!ser_value(pair->value.child, &sb, pretty, 0)) { free(sb.buf); return NULL; }
            }
        }
    } else {
        if (!ser_value(node, &sb, pretty, 0)) { free(sb.buf); return NULL; }
        if (pretty) {
            sb_append_ch(&sb, '\n');
        }
    }

    if (!sb.buf) {
        sb.buf = (char*)malloc(1);
        if (!sb.buf) return NULL;
        sb.buf[0] = '\0';
    }
    return sb.buf;
}

int asf_save_file(const char* filename, const DataNode* node, int pretty) {
    if (!filename || !node) return 0;

    char* text = asf_serialize_node(node, pretty);
    if (!text) return 0;

    FILE* f = fopen(filename, "w");
    if (!f) {
        free(text);
        return 0;
    }

    // Заголовок (комментарий допустим по ТЗ)
    fputs("// AutoService data file (ASF)\n", f);
    fputs("// Generated by program\n\n", f);

    fputs(text, f);
    fclose(f);
    free(text);
    return 1;
}
