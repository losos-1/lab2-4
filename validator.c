#include "validator.h"

#include <stdarg.h>

static void set_err(char* buf, size_t cap, const char* fmt, ...) {
    if (!buf || cap == 0) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    buf[cap - 1] = '\0';
}

static int validate_ast_rec(const DataNode* n, char* err, size_t cap, int depth) {
    if (!n) {
        set_err(err, cap, "AST: null node");
        return 0;
    }
    if (depth > 1024) {
        set_err(err, cap, "AST: too deep (possible cycle)");
        return 0;
    }

    switch (n->type) {
        case NODE_STRING:
            // string_value может быть NULL (трактуем как "")
            return 1;
        case NODE_INTEGER:
        case NODE_FLOAT:
        case NODE_BOOLEAN:
        case NODE_NULL:
            return 1;

        case NODE_ARRAY: {
            if (n->value.array.count < 0 || n->value.array.capacity < 0) {
                set_err(err, cap, "AST: invalid array counters");
                return 0;
            }
            for (int i = 0; i < n->value.array.count; i++) {
                if (!n->value.array.items || !n->value.array.items[i]) {
                    set_err(err, cap, "AST: null array item at %d", i);
                    return 0;
                }
                if (!validate_ast_rec(n->value.array.items[i], err, cap, depth + 1)) return 0;
            }
            return 1;
        }

        case NODE_OBJECT: {
            if (n->value.object.count < 0 || n->value.object.capacity < 0) {
                set_err(err, cap, "AST: invalid object counters");
                return 0;
            }
            for (int i = 0; i < n->value.object.count; i++) {
                if (!n->value.object.pairs || !n->value.object.pairs[i]) {
                    set_err(err, cap, "AST: null pair at %d", i);
                    return 0;
                }
                const DataNode* p = n->value.object.pairs[i];
                if (p->type != NODE_KEY_VALUE) {
                    set_err(err, cap, "AST: object contains non-pair at %d", i);
                    return 0;
                }
                if (!p->key || !p->key[0]) {
                    set_err(err, cap, "AST: pair without key at %d", i);
                    return 0;
                }
                if (!p->value.child) {
                    set_err(err, cap, "AST: pair '%s' has null value", p->key);
                    return 0;
                }

                // check duplicate keys (строго)
                for (int j = i + 1; j < n->value.object.count; j++) {
                    const DataNode* q = n->value.object.pairs[j];
                    if (q && q->type == NODE_KEY_VALUE && q->key && strcmp(q->key, p->key) == 0) {
                        set_err(err, cap, "AST: duplicate key '%s'", p->key);
                        return 0;
                    }
                }

                if (!validate_ast_rec(p->value.child, err, cap, depth + 1)) return 0;
            }
            return 1;
        }

        case NODE_KEY_VALUE:
            if (!n->key || !n->key[0]) {
                set_err(err, cap, "AST: key-value without key");
                return 0;
            }
            if (!n->value.child) {
                set_err(err, cap, "AST: key '%s' without value", n->key);
                return 0;
            }
            return validate_ast_rec(n->value.child, err, cap, depth + 1);

        default:
            set_err(err, cap, "AST: unknown node type");
            return 0;
    }
}

int asf_validate_node(const DataNode* node, char* error, size_t error_cap) {
    return validate_ast_rec(node, error, error_cap, 0);
}

static const DataNode* object_get(const DataNode* obj, const char* key) {
    if (!obj || obj->type != NODE_OBJECT) return NULL;
    for (int i = 0; i < obj->value.object.count; i++) {
        const DataNode* p = obj->value.object.pairs[i];
        if (p && p->type == NODE_KEY_VALUE && p->key && strcmp(p->key, key) == 0) {
            return p->value.child;
        }
    }
    return NULL;
}

static const DataNode* unwrap_db_root(const DataNode* root) {
    if (!root || root->type != NODE_OBJECT) return NULL;
    const DataNode* db = object_get(root, "database");
    if (db && db->type == NODE_OBJECT) return db;
    return root;
}

int asf_validate_database_ast(const DataNode* root, char* error, size_t error_cap) {
    if (!asf_validate_node(root, error, error_cap)) return 0;

    const DataNode* base = unwrap_db_root(root);
    if (!base) {
        set_err(error, error_cap, "schema: root must be an object");
        return 0;
    }

    const DataNode* meta = object_get(base, "metadata");
    const DataNode* recs = object_get(base, "records");

    if (!meta || meta->type != NODE_OBJECT) {
        set_err(error, error_cap, "schema: 'metadata' must be an object");
        return 0;
    }
    if (!recs || recs->type != NODE_ARRAY) {
        set_err(error, error_cap, "schema: 'records' must be an array");
        return 0;
    }

    // records elements: object
    for (int i = 0; i < recs->value.array.count; i++) {
        const DataNode* item = recs->value.array.items[i];
        if (!item || item->type != NODE_OBJECT) {
            set_err(error, error_cap, "schema: records[%d] must be an object", i);
            return 0;
        }
    }

    // optional: record_count type
    const DataNode* rc = object_get(meta, "record_count");
    if (rc && rc->type != NODE_INTEGER) {
        set_err(error, error_cap, "schema: metadata.record_count must be integer");
        return 0;
    }

    return 1;
}
