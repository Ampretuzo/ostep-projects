struct data;
struct kv {
    char *dbpath;
    struct data *data;
};
void kv_init(struct kv*, char *dbpath);
void kv_free(struct kv*);
void kv_put(struct kv*, char *key, char *value);
