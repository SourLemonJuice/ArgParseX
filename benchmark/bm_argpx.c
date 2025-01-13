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
    ArgpxAppendGroup(&style, ARGPX_GROUP_GNU);
    ArgpxAppendGroup(&style, ARGPX_GROUP_UNIX);

    ArgpxAppendSymbol(&style, ARGPX_SYMBOL_STOP_PARSING("--"));

    struct BmAnswer ans;
    BmAnswerInit(&ans);

    // clang-format off

    struct ArgpxFlagSet flag = {0};
    ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "config",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &ans.f_config},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "verbose",
        .action_type = kArgpxActionSetBool,
        .action_load.set_bool = {.source = true, .target_ptr = &ans.f_verbose},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "log-level",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarInt, .var_ptr = &ans.f_log_level},
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
        .group_idx = 0,
        .name = "retry",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarInt, .var_ptr = &ans.f_retry},
    });

    ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
        .group_idx = 1,
        .name = "a",
        .action_type = kArgpxActionCallbackOnly,
        .callback = NULL,
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
        .group_idx = 1,
        .name = "b",
        .action_type = kArgpxActionCallbackOnly,
        .callback = NULL,
    });
    ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
        .group_idx = 1,
        .name = "impl_c",
        .action_type = kArgpxActionParamSingle,
        .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &ans.f_param_of_c},
    });

    // clang-format on

    for (int i = 0; i < 10 * 1000 * 1000; i++) {
        struct ArgpxResult *res = ArgpxParse(bm_argc, bm_argv, &style, &flag, NULL);
        if (res == NULL)
            exit(EXIT_FAILURE);
        if (res->status != kArgpxStatusSuccess) {
            printf("ArgParseX error: %s\n", ArgpxStatusString(res->status));
            printf("on '%s'\n", res->current_argv_ptr);
            exit(EXIT_FAILURE);
        }
        ArgpxFreeResult(res);

        free(ans.f_config);
        free(ans.f_param_of_c);
    }

    ArgpxFreeStyle(&style);
    ArgpxFreeFlag(&flag);

    return 0;
}
