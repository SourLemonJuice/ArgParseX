/*
    Test it with these arguments:
    ["hello", "--test", "testStr", "world"]
 */

#include <stdio.h>

#include "parser.h"

int main(int argc, char *argv[])
{
    char *test_str;
    struct parser parser[] = {
        {
            .long_name = "test",
            .var_type = kTypeString,
            .variable_ptr = &test_str,
        },
    };

    struct parser_result *res = ArgParser(argc, 0, argv, parser, 1);
    printf("test_str: %s\n", test_str);
    for (int i = 0; i < res->params_count; i++) {
        printf("%s\n", res->parameters[i]);
    }

    return 0;
}
