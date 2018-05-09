#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>

#include "metric.h"
#include "map.h"
#include "list.h"
#include "str.h"

static int keycmp(void *key1, void *key2) {
    return strcmp((char *)key1, (char *)key2);
}

static void metric_destroy_value(struct metric *m);

struct metric *metric_new() {
    struct metric *m = malloc(sizeof(struct metric));
    m->name = NULL;
    m->tags = map_new(keycmp);
    m->value = NULL;
    m->time = 0;
    return m;
}

/*
 * only allow spaces between name, tags, value and time.
 */
static size_t skip_space(wchar_t *wbuf, size_t len) {
    for (size_t offset = 0; offset < len; offset++) {
        if (wbuf[offset] != L' ') {
            return offset;
        }
    }
}

struct list *metric_parse(const char *buf) {
    wchar_t *wbuf = strutf8dec(buf);
    size_t end, len = wcslen(wbuf);
    struct list *ms = list_new();

parse_name:
    offset = skip_space(wbuf, len);
    if (offset == len)
        return ms;
    for (size_t end = offset; end < len; end++) {
        if (wbuf[end] == '\n') {
            offset++;
            goto parse_name;
        }
        if (wbuf[end] == ' ') {
            if (end > offset && wbuf[end - 1] != '\\') {
                end++;
                break;
            }
        }
    }
    /* end must great than offset */
    char *name = strndup(wbuf + offset, end - offset);
    offset = end;

    offset = skip_space(wbuf, len);
    if (offset == len)
        goto free_name;

    /* the content behind name may be tags or value.
     * first, check if it is string value, if not, try
     * to find a equal sign ('='), if not found, then
     * no tags.
     */

    /* create tags map first */
    struct map *tags = map_new(keycmp);

parse_tags:

    /* parse tag key */
    for (size_t end = offset; end < len; end++) {
        if (wbuf[end] == '\n') {
            offset++;
            goto free_name;
        }
        if (wbuf[end] == '=') {
            end++;
            break;
        }
    }
    char *key = strndup(wbuf + offset, end - offset);
    offset = ++end;

    /* parse tag value */
    for (size_t end = offset; end < len; end++) {
        if (wbuf[end] == '\n') {
            free(key);
            offset++;
            goto free_name;
        }
        if (wbuf[end] == ',') {
            map_add(tags, key, value);
            end++;
            goto parse_tags;
        }
        if (wbuf[end] == ' ') {
            map_add(tags, key, value)
            break;
        }
    }

parse_value:
    offset = skip_space(wbuf + offset, len);
    if (offset == len)
        goto free_tags;
    if () {
    
    }

free_value:
    destroy_value();
free_tags:
    map_destroy();
free_name:
    free(name);
    goto parse_name;
}

bool metric_validate(struct metric *m) {
    if (m->name != NULL && m->value != NULL && m->time != 0)
        return true;
    return false;
}

char *metric_serialize(struct metric *m) {
    struct strbuf *sb = strbufnew(128);

    if (m->name) {
        strbufexts(sb, m->name);
    }

    strbufexts(sb, " ");
    
    struct list *p, *keys = map_keys(m->tags);
    for (p = keys; p != NULL; p = list_next(p)) {
        void *data;
        map_get(m->tags, list_data(p), &data);
        strbufexts(sb, list_data(p));
        strbufexts(sb, "=");
        strbufexts(sb, data);
        if (list_next(p) != NULL) {
            strbufexts(sb, ",");
        }
    }

    strbufexts(sb, " ");

    if (m->value) {
        if (m->value->type == METRIC_VALUE_STRING_TYPE) {
            strbufexts(sb, "\"");
            strbufexts(sb, m->value->value);
            strbufexts(sb, "\"");
        } else if (m->value->type == METRIC_VALUE_INTEGER_TYPE) {
            strbufextf(sb, "%lld", *(long long *)m->value->value);
        } else if (m->value->type == METRIC_VALUE_REAL_TYPE) {
            strbufextf(sb, "%f", *(double *)m->value->value);
        } else if (m->value->type == METRIC_VALUE_BOOLEAN_TYPE) {
            if (*(bool *)m->value->value) {
                strbufexts(sb, "true");
            } else {
                strbufexts(sb, "false");
            }
        } else if (m->value->type == METRIC_VALUE_NULL_TYPE) {
            strbufexts(sb, "null");
        }
    }

    strbufexts(sb, " ");

    strbufextf(sb, "%lld", m->time);

    strbufexts(sb, "\n");

    char *s = strdup(strbufstr(sb));
    strbuffree(sb);
    return s;
}

char *metric_serialize_list(struct list *l) {
    struct list *p;
    struct metric *m;
    struct strbuf *sb = strbufnew(512);

    for (p = l; p != NULL; p = list_next(p)) {
        m = list_data(p);
        char *tmp = metric_serialize(m);
        strbufexts(sb, tmp);
    }

    char *s = strdup(strbufstr(sb));
    strbuffree(sb);
    return s;
}

void metric_set_name(struct metric *m, const char *name) {
    if (m->name != NULL) {
        free(m->name);
    }
    m->name = strdup(name);
}

const char *metric_get_name(struct metric *m) {
    return m->name;
}

struct list *metric_tag_keys(struct metric *m) {
    return map_keys(m->tags);
}

void metric_add_tag(struct metric *m, const char *key, const char *value) {
    void *orig_data;
    if (map_has(m->tags, (void *)key)) {
        map_update(m->tags, (void *)key, strdup(value), &orig_data);
        free(orig_data);
    } else {
        map_add(m->tags, strdup(key), strdup(value));
    }
}

const char *metric_get_tag(struct metric *m, const char *key) {
    void *data;
    if (map_get(m->tags, (void *)key, &data)) {
        return data;
    }
    return NULL;
}

void metric_set_string_value(struct metric *m, const char *v) {
    if (m->value != NULL) {
        metric_destroy_value(m);
    }
    struct metric_value *value = malloc(sizeof(struct metric_value));
    value->type = METRIC_VALUE_STRING_TYPE;
    value->value = strdup(v);
    m->value = value;
}

void metric_set_integer_value(struct metric *m, long long v) {
    if (m->value != NULL) {
        metric_destroy_value(m);
    }
    struct metric_value *value = malloc(sizeof(struct metric_value));
    value->type = METRIC_VALUE_INTEGER_TYPE;
    value->value = malloc(sizeof(long long));
    *(long long *)value->value = v;
    m->value = value;
}

void metric_set_real_value(struct metric *m, double v) {
    if (m->value != NULL) {
        metric_destroy_value(m);
    }
    struct metric_value *value = malloc(sizeof(struct metric_value));
    value->type = METRIC_VALUE_REAL_TYPE;
    value->value = malloc(sizeof(double));
    *(double *)value->value = v;
    m->value = value;
}

void metric_set_boolean_value(struct metric *m, bool v) {
    if (m->value != NULL) {
        metric_destroy_value(m);
    }
    struct metric_value *value = malloc(sizeof(struct metric_value));
    value->type = METRIC_VALUE_BOOLEAN_TYPE;
    value->value = malloc(sizeof(bool));
    *(bool *)value->value = v;
    m->value = value;
}

void metric_set_null_value(struct metric *m) {
    if (m->value != NULL) {
        metric_destroy_value(m);
    }
    struct metric_value *value = malloc(sizeof(struct metric_value));
    value->type = METRIC_VALUE_NULL_TYPE;
    value->value = NULL;
    m->value = value;
}

const char *metric_get_string_value(struct metric *m) {
    if (m->value && m->value->type == METRIC_VALUE_STRING_TYPE)
        return m->value->value;
    return NULL;
}

long long metric_get_integer_value(struct metric *m) {
    if (m->value && m->value->type == METRIC_VALUE_INTEGER_TYPE)
        return *(long long *)m->value->value;
    return 0;
}

double metric_get_real_value(struct metric *m) {
    if (m->value && m->value->type == METRIC_VALUE_REAL_TYPE)
        return *(double *)m->value->value;
    return 0;
}

bool metric_get_boolean_value(struct metric *m) {
    if (m->value && m->value->type == METRIC_VALUE_BOOLEAN_TYPE)
        return *(bool *)m->value->value;
    return false;
}

void metric_set_time(struct metric *m, time_t time) {
    m->time = time;
}

time_t metric_get_time(struct metric *m) {
    return m->time;
}

static void metric_destroy_value(struct metric *m) {
    if (m->value) {
        if (m->value->type != METRIC_VALUE_NULL_TYPE) {
            free(m->value->value);
        }
        free(m->value);
        m->value = NULL;
    }
}

void metric_destroy(struct metric *m) {
    free(m->name);

    void *orig_key, *orig_data;
    struct list *p, *keys = map_keys(m->tags);
    for (p = keys; p != NULL; p = list_next(p)) {
        map_remove(m->tags, list_data(p), &orig_key, &orig_data);
        free(orig_key);
        free(orig_data);
    }
    map_destroy(m->tags);
    list_destroy(keys);

    metric_destroy_value(m);
}

/*
 * do myself work, i do not care the list.
 * the list must be freed by user.
 */
void metric_destroy_list(struct list *l) {
    struct list *p;
    for (p = l; p != NULL; p = list_next(p)) {
        metric_destroy(list_data(p));
    }
}
