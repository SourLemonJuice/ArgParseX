#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

#if ARGPX_VERSION_MAIN != 0
    #error ArgParseX version mismatch
#endif

static void Error_(struct ArgpxResult *res)
{
    printf("Error, parser status: %d\n", res->status);
    printf("Status to string: %s\n", ArgpxStatusToString(res->status));
    printf("Problematic argument(index %d): %s\n", res->current_argv_idx, res->current_argv_ptr);
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
    bool test_bool2 = false;

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
            .name = "test",
            .action_type = kArgpxActionParamMulti,
            .action_load.param_multi.count = 2,
            .action_load.param_multi.units =
                (struct ArgpxParamUnit[]){
                    {.type = kArgpxVarTypeString, .ptr = &test_str1},
                    {.type = kArgpxVarTypeString, .ptr = &test_str2},
                },
        },
        {
            .group_idx = 0,
            .name = "bool",
            .action_type = kArgpxActionSetBool,
            .action_load.set_bool = {.source = true, .target_ptr = &test_bool},
        },
        {
            .group_idx = 1,
            .name = "test2",
            .action_type = kArgpxActionParamMulti,
            .action_load.param_multi.count = 2,
            .action_load.param_multi.units =
                (struct ArgpxParamUnit[]){
                    {.type = kArgpxVarTypeString, .ptr = &test_str21},
                    {.type = kArgpxVarTypeString, .ptr = &test_str22},
                },
        },
        {
            .group_idx = 2,
            .name = "a",
            .action_type = kArgpxActionSetBool,
            .action_load.set_bool = {.source = true, .target_ptr = &test_bool2},
        },
        {
            .group_idx = 2,
            .name = "b",
            .action_type = kArgpxActionParamSingle,
            .action_load.param_single = {.type = kArgpxVarTypeString, .ptr = &test_str31},
        },
    };

    struct ArgpxResult *res = ArgpxMain(argc, 0, argv, group, sizeof(group) / sizeof(struct ArgpxFlagGroup), ArgpxFlag,
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

    printf("==== command parameters ====\n");
    for (int i = 0; i < res->params_count; i++) {
        printf("%s\n", res->params[i]);
    }

    return 0;
}
