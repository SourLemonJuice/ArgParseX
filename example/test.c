#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "argpx/argpx.h"

static void Error_(struct ArgpxResult *res)
{
    printf("Error, parser status: %d\n", res->status);
    printf("%s\n", ArgpxStatusToString(res->status));
    printf("Problematic argument(index %d): %s\n", res->current_argv_idx, res->current_argv_ptr);
    exit(EXIT_FAILURE);
}

static char *BoolToString_(bool input)
{
    if (input == true)
        return "true";
    else
        return "false";
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

    int test_int = 0;

    int test_param_list_count = 0;
    char **test_param_list = NULL;

    // clang-format off

    struct ArgpxStyle style = {0};
    ArgpxAppendGroup(&style, ARGPX_GROUP_GNU);
    // https://stackoverflow.com/a/11152199/25416550
    // other element will be initialized implicitly
    ArgpxAppendGroup(&style, &(struct ArgpxGroupItem){
        .prefix = "++",
        .assigner = "~",
        .delimiter = "-",
    });
    ArgpxAppendGroup(&style, ARGPX_GROUP_UNIX);
    ArgpxAppendGroup(&style, &(struct ArgpxGroupItem){
        .prefix = "/",
        .assigner = "=",
        .delimiter = ",",
        .attribute = ARGPX_ATTR_COMPOSABLE | ARGPX_ATTR_COMPOSABLE_NEED_PREFIX,
    });

    ArgpxAppendSymbol(&style, ARGPX_SYMBOL_STOP_PARSING("--"));
    ArgpxAppendSymbol(&style, ARGPX_SYMBOL_STOP_PARSING("-"));

    struct ArgpxFlagSet flag = {0};
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 0,
        .name = "test",
        .action_type = kArgpxActionParamMulti,
        .action_load.param_multi.count = 2,
        .action_load.param_multi.units =
            (struct ArgpxParamUnit[]){
                {.type = kArgpxVarString, .ptr = &test_str1},
                {.type = kArgpxVarString, .ptr = &test_str2},
            },
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 0,
        .name = "setbool",
        .action_type = kArgpxActionSetBool,
        .action_load.set_bool = {.source = true, .target_ptr = &test_bool},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 0,
        .name = "setint",
        .action_type = kArgpxActionSetInt,
        .action_load.set_int = {.source = 123, .target_ptr = &test_int},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 0,
        .name = "setint",
        .action_type = kArgpxActionSetInt,
        .action_load.set_int = {.source = 123, .target_ptr = &test_int},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 1,
        .name = "test2",
        .action_type = kArgpxActionParamMulti,
        .action_load.param_multi.count = 2,
        .action_load.param_multi.units =
            (struct ArgpxParamUnit[]){
                {.type = kArgpxVarString, .ptr = &test_str21},
                {.type = kArgpxVarString, .ptr = &test_str22},
            },
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 0,
        .name = "paramlist",
        .action_type = kArgpxActionParamList,
        .action_load.param_list =
            {
                .count = &test_param_list_count,
                .params = &test_param_list,
            },
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 3,
        .name = "win1",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .ptr = &test_win_str1},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 3,
        .name = "win2",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .ptr = &test_win_str2},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 2,
        .name = "a",
        .action_type = kArgpxActionSetBool,
        .action_load.set_bool = {.source = true, .target_ptr = &test_bool2},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
        .group_idx = 2,
        .name = "b",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .ptr = &test_str31},
    });

    struct ArgpxResult *res = ArgpxMain(&(struct ArgpxMainOption){
        // skip first arg
        .argc = argc - 1,
        .argv = argv + 1,
        .style = &style,
        .flag = &flag,
        .terminate.method = kArgpxTerminateNone,
        // .terminate.method = kArgpxTerminateAtNumberOfCommandParam,
        // .terminate.load.num_of_cmd_param.limit = 2,
    });

    // clang-format on

    // TODO temp test
    if (res == NULL)
        Error_(res);
    if (res->status != kArgpxStatusSuccess)
        Error_(res);

    printf("test_str group 1: %s, %s\n", test_str1, test_str2);
    printf("test_str group 2: %s, %s\n", test_str21, test_str22);
    printf("test_str group 3: %s\n", test_str31);
    printf("/win1 and /win2: %s, %s\n", test_win_str1, test_win_str2);
    printf("--setbool: %s\n", BoolToString_(test_bool));
    printf("-a: %s\n", BoolToString_(test_bool2));
    printf("--setint: %d\n", test_int);

    for (int i = 0; i < test_param_list_count; i++)
        printf("paramlist index: %d: %s\n", i, test_param_list[i]);

    printf("==== command parameters ====\n");
    for (int i = 0; i < res->param_count; i++)
        printf("%s\n", res->paramv[i]);

    return 0;
}
