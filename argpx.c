#include "argpx.h"

#include <iso646.h>
#include <limits.h>
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

struct UnifiedGroupCache_ {
    int idx;
    struct ArgpxFlagGroup *ptr;
    int prefix_len;
    int assigner_len;
    bool assigner_toggle;
    int delimiter_len;
    int delimiter_toggle;
};

struct ArgpxFlagGroup argpx_hidden_builtin_group[kArgpxHidden_BuiltinGroupCount] = {
    [kArgpxHidden_BuiltinGroupGnu] =
        {
            .prefix = "--",
            .assigner = "=",
            .delimiter = ",",
            .attribute = ARGPX_ATTR_PARAM_DISABLE_ARG,
        },
    [kArgpxHidden_BuiltinGroupUnix] =
        {
            .prefix = "-",
            .assigner = "=",
            .delimiter = ",",
            .attribute = ARGPX_ATTR_COMPOSABLE | ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG,
        },
};

/*
    Search "needle" in "haystack", limited to the first "len" chars of haystack
 */
static char *strnstr_(char haystack[const restrict static 1], char needle[const restrict static 1], int len)
{
    if (len <= 0)
        return NULL;

    char *temp_str;
    int needle_len = strlen(needle);
    while (true) {
        temp_str = strchr(haystack, needle[0]);
        if (temp_str == NULL)
            return NULL;

        if ((temp_str - haystack) + needle_len > len)
            return NULL;
        if (strncmp(temp_str, needle, needle_len) == 0)
            return temp_str;
    }
}

/*
    Call the error callback registered by the user
 */
static void ArgpxExit_(struct UnifiedData_ data[static 1], enum ArgpxStatus status)
{
    data->res->status = status;

    // update current arg record
    data->res->current_argv_idx = data->arg_idx;
    data->res->current_argv_ptr = data->args[data->arg_idx];

    if (data->ErrorCallback != NULL)
        data->ErrorCallback(data->res);

    exit(EXIT_FAILURE);
}

/*
    Convert structure ArgpxResult's ArgpxStatus enum to string
 */
char *ArgpxStatusToString(enum ArgpxStatus status)
{
    switch (status) {
    case kArgpxStatusSuccess:
        return "Processing success";
    case kArgpxStatusFailure:
        return "Generic unknown error, I must be lazy~~~";
    case kArgpxStatusUnknownFlag:
        return "Unknown flag name but the group matched(by prefix)";
    case kArgpxStatusActionUnavailable:
        return "Flag action type unavailable. Not implemented or configuration conflict";
    case kArgpxStatusNoArgAvailableToShifting:
        return "There is no more argument available to get";
    case kArgpxStatusFlagParamNoNeeded:
        return "This flag don't need parameter, but the input seems to be assigning a value";
    case kArgpxStatusAssignmentDisallowAssigner:
        return "Detected assignment mode is Assigner, but for some reason, unavailable";
    case kArgpxStatusAssignmentDisallowTrailing:
        return "Detected assignment mode is Trailing, but for some reason, unavailable";
    case kArgpxStatusAssignmentDisallowArg:
        return "Detected assignment mode is Arg(argument), but for some reason, unavailable";
    case kArgpxStatusParamDisallowDelimiter:
        return "Detected parameter partition mode is Delimiter, but for some reason, unavailable";
    case kArgpxStatusParamDisallowArg:
        return "Detected parameter partition mode is Arg(argument), but for some reason, unavailable";
    case kArgpxStatusFlagParamDeficiency:
        return "The flag gets insufficient parameters";
    case kArgpxStatusGroupConfigEmptyString:
        return "One or more empty string in group configs are invalid";
    }

    return NULL;
}

/*
    Using the offset shift arguments, it will be safe.
    Return a pointer to the new argument
 */
static char *ShiftArguments_(struct UnifiedData_ data[static 1], int offset)
{
    if (data->arg_idx + offset >= data->arg_c)
        ArgpxExit_(data, kArgpxStatusNoArgAvailableToShifting);
    data->arg_idx += offset;

    return data->args[data->arg_idx];
}

/*
    Copy the current argument to result data structure as a command parameter
 */
static void AppendCommandParameter_(struct UnifiedData_ data[static 1])
{
    char *arg = data->args[data->arg_idx];
    struct ArgpxResult *res = data->res;

    res->params_count += 1;
    size_t this_arg_size = strlen(arg) + 1;

    res->params = realloc(res->params, sizeof(char * [res->params_count]));
    res->params[res->params_count - 1] = arg;
}

/*
    Converting a string to a specific type.
    And assign it to a pointer

    The "number" is similar to strncmp()'s "n"
 */
static void StringNumberToVariable_(char *source_str, int number, enum ArgpxVarType type, void **ptr)
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
    switch (type) {
    case kArgpxVarString:
        *ptr = value_str;
        return;
    default:
        // TODO implement other types
        *ptr = NULL;
        break;
    }

    free(value_str);
}

enum ParamPartitionMode_ {
    kPartitionSingleParam_,
    kPartitionByArguments_,
    kPartitionByDelimiter_,
};

/*
    Use for ActionParamAny_()
 */
static enum ParamPartitionMode_ DetectParamPartitionMode_(struct UnifiedData_ data[static 1],
    struct UnifiedGroupCache_ grp[static 1], struct ArgpxFlag conf_ptr[static 1], char *param, int param_range_len)
{
    enum ParamPartitionMode_ mode;

    // TODO
    // we can't just use strstr(), because this:
    // cmd /win1=str1/win2=str3,str4
    // then function will think /win1 is partitioned by delimiter
    // but with the advent of the SingleParam mode, it's doesn't to be a problem anymore?
    char *delim_ptr;
    if (grp->delimiter_toggle == true)
        delim_ptr = strnstr_(param, grp->ptr->delimiter, param_range_len);
    else
        delim_ptr = NULL;

    if (delim_ptr == NULL) {
        if ((grp->ptr->attribute & ARGPX_ATTR_PARAM_DISABLE_ARG) != 0)
            ArgpxExit_(data, kArgpxStatusParamDisallowArg);
        mode = kPartitionByArguments_;
    } else {
        if ((grp->ptr->attribute & ARGPX_ATTR_PARAM_DISABLE_DELIMITER) != 0)
            ArgpxExit_(data, kArgpxStatusParamDisallowDelimiter);
        mode = kPartitionByDelimiter_;
    }

    return mode;
}

/*
    If param_start_ptr is NULL, then use the next argument string, which also respect the delimiter

    If max_param_len <= 0, ignore it
 */
static void ActionParamAny_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    struct ArgpxFlag conf_ptr[static 1], char *param_start_ptr, int max_param_len, bool allow_multi_arg)
{
    int param_count;

    // for loop
    bool first_param = true;
    char *final_param_start;
    int param_len;
    char *next_delim_ptr;
    struct ArgpxParamUnit *unit_ptr;

    // init
    if (param_start_ptr != NULL)
        final_param_start = param_start_ptr;
    else
        final_param_start = ShiftArguments_(data, 1);

    enum ParamPartitionMode_ partition_mode;

    switch (conf_ptr->action_type) {
    case kArgpxActionParamMulti:
        param_count = conf_ptr->action_load.param_multi.count;
        unit_ptr = conf_ptr->action_load.param_multi.units;

        partition_mode =
            DetectParamPartitionMode_(data, grp, conf_ptr, final_param_start, strlen(final_param_start));
        // if caller doesn't want the parameter to be split by args, exit
        if (partition_mode == kPartitionByArguments_ and allow_multi_arg == false)
            ArgpxExit_(data, kArgpxStatusParamDisallowDelimiter);
        break;
    case kArgpxActionParamSingle:
        param_count = 1;
        unit_ptr = &conf_ptr->action_load.param_single;

        partition_mode = kPartitionSingleParam_;
        break;
    default:
        ArgpxExit_(data, kArgpxStatusActionUnavailable);
        break;
    }

    for (int var_idx = 0; var_idx < param_count; var_idx++) {
        switch (partition_mode) {
        case kPartitionSingleParam_:
            param_len = strlen(final_param_start);
            break;
        case kPartitionByArguments_:
            if (first_param == true)
                first_param = false;
            else
                final_param_start = ShiftArguments_(data, 1);

            param_len = strlen(final_param_start);
            break;
        case kPartitionByDelimiter_:
            if (first_param == true)
                first_param = false;
            else
                final_param_start = next_delim_ptr + 1;

            next_delim_ptr = strstr(final_param_start, grp->ptr->delimiter);
            if (next_delim_ptr == NULL) {
                // if this is not the last parameter, the format is incorrect
                if (var_idx + 1 < param_count)
                    ArgpxExit_(data, kArgpxStatusFlagParamDeficiency);
                param_len = strlen(final_param_start);
            } else {
                param_len = next_delim_ptr - final_param_start;
            }
            break;
        }

        if (param_len == 0)
            ArgpxExit_(data, kArgpxStatusFlagParamDeficiency);

        // add a limit
        if (param_len > max_param_len and max_param_len > 0)
            param_len = max_param_len;

        // i know it's confusing...
        StringNumberToVariable_(final_param_start, param_len, unit_ptr[var_idx].type,
            unit_ptr[var_idx].ptr /* passing secondary pointer */);
    }
}

/*
    Invert the bool value of "toggle_ptr" in flag config
 */
static void ActionSetBool_(struct UnifiedData_ data[static 1], struct ArgpxFlag conf_ptr[static 1])
{
    struct ArgpxHidden_OutcomeSetBool *ptr = &conf_ptr->action_load.set_bool;
    *ptr->target_ptr = ptr->source;
}

/*
    Detect the group where the argument is located.
    A group index will be returned. Use GroupIndexToPointer_() convert it to a pointer.
    return:
      - >= 0: valid index of data.groups[]
      - < 0: no group matched
 */
static int MatchingGroup_(struct UnifiedData_ data[static 1])
{
    char *arg_ptr = data->args[data->arg_idx];
    struct ArgpxFlagGroup *g_ptr;
    int no_prefix_group_idx = -1;
    for (int g_idx = 0; g_idx < data->group_c; g_idx++) {
        g_ptr = &data->groups[g_idx];

        if (g_ptr->prefix[0] == '\0')
            no_prefix_group_idx = g_idx;

        if (strncmp(arg_ptr, g_ptr->prefix, strlen(g_ptr->prefix)) == 0)
            return g_idx;
    }

    if (no_prefix_group_idx >= 0)
        return no_prefix_group_idx;

    return INT_MIN;
}

/*
    Should the assigner of this flag config is mandatory?
 */
static bool ShouldFlagTypeHaveParam_(
    struct UnifiedData_ data[static 1], struct ArgpxFlagGroup group_ptr[static 1], struct ArgpxFlag conf_ptr[static 1])
{
    switch (conf_ptr->action_type) {
    case kArgpxActionSetBool:
        return false;
    case kArgpxActionParamMulti:
    case kArgpxActionParamSingle:
        return true;
    default:
        ArgpxExit_(data, kArgpxStatusActionUnavailable);
    }

    return false;
}

/*
    Matching a name in all flag configs.
    It will find the conf with the highest match length in name_start.
    But if search_first == true, it will find the first matching one:

    e.g. Find "TestTest" and "Test" in "TestTest"
    false: -> "TestTest"
    true:  -> "Test"

    And flag may have assignment symbol, I think the caller should already known the names range.
    In this case, it make sense to get a max_name_len.
    Set max_name_len to <= 0 to disable it

    If "shortest" is true, then shortest matching flag name and ignore tail
 */
static struct ArgpxFlag *MatchingConf_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    char name_start[static 1], int max_name_len, bool shortest)
{
    struct ArgpxFlag *conf_ptr;
    int conf_name_len;
    // we may need to find the longest match
    struct ArgpxFlag *final_conf_ptr = NULL;
    int final_name_len = 0;

    for (int conf_idx = 0; conf_idx < data->conf_c; conf_idx++) {
        conf_ptr = &data->confs[conf_idx];
        if (conf_ptr->group_idx != grp->idx)
            continue;

        conf_name_len = strlen(conf_ptr->name);
        if (max_name_len < conf_name_len and max_name_len > 0)
            continue;

        // matching name
        if (strncmp(name_start, conf_ptr->name, conf_name_len) != 0)
            continue;

        // if matched, update max length record
        if (final_name_len < conf_name_len) {
            final_name_len = conf_name_len;
            final_conf_ptr = conf_ptr;
        }

        if (shortest == true)
            break;
    }

    return final_conf_ptr;
}

/*
    Unlike composable mode, independent mode need to know the exact length of the flag name.
    So it must determine in advance if the assignment symbol exist
 */
static void ParseArgumentIndependent_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1])
{
    char *arg = data->args[data->arg_idx];
    char *name_start = arg + grp->prefix_len;

    char *assigner_ptr;
    if (grp->assigner_toggle == true)
        assigner_ptr = strstr(name_start, grp->ptr->assigner);
    else
        assigner_ptr = NULL;

    int name_len;
    if (assigner_ptr != NULL)
        name_len = assigner_ptr - name_start;
    else
        name_len = strlen(name_start);

    struct ArgpxFlag *conf_ptr = MatchingConf_(data, grp, name_start, name_len, false);
    // some check
    if (conf_ptr == NULL)
        ArgpxExit_(data, kArgpxStatusUnknownFlag);
    if (assigner_ptr != NULL and ShouldFlagTypeHaveParam_(data, grp->ptr, conf_ptr) == false)
        ArgpxExit_(data, kArgpxStatusFlagParamNoNeeded);
    if (ShouldFlagTypeHaveParam_(data, grp->ptr, conf_ptr) == true) {
        if (assigner_ptr != NULL and (grp->ptr->attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER) != 0)
            ArgpxExit_(data, kArgpxStatusAssignmentDisallowAssigner);
        if (assigner_ptr == NULL and (grp->ptr->attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG) != 0)
            ArgpxExit_(data, kArgpxStatusAssignmentDisallowArg);
    }

    // get flag parameters
    switch (conf_ptr->action_type) {
    case kArgpxActionParamMulti:
    case kArgpxActionParamSingle:
        ActionParamAny_(data, grp, conf_ptr, assigner_ptr != NULL ? assigner_ptr + 1 : NULL, 0, true);
        break;
    case kArgpxActionSetBool:
        ActionSetBool_(data, conf_ptr);
        break;
    default:
        ArgpxExit_(data, kArgpxStatusActionUnavailable);
        break;
    }
}

static void ParseArgumentComposable_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1])
{
    char *arg = data->args[data->arg_idx];

    // believe that the prefix exists
    char *base_ptr = arg + grp->prefix_len;
    int remaining_len = strlen(arg) - grp->prefix_len;

    while (remaining_len > 0) {
        struct ArgpxFlag *conf = MatchingConf_(data, grp, base_ptr, 0, true);
        if (conf == NULL)
            ArgpxExit_(data, kArgpxStatusUnknownFlag);
        int name_len = strlen(conf->name);
        remaining_len -= name_len;

        // some windows style...
        // if group attribute not set, next_prefix will always be NULL
        char *next_prefix = NULL;
        if ((grp->ptr->attribute & ARGPX_ATTR_COMPOSABLE_NEED_PREFIX) != 0)
            next_prefix = strstr(base_ptr, grp->ptr->prefix);

        // parameter stuff
        char *param_start = base_ptr + name_len;
        int param_len = 0;
        bool assigner_exist = false;

        switch (conf->action_type) {
        case kArgpxActionParamMulti:
        case kArgpxActionParamSingle:
            // is the assigner exist?
            if (grp->assigner_toggle == true)
                assigner_exist = strncmp(base_ptr + name_len, grp->ptr->assigner, grp->assigner_len) == 0;
            else
                assigner_exist = false;

            if (assigner_exist == true and ShouldFlagTypeHaveParam_(data, grp->ptr, conf) == false)
                ArgpxExit_(data, kArgpxStatusFlagParamNoNeeded);
            if (assigner_exist == true and (grp->ptr->attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER) != 0)
                ArgpxExit_(data, kArgpxStatusAssignmentDisallowAssigner);

            if (assigner_exist) {
                param_start += grp->assigner_len;
                remaining_len -= grp->assigner_len;
            }

            // get parameter length
            if (next_prefix == NULL)
                param_len = strlen(param_start);
            else
                param_len = next_prefix - param_start;
            remaining_len -= param_len;

            // determine the parameter pointer
            if (param_len <= 0)
                param_start = NULL;

            // and do some check
            if (param_start == NULL and (grp->ptr->attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG) != 0)
                ArgpxExit_(data, kArgpxStatusAssignmentDisallowArg);
            if (param_start != NULL and assigner_exist == false
                and (grp->ptr->attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING) != 0)
                ArgpxExit_(data, kArgpxStatusAssignmentDisallowTrailing);

            ActionParamAny_(data, grp, conf, param_start, param_len, remaining_len <= 0 ? true : false);
            break;
        case kArgpxActionSetBool:
            ActionSetBool_(data, conf);
            break;
        default:
            ArgpxExit_(data, kArgpxStatusActionUnavailable);
            break;
        }

        // update base_ptr
        // don't forget the prefix length in the NEED_PREFIX mode
        if (next_prefix == NULL) {
            base_ptr += name_len + param_len;
            if (assigner_exist == true)
                base_ptr += grp->assigner_len;
        } else {
            base_ptr = next_prefix + grp->prefix_len;
            remaining_len -= grp->prefix_len;
        }
    }
}

/*
    If ErrorCallback is NULL then use exit(EXIT_FAILURE),
    if is a function then need accept a result structure, this should help

    The result struct ArgpxResult needs to be freed up manually

    TODO I don't like this parameter style
 */
struct ArgpxResult *ArgpxMain(int argc, int arg_base, char *argv[static argc], int group_count,
    struct ArgpxFlagGroup groups[static group_count], int opt_count, struct ArgpxFlag opts[static opt_count],
    void (*ErrorCallback)(struct ArgpxResult *))
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

    if (data.res == NULL)
        ArgpxExit_(&data, kArgpxStatusFailure);
    *data.res = (struct ArgpxResult){.status = kArgpxStatusSuccess, .argc = argc, .argv = argv};

    for (; data.arg_idx < data.arg_c == true; data.arg_idx++) {
        // update index record
        data.res->current_argv_idx = data.arg_idx;
        data.res->current_argv_ptr = data.args[data.arg_idx];

        struct UnifiedGroupCache_ grp = {0};
        grp.idx = MatchingGroup_(&data);
        if (grp.idx < 0) {
            AppendCommandParameter_(&data);
            continue;
        }
        grp.ptr = &data.groups[grp.idx];

        grp.prefix_len = strlen(grp.ptr->prefix);

        // empty string checks
        grp.assigner_toggle = grp.ptr->assigner != NULL ? true : false;
        if (grp.assigner_toggle == true)
            grp.assigner_len = strlen(grp.ptr->assigner);
        if (grp.assigner_len == 0 and grp.assigner_toggle == true)
            ArgpxExit_(&data, kArgpxStatusGroupConfigEmptyString);

        grp.delimiter_toggle = grp.ptr->delimiter != NULL ? true : false;
        if (grp.delimiter_toggle == true)
            grp.delimiter_len = strlen(grp.ptr->delimiter);
        if (grp.delimiter_len == 0 and grp.delimiter_toggle == true)
            ArgpxExit_(&data, kArgpxStatusGroupConfigEmptyString);


        if ((grp.ptr->attribute & ARGPX_ATTR_COMPOSABLE) != 0)
            ParseArgumentComposable_(&data, &grp);
        else
            ParseArgumentIndependent_(&data, &grp);
    }

    return data.res;
}
