#include "data_adapter.h"

#include <time.h>

// ============================================================================
// Helpers
// ============================================================================

static DataNode* make_record_object(const technical_maintenance* r) {
    if (!r) return NULL;

    DataNode* obj = asf_node_create(NODE_OBJECT);
    if (!obj) return NULL;

    if (!asf_object_put(obj, "id", asf_node_integer(r->id)) ||
        !asf_object_put(obj, "date", asf_node_string(r->date)) ||
        !asf_object_put(obj, "type_work", asf_node_string(r->type_work)) ||
        !asf_object_put(obj, "mileage", asf_node_integer(r->mileage)) ||
        !asf_object_put(obj, "price", asf_node_float(r->price))) {
        asf_free_node(obj);
        return NULL;
    }

    return obj;
}

// Поиск значения по ключу в объекте
DataNode* find_node_by_key(DataNode* object, const char* key) {
    if (!object || !key) return NULL;

    if (object->type == NODE_KEY_VALUE) {
        object = object->value.child;
    }

    if (!object || object->type != NODE_OBJECT) return NULL;

    for (int i = 0; i < object->value.object.count; i++) {
        DataNode* pair = object->value.object.pairs[i];
        if (pair && pair->type == NODE_KEY_VALUE && pair->key && strcmp(pair->key, key) == 0) {
            return pair->value.child;
        }
    }
    return NULL;
}

const DataNode* find_node_by_key_const(const DataNode* object, const char* key) {
    return (const DataNode*)find_node_by_key((DataNode*)object, key);
}

// ============================================================================
// Convert business -> AST
// ============================================================================

DataNode* technical_maintenance_to_asf(const technical_maintenance* records, int count) {
    if (!records || count < 0) return NULL;

    DataNode* arr = asf_node_create(NODE_ARRAY);
    if (!arr) return NULL;

    for (int i = 0; i < count; i++) {
        DataNode* obj = make_record_object(&records[i]);
        if (!obj) {
            asf_free_node(arr);
            return NULL;
        }
        if (!asf_array_add(arr, obj)) {
            asf_free_node(obj);
            asf_free_node(arr);
            return NULL;
        }
    }

    return arr;
}

DataNode* create_metadata(const char* username, int record_count) {
    DataNode* meta = asf_node_create(NODE_OBJECT);
    if (!meta) return NULL;

    // timestamp
    time_t now = time(NULL);
    struct tm* ti = localtime(&now);
    char stamp[64];
    if (ti) {
        strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", ti);
    } else {
        strcpy(stamp, "unknown");
    }

    if (!asf_object_put(meta, "version", asf_node_string("1.0")) ||
        !asf_object_put(meta, "created", asf_node_string(stamp)) ||
        !asf_object_put(meta, "user", asf_node_string(username ? username : "unknown")) ||
        !asf_object_put(meta, "record_count", asf_node_integer(record_count))) {
        asf_free_node(meta);
        return NULL;
    }

    return meta;
}

DataNode* database_to_asf(const data_base* db, const char* username) {
    if (!db) return NULL;

    DataNode* root = asf_node_create(NODE_OBJECT);
    if (!root) return NULL;

    DataNode* meta = create_metadata(username, db->size);
    DataNode* records = technical_maintenance_to_asf(db->records, db->size);

    if (!meta || !records) {
        asf_free_node(meta);
        asf_free_node(records);
        asf_free_node(root);
        return NULL;
    }

    if (!asf_object_put(root, "metadata", meta) ||
        !asf_object_put(root, "records", records)) {
        asf_free_node(root);
        return NULL;
    }

    return root;
}

// ============================================================================
// Convert AST -> business
// ============================================================================

static int node_to_long(const DataNode* n, long* out) {
    if (!n || !out) return 0;
    if (n->type == NODE_INTEGER) { *out = n->value.int_value; return 1; }
    if (n->type == NODE_FLOAT) { *out = (long)n->value.float_value; return 1; }
    return 0;
}

static int node_to_double(const DataNode* n, double* out) {
    if (!n || !out) return 0;
    if (n->type == NODE_FLOAT) { *out = n->value.float_value; return 1; }
    if (n->type == NODE_INTEGER) { *out = (double)n->value.int_value; return 1; }
    return 0;
}

static const char* node_to_cstr(const DataNode* n) {
    if (!n) return NULL;
    if (n->type == NODE_STRING) return n->value.string_value;
    return NULL;
}

static int convert_asf_to_record(const DataNode* obj, technical_maintenance* r) {
    if (!obj || obj->type != NODE_OBJECT || !r) return 0;

    memset(r, 0, sizeof(*r));

    const DataNode* idn = find_node_by_key_const(obj, "id");
    const DataNode* daten = find_node_by_key_const(obj, "date");
    const DataNode* typen = find_node_by_key_const(obj, "type_work");
    const DataNode* milen = find_node_by_key_const(obj, "mileage");
    const DataNode* pricen = find_node_by_key_const(obj, "price");

    long idv = 0;
    if (node_to_long(idn, &idv)) r->id = (int)idv;

    const char* ds = node_to_cstr(daten);
    if (ds) {
        strncpy(r->date, ds, sizeof(r->date) - 1);
        r->date[sizeof(r->date) - 1] = '\0';
    }

    const char* ts = node_to_cstr(typen);
    if (ts) {
        strncpy(r->type_work, ts, sizeof(r->type_work) - 1);
        r->type_work[sizeof(r->type_work) - 1] = '\0';
    }

    long mv = 0;
    if (node_to_long(milen, &mv)) r->mileage = (int)mv;

    double pv = 0.0;
    if (node_to_double(pricen, &pv)) r->price = (float)pv;

    return 1;
}

technical_maintenance* asf_to_technical_maintenance(const DataNode* node, int* count) {
    if (!count) return NULL;
    *count = 0;
    if (!node || node->type != NODE_ARRAY) return NULL;

    int n = node->value.array.count;
    if (n <= 0) return NULL;

    technical_maintenance* recs = (technical_maintenance*)malloc((size_t)n * sizeof(technical_maintenance));
    if (!recs) return NULL;

    int ok = 0;
    for (int i = 0; i < n; i++) {
        const DataNode* item = node->value.array.items[i];
        if (convert_asf_to_record(item, &recs[ok])) ok++;
    }

    if (ok == 0) {
        free(recs);
        return NULL;
    }

    *count = ok;
    return recs;
}

static const DataNode* unwrap_database_root(const DataNode* root) {
    if (!root) return NULL;
    if (root->type != NODE_OBJECT) return NULL;

    // Совместимость: если есть ключ "database" и это объект, используем его
    const DataNode* db = find_node_by_key_const(root, "database");
    if (db && db->type == NODE_OBJECT) return db;

    return root;
}

data_base* asf_to_database(const DataNode* root) {
    const DataNode* base = unwrap_database_root(root);
    if (!base) return NULL;

    const DataNode* records = find_node_by_key_const(base, "records");
    if (!records || records->type != NODE_ARRAY) return NULL;

    data_base* db = (data_base*)malloc(sizeof(data_base));
    if (!db) return NULL;

    int init_cap = records->value.array.count;
    if (init_cap < 10) init_cap = 10;
    init_system(db, init_cap);

    for (int i = 0; i < records->value.array.count; i++) {
        technical_maintenance rec;
        if (convert_asf_to_record(records->value.array.items[i], &rec)) {
            add_item(db, rec);
        }
    }

    return db;
}
