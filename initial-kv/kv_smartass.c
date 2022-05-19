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
typedef int processor(void *ctx, struct data *data, struct data **prev);

// No error checks no bs

struct data* callforeachuntilfound(struct data **nextaddr, processor *fn, void *ctx) {
    struct data *data = *nextaddr;

    while (data) {
        if(fn(ctx, data, nextaddr)) {
            return data;
        }
        nextaddr = &data->next;
        data = data->next;
    }

    return NULL;
}

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
        free(data->value);
        free(data);
        data = datanext;
    }
}

int writeentry(void *ctx, struct data *data, struct data **_) {
    fprintf((FILE*) ctx, "%s,%s\n", data->key, data->value);
    return 0;
}

void persistall(struct kv *kv) {
    FILE *fp = fopen(kv->dbpath, "w");
    callforeachuntilfound(&kv->data, writeentry, fp);
    fclose(fp);
}

struct kvpair {
    char *key;
    char *value;
};

int overwrite(void *ctx, struct data *data, struct data **_) {
    struct kvpair *pair = (struct kvpair*) ctx;
    if (!strcmp(data->key, pair->key)) {
        free(data->value);
        data->value = strdup(pair->value);
        return 1;
    }
    return 0;
}

// Callers make sure key and value does not contain
// any commas or newlines.
void kv_put(struct kv *kv, char *key, char *value) {
    struct kvpair pair = {key, value};

    if (!callforeachuntilfound(&kv->data, overwrite, &pair)) {
        struct data *new = malloc(sizeof(struct data));
        new->next = kv->data;
        kv->data = new;
        new->key = strdup(key);
        new->value = strdup(value);
    }

    persistall(kv);
}

int comparekey(void *ctx, struct data *data, struct data **prev) {
    if (!strcmp((char*) ctx, data->key)) {
        return 1;
    }
    return 0;
}

char* kv_get(struct kv *kv, char *key) {
    struct data *found = callforeachuntilfound(&kv->data, comparekey, key);
    if (!found) {
        return NULL;
    }
    return found->value;
}

int deleteentry(void *ctx, struct data *data, struct data **prev) {
    if (!strcmp((char*) ctx, data->key)) {
        *prev = data->next;
        free(data->key);
        free(data->value);
        free(data);
        return 1;
    }
    return 0;
}

int kv_delete(struct kv* kv, char *key) {
    if(!callforeachuntilfound(&kv->data, deleteentry, key)) {
        return -1;
    }
    persistall(kv);
    return 0;
}

int callcallable(void *ctx, struct data *data, struct data **_) {
    ((callable*) ctx)(data->key, data->value);
    return 0;
}

void kv_callforeach(struct kv* kv, callable *fn) {
    callforeachuntilfound(&kv->data, callcallable, fn);
}

void kv_clear(struct kv *kv) {
    kv_free(kv);
    kv->data = NULL;
    persistall(kv);
}
