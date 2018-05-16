#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "../src/metric.h"
#include "../src/list.h"

int main() {
    setlocale(LC_CTYPE, "C.UTF-8");

    struct metric *m;
    time_t timestamp = time(NULL);
    struct json *v;
    char *ds;

    m = metric_new();
    metric_set_name(m, "test");
    metric_add_tag(m, "host", "debian");

    v = json_object();
    json_object_add(v, "user", json_integer(23));
    json_object_add(v, "sys", json_integer(36));

    metric_set_value(m, v);
    metric_set_time(m, timestamp);

    printf("name: %s\n", metric_get_name(m));
    printf("tags:\n", metric_get_name(m));
    struct list *p, *keys = metric_tag_keys(m);
    for(p = keys; p != NULL; p = list_next(p)) {
        printf("\t%s: %s\n", list_data(p), metric_get_tag(m, list_data(p)));
    }
    list_destroy(keys);
    struct json *mv = metric_get_value(m);
    char *smv = json_dumps(mv);
    printf("value:\n%s\n", smv);
    free(smv);
    printf("time:%ld\n\n", metric_get_time(m));

    ds = metric_dumps(m);
    printf("%s\n\n", ds);
    free(ds);

    struct list *list = list_new();
    list = list_append(list, m);
    ds = metric_list_dumps(list);
    printf("%s\n\n", ds);
    free(ds);
    list_destroy(list);
    metric_destroy(m);


    FILE *fp = fopen("./test_metric.json", "r");
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char buf[len + 1];
    fread(buf, len, 1, fp);
    fclose(fp);
    buf[len] = '\0';

    struct list *ms = metric_loads(buf);
    ds = metric_list_dumps(ms);
    printf("%s\n\n", ds);
    free(ds);

    metric_list_destroy(ms);

    fp = fopen("./test_metric1.json", "r");
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf[len + 1];
    fread(buf, len, 1, fp);
    fclose(fp);
    buf[len] = '\0';

    ms = metric_loads(buf);
    ds = metric_list_dumps(ms);
    printf("%s\n\n", ds);
    free(ds);

    metric_list_destroy(ms);
}
