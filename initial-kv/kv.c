#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kv.h"

struct data {
    char *key;
    char *value;
    struct data *next;
};

typedef void entryprocessor(struct data *data, struct data **prev);

// No error checks no bs

void kv_init(struct kv *kv, char *dbpath) {
    FILE *fp;
    char *line = NULL;
    size_t linelen = 0;
    struct data *new;
    struct data **next;

    kv->data = NULL;
    kv->dbpath = dbpath == NULL ? "kvdata.text" : dbpath;
    fp = fopen(kv->dbpath, "r");
    next = &kv->data;
    while (getline(&line, &linelen, fp) != -1) {
        struct data *new = malloc(sizeof(struct data));
        new->next = NULL;
        new->value = strdup(line);
        new->value[strlen(new->value) - 1] = 0;
        new->key = strsep(&new->value, ",");
        new->value = strdup(new->value);
        *next = new;
        next = &new->next;
    }
    free(line);
    fclose(fp);
}

void kv_free(struct kv *kv) {
    struct data *data;
    struct data *datanext;

    data = kv->data;
    while(data) {
        datanext = data->next;
        free(data->key);
        free(data);
        data = datanext;
    }
}

void persistall(struct kv *kv) {
    struct data *data = kv->data;
    FILE *fp = fopen(kv->dbpath, "w");
    while (data) {
        fprintf(fp, "%s,%s\n", data->key, data->value);
        data = data->next;
    }
    fclose(fp);
}

// Callers make sure key and value does not contain
// any commas or newlines.
void kv_put(struct kv *kv, char *key, char *value) {
    struct data *data = kv->data;

    while (data) {
        if (!strcmp(key, data->key)) {
            break;
        }
        data = data->next;
    }

    if (data) {
        free(data->value);
        data->value = value;
    } else {
        struct data *new = malloc(sizeof(struct data));
        new->next = kv->data;
        kv->data = new;
        new->key = strdup(key);
        new->value = strdup(value);
    }

    persistall(kv);
}

char* kv_get(struct kv *kv, char *key) {
    struct data *data = kv->data;

    while (data) {
        if (!strcmp(key, data->key)) {
            break;
        }
        data = data->next;
    }

    if (data) {
        return data->value;
    } else {
        return NULL;
    }
}

struct data* callwhenfound(struct data **data0, char *key, entryprocessor* fn) {
    struct data *data = *data0;
    struct data **dataprev = data0;

    while (data) {
        if (!strcmp(key, data->key)) {
            break;
        }
        dataprev = &data->next;
        data = data->next;
    }

    if (data) {
        fn(data, dataprev);
    }

    return data;
}

void callforeach(struct data *d, callable *fn) {
    while (d) {
        fn(d->key, d->value);
        d = d->next;
    }
}

void deletentry(struct data *d, struct data **prev) {
    *prev = d->next;
    free(d->key);
    free(d->value);
    free(d);
}

int kv_delete(struct kv* kv, char *key) {
    struct data *data = callwhenfound(&kv->data, key, &deletentry);
    if (!data) {
        return -1;
    }
    persistall(kv);
    return 0;
}

void kv_callforeach(struct kv* kv, callable *fn) {
    callforeach(kv->data, fn);
}

void kv_clear(struct kv *kv) {
    kv_free(kv);
    kv->data = NULL;
    persistall(kv);
}
