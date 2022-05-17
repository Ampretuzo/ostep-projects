struct data;
struct kv {
    char *dbpath;
    struct data *data;
};
typedef void callable(char *key, char *value);

void kv_init(struct kv*, char *dbpath);
void kv_free(struct kv*);
void kv_put(struct kv*, char *key, char *value);
char* kv_get(struct kv*, char *key);
int kv_delete(struct kv*, char *key);
void kv_callforeach(struct kv*, callable*);
void kv_clear(struct kv*);
