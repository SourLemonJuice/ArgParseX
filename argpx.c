#include "argpx/argpx.h"

#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// like errno, non public
static enum ArgpxStatus argpx_errno;

/*
    An unified data of this library
 */
struct UnifiedData_ {
    // the result structure of the main function
    struct ArgpxResult *res;
    // arguments count
    int arg_c;
    // pointer to arguments array
    char **args;
    // the arg index being processed, it records the first unprocessed arg.
    // no no no, try to make it to record the last processed arg
    int arg_idx;
    struct ArgpxStyle style;
    struct ArgpxFlagSet conf;
    struct ArgpxTerminateMethod terminate;
};

struct UnifiedGroupCache_ {
    int idx;
    struct ArgpxGroupItem item;
    int prefix_len;
    int assigner_len;
    bool assigner_toggle;
    int delimiter_len;
    int delimiter_toggle;
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
    Convert structure ArgpxResult's ArgpxStatus enum to string

    // TODO change name
 */
char *ArgpxStatusToString(enum ArgpxStatus status)
{
    switch (status) {
    case kArgpxStatusSuccess:
        return "Processing success";
    case kArgpxStatusFailure:
        return "Generic unknown error, I must be lazy~~~";
    case kArgpxStatusGroupConfigInvalid:
        return "There is an invalid group config, perhaps empty string";
    case kArgpxStatusUnknownFlag:
        return "Unknown flag name but the group matched(by prefix)";
    case kArgpxStatusActionUnavailable:
        return "Flag action type unavailable. Not implemented or configuration conflict";
    case kArgpxStatusArgumentsDeficiency:
        return "There is no more argument available to get";
    case kArgpxStatusParamNoNeeded:
        return "This flag don't need parameter, but the input seems to be assigning a value";
    case kArgpxStatusAssignmentDisallowAssigner:
        return "Detected assignment mode is Assigner, but for some reason, unavailable";
    case kArgpxStatusAssignmentDisallowTrailing:
        return "Detected assignment mode is Trailing, but for some reason, unavailable";
    case kArgpxStatusAssignmentDisallowArg:
        return "Detected assignment mode is Arg(argument), but for some reason, unavailable";
    case kArgpxStatusParamDeficiency:
        return "Flag gets insufficient parameters";
    case kArgpxStatusParamBizarreFormat:
        return "Bizarre format occurs within the range of parameter string";
    default:
        return "[ArgParseX - ArgpxStatusToString(): not added]";
    }
}

void ArgpxAppendGroup(struct ArgpxStyle style[static 1], const struct ArgpxGroupItem new[static 1])
{
    style->group_c += 1;

    style->group_v = realloc(style->group_v, sizeof(struct ArgpxGroupItem[style->group_c]));
    style->group_v[style->group_c - 1] = *new;
}

void ArgpxAppendSymbol(struct ArgpxStyle style[static 1], struct ArgpxKeySymbolItem new[static 1])
{
    style->symbol_c += 1;
    style->symbol_v = realloc(style->symbol_v, sizeof(struct ArgpxKeySymbolItem[style->symbol_c]));
    style->symbol_v[style->symbol_c - 1] = *new;
}

void ArgpxAppendFlag(struct ArgpxFlagSet set[static 1], const struct ArgpxFlagItem new[static 1])
{
    set->count += 1;

    set->ptr = realloc(set->ptr, sizeof(struct ArgpxFlagItem[set->count]));
    set->ptr[set->count - 1] = *new;
}

/*
    Using the offset shift arguments, it will be safe.
    Return a pointer to the new argument

    return NULL: error
 */
static char *ShiftArguments_(struct UnifiedData_ data[static 1], int offset)
{
    if (data->arg_idx + offset >= data->arg_c) {
        argpx_errno = kArgpxStatusArgumentsDeficiency;
        return NULL;
    }
    data->arg_idx += offset;

    return data->args[data->arg_idx];
}

/*
    Copy the current argument to result data structure as a command parameter

    return:
        0: ok
        1(> 0): need terminate processing as required by MainOption
        < 0: error
 */
static int AppendCommandParameter_(struct UnifiedData_ data[static 1], char *str)
{
    struct ArgpxResult *res = data->res;

    res->param_count += 1;
    res->paramv = realloc(res->paramv, sizeof(char * [res->param_count])); // TODO error check
    res->paramv[res->param_count - 1] = str;

    if (data->terminate.method == kArgpxTerminateAtNumberOfCommandParam) {
        if (res->param_count >= data->terminate.load.num_of_cmd_param.limit)
            return 1;
    }

    return 0;
}

/*
    return:
        < 0 == Error
        0 == false
        1 == true
 */
static int StringIsBool_(char *string, int length)
{
    if (length <= 0)
        return -1;

    char *true_list[] = {
        "true",
        "True",
        "TRUE",
    };
    char *false_list[] = {
        "false",
        "False",
        "FALSE",
    };

    for (int i = 0; i < sizeof(true_list) / sizeof(char *); i++) {
        if (strncmp(string, true_list[i], length) == 0)
            return true;
    }

    for (int i = 0; i < sizeof(false_list) / sizeof(char *); i++) {
        if (strncmp(string, false_list[i], length) == 0)
            return false;
    }

    return -1;
}

/*
    Converting a string to a specific type.
    And assign it to a pointer

    The "max_len" is similar to strncmp()'s "n"
 */
static void StringToType_(char *source_str, int max_len, enum ArgpxVarType type, void *ptr)
{
    // prepare a separate string
    int str_len = strlen(source_str);
    int actual_len;
    size_t actual_size;
    // chose the smallest one
    if (max_len < str_len)
        actual_len = max_len;
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
        *(char **)ptr = value_str;
        return;
    case kArgpxVarInt:
        *(int *)ptr = strtoimax(value_str, NULL, 0);
        break;
    case kArgpxVarBool:
        *(bool *)ptr = StringIsBool_(value_str, strlen(value_str));
        break;
    case kArgpxVarFloat:
        *(float *)ptr = strtof(value_str, NULL);
        break;
    case kArgpxVarDouble:
        *(double *)ptr = strtod(value_str, NULL);
        break;
    }

    free(value_str);
}

/*
    If param_len <= 0 then no limit.

    return negative: error
 */
static int ActionParamSingle_(
    struct UnifiedData_ data[static 1], struct ArgpxFlagItem conf[static 1], char *param_start, int param_len)
{
    struct ArgpxParamUnit *unit_ptr = &conf->action_load.param_single;
    if (param_len <= 0)
        param_len = strlen(param_start);

    if (param_len <= 0) {
        argpx_errno = kArgpxStatusParamDeficiency;
        return -1;
    }

    StringToType_(param_start, param_len, unit_ptr->type, unit_ptr->ptr);

    return 0;
}

/*
    If param_start_ptr is NULL, then use the next argument string, which also respect the delimiter

    If range <= 0 then no limit

    return negative: error
 */
static int ActionParamMulti_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    struct ArgpxFlagItem conf[static 1], char *param_base, int range)
{
    char *param_now;
    int remaining;
    for (int unit_idx = 0; unit_idx < conf->action_load.param_multi.count; unit_idx++) {
        struct ArgpxParamUnit *unit = &conf->action_load.param_multi.units[unit_idx];

        if (unit_idx == 0) {
            param_now = param_base != NULL ? param_base : ShiftArguments_(data, 1);
            if (param_now == NULL)
                return -1;
            remaining = range > 0 ? range : strlen(param_base);
        }

        int param_len;
        char *delimiter_ptr = strnstr_(param_now, grp->item.delimiter, remaining);
        if (delimiter_ptr == NULL) {
            if (unit_idx == conf->action_load.param_multi.count - 1) {
                param_len = remaining;
            } else {
                argpx_errno = kArgpxStatusParamDeficiency;
                return -1;
            }
        } else {
            param_len = delimiter_ptr - param_now;
        }

        StringToType_(param_now, param_len, unit->type, unit->ptr);

        int used_len = param_len + grp->delimiter_len;
        param_now += used_len;
        remaining -= used_len;
    }
    // normally, "remaining" is negative. I didn't think to reuse it

    if (remaining >= 0) {
        argpx_errno = kArgpxStatusParamBizarreFormat;
        return -1;
    }
    return 0;
}

/*
    Append a new item into a ParamList action. It can only be attached to the tail.
    The last_idx acts as both the counter(index + 1) and new item index

    return negative: error
    TODO error check
 */
static int AppendParamList_(struct ArgpxHidden_OutcomeParamList outcome[static 1], int last_idx, char *str, int str_len)
{
    *outcome->count = last_idx + 1;

    char **list;
    size_t list_size = sizeof(char * [last_idx + 1]);

    if (last_idx == 0) {
        list = malloc(list_size);
    } else {
        list = realloc(*outcome->params, list_size);
    }

    char *new_str = malloc(sizeof(char[str_len + 1]));
    memcpy(new_str, str, str_len);
    new_str[str_len] = '\0';

    list[last_idx] = new_str;

    *outcome->params = list;

    return 0;
}

/*
    If max_param_len <= 0 then no limit

    return negative: error
 */
static int ActionParamList_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    struct ArgpxFlagItem conf_ptr[static 1], char *param_start_ptr, int max_param_len)
{
    char *param_now = param_start_ptr != NULL ? param_start_ptr : ShiftArguments_(data, 1);
    if (param_now == NULL)
        return -1;
    int remaining_len = max_param_len > 0 ? max_param_len : strlen(param_now);

    char *delimiter_ptr;
    for (int param_idx = 0; remaining_len > 0; param_idx++) {
        delimiter_ptr = strnstr_(param_now, grp->item.delimiter, remaining_len);

        int param_len;
        if (delimiter_ptr == NULL) {
            param_len = remaining_len;
        } else {
            param_len = delimiter_ptr - param_now;
            // delimiter shouldn't exist at the last parameter's tail
            if (param_len + grp->delimiter_len >= remaining_len) {
                argpx_errno = kArgpxStatusParamBizarreFormat;
                return -1;
            }
        }

        if (AppendParamList_(&conf_ptr->action_load.param_list, param_idx, param_now, param_len) < 0)
            return -1;

        int used_len = param_len + grp->delimiter_len;
        // each parameter will have a delimiter, expect the last one
        // it will reduce remaining_len to a negative number, then no next loop
        remaining_len -= used_len;
        param_now += used_len;
    }

    return 0;
}

/*
    kArgpxActionSetMemory
 */
static void ActionSetMemory_(struct UnifiedData_ data[static 1], struct ArgpxFlagItem conf_ptr[static 1])
{
    struct ArgpxHidden_OutcomeSetMemory *ptr = &conf_ptr->action_load.set_memory;
    memcpy(ptr->target_ptr, ptr->source_ptr, ptr->size);
}

/*
    kArgpxActionSetBool
 */
static void ActionSetBool_(struct UnifiedData_ data[static 1], struct ArgpxFlagItem conf_ptr[static 1])
{
    struct ArgpxHidden_OutcomeSetBool *ptr = &conf_ptr->action_load.set_bool;
    *ptr->target_ptr = ptr->source;
}

/*
    kArgpxActionSetInt
 */
static void ActionSetInt_(struct UnifiedData_ data[static 1], struct ArgpxFlagItem conf_ptr[static 1])
{
    struct ArgpxHidden_OutcomeSetInt *ptr = &conf_ptr->action_load.set_int;
    *ptr->target_ptr = ptr->source;
}

static void ActionCallback_(struct UnifiedData_ data[static 1], struct ArgpxFlagItem conf_ptr[static 1])
{
    struct ArgpxHidden_OutcomeCallback *ptr = &conf_ptr->action_load.callback;
    ptr->callback(ptr->param);
}

/*
    Detect the group where the argument is located.
    A group index will be returned. Use GroupIndexToPointer_() convert it to a pointer.
    return:
      >= 0: valid index of data.groups[]
      < 0: it's a command parameter, not flag
 */
static int MatchingGroup_(struct UnifiedData_ data[static 1], char arg[static 1])
{
    struct ArgpxGroupItem grp;
    int no_prefix_group_idx = -1;
    for (int g_idx = 0; g_idx < data->style.group_c; g_idx++) {
        grp = data->style.group_v[g_idx];

        if (grp.prefix[0] == '\0')
            no_prefix_group_idx = g_idx;

        if (strncmp(arg, grp.prefix, strlen(grp.prefix)) == 0)
            return g_idx;
    }

    if (no_prefix_group_idx >= 0)
        return no_prefix_group_idx;

    return -1;
}

/*
    Should the assigner of this flag config is mandatory?
 */
static bool ShouldFlagTypeHaveParam_(struct UnifiedData_ data[static 1], struct ArgpxGroupItem group_ptr[static 1],
    struct ArgpxFlagItem conf_ptr[static 1])
{
    switch (conf_ptr->action_type) {
    case kArgpxActionSetMemory:
    case kArgpxActionSetBool:
    case kArgpxActionSetInt:
    case kArgpxActionCallback:
        return false;
    case kArgpxActionParamSingle:
    case kArgpxActionParamMulti:
    case kArgpxActionParamList:
        return true;
    }

    return false; // make compiler happy... noreturn is baster then this
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

    return NULL: error
 */
static struct ArgpxFlagItem *MatchingConf_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    char name_start[static 1], int max_name_len, bool shortest)
{
    bool tail_limit = max_name_len <= 0 ? false : true;
    // we may need to find the longest match
    struct ArgpxFlagItem *final_conf_ptr = NULL;
    int final_name_len = 0;

    for (int conf_idx = 0; conf_idx < data->conf.count; conf_idx++) {
        struct ArgpxFlagItem *conf_ptr = &data->conf.ptr[conf_idx];
        if (conf_ptr->group_idx != grp->idx)
            continue;

        int conf_name_len = strlen(conf_ptr->name);
        if (tail_limit == true and conf_name_len > max_name_len)
            continue;

        int match_len;
        if (shortest != true) {
            match_len = tail_limit == true ? max_name_len : strlen(name_start);
            if (match_len != conf_name_len)
                continue;
        } else {
            match_len = conf_name_len;
        }

        // matching name
        if (strncmp(name_start, conf_ptr->name, match_len) != 0)
            continue;

        // if matched, update max length record
        if (final_name_len < conf_name_len) {
            final_name_len = conf_name_len;
            final_conf_ptr = conf_ptr;
        }

        if (shortest == true)
            break;
    }

    if (final_conf_ptr == NULL)
        argpx_errno = kArgpxStatusUnknownFlag;
    return final_conf_ptr;
}

/*
    Return matched symbol item index of sym_v.
    Return negative num is not match.
 */
static int MatchingSymbol_(char *target, int sym_c, struct ArgpxKeySymbolItem *sym_v)
{
    for (int i = 0; i < sym_c; i++) {
        if (strcmp(target, sym_v[i].str) == 0)
            return sym_c;
    }

    return -1;
}

/*
    Unlike composable mode, independent mode need to know the exact length of the flag name.
    So it must determine in advance if the assignment symbol exist.

    return negative: error
 */
static int ParseArgumentIndependent_(
    struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1], char *arg)
{
    char *name_start = arg + grp->prefix_len;

    char *assigner_ptr;
    if (grp->assigner_toggle == true)
        assigner_ptr = strstr(name_start, grp->item.assigner);
    else
        assigner_ptr = NULL;

    int name_len;
    if (assigner_ptr != NULL)
        name_len = assigner_ptr - name_start;
    else
        name_len = strlen(name_start);

    struct ArgpxFlagItem *conf_ptr = MatchingConf_(data, grp, name_start, name_len, false);
    // some check
    if (conf_ptr == NULL)
        return -1;
    if (assigner_ptr != NULL and ShouldFlagTypeHaveParam_(data, &grp->item, conf_ptr) == false) {
        argpx_errno = kArgpxStatusParamNoNeeded;
        return -1;
    }
    if (ShouldFlagTypeHaveParam_(data, &grp->item, conf_ptr) == true) {
        if (assigner_ptr != NULL and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER) != 0) {
            argpx_errno = kArgpxStatusAssignmentDisallowAssigner;
            return -1;
        }
        if (assigner_ptr == NULL and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG) != 0) {
            argpx_errno = kArgpxStatusAssignmentDisallowArg;
            return -1;
        }
    }

    char *param_base = assigner_ptr != NULL ? assigner_ptr + grp->assigner_len : NULL;
    // get flag parameters
    switch (conf_ptr->action_type) {
    case kArgpxActionParamSingle:
        if (ActionParamSingle_(data, conf_ptr, param_base, 0) < 0)
            return -1;
        break;
    case kArgpxActionParamMulti:
        if (ActionParamMulti_(data, grp, conf_ptr, param_base, 0) < 0)
            return -1;
        break;
    case kArgpxActionParamList:
        if (ActionParamList_(data, grp, conf_ptr, param_base, 0) < 0)
            return -1;
        break;
    case kArgpxActionSetMemory:
        ActionSetMemory_(data, conf_ptr);
        break;
    case kArgpxActionSetBool:
        ActionSetBool_(data, conf_ptr);
        break;
    case kArgpxActionSetInt:
        ActionSetInt_(data, conf_ptr);
        break;
    case kArgpxActionCallback:
        ActionCallback_(data, conf_ptr);
        break;
    }

    return 0;
}

/*
    return negative: error
 */
static int ParseArgumentComposable_(
    struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1], char *arg)
{
    // believe that the prefix exists
    char *base_ptr = arg + grp->prefix_len;
    int remaining_len = strlen(arg) - grp->prefix_len;

    while (remaining_len > 0) {
        struct ArgpxFlagItem *conf = MatchingConf_(data, grp, base_ptr, 0, true);
        if (conf == NULL)
            return -1;
        int name_len = strlen(conf->name);
        remaining_len -= name_len;

        // some windows style...
        // if group attribute not set, next_prefix will always be NULL
        char *next_prefix = NULL;
        if ((grp->item.attribute & ARGPX_ATTR_COMPOSABLE_NEED_PREFIX) != 0)
            next_prefix = strstr(base_ptr, grp->item.prefix);

        // parameter stuff
        char *param_start = base_ptr + name_len;
        bool conf_have_param = ShouldFlagTypeHaveParam_(data, &grp->item, conf);

        bool assigner_exist;
        // is the assigner exist?
        if (grp->assigner_toggle == true)
            assigner_exist = strncmp(param_start, grp->item.assigner, grp->assigner_len) == 0;
        else
            assigner_exist = false;

        if (assigner_exist == true and conf_have_param == false) {
            argpx_errno = kArgpxStatusParamNoNeeded;
            return -1;
        }
        if (assigner_exist == true and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER) != 0) {
            argpx_errno = kArgpxStatusAssignmentDisallowAssigner;
            return -1;
        }

        if (assigner_exist == true) {
            param_start += grp->assigner_len;
            remaining_len -= grp->assigner_len;
        }

        int param_len = 0;
        if (conf_have_param == true) {
            // get parameter length
            if (next_prefix == NULL)
                param_len = strlen(param_start);
            else
                param_len = next_prefix - param_start;

            // determine the parameter pointer
            if (param_len <= 0)
                param_start = NULL;

            remaining_len -= param_len;
        }

        // and do some check
        if (param_start == NULL and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG) != 0) {
            argpx_errno = kArgpxStatusAssignmentDisallowArg;
            return -1;
        }
        if (param_start != NULL and assigner_exist == false
            and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING) != 0)
        {
            argpx_errno = kArgpxStatusAssignmentDisallowTrailing;
            return -1;
        }

        switch (conf->action_type) {
        case kArgpxActionParamSingle:
            if (ActionParamSingle_(data, conf, param_start, param_len) < 0)
                return -1;
            break;
        case kArgpxActionParamMulti:
            if (ActionParamMulti_(data, grp, conf, param_start, param_len) < 0)
                return -1;
            break;
        case kArgpxActionParamList:
            if (ActionParamList_(data, grp, conf, param_start, param_len) < 0)
                return -1;
            break;
        case kArgpxActionSetMemory:
            ActionSetMemory_(data, conf);
            break;
        case kArgpxActionSetBool:
            ActionSetBool_(data, conf);
            break;
        case kArgpxActionSetInt:
            ActionSetInt_(data, conf);
            break;
        case kArgpxActionCallback:
            ActionCallback_(data, conf);
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

    return 0;
}

/*
    The result struct ArgpxResult needs to be freed up manually.

    return NULL: result structure can't allocated.
 */
struct ArgpxResult *ArgpxMain(struct ArgpxMainOption *func)
{
    argpx_errno = kArgpxStatusSuccess;

    struct UnifiedData_ data = {
        .res = malloc(sizeof(struct ArgpxResult)),
        .arg_c = func->argc,
        .args = func->argv,
        .arg_idx = 0,
        .style = *func->style,
        .conf = *func->flag,
        .terminate = func->terminate,
    };

    if (data.res == NULL) {
        argpx_errno = kArgpxStatusFailure;
        return NULL;
    }
    *data.res = (struct ArgpxResult){.status = kArgpxStatusSuccess};

    bool stop_parsing = false;
    for (; data.arg_idx < data.arg_c; data.arg_idx++) {
        // update index record
        data.res->current_argv_idx = data.arg_idx;
        data.res->current_argv_ptr = data.args[data.arg_idx];

        char *arg = data.args[data.arg_idx];
        int symbol_idx = MatchingSymbol_(arg, data.style.symbol_c, data.style.symbol_v);
        if (symbol_idx >= 0) {
            struct ArgpxKeySymbolItem *sym = &data.style.symbol_v[symbol_idx];
            switch (sym->type) {
            case kArgpxSymbolStopParsing:
                stop_parsing = true;
                continue;
            case kArgpxSymbolTerminateProcessing:
                return data.res;
            case kArgpxSymbolCallback:
                sym->callback(sym->callback_param);
                continue;
            }
        }

        struct UnifiedGroupCache_ grp = {0};
        grp.idx = MatchingGroup_(&data, arg); // TODO error throw

        if (grp.idx < 0 or stop_parsing == true) {
            if (AppendCommandParameter_(&data, arg) > 0) // TODO error throw
                return data.res;
            else
                continue;
        }
        grp.item = data.style.group_v[grp.idx];

        grp.prefix_len = strlen(grp.item.prefix);

        // TODO update those check

        // empty string checks
        grp.assigner_toggle = grp.item.assigner != NULL;
        grp.assigner_len = grp.assigner_toggle == true ? strlen(grp.item.assigner) : 0;

        // don't need update argpx_errno here
        if (grp.assigner_toggle == true and grp.assigner_len == 0) {
            data.res->status = kArgpxStatusGroupConfigInvalid;
            return data.res;
        }

        grp.delimiter_toggle = grp.item.delimiter != NULL;
        grp.delimiter_len = grp.delimiter_toggle == true ? strlen(grp.item.delimiter) : 0;

        if (grp.delimiter_toggle == true and grp.delimiter_len == 0) {
            data.res->status = kArgpxStatusGroupConfigInvalid;
            return data.res;
        }

        int ret;
        if ((grp.item.attribute & ARGPX_ATTR_COMPOSABLE) != 0)
            ret = ParseArgumentComposable_(&data, &grp, arg);
        else
            ret = ParseArgumentIndependent_(&data, &grp, arg);

        if (ret < 0) {
            data.res->status = argpx_errno;
            return data.res;
        }
    }

    return data.res;
}
