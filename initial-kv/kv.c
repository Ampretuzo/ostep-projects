#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kv.h"

struct data {
    char *key;
    char *value;
    struct data *next;
};

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
        memmove(next, &new, sizeof(struct data *));
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

void kv_put(struct kv *kv, char *key, char *value) {
    struct data *data = kv->data;
    printf("Someone wants to write '%s' for key '%s'\n", value, key);
    printf("Printing existing entries:\n");
    while(data) {
        printf("Key: '%s', Value: '%s'\n", data->key, data->value);
        data = data->next;
    }
}
