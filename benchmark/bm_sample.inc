// clang-format off

#ifndef ARGPX_DEV_BENCHMARK_SAMPLE_H_
#define ARGPX_DEV_BENCHMARK_SAMPLE_H_

#include <stdbool.h>
#include <stdlib.h>

#define ARGPX_DEV_BM_SAMPLE_ARR \
    (char *[]){ \
        "--config=./a_file.txt", \
        "-abcParamOfC",\
        "--verbose", \
        "param1", \
        "--log-level=1", \
        "param2", \
        "--retry=7", \
    }

/* do this manually... */
#define ARGPX_DEV_BM_SAMPLE_COUNT 6

struct BmAnswer
{
    char *f_config;
    bool f_verbose;
    int f_log_level;
    int f_retry;
    char *f_param_of_c;
};

void BmAnswerInit(struct BmAnswer ans[static 1])
{
    *ans = (struct BmAnswer){
        .f_config = NULL,
        .f_verbose = false,
        .f_log_level = -1,
        .f_retry = -1,
        .f_param_of_c = NULL,
    };
}

#endif
