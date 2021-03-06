#include "map.h"
#include "list.h"

/*
 * create a new rbtree.
 * 
 * same with "struct rbtree *tree = NULL;"
 *
 * @return a NULL pointer.
 */
struct map *map_new(int (*keycmp)(void *key1, void *key2)) {
    struct map *map = malloc(sizeof(struct map));
    map->entries = NULL;
    map->keycmp = keycmp;
    return map;
}

/*
 * test if map contains the key.
 */
bool map_has(struct map *map, void *key) {
    struct map_entry *p = map->entries;
    while (p != NULL) {
        int c = map->keycmp(key, p->key);
        if (c == 0) {
            return true;
        } else if (c < 0) {
            p = p->left;
        } else if (c > 0) {
            p = p->right;
        }
    }
    return false;
}

static struct map_entry *get_grandparent(struct map_entry *entry) {
    return entry->parent ? entry->parent->parent : NULL;
}

static struct map_entry *get_parent(struct map_entry *entry) {
    return entry->parent;
}

static struct map_entry *get_uncle(struct map_entry *entry) {
    if (entry->parent && entry->parent->parent) {
        return entry->parent != entry->parent->parent->left ? entry->parent->parent->left : entry->parent->parent->right;
    }
    return NULL;
}

static struct map_entry *get_sibling(struct map_entry *entry) {
    if (entry->parent) {
        return entry == entry->parent->left ? entry->parent->right : entry->parent->left;
    }
}

static void rotate_left(struct map *map, struct map_entry *entry) {
    if (entry->right->left) {
        entry->right->left->parent = entry;
    }
    entry->right->parent = entry->parent;
    entry->parent = entry->right;
    entry->right = entry->parent->left;
    entry->parent->left = entry;

    if (entry->parent->parent == NULL) {
        map->entries = entry->parent;
    } else if (entry == entry->parent->parent->left) {
        entry->parent->parent->left = entry->parent;
    } else {
        entry->parent->parent->right = entry->parent;
    }
}

static void rotate_right(struct map *map, struct map_entry *entry) {
    if (entry->left->right) {
        entry->left->right->parent = entry;
    }
    entry->left->parent = entry->parent;
    entry->parent = entry->left;
    entry->left = entry->parent->right;
    entry->parent->right = entry;

    if (entry->parent->parent == NULL) {
        map->entries = entry->parent;
    } else if (entry == entry->parent->parent->left) {
        entry->parent->parent->left = entry->parent;
    } else {
        entry->parent->parent->right = entry->parent;
    }
}

static void map_add_internal(struct map *map, struct map_entry *entry) {
    struct map_entry *grandparent = get_grandparent(entry);
    struct map_entry *parent = get_parent(entry);
    struct map_entry *uncle = get_uncle(entry);

    if (parent == NULL) {
        entry->color = BLACK;
    } else if (parent->color == RED ) {
        if(uncle && uncle->color == RED) {
            parent->color = BLACK;
            uncle->color = BLACK;
            grandparent->color = RED;
            map_add_internal(map, grandparent);
        } else {
            // no uncle or uncle's color is BLACK
            if (parent == grandparent->left) {
                if (entry == parent->right) {
                    // rotate parent
                    rotate_left(map, parent);
                    parent = parent->parent;
                }
                // rotate grandparent.
                parent->color = BLACK;
                grandparent->color = RED;
                rotate_right(map, grandparent);
            } else {
                if (entry == parent->left) {
                    rotate_right(map, parent);
                    parent = parent->parent;
                }
                parent->color = BLACK;
                grandparent->color = RED;
                rotate_left(map, grandparent);
            }
        }
    }
}

/*
 * add key value pairs to map.
 *
 * before calling this function, must call map_has() to check if key
 * already exisits.
 */
bool map_add(struct map *map, void *key, void *data) {
    struct map_entry *p = map->entries;
    int c;
    while (p != NULL) {
        c = map->keycmp(key, p->key);
        if (c == 0) {
            return false;
        } else if (c < 0 && p->left != NULL) {
            p = p->left;
        } else if (c > 0 && p->right != NULL) {
            p = p->right;
        } else {
            break;
        }
    }
    struct map_entry *entry = malloc(sizeof(struct map_entry));
    entry->key = key;
    entry->data = data;
    entry->parent = p;
    entry->left = NULL;
    entry->right = NULL;
    entry->color = RED;
    if (p == NULL) {
        map->entries = entry;
    } else if (c < 0) {
        p->left = entry;
    } else {
        p->right = entry;
    }
    map_add_internal(map, entry);
    return true;
}

static struct map_entry *get_min(struct map_entry *entry) {
    while (entry->left) {
        entry = entry->left;
    }
    return entry;
}

static void free_entry(struct map_entry *entry, void **orig_key, void **orig_data) {
    *orig_key = entry->key;
    *orig_data = entry->data;
    free(entry);
}

enum color color(struct map_entry *entry) {
    if (entry) {
        return entry->color;
    }
    return BLACK;
}

static void map_remove_fixup(struct map *map, struct map_entry *entry) {
    struct map_entry *grandparent = get_grandparent(entry);
    struct map_entry *parent = get_parent(entry);
    struct map_entry *uncle = get_uncle(entry);
    struct map_entry *sibling = get_sibling(entry);

    if (parent == NULL) {
        entry->color = BLACK; // really nead?
        return;
    }
    if (sibling->color == BLACK) {
        if (color(sibling->left) == BLACK && color(sibling->right) == BLACK) {
            if (parent->color == RED) {
                parent->color = BLACK;
                sibling->color = RED;
            } else {
                sibling->color = RED;
                map_remove_fixup(map, parent);
            }
        } else if (entry == parent->left && color(sibling->left) == RED && color(sibling->right) == BLACK) {
            rotate_right(map, sibling);
            sibling->color = RED;
            sibling->parent->color = BLACK;
            map_remove_fixup(map, entry);
        } else if (entry == parent->right && color(sibling->right) == RED && color(sibling->left) == BLACK) {
            rotate_left(map, sibling);
            sibling->color = RED;
            sibling->parent->color = BLACK;
            map_remove_fixup(map, entry);
        } else if (entry == parent->left && color(sibling->right) == RED) {
            rotate_left(map, parent);
            enum color tmp = parent->color;
            parent->color = sibling->color;
            sibling->color = tmp;
            sibling->right->color = BLACK;
        } else if (entry == parent->right && color(sibling->left) == RED) {
            rotate_right(map, parent);
            enum color tmp = parent->color;
            parent->color = sibling->color;
            sibling->color = tmp;
            sibling->left->color = BLACK;
        }
    } else {
        sibling->color = BLACK;
        parent->color = RED;
        if (entry == parent->left) {
            rotate_left(map, parent);
        } else {
            rotate_right(map, parent);
        }
        map_remove_fixup(map, entry);
    }
}

static void map_remove_child(struct map *map, struct map_entry *entry, void **orig_key, void **orig_data) {
    // root without children
    if (entry->parent == NULL && entry->left == NULL && entry->right == NULL) {
        map->entries = NULL;
        free_entry(entry, orig_key, orig_data);
        return;
    }

    // a red leaf
    if (entry->color == RED) {
        if (entry == entry->parent->left) {
            entry->parent->left = NULL;
        } else {
            entry->parent->right = NULL;
        }
        free_entry(entry, orig_key, orig_data);
        return;
    }

    // black node with a red leaf
    if (entry->left != NULL || entry->right != NULL) {
        struct map_entry *child = entry->left != NULL ? entry->left : entry->right;
        child->parent = entry->parent;
        child->color = BLACK;
        if (child->parent == NULL) { // new root
            map->entries = child;
        } else {
            if (entry == entry->parent->left) {
                entry->parent->left = child;
            } else {
                entry->parent->right = child;
            }
        }
        free_entry(entry, orig_key, orig_data);
        return;
    }

    // non root black node without children
    map_remove_fixup(map, entry);

    if (entry == entry->parent->left) {
        entry->parent->left = NULL;
    } else {
        entry->parent->right = NULL;
    }
    free_entry(entry, orig_key, orig_data);
}

/*
 * delete key value pair in map.
 *
 * before calling this function, must call map_has() to check if key
 * already exisits.
 */
bool map_remove(struct map *map, void *key, void **orig_key, void **orig_data) {
    struct map_entry *p = map->entries;
    *orig_key = *orig_data = NULL;
    while (p != NULL) {
        int c = map->keycmp(key, p->key);
        if (c == 0) {
            if (p->left == NULL || p->right == NULL) {
                map_remove_child(map, p, orig_key, orig_data);
                return true;
            }
            struct map_entry *sm = get_min(p->right);
            void *tmp;
            tmp = p->key;
            p->key = sm->key;
            sm->key = tmp;
            tmp = p->data;
            p->data = sm->data;
            sm->data = tmp;
            map_remove_child(map, sm, orig_key, orig_data);
            return true;
        } else if (c < 0) {
            p = p->left;
        } else {
            p = p->right;
        }
    }
    return false;
}

/*
 * update the data of an exists key.
 *
 * before calling this function, must call map_has() to check if key
 * already exisits.
 *
 * @param[output] if not NULL, stores the orig_data.
 */
bool map_update(struct map *map, void *key, void *data, void **orig_data) {
    struct map_entry *p = map->entries;
    while (p != NULL) {
        int c = map->keycmp(key, p->key);
        if (c == 0) {
            if (orig_data) {
                *orig_data = p->data;
            }
            p->data = data;
            return true;
        } else if (c < 0) {
            p = p->left;
        } else {
            p = p->right;
        }
    }
    return false;
}

/*
 * replace the key and the data of an exists entry.
 *
 * before calling this function, must call map_has() to check if key
 * already exisits. orig_key and orig_data use as storing the origin key and data.
 *
 * @param[putput] orig_key if not NULL, stores the origin key.
 * @param[output] orig_data if not NULL, stores the origin key's data.
 */
bool map_replace(struct map *map, void *key, void *data, void **orig_key, void **orig_data) {
    struct map_entry *p = map->entries;
    while (p != NULL) {
        int c = map->keycmp(key, p->key);
        if (c == 0) {
            if (orig_key) {
                *orig_key = p->key;
            }
            if (orig_data) {
                *orig_data = p->data;
            }
            p->key = key;
            p->data = data;
            return true;
        } else if (c < 0) {
            p = p->left;
        } else {
            p = p->right;
        }
    }
    return false;
}

/*
 * get data for key.
 *
 * before calling this function, must call map_has() to check if key
 * already exisits, or test data if is NULL.
 * 
 * @param map   the map to search
 * @param key   the key to lookup.
 * @param[output] data  store the found data, if data is NULL, this
 *              function will test if key is in map.
 */
bool map_get(struct map *map, void *key, void **data) {
    struct map_entry *p = map->entries;
    while (p != NULL) {
        int c = map->keycmp(key, p->key);
        if (c == 0) {
            *data = p->data;
            return true;
        } else if (c < 0) {
            p = p->left;
        } else if (c > 0) {
            p = p->right;
        }
    }
    return false;
}

static struct list *map_keys_internal(struct map_entry *entry) {
    if (entry) {
        struct list *list = list_new();
        list = list_extend(list, map_keys_internal(entry->left));
        list = list_append(list, entry->key);
        list = list_extend(list, map_keys_internal(entry->right));
        return list;
    }
    return list_new();
}

/*
 * get map keys which has been sorted.
 *
 * @param[output] list stores the keys, must be freed by user after use.
 */
struct list *map_keys(struct map *map) {
    return map_keys_internal(map->entries);
}

static void map_destroy_internal(struct map_entry *entry) {
    if (entry) {
        map_destroy_internal(entry->left);
        map_destroy_internal(entry->right);
        free(entry);
    }
}
/*
 * destroy a map. map data must be first freed by user.
 */
void map_destroy(struct map *map) {
    map_destroy_internal(map->entries);
    free(map);
}
