#include <stdio.h>

#include "parser.h"

int main(int argc, char *argv[])
{
    struct parser parser[] = {};

    struct parser_result *res = ArgParser(argc, 0, argv, parser, 0);
    for (int i = 0; i < res->params_count; i++) {
        printf("%s\n", res->parameters[i]);
    }

    return 0;
}
