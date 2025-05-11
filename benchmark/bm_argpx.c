#include "bm_sample.inc"

#include "stdio.h"

#include "argpx/argpx.h"

int main(void)
{
    char **bm_argv = ARGPX_DEV_BM_SAMPLE_ARR;
    int bm_argc = ARGPX_DEV_BM_SAMPLE_COUNT;

    // no io operation by default
    // printf("Start ArgParseX sample with argc: %d\n", bm_argc);

    struct ArgpxStyle style = {0};
    ArgpxGroupAppend(&style, ARGPX_GROUP_GNU);
    ArgpxGroupAppend(&style, ARGPX_GROUP_UNIX);

    ArgpxSymbolAppend(&style, ARGPX_SYMBOL_STOP_PARSING("--"));

    struct BmAnswer ans;
    BmAnswerInit(&ans);

    // clang-format off

    struct ArgpxFlagSet flag = {0};
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "config",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &ans.f_config},
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "verbose",
        .action_type = kArgpxActionSetBool,
        .action_load.set_bool = {.source = true, .target_ptr = &ans.f_verbose},
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "log-level",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarInt, .var_ptr = &ans.f_log_level},
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "retry",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarInt, .var_ptr = &ans.f_retry},
    });

    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 1,
        .name = "a",
        .action_type = kArgpxActionCallbackOnly,
        .callback = NULL,
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 1,
        .name = "b",
        .action_type = kArgpxActionCallbackOnly,
        .callback = NULL,
    });
    ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
        .group_idx = 1,
        .name = "c",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &ans.f_param_of_c},
    });

    // clang-format on

    for (int i = 0; i < 10 * 1000 * 1000; i++) {
        struct ArgpxParseOption opt = ARGPX_PARSE_OPTION_INIT;
        opt.use_hash = true;
        struct ArgpxResult res;
        if (ArgpxParse(&res, bm_argc, bm_argv, &style, &flag, &opt) != kArgpxStatusSuccess) {
            printf("ArgParseX error: %s\n", ArgpxStatusString(res.status));
            printf("on '%s'\n", res.current_argv_ptr);
            exit(EXIT_FAILURE);
        }
        ArgpxResultFree(&res);

        free(ans.f_config);
        free(ans.f_param_of_c);
    }

    ArgpxStyleFree(&style);
    ArgpxFlagFree(&flag);

    return 0;
}
