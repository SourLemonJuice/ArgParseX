/*
    Test it with these arguments:
    ["hello", "--test", "testStr", "world"]
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

static void Error_(struct parser_result *result)
{
    printf("Error, parser status: %d\n", result->status);
    exit(EXIT_FAILURE);

    return;
}

int main(int argc, char *argv[])
{
    char *test_str1 = NULL;
    char *test_str2 = NULL;
    bool test_bool = false;

    struct flag_group group[] = {
        {
            .prefix = "--",
            .assigner = "",
            .divider = "",
        },
    };

    struct parser parser[] = {
        {
            .group_idx = 0,
            .method = kMethodMultipleVariable,
            .name = "test",
            .var_count = 2,
            .var_types =
                (ARG_PARSER_VAR_TYPE[]){
                    kTypeString,
                    kTypeString,
                },
            .var_ptrs =
                (void *[]){
                    // we need change those pointer(string)'s value, so passing a secondary pointer
                    &test_str1,
                    &test_str2,
                },
        },
        {
            .group_idx = 0,
            .method = kMethodToggle,
            .name = "toggle",
            .toggle_ptr = &test_bool,
        },
    };

    struct parser_result *res = ArgParser(argc, 0, argv, group, parser, sizeof(parser) / sizeof(struct parser), Error_);

    printf("test_str: %s, %s\n", test_str1, test_str2);
    printf("--toggle: ");
    if (test_bool == true) {
        printf("true\n");
    } else {
        printf("false\n");
    }

    for (int i = 0; i < res->params_count; i++) {
        printf("%s\n", res->parameters[i]);
    }

    return 0;
}
