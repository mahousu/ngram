int fnv_32_buf_range(void *buf, size_t len, Fnv32_t hval, int low, int high, Fnv32_t *hvalout);
int fnv_32_str_range(char *str, Fnv32_t hval, int low, int high, Fnv32_t *hvalout);

int fnv_64_buf_range(void *buf, size_t len, Fnv64_t hval, int low, int high, Fnv64_t *hvalout);
int fnv_64_str_range(char *str, Fnv64_t hval, int low, int high, Fnv64_t *hvalout);
