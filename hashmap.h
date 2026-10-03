#ifndef HASHMAP_H
#define HASHMAP_H

#include <pthread.h>

typedef struct HashMap HashMap;

HashMap *hashmap_create(void);

void hashmap_destroy(HashMap *map);

int hashmap_get_or_create(HashMap *map, const char *key, pthread_mutex_t **out);

int hashmap_remove(HashMap *map, const char *key);

#endif
