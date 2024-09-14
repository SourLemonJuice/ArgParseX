#include "parser.h"

#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
    An unified data of this library
 */
struct UnifiedData_ {
    // the result structure of the main function
    struct ArgpxResult *res;
    // error callback function pointer, NULL is exit(EXIT_FAILURE)
    void (*ErrorCallback)(struct ArgpxResult *);
    // arguments count
    int arg_c;
    // pointer to arguments array
    char **args;
    // the arg index being processed, it records the first unprocessed arg.
    // no no no, try to make it to record the last processed arg
    int arg_idx;
    // groups count
    int group_c;
    // pointer to groups array.
    struct ArgpxFlagGroup *groups;
    // configs count
    int conf_c;
    // pointer to configs array
    struct ArgpxFlag *confs;
};

/*
    Call the error callback registered by the user
 */
static void CallError_(struct UnifiedData_ data[static 1], enum ArgpxStatus status)
{
    data->res->status = status;

    if (data->ErrorCallback != NULL)
        data->ErrorCallback(data->res);

    exit(EXIT_FAILURE);
}

/*
    Check if the arg has reached the boundary.
    idx_offset will be added to data->arg_idx. This may save something?
 */
static bool ArgIndexWithinBoundary_(struct UnifiedData_ data[static 1], int idx_offset)
{
    if (data->arg_idx + idx_offset < data->arg_c)
        return true;
    else
        return false;
}

/*
    Safe shift arguments
 */
static void ShiftArguments_(struct UnifiedData_ data[static 1], int offset)
{
    if (ArgIndexWithinBoundary_(data, offset) == false)
        CallError_(data, kArgpxStatusShiftingArg);
    data->arg_idx += offset;
}

/*
    Convert group index to pointer.
    Some special index(negative number) will be resolved to built-in group
 */
static struct ArgpxFlagGroup *GroupIndexToPointer_(struct UnifiedData_ data[static 1], int index)
{
    struct ArgpxFlagGroup *result = NULL;
    if (index >= 0) {
        result = &data->groups[index];
    } else {
        // TODO
    }

    return result;
}

/*
    Should the assigner of this flag config is mandatory?
 */
static bool ShouldAssignerExist(struct UnifiedData_ data[static 1], struct ArgpxFlagGroup group_ptr[static 1],
                                struct ArgpxFlag conf_ptr[static 1])
{
    switch (conf_ptr->method) {
    case kMethodToggle:
        return false;
        break;
    case kMethodMultipleVariable:
    case kMethodSingleVariable:
        if ((group_ptr->flag & ARGPX_GROUP_MANDATORY_ASSIGNER) != 0)
            return true;
        return false;
        break;
    }

    return true;
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
    Copy the current argument to result data structure as a command parameter
 */
static void GetCommandParameter_(struct UnifiedData_ data[static 1])
{
    char *arg = data->args[data->arg_idx];
    struct ArgpxResult *res = data->res;

    res->params_count += 1;
    size_t this_arg_size = strlen(arg) + 1;

    res->parameters = realloc(res->parameters, sizeof(char * [res->params_count]));
    res->parameters[res->params_count - 1] = arg;
}

/*
    Invert the bool value of "toggle_ptr" in flag config
 */
static void GetFlagBoolToggle_(struct UnifiedData_ data[static 1], struct ArgpxFlag conf_ptr[static 1])
{
    bool *ptr = conf_ptr->toggle_ptr;
    *ptr = not *ptr;
}

// TODO implement the other methods
/*
    If assigner_ptr is NULL, then use the next argument string, which also respect the delimiter
 */
static void GetFlagMultiArgs_(struct UnifiedData_ data[static 1], struct ArgpxFlag conf_ptr[static 1],
                              char *assigner_ptr)
{
    struct ArgpxFlagGroup *group_ptr = &data->groups[conf_ptr->group_idx];
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

    for (int var_idx = 0; var_idx < conf_ptr->var_count; var_idx++) {
        // fix the parameter get type
        if (partition_type == kPartitionNotSet) {
            next_delim_ptr = strchr(param_start, delim);
            if (next_delim_ptr == NULL) {
                if ((group_ptr->flag & ARGPX_GROUP_MANDATORY_DELIMITER) != 0)
                    CallError_(data, kArgpxStatusFlagParamFormatIncorrect);
                partition_type = kPartitionByArguments;
            } else {
                partition_type = kPartitionByDelimiter;
            }
        }

        switch (partition_type) {
        case kPartitionNotSet:
            // never impossible to enter. just make compiler don't take warning
            CallError_(data, kArgpxStatusFailure);
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
                if (var_idx + 1 < conf_ptr->var_count) {
                    CallError_(data, kArgpxStatusFlagParamFormatIncorrect);
                }
                param_len = strlen(param_start);
            } else {
                param_len = next_delim_ptr - param_start;
            }
            break;
        }

        // i know it's confusing...
        StringNumberToVariable_(param_start, param_len, conf_ptr->var_types[var_idx],
                                conf_ptr->var_ptrs[var_idx] /* passing secondary pointer */);
    }
}

/*
    Detect the group where the argument is located.
    A group pointer will be returned.
    Return NULL: no group matched
 */
static struct ArgpxFlagGroup *DetectGroup_(struct UnifiedData_ data[static 1])
{
    char *arg_ptr;
    struct ArgpxFlagGroup *g_ptr;
    for (int g_idx = 0; g_idx < data->group_c; g_idx++) {
        arg_ptr = data->args[data->arg_idx];
        g_ptr = GroupIndexToPointer_(data, g_idx);

        if (strncmp(arg_ptr, g_ptr->prefix, strlen(g_ptr->prefix)) == 0)
            return g_ptr;
    }

    return NULL;
}

static void IterationConfigs_(struct UnifiedData_ data[static 1])
{
    struct ArgpxFlagGroup *g_ptr = DetectGroup_(data);
    if (g_ptr == NULL) {
        GetCommandParameter_(data);
        return;
    }

    char *arg = data->args[data->arg_idx];
    char *assigner_ptr = strchr(arg, g_ptr->assigner); // if NULL no assigner
    char *name_start = arg + strlen(g_ptr->prefix);
    int name_len;
    if (assigner_ptr != NULL)
        name_len = assigner_ptr - name_start;
    else
        name_len = strlen(name_start);

    // iteration all the flag configs
    struct ArgpxFlag *conf_ptr = NULL;
    for (int idx = 0; idx < data->conf_c; idx++) {
        struct ArgpxFlag *temp_conf = &data->confs[idx];
        // matching name
        if (strncmp(name_start, temp_conf->name, name_len) == 0) {
            conf_ptr = temp_conf;
            break;
        }
    }
    // some check
    if (conf_ptr == NULL)
        CallError_(data, kArgpxStatusUnknownFlag);
    if (ShouldAssignerExist(data, g_ptr, conf_ptr))
        CallError_(data, kArgpxStatusNoAssigner);

    // get flag parameters
    switch (conf_ptr->method) {
    case kMethodToggle:
        GetFlagBoolToggle_(data, conf_ptr);
        break;
    case kMethodSingleVariable:
        // TODO
        break;
    case kMethodMultipleVariable:
        GetFlagMultiArgs_(data, conf_ptr, assigner_ptr);
        break;
    }
}

/*
    If ErrorCallback is NULL then use exit(EXIT_FAILURE),
    if is a function then need accept a result structure, this should help

    TODO I don't like this parameter style
 */
struct ArgpxResult *ArgParser(int argc, int arg_base, char *argv[], struct ArgpxFlagGroup *groups, int group_count,
                              struct ArgpxFlag *opts, int opt_count, void (*ErrorCallback)(struct ArgpxResult *))
{
    struct UnifiedData_ data = {
        .res = malloc(sizeof(struct ArgpxResult)),
        .ErrorCallback = ErrorCallback,
        .arg_c = argc,
        .args = argv,
        .arg_idx = arg_base,
        .groups = groups,
        .group_c = group_count,
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
