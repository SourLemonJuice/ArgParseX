#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

static void Error_(struct ArgpxResult *result)
{
    printf("Error, parser status: %d\n", result->status);
    printf("Status to string: %s\n", ArgpxStatusToString(result->status));
    exit(EXIT_FAILURE);

    return;
}

int main(int argc, char *argv[])
{
    char *test_str1 = NULL;
    char *test_str2 = NULL;
    char *test_str21 = NULL;
    char *test_str22 = NULL;
    char *test_str31 = NULL;
    bool test_bool = false;

    struct ArgpxFlagGroup group[] = {
        {
            .flag = ARGPX_GROUP_MANDATORY_ASSIGNER | ARGPX_GROUP_MANDATORY_DELIMITER,
            .prefix = "--",
            .assigner = '=',
            .delimiter = ',',
        },
        {
            .flag = 0,
            .prefix = "++",
            .assigner = '~',
            .delimiter = '-',
        },
        {
            .flag = ARGPX_GROUP_FLAG_GROUPABLE,
            .prefix = "-",
            .assigner = '=',
            .delimiter = ',',
        },
    };

    struct ArgpxFlag ArgpxFlag[] = {
        {
            .group_idx = 0,
            .method = kArgpxMethodMultipleParam,
            .name = "test",
            .var_count = 2,
            .var_types =
                (ARG_PARSER_VAR_TYPE[]){
                    kArgpxVarTypeString,
                    kArgpxVarTypeString,
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
            .method = kArgpxMethodToggleBool,
            .name = "toggle",
            .single_var_ptr = &test_bool,
        },
        {
            .group_idx = 1,
            .method = kArgpxMethodMultipleParam,
            .name = "test2",
            .var_count = 2,
            .var_types =
                (ARG_PARSER_VAR_TYPE[]){
                    kArgpxVarTypeString,
                    kArgpxVarTypeString,
                },
            .var_ptrs =
                (void *[]){
                    &test_str21,
                    &test_str22,
                },
        },
        {
            .group_idx = 2,
            .method = kArgpxMethodMultipleParam,
            .name = "a",
            .var_count = 0,
        },
        {
            .group_idx = 2,
            .method = kArgpxMethodMultipleParam,
            .name = "b",
            .var_count = 1,
            .var_types =
                (ARG_PARSER_VAR_TYPE[]){
                    kArgpxVarTypeString,
                },
            .var_ptrs =
                (void *[]){
                    &test_str31,
                },
        },
    };

    struct ArgpxResult *res = ArgParser(argc, 0, argv, group, sizeof(group) / sizeof(struct ArgpxFlagGroup), ArgpxFlag,
                                        sizeof(ArgpxFlag) / sizeof(struct ArgpxFlag), Error_);

    printf("test_str group 0: %s, %s\n", test_str1, test_str2);
    printf("test_str group 1: %s, %s\n", test_str21, test_str22);
    printf("test_str group 1: %s\n", test_str31);
    printf("--toggle: false -> ");
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
