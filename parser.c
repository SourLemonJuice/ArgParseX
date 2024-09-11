#include "parser.h"

#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
    An unified data of this unit
 */
struct ArgParserData_ {
    // the result structure of the main function
    struct parser_result *res;
    // error callback function pointer, NULL is exit(EXIT_FAILURE)
    void (*ErrorCallback)(struct parser_result *);
    // arguments count
    int arg_c;
    // pointer to arguments array
    char **args;
    // the arg index being processed, it records the first unprocessed arg.
    // no no no, try to make it to record the last processed arg
    int arg_idx;
    // pointer to groups array.
    // if you ask me, why don't use "group_c"?
    // emm... I trust the programer can figure out the array index in their hands.
    // then there is no need for a "new" hash table here
    struct flag_group *groups;
    // configs count
    int conf_c;
    // pointer to configs array
    struct parser *confs;
};

/*
    Call the error callback registered by the user
 */
static void CallError_(struct ArgParserData_ *data, enum parser_status status)
{
    data->res->status = status;

    if (data->ErrorCallback == NULL)
        exit(EXIT_FAILURE);
    else
        data->ErrorCallback(data->res);
}

/*
    Check if the arg has reached the boundary.
    idx_offset will be added to data->arg_idx. This may save something?
 */
static bool ArgIndexWithinBoundary_(struct ArgParserData_ *data, int idx_offset)
{
    if (data->arg_idx + idx_offset < data->arg_c)
        return true;
    else
        return false;
}

/*
    Safe shift arguments
 */
static void ShiftArguments_(struct ArgParserData_ *data, int offset)
{
    if (ArgIndexWithinBoundary_(data, offset) == false)
        CallError_(data, kArgParserShiftingArg);
    data->arg_idx += offset;
}

/*
    Converting a string to a specific type.
    And assign it to a pointer

    The "number" is similar to strncmp()'s "n"
 */
static void StringNumberToVariable_(char *source_str, int number, enum parser_var_type type, void **ptr)
{
    // prepare a separate string
    int str_len = strlen(source_str);
    int actual_len;
    size_t actual_size;
    // chose the smallest one
    if (number < str_len)
        actual_len = number;
    else
        actual_len = str_len;
    actual_size = actual_len * sizeof(char) + 1;
    // allocate a new string
    char *value_str = malloc(actual_size);
    memcpy(value_str, source_str, actual_size);
    value_str[actual_len] = '\0'; // actual_len is the last index of this string

    // remember to change the first level pointer, but not just change secondary one
    // if can, the value_str needs to free up
    // TODO implement other types
    switch (type) {
    case kTypeString:
        *ptr = value_str;
        break;
    case kTypeInteger:
        break;
    case kTypeBoolean:
        break;
    case kTypeFloat:
        break;
    case kTypeDouble:
        break;
    }
}

/*
    Invert the bool value of "toggle_ptr" in flag config
 */
static void GetFlagBoolToggle_(struct ArgParserData_ *data, int conf_idx)
{
    bool *ptr = data->confs[conf_idx].toggle_ptr;
    *ptr = not *ptr;
}

// TODO implement the other methods
/*
    If assigner_ptr is NULL, then use the next argument string, which also respect the delimiter
 */
static void GetFlagMultiArgs_(struct ArgParserData_ *data, int conf_idx /* config index */, char *assigner_ptr)
{
    struct flag_group *group_ptr = &data->groups[data->confs[conf_idx].group_idx];
    char delim = group_ptr->delimiter;
    char *param_start;
    int param_len;
    char *next_delim_ptr;
    bool first_param = true;

    // init param_start
    if (assigner_ptr != NULL) {
        param_start = assigner_ptr + 1;
    } else {
        ShiftArguments_(data, 1);
        param_start = data->args[data->arg_idx];
    }

    enum {
        kPartitionNotSet,
        kPartitionByArguments,
        kPartitionByDelimiter,
    } partition_type = kPartitionNotSet;

    for (int var_idx = 0; var_idx < data->confs[conf_idx].var_count; var_idx++) {
        // fix the parameter get type
        if (partition_type == kPartitionNotSet) {
            next_delim_ptr = strchr(param_start, delim);
            if (next_delim_ptr == NULL) {
                if ((group_ptr->flag & ARG_GROUP_MANDATORY_DELIMITER) != 0)
                    CallError_(data, kArgParserFlagParamFormatIncorrect);
                partition_type = kPartitionByArguments;
            } else {
                partition_type = kPartitionByDelimiter;
            }
        }

        switch (partition_type) {
        case kPartitionNotSet:
            // never impossible to enter. just make compiler don't take warning
            CallError_(data, kArgParserUnknownError);
            break;
        case kPartitionByArguments:
            if (first_param == true) {
                first_param = false;
            } else {
                // if args has reached the end, but this flag still need more parameter, call the error
                ShiftArguments_(data, 1);
                param_start = data->args[data->arg_idx];
            }
            param_len = strlen(param_start);
            break;
        case kPartitionByDelimiter:
            if (first_param == true)
                first_param = false;
            else
                param_start = next_delim_ptr + 1;

            next_delim_ptr = strchr(param_start, delim);
            if (next_delim_ptr == NULL) {
                // if this is not the last parameter, the format is incorrect
                if (var_idx + 1 < data->confs[conf_idx].var_count) {
                    CallError_(data, kArgParserFlagParamFormatIncorrect);
                }
                param_len = strlen(param_start);
            } else {
                param_len = next_delim_ptr - param_start;
            }
            break;
        }

        // i know it's confusing...
        StringNumberToVariable_(param_start, param_len, data->confs[conf_idx].var_types[var_idx],
                                data->confs[conf_idx].var_ptrs[var_idx] /* passing secondary pointer */);
    }
}

static void IterationConfigs_(struct ArgParserData_ *data)
{
    // iteration all the flag configs
    for (int i = 0; i < data->conf_c; i++) {
        // some... alias?
        // those pointers are so long
        struct flag_group *group_ptr = &data->groups[data->confs[i].group_idx];

        // matching prefix
        char *prefix = group_ptr->prefix;
        int prefix_len = 0;
        if (prefix[0] != '\0') {
            prefix_len = strlen(prefix);
            if (strncmp(prefix, data->args[data->arg_idx], prefix_len) != 0)
                continue;
        }

        // matching name
        char *assigner_ptr = NULL;
        int name_len = 0;
        // if assigner set to '\0', skip it
        if (group_ptr->assigner == '\0') {
            name_len = strlen(data->args[data->arg_idx]) - prefix_len;
        } else {
            assigner_ptr = strchr(data->args[data->arg_idx], group_ptr->assigner);
            // TODO this block breaks the no parameter flag's parsing
            if (assigner_ptr == NULL) {
                if ((group_ptr->flag & ARG_GROUP_MANDATORY_ASSIGNER) != 0)
                    CallError_(data, kArgParserShiftingArg);
                name_len = strlen(data->args[data->arg_idx]) - prefix_len;
                ShiftArguments_(data, 1);
            } else {
                name_len = assigner_ptr - data->args[data->arg_idx];
                name_len -= prefix_len;
            }
        }
        if (strncmp(data->confs[i].name, data->args[data->arg_idx] + prefix_len, name_len) != 0)
            continue;

        // get args
        switch (data->confs[i].method) {
        case kMethodToggle:
            GetFlagBoolToggle_(data, i);
            break;
        case kMethodSingleVariable:
            // TODO
            break;
        case kMethodMultipleVariable:
            GetFlagMultiArgs_(data, i, assigner_ptr);
            break;
        }

        // don't continue to iterate other configs
        return;
    }

    // no any config matched this arg. so... it's a parameter, add it to result structure
    // get flag functions may have run out of args, so check it at first
    if (ArgIndexWithinBoundary_(data, 0) == true) {
        data->res->params_count += 1;
        size_t this_arg_size = strlen(data->args[data->arg_idx]) + 1;

        data->res->parameters = realloc(data->res->parameters, sizeof(char **) * data->res->params_count);
        data->res->parameters[data->res->params_count - 1] = data->args[data->arg_idx];
    }
}

/*
    If ErrorCallback is NULL then use exit(EXIT_FAILURE),
    if is a function then need accept a result structure, this should help

    TODO I don't like this parameter style
 */
struct parser_result *ArgParser(int argc, int last_arg, char *argv[], struct flag_group *groups, struct parser *opts,
                                int opt_count, void (*ErrorCallback)(struct parser_result *))
{
    struct ArgParserData_ data = {
        .res = malloc(sizeof(struct parser_result)),
        .ErrorCallback = ErrorCallback,
        .arg_c = argc,
        .args = argv,
        .arg_idx = last_arg,
        .groups = groups,
        .conf_c = opt_count,
        .confs = opts,
    };

    for (; ArgIndexWithinBoundary_(&data, 0) == true; data.arg_idx++) {
        IterationConfigs_(&data);
    }

    // last_arg is equal to argc now, but here need a valid index
    data.res->parsed_argc_index = data.arg_idx - 1;

    // TODO find a way to free those malloc()
    return data.res;
}
