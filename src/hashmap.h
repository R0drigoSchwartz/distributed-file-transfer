/*Authors: Rodrigo Schwartz (R0drigoSchwartz) and Vinicius Henrique Ribeiro (vini-ribeiro)*/

#ifndef HASHMAP_H
#define HASHMAP_H

#include <pthread.h>
#include <stdbool.h>

typedef struct HashMap HashMap;

HashMap *hashmap_create(void);

// Call only after all users have released their references
void hashmap_destroy(HashMap *map);

// Each successful call requires one release, after unlocking the mutex
// Waiting threads also hold references
int hashmap_get_or_create(HashMap *map, const char *key, pthread_mutex_t **out);

// complete requests removal when the last reference is released
int hashmap_release(HashMap *map, const char *key, bool complete);

// Requests removal
int hashmap_remove(HashMap *map, const char *key);

#endif
