#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "argpx.h"

#if ARGPX_VERSION_MAJOR != 0
    #error ArgParseX major version mismatch
#endif

static void Error_(struct ArgpxResult *res)
{
    printf("Error, parser status: %d\n", res->status);
    printf("%s\n", ArgpxStatusToString(res->status));
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
    char *test_win_str1 = NULL;
    char *test_win_str2 = NULL;
    bool test_bool = false;
    bool test_bool2 = false;

    struct ArgpxFlagGroup group[] = {
        {
            .attribute = ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG | ARGPX_ATTR_PARAM_DISABLE_ARG,
            .prefix = "--",
            .assigner = "=",
            .delimiter = ",",
        },
        {
            .attribute = 0,
            .prefix = "++",
            .assigner = "~",
            .delimiter = "-",
        },
        {
            .attribute = ARGPX_ATTR_COMPOSABLE,
            .prefix = "-",
            .assigner = "=",
            .delimiter = ",",
        },
        {
            .attribute = ARGPX_ATTR_COMPOSABLE | ARGPX_ATTR_COMPOSABLE_NEED_PREFIX,
            .prefix = "/",
            .assigner = "=",
            .delimiter = ",",
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
                    {.type = kArgpxVarString, .ptr = &test_str1},
                    {.type = kArgpxVarString, .ptr = &test_str2},
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
                    {.type = kArgpxVarString, .ptr = &test_str21},
                    {.type = kArgpxVarString, .ptr = &test_str22},
                },
        },
        {
            .group_idx = 3,
            .name = "win1",
            .action_type = kArgpxActionParamSingle,
            .action_load.param_single = {
                .type = kArgpxVarString,
                .ptr = &test_win_str1,
            }
        },
        {
            .group_idx = 3,
            .name = "win2",
            .action_type = kArgpxActionParamSingle,
            .action_load.param_single = {
                .type = kArgpxVarString,
                .ptr = &test_win_str2,
            }
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
            .action_load.param_single = {.type = kArgpxVarString, .ptr = &test_str31},
        },
    };

    struct ArgpxResult *res = ArgpxMain(argc, 1, argv, group, sizeof(group) / sizeof(struct ArgpxFlagGroup), ArgpxFlag,
                                        sizeof(ArgpxFlag) / sizeof(struct ArgpxFlag), Error_);

    printf("test_str group 0: %s, %s\n", test_str1, test_str2);
    printf("test_str group 1: %s, %s\n", test_str21, test_str22);
    printf("test_str group 1: %s\n", test_str31);
    printf("win group 3: %s, %s\n", test_win_str1, test_win_str2);
    printf("--bool and -a: ");

    if (test_bool == true)
        printf("true ");
    else
        printf("false ");

    if (test_bool2 == true)
        printf("true");
    else
        printf("false");

    printf("\n");

    printf("==== command parameters ====\n");
    for (int i = 0; i < res->params_count; i++) {
        printf("%s\n", res->params[i]);
    }

    return 0;
}
