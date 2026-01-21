#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../utils.h"

typedef struct test_case{
    void *result;
    void* expected;
}test_case;

static int fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

static int run_split_case(const char *name,
                          const char *input,
                          const char *expected[],
                          size_t expected_n) {
    char *buf = strdup(input);
    if (!buf) return fail("strdup failed");

    size_t out_n = 0;
    char **out = split(buf, &out_n);
    if (!out) { free(buf); return fail("split returned NULL"); }

    if (out_n != expected_n) {
        fprintf(stderr, "FAIL %s: token count mismatch: got %zu, expected %zu\n",
                name, out_n, expected_n);
        split_free(out, out_n);
        free(buf);
        return 1;
    }

    test_case cases[expected_n];
    for (size_t i = 0; i < expected_n; ++i) {
        cases[i].result = out[i];
        cases[i].expected = (void *)expected[i];
    }

    int failed = 0;
    for (size_t i = 0; i < expected_n; ++i) {
        const char *r = (const char *)cases[i].result;
        const char *e = (const char *)cases[i].expected;

        if (!r || strcmp(r, e) != 0) {
            fprintf(stderr, "FAIL %s: token[%zu]: got \"%s\", expected \"%s\"\n",
                    name, i, r ? r : "(null)", e);
            failed = 1;
        }
    }

    split_free(out, out_n);
    free(buf);

    if (!failed) printf("OK: %s\n", name);
    return failed;
}

int test_split(void) {
    int failed = 0;

    {
        const char *input = "I can`t get married, I`m a thirty year old boy";
        const char *expected[] = {
            "I","can`t","get","married,","I`m","a","thirty","year","old","boy",
        };
        failed |= run_split_case("split_basic", input, expected, sizeof(expected)/sizeof(expected[0]));
    }

    {
        const char *input = "one  ";
        const char *expected[] = { "one" };
        failed |= run_split_case("split_single", input, expected, sizeof(expected)/sizeof(expected[0]));
    }

    {
        const char *input = "  leading  spaces";
        const char *expected[] = { "leading", "spaces" };
        failed |= run_split_case("split_leading", input, expected, sizeof(expected)/sizeof(expected[0]));
    }

    {
        const char *input = "a   b    c";
        const char *expected[] = { "a", "b", "c" };
        failed |= run_split_case("split_multi_spaces", input, expected, sizeof(expected)/sizeof(expected[0]));
    }

    {
        const char *input = "  ";
        const char **expected = NULL;
        failed |= run_split_case("split empty", input, expected, 0);
    }

    return failed ? 1 : 0;
}

int main(){
    test_split();
}