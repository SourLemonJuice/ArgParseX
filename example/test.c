/*
    Test it with these arguments:
    ["hello", "--test", "testStr", "world"]
 */

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
    char test_str1[] = "test_str1 src";
    char test_str2[] = "test_str2 src";

    struct parser parser[] = {
        {
            .method = kMethodMultipleVariable,
            .prefix = "--",
            .name = "test",
            .var_count = 2,
            .var_types =
                (ARG_PARSER_VAR_TYPE[]){
                    kTypeString,
                    kTypeString,
                },
            .var_ptrs =
                (void *[]){
                    &test_str1,
                    &test_str2,
                },
        },
    };

    struct parser_result *res = ArgParser(argc, 0, argv, parser, sizeof(parser) / sizeof(struct parser), Error_);

    printf("test_str: %s, %s\n", test_str1, test_str2);
    for (int i = 0; i < res->params_count; i++) {
        printf("%s\n", res->parameters[i]);
    }

    return 0;
}
