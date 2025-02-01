#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "argpx/argpx.h"

static void Error_(struct ArgpxResult *res)
{
    if (res == NULL) {
        printf("result is NULL\n");
        exit(EXIT_FAILURE);
    }

    printf("ArgParseX error [%d]: %s\n", res->status, ArgpxStatusString(res->status));
    printf("index: %d, str: %s\n", res->current_argv_idx, res->current_argv_ptr);
    exit(EXIT_FAILURE);
}

static char *BoolToString_(bool input)
{
    if (input == true)
        return "true";
    else
        return "false";
}

static void CbWin2_(void *action_load, void *param_in)
{
    struct ArgpxOutParamSingle *out = action_load;
    printf("%s\n", *(char **)out->var_ptr);

    ArgpxOutParamSingleFree(out);
}

static void CbParamList_(void *action_load, void *param_in)
{
    struct ArgpxOutParamList *out = action_load;

    for (int i = 0; i < *out->count_ptr; i++) {
        printf("idx: %d, str: %s\n", i, (*out->list_ptr)[i]);
    }

    ArgpxOutParamListFree(out);
}

int main(int argc, char *argv[])
{
    // char *test_str1 = NULL;
    // char *test_str2 = NULL;
    // char *test_str21 = NULL;
    // char *test_str22 = NULL;
    char *test_str31 = NULL;
    char *test_win_str1 = NULL;
    char *test_win_str2 = NULL;
    bool test_bool = false;
    bool test_bool2 = false;

    int test_int = 0;

    // int test_param_list_count = 0;
    // char **test_param_list = NULL;

    // clang-format off

    struct ArgpxStyle style = ARGPX_STYLE_INIT;
    ArgpxGroupAppend(&style, ARGPX_GROUP_GNU);
    // https://stackoverflow.com/a/11152199/25416550
    // other element will be initialized implicitly
    ArgpxGroupAppend(&style, &(struct ArgpxGroup){
        .prefix = "++",
        .assigner = "~",
        .delimiter = "-",
    });
    ArgpxGroupAppend(&style, ARGPX_GROUP_UNIX);
    ArgpxGroupAppend(&style, &(struct ArgpxGroup){
        .prefix = "/",
        .assigner = "=",
        .delimiter = ",",
        .attribute = ARGPX_ATTR_COMPOSABLE | ARGPX_ATTR_COMPOSABLE_NEED_PREFIX,
    });

    ArgpxSymbolAppend(&style, ARGPX_SYMBOL_STOP_PARSING("--"));
    ArgpxSymbolAppend(&style, ARGPX_SYMBOL_STOP_PARSING("-"));

    struct ArgpxFlagSet flag = ARGPX_FLAGSET_INIT;
    // ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
    //     .group_idx = 0,
    //     .name = "test",
    //     .action_type = kArgpxActionParamMulti,
    //     .action_load.param_multi.count = 2,
    //     .action_load.param_multi.unit_v =
    //         (struct ArgpxOutParamSingle[]){
    //             {.type = kArgpxVarString, .var_ptr = &test_str1},
    //             {.type = kArgpxVarString, .var_ptr = &test_str2},
    //         },
    // });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "setbool",
        .action_type = kArgpxActionSetBool,
        .action_load.set_bool = {.source = true, .target_ptr = &test_bool},
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "setint",
        .action_type = kArgpxActionSetInt,
        .action_load.set_int = {.source = 123, .target_ptr = &test_int},
    });
    // ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
    //     .group_idx = 1,
    //     .name = "test2",
    //     .action_type = kArgpxActionParamMulti,
    //     .action_load.param_multi.count = 2,
    //     .action_load.param_multi.unit_v =
    //         (struct ArgpxOutParamSingle[]){
    //             {.type = kArgpxVarString, .var_ptr = &test_str21},
    //             {.type = kArgpxVarString, .var_ptr = &test_str22},
    //         },
    // });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "paramlist",
        .action_type = kArgpxActionParamListOnDemand,
        .action_load.param_list = {.max = 3},
        .callback = CbParamList_,
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 3,
        .name = "win1",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &test_win_str1},
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 3,
        .name = "win2",
        .action_type = kArgpxActionParamSingleOnDemand,
        .action_load.param_single = {.type = kArgpxVarString},
        .callback = CbWin2_,
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 2,
        .name = "a",
        .action_type = kArgpxActionSetBool,
        .action_load.set_bool = {.source = true, .target_ptr = &test_bool2},
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 2,
        .name = "b",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &test_str31},
    });

    // skip the first arg, that's the exec command name
    struct ArgpxResult *res = ArgpxParse(argc - 1, argv + 1, &style, &flag, NULL);

    // clang-format on

    if (res == NULL)
        Error_(NULL);
    if (res->status != kArgpxStatusSuccess)
        Error_(res);

    // printf("test_str group 1:\t%s, %s\n", test_str1, test_str2);
    // printf("test_str group 2:\t%s, %s\n", test_str21, test_str22);
    printf("test_str group 3:\t%s\n", test_str31);
    printf("/win1 and /win2:\t%s, %s\n", test_win_str1, test_win_str2);
    printf("--setbool:\t\t%s\n", BoolToString_(test_bool));
    printf("-a:\t\t\t%s\n", BoolToString_(test_bool2));
    printf("--setint:\t\t%d\n", test_int);

    printf("==== command parameters ====\n");
    for (int i = 0; i < res->param_c; i++)
        printf("%s\n", res->param_v[i]);

    ArgpxResultFree(res);
    ArgpxStyleFree(&style);
    ArgpxFlagFree(&flag);

    return 0;
}
