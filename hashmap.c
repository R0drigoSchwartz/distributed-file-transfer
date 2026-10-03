/*Authors: Rodrigo Schwartz (R0drigoSchwartz) and Vinicius Henrique Ribeiro (vini-ribeiro)*/

#include "hashmap.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 64

typedef struct HashMapEntry {
    char *key;
    size_t hash;
    pthread_mutex_t mutex;
    size_t references;
    bool remove_pending;
    struct HashMapEntry *next;
} HashMapEntry;

struct HashMap {
    HashMapEntry **buckets;
    size_t capacity;
    size_t size;
    pthread_mutex_t table_mutex;
};

static size_t hash_key(const char *key) {
    size_t hash = 5381;
    const unsigned char *cursor = (const unsigned char *)key;

    while (*cursor != '\0') {
        hash = hash * 33 + *cursor++;
    }
    return hash;
}

/* Lookup and growth helpers require table_mutex to be held. */
static HashMapEntry *find_entry(HashMap *map, const char *key, size_t hash) {
    HashMapEntry *entry = map->buckets[hash % map->capacity];

    while (entry != NULL) {
        if (entry->hash == hash && strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static int grow(HashMap *map) {
    if (map->capacity > SIZE_MAX / 2) {
        return ENOMEM;
    }
    size_t capacity = map->capacity * 2;
    if (capacity > SIZE_MAX / sizeof(*map->buckets)) {
        return ENOMEM;
    }

    HashMapEntry **buckets = calloc(capacity, sizeof(*buckets));
    if (buckets == NULL) {
        return ENOMEM;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry *entry = map->buckets[i];
        while (entry != NULL) {
            HashMapEntry *next = entry->next;
            size_t bucket = entry->hash % capacity;
            entry->next = buckets[bucket];
            buckets[bucket] = entry;
            entry = next;
        }
    }

    free(map->buckets);
    map->buckets = buckets;
    map->capacity = capacity;
    return 0;
}

HashMap *hashmap_create(void) {
    HashMap *map = calloc(1, sizeof(*map));
    if (map == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    map->capacity = INITIAL_CAPACITY;
    map->buckets = calloc(map->capacity, sizeof(*map->buckets));
    if (map->buckets == NULL) {
        free(map);
        errno = ENOMEM;
        return NULL;
    }

    int error = pthread_mutex_init(&map->table_mutex, NULL);
    if (error != 0) {
        free(map->buckets);
        free(map);
        errno = error;
        return NULL;
    }
    return map;
}

void hashmap_destroy(HashMap *map) {
    if (map == NULL) {
        return;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry *entry = map->buckets[i];
        while (entry != NULL) {
            HashMapEntry *next = entry->next;
            pthread_mutex_destroy(&entry->mutex);
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    pthread_mutex_destroy(&map->table_mutex);
    free(map->buckets);
    free(map);
}

int hashmap_get_or_create(HashMap *map, const char *key, pthread_mutex_t **out) {
    if (out == NULL) {
        return EINVAL;
    }
    *out = NULL;
    if (map == NULL || key == NULL) {
        return EINVAL;
    }

    int error = pthread_mutex_lock(&map->table_mutex);
    if (error != 0) {
        return error;
    }

    size_t hash = hash_key(key);
    HashMapEntry *entry = find_entry(map, key, hash);
    if (entry != NULL) {
        if (entry->references == SIZE_MAX) {
            pthread_mutex_unlock(&map->table_mutex);
            return EOVERFLOW;
        }
        entry->references++;
        *out = &entry->mutex;
        pthread_mutex_unlock(&map->table_mutex);
        return 0;
    }

    entry = malloc(sizeof(*entry));
    if (entry == NULL) {
        pthread_mutex_unlock(&map->table_mutex);
        return ENOMEM;
    }
    size_t key_size = strlen(key) + 1;
    entry->key = malloc(key_size);
    if (entry->key == NULL) {
        free(entry);
        pthread_mutex_unlock(&map->table_mutex);
        return ENOMEM;
    }
    memcpy(entry->key, key, key_size);
    entry->hash = hash;
    entry->references = 1;
    entry->remove_pending = false;

    error = pthread_mutex_init(&entry->mutex, NULL);
    if (error == 0 && map->size >= map->capacity - map->capacity / 4) {
        error = grow(map);
        if (error != 0) {
            pthread_mutex_destroy(&entry->mutex);
        }
    }
    if (error != 0) {
        free(entry->key);
        free(entry);
        pthread_mutex_unlock(&map->table_mutex);
        return error;
    }

    size_t bucket = hash % map->capacity;
    entry->next = map->buckets[bucket];
    map->buckets[bucket] = entry;
    map->size++;
    *out = &entry->mutex;
    pthread_mutex_unlock(&map->table_mutex);
    return 0;
}

static int release_entry(HashMap *map, const char *key, bool release_reference, bool remove_when_idle) {
    if (map == NULL || key == NULL) {
        return EINVAL;
    }
    int error = pthread_mutex_lock(&map->table_mutex);
    if (error != 0) {
        return error;
    }

    size_t hash = hash_key(key);
    HashMapEntry **link = &map->buckets[hash % map->capacity];
    while (*link != NULL) {
        HashMapEntry *entry = *link;
        if (entry->hash == hash && strcmp(entry->key, key) == 0) {
            if (release_reference) {
                if (entry->references == 0) {
                    pthread_mutex_unlock(&map->table_mutex);
                    return EINVAL;
                }
                entry->references--;
            }
            entry->remove_pending |= remove_when_idle;
            if (entry->remove_pending && entry->references == 0) {
                error = pthread_mutex_destroy(&entry->mutex);
                if (error == 0) {
                    *link = entry->next;
                    free(entry->key);
                    free(entry);
                    map->size--;
                }
            }
            pthread_mutex_unlock(&map->table_mutex);
            return error;
        }
        link = &entry->next;
    }

    pthread_mutex_unlock(&map->table_mutex);
    return ENOENT;
}

int hashmap_release(HashMap *map, const char *key, bool complete) {
    return release_entry(map, key, true, complete);
}

int hashmap_remove(HashMap *map, const char *key) {
    return release_entry(map, key, false, true);
}
