/*
    By ChatGPT-4o and me
 */

#define _POSIX_C_SOURCE 200809L

#include "bm_sample.inc"

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

static void Parse_(struct BmAnswer *answer, char *argv[], int argc) {
    int opt;
    int long_index = 0;

    static struct option long_options[] = {
        {"config", required_argument, 0, 0},
        {"verbose", no_argument, 0, 0},
        {"log-level", required_argument, 0, 0},
        {"retry", required_argument, 0, 0},
        {"abcParamOfC", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    optind = 0;
    while ((opt = getopt_long(argc, argv, "abc:", long_options, &long_index)) != -1) {
        switch (opt) {
        case 0: // long options
            if (strcmp(long_options[long_index].name, "config") == 0) {
                answer->f_config = strdup(optarg);
            } else if (strcmp(long_options[long_index].name, "verbose") == 0) {
                answer->f_verbose = true;
            } else if (strcmp(long_options[long_index].name, "log-level") == 0) {
                answer->f_log_level = atoi(optarg);
            } else if (strcmp(long_options[long_index].name, "retry") == 0) {
                answer->f_retry = atoi(optarg);
            }
            break;
        case 'a': // short options
            break;
        case 'b':
            break;
        case 'c':
            answer->f_param_of_c = strdup(optarg);
            break;
        case '?':
        default:
            fprintf(stderr, "Unknown flag\n");
            break;
        }
    }

    for (int i = optind; i < argc; ++i) {
        // printf("command argument(%d): %s\n", i, argv[i]);
    }
}

int main() {
    struct BmAnswer answer;
    BmAnswerInit(&answer);

    char **argv = ARGPX_DEV_BM_SAMPLE_ARR;
    int argc = ARGPX_DEV_BM_SAMPLE_COUNT;

    for (int i = 0; i < 10 * 1000 * 1000; i++) {
        Parse_(&answer, argv, argc);

        free(answer.f_config);
        free(answer.f_param_of_c);
    }

    return 0;
}
