#include "argpx/argpx.h"

#include <inttypes.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "argpx_hash.h"

#define ARGPX_FLAG_TABLE_LOADFACTOR 0.75
// #define ARGPX_USE_HASH // TODO change name

struct FlagTableUnit_ {
    // only check if the root key is used
    bool used;
    struct ArgpxFlag conf;
    // if not NULL, then it is a linked list
    struct FlagTableUnit_ *next;
};

struct FlagTable_ {
    int row_c;
    int col_c; // calculated by TABLE_LOADFACTOR dynamics
    struct FlagTableUnit_ *array;
};

/*
    An unified data of this library.
 */
struct UnifiedData_ {
    // the result structure of the main function
    // this structure must not be on the stack
    struct ArgpxResult *res;
    // arguments count
    int arg_c;
    // pointer to arguments array
    char **arg_v;
    // the arg index being processed, it records the first unprocessed arg.
    // no no no, try to make it to record the last processed arg
    int arg_idx;
    struct ArgpxStyle style;
    // config set
    struct ArgpxFlagSet conf;
    // config table
    struct FlagTable_ conf_table;
    struct ArgpxTerminateMethod terminate;
};

struct UnifiedGroupCache_ {
    int idx;
    struct ArgpxGroup item;
    size_t prefix_len;
    size_t assigner_len;
    bool assigner_toggle;
    size_t delimiter_len;
    bool delimiter_toggle;
};

/*
    Search "needle" in "haystack", limited to the first "len" chars of haystack.
 */
static char *strnstr_(const void *haystack_in, const void *needle_in, size_t haystack_len)
{
    // return memmem(haystack, len, needle, strlen(needle));
    // from GNU libc
    // this won't make benchmark faster, the haystack is too short, not even now

    const char *haystack = haystack_in;
    const char *needle = needle_in;

    if (haystack == NULL or needle == NULL)
        return NULL;
    if (needle[0] == '\0')
        return (char *)haystack;
    if (haystack_len == 0)
        return NULL;

    for (size_t i = 0; i < haystack_len; i++) {
        if (haystack[i] != needle[0])
            continue;
        for (size_t j = 0; i + j < haystack_len; j++) {
            if (haystack[i + j] != needle[j])
                break;
            if (needle[j + 1] == '\0')
                return (char *)&haystack[i];
        }
    }

    return NULL;
}

/*
    Grow 1 slot for given array, but batch allocation of memorys.
    Return the array base pointer.

    return NULL: error
 */
static void *BatchGrow_(void *base, size_t unit_size, unsigned int current_c, unsigned int batch)
{
    if (unit_size == 0 or batch == 0)
        return NULL;

    if (base == NULL or current_c == 0) {
        base = malloc(unit_size * batch);
    } else if (current_c % batch == 0) {
        // is full
        base = realloc(base, unit_size * (current_c + batch));
    }
    if (base == NULL)
        return NULL;

    return base;
}

/*
    Convert structure ArgpxResult's ArgpxStatus enum to string.
 */
char *ArgpxStatusString(const enum ArgpxStatus status)
{
    switch (status) {
    case kArgpxStatusSuccess:
        return "Process success";
    case kArgpxStatusFailure:
        return "Generic unknown error";
    case kArgpxStatusMemoryError:
        return "Memory error";
    case kArgpxStatusConfigInvalid:
        return "There is an invalid group config, perhaps empty string";
    case kArgpxStatusUnknownFlag:
        return "Unknown flag name but the group prefix matched";
    case kArgpxStatusActionUnavailable:
        return "Flag action unavailable. Not implemented or configuration conflict";
    case kArgpxStatusArgumentsDeficiency:
        return "There is no more argument available to get";
    case kArgpxStatusParamNoNeeded:
        return "This flag don't need parameter, but the input seems to be assigning a value";
    case kArgpxStatusAssignmentDisallowAssigner:
        return "Detected assignment mode is Assigner, but unavailable";
    case kArgpxStatusAssignmentDisallowTrailing:
        return "Detected assignment mode is Trailing, but unavailable";
    case kArgpxStatusAssignmentDisallowArg:
        return "Detected assignment mode is Arg(argument), but unavailable";
    case kArgpxStatusParamDeficiency:
        return "Flag gets insufficient parameters";
    case kArgpxStatusBizarreFormat:
        return "Bizarre format occurs";
    default:
        return "[Status code not recorded]";
    }
}

/*
    For debug and more right to know, function will return the new group's index number.
    It can be used in .group_idx of struct ArgpxFlag.
    Expand 3 slots at once.

    return negative: error
 */
int ArgpxGroupAppend(struct ArgpxStyle style[static 1], const struct ArgpxGroup new[static 1])
{
    if (style == NULL or new == NULL)
        return -1;

    style->group_v = BatchGrow_(style->group_v, sizeof(struct ArgpxGroup), style->group_c, 3);
    if (style->group_v == NULL)
        return -1;

    style->group_c += 1;
    int new_idx = style->group_c - 1;
    style->group_v[new_idx] = *new;

    return new_idx;
}

/*
    Like ArgpxGroupAppend(), for debug or other reason it'll return the new flag index.
    Expand 3 slots at once.
 */
int ArgpxSymbolAppend(struct ArgpxStyle style[static 1], const struct ArgpxSymbol new[static 1])
{
    if (style == NULL or new == NULL)
        return -1;

    style->symbol_v = BatchGrow_(style->symbol_v, sizeof(struct ArgpxSymbol), style->symbol_c, 3);
    if (style->symbol_v == NULL)
        return -1;

    style->symbol_c += 1;
    int new_idx = style->symbol_c - 1;
    style->symbol_v[new_idx] = *new;

    return new_idx;
}

void ArgpxStyleFree(struct ArgpxStyle style[static 1])
{
    free(style->group_v);
    free(style->symbol_v);
}

/*
    Return the new flag's index.
    Like ArgpxGroupAppend(), for debug or other reason it'll return the new flag index.
    Expand 16 slots at once.

    return negative: error
 */
int ArgpxFlagAppend(struct ArgpxFlagSet set[static 1], const struct ArgpxFlag new[static 1])
{
    if (set == NULL or new == NULL)
        return -1;

    set->ptr = BatchGrow_(set->ptr, sizeof(struct ArgpxFlag), set->count, 16);
    if (set->ptr == NULL)
        return -1;

    set->count += 1;
    int new_idx = set->count - 1;
    set->ptr[new_idx] = *new;

    return new_idx;
}

void ArgpxFlagFree(struct ArgpxFlagSet set[static 1])
{
    free(set->ptr);
}

void ArgpxResultFree(struct ArgpxResult res[static 1])
{
    free(res->param_v);
    free(res);
}

#ifdef ARGPX_USE_HASH

/*
    Return the pointer of table self.
    return NULL: error
 */
static struct FlagTable_ *FlagTableAlloc_(struct FlagTable_ table[static 1], int group_count, int flag_count)
{
    table->row_c = group_count;
    table->col_c = flag_count / ARGPX_FLAG_TABLE_LOADFACTOR;
    table->array = malloc(sizeof(struct FlagTableUnit_) * table->row_c * table->col_c);
    if (table->array == NULL)
        return NULL;

    return table;
}

/*
    return NULL: error
 */
static struct FlagTableUnit_ *FlagTableEnterUnit_(struct FlagTableUnit_ unit[static 1])
{
    while (unit->used == true) {
        if (unit->next == NULL) {
            unit->next = malloc(sizeof(struct FlagTableUnit_));
            if (unit->next == NULL)
                return NULL;
        }
        unit = unit->next;
    }

    return unit;
}

/*
    Initialize the incoming table. That should be on the stack.

    return the "table" parameter self.
    return NULL: error
 */
static struct FlagTable_ *FlagTableMake_(
    struct ArgpxStyle style[static 1], struct ArgpxFlagSet flagset[static 1], struct FlagTable_ table[static 1])
{
    table = FlagTableAlloc_(table, style->group_c, flagset->count);
    if (table == NULL) {
        return NULL;
    }

    for (int i = 0; i < table->row_c * table->col_c; i++) {
        table->array[i] = (struct FlagTableUnit_){.used = false};
    }

    // init finished

    for (int i = 0; i < flagset->count; i++) {
        struct ArgpxFlag *conf = &flagset->ptr[i];

        uint32_t name_hash = ArgpxHashFnv1aB32(conf->name, strlen(conf->name), ARGPX_HASH_FNV1A_32_INIT);
        struct FlagTableUnit_ *unit = table->array + conf->group_idx * table->col_c;
        unit += name_hash % table->col_c;

        unit = FlagTableEnterUnit_(unit);
        if (unit == NULL) {
            free(table->array);
            return NULL;
        }

        *unit = (struct FlagTableUnit_){.used = true, .conf = *conf, .next = NULL};
    }

    return table;
}

/*
    Note: it won't free up the parameter "unit", that's managed by table.array.
    Note: this function won't check .used element of root unit.
 */
static void FlagTableFreeRecursiveUnit_(struct FlagTableUnit_ unit[static 1])
{
    if (unit->next == NULL)
        return;

    unit = unit->next;
    do {
        struct FlagTableUnit_ *ahead_unit = unit->next;
        free(unit);
        unit = ahead_unit;
    } while (unit != NULL);
}

static void FlagTableFree_(struct FlagTable_ table[static 1])
{
    for (int i = 0; i < table->row_c * table->col_c; i++) {
        struct FlagTableUnit_ *unit = &table->array[i];
        if (unit->used == false)
            continue;
        FlagTableFreeRecursiveUnit_(unit);
    }
    free(table->array);
}

#endif

/*
    Using the offset shift arguments, it will be safe.
    Return a pointer to the new argument.

    return NULL: error and set status
 */
static char *ShiftArguments_(struct UnifiedData_ data[static 1], const int offset)
{
    if (data->arg_idx + offset >= data->arg_c) {
        data->res->status = kArgpxStatusArgumentsDeficiency;
        return NULL;
    }
    data->arg_idx += offset;

    return data->arg_v[data->arg_idx];
}

/*
    Copy the current argument to result data structure as a command parameter.

    return:
        0: ok
        -1: error, status is set
        -2: need terminate processing as required by MainOption
 */
static int AppendCommandParameter_(struct UnifiedData_ data[static 1], char *str)
{
    struct ArgpxResult *res = data->res;

    if (res->param_c == 0) {
        res->param_c = 1;
        res->param_v = malloc(sizeof(char *) * 3);
    } else {
        res->param_c += 1;
        if (res->param_c % 4 == 0)
            res->param_v = realloc(res->param_v, sizeof(char *) * (res->param_c + 3));
    }
    if (res->param_v == NULL) {
        res->status = kArgpxStatusMemoryError;
        return -1;
    }

    res->param_v[res->param_c - 1] = str;

    if (data->terminate.method == kArgpxTerminateCmdparamLimit) {
        if (res->param_c >= data->terminate.load.cmdparam_limit.limit)
            return -2;
    }

    return 0;
}

/*
    return:
        < 0 == Error
        0 == false
        1 == true
 */
static int StringIsBool_(const char *string, const size_t length)
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
    And assign it to a pointer.

    The "max_len" is similar to strncmp()'s "n"
 */
static void StringToType_(const char *source_str, const int max_len, const enum ArgpxVarType type, void *ptr)
{
    // prepare a separate string
    size_t str_size = strlen(source_str);
    // chose the smallest one
    if (max_len < str_size)
        str_size = max_len;
    str_size += 1;

    // allocate a new string
    char *value_str = malloc(str_size);
    memcpy(value_str, source_str, str_size);
    value_str[str_size - 1] = '\0'; // actual_len is the last index of this string

    // remember to change the first level pointer, but not just change secondary one
    // if can, the value_str needs to free up
    switch (type) {
    case kArgpxVarString:
        *(char **)ptr = value_str;
        return; // dont't free up
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
    If param_start is NULL, shift to the next argument.

    return negative: error and set status
 */
static int ActionParamSingle_(
    struct UnifiedData_ data[static 1], struct ArgpxFlag conf[static 1], char *param_start, int param_len)
{
    const struct ArgpxOutParamSingle *unit = &conf->action_load.param_single;

    if (param_start == NULL)
        param_start = ShiftArguments_(data, 1);
    if (param_start == NULL)
        return -1;

    if (param_len <= 0)
        param_len = strlen(param_start);

    if (param_len <= 0) {
        data->res->status = kArgpxStatusParamDeficiency;
        return -1;
    }

    StringToType_(param_start, param_len, unit->type, unit->var_ptr);

    return 0;
}

/*
    If param_start_ptr is NULL, then use the next argument string, which also respect the delimiter.

    If range <= 0 then no limit.

    return negative: error and set status
 */
static int ActionParamMulti_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    struct ArgpxFlag conf[static 1], char *param_base, size_t range)
{
    char *param_now;
    size_t remaining;
    for (int unit_idx = 0; unit_idx < conf->action_load.param_multi.count; unit_idx++) {
        struct ArgpxOutParamSingle *unit = &conf->action_load.param_multi.unit_v[unit_idx];

        if (unit_idx == 0) {
            param_now = param_base != NULL ? param_base : ShiftArguments_(data, 1);
            if (param_now == NULL)
                return -1;
            remaining = range > 0 ? range : strlen(param_base);
        }

        size_t param_len;
        char *delimiter_ptr = strnstr_(param_now, grp->item.delimiter, remaining);
        if (delimiter_ptr == NULL) {
            if (unit_idx == conf->action_load.param_multi.count - 1) {
                param_len = remaining;
            } else {
                data->res->status = kArgpxStatusParamDeficiency;
                return -1;
            }
        } else {
            param_len = delimiter_ptr - param_now;
        }

        StringToType_(param_now, param_len, unit->type, unit->var_ptr);

        size_t seq_len = param_len + grp->delimiter_len;
        param_now += seq_len;
        // no --test=a,b,
        // remaining always > 1 * seq or = seq - delimiter
        if (remaining == seq_len) {
            data->res->status = kArgpxStatusBizarreFormat;
            return -1;
        }
        remaining -= seq_len;
    }

    return 0;
}

/*
    Append a new item into a ParamList action. It can only be attached to the tail.
    The last_idx acts as both the counter(index + 1) and new item index.

    return negative: error(memory error)
 */
static int ParamListAppend_(
    const struct ArgpxOutParamList outcome[static 1], const int last_idx, const char *str, const size_t str_len)
{
    *outcome->count_ptr = last_idx + 1;

    char **list = *outcome->list_ptr;

    if (last_idx == 0) {
        list = malloc(sizeof(char *) * 3);
    } else if ((last_idx + 1) % 4 == 0) {
        list = realloc(*outcome->list_ptr, sizeof(char *) * (last_idx + 1 + 3));
    }
    if (list == NULL)
        return -1;

    char *new_str = malloc(sizeof(char) * str_len + 1);
    if (new_str == NULL)
        return -1;

    memcpy(new_str, str, str_len);
    new_str[str_len] = '\0';

    list[last_idx] = new_str;

    *outcome->list_ptr = list;

    return 0;
}

/*
    Clean up the struct ArgpxOutParamList.
 */
void ArgpxParamListFree(const int count, char **list)
{
    for (int i = 0; i < count; i++)
        free(list[i]);
    free(list);
}

/*
    If max_param_len <= 0 then no limit.

    return negative: error and set status
 */
static int ActionParamList_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    struct ArgpxFlag conf_ptr[static 1], char *param_start_ptr, size_t max_param_len)
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
                data->res->status = kArgpxStatusBizarreFormat;
                return -1;
            }
        }

        if (ParamListAppend_(&conf_ptr->action_load.param_list, param_idx, param_now, param_len) < 0) {
            data->res->status = kArgpxStatusMemoryError;
            return -1;
        }

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
static void ActionSetMemory_(struct UnifiedData_ data[static 1], struct ArgpxFlag conf_ptr[static 1])
{
    struct ArgpxOutSetMemory *ptr = &conf_ptr->action_load.set_memory;
    memcpy(ptr->target_ptr, ptr->source_ptr, ptr->size);
}

/*
    kArgpxActionSetBool
 */
static void ActionSetBool_(struct UnifiedData_ data[static 1], struct ArgpxFlag conf_ptr[static 1])
{
    struct ArgpxOutSetBool *ptr = &conf_ptr->action_load.set_bool;
    *ptr->target_ptr = ptr->source;
}

/*
    kArgpxActionSetInt
 */
static void ActionSetInt_(struct UnifiedData_ data[static 1], struct ArgpxFlag conf_ptr[static 1])
{
    struct ArgpxOutSetInt *ptr = &conf_ptr->action_load.set_int;
    *ptr->target_ptr = ptr->source;
}

/*
    Detect the group where the argument is located.
    A group index will be returned. Use GroupIndexToPointer_() convert it to a pointer.
    return:
      >= 0: valid index of data.groups[]
      < 0: it's a command parameter, not flag
 */
static int MatchingGroup_(int group_c, struct ArgpxGroup group_v[static 1], char arg[static 1])
{
    struct ArgpxGroup grp;
    int no_prefix_group_idx = -1;
    for (int g_idx = 0; g_idx < group_c; g_idx++) {
        grp = group_v[g_idx];

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
    return negative: error
 */
static int GroupCacheInit_(struct UnifiedGroupCache_ grp[static 1])
{
    grp->prefix_len = strlen(grp->item.prefix);

    // empty string checks
    grp->assigner_toggle = grp->item.assigner != NULL;
    grp->assigner_len = grp->assigner_toggle == true ? strlen(grp->item.assigner) : 0;

    // don't need update argpx_errno here
    if (grp->assigner_toggle == true and grp->assigner_len == 0)
        return -1;

    grp->delimiter_toggle = grp->item.delimiter != NULL;
    grp->delimiter_len = grp->delimiter_toggle == true ? strlen(grp->item.delimiter) : 0;

    if (grp->delimiter_toggle == true and grp->delimiter_len == 0)
        return -1;

    return 0;
}

/*
    Should the assigner of this flag config is mandatory?
 */
static bool ShouldFlagTypeHaveParam_(struct UnifiedData_ data[static 1], const struct ArgpxFlag conf_ptr[static 1])
{
    switch (conf_ptr->action_type) {
    case kArgpxActionSetMemory:
    case kArgpxActionSetBool:
    case kArgpxActionSetInt:
    case kArgpxActionCallbackOnly:
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
    But if shortest == true, it will find the first matching one:

    e.g. Find "TestTest" and "Test" in "TestTest"
    false: -> "TestTest"
    true:  -> "Test"

    And flag may have assignment symbol, I think the caller should already known the names range.
    In this case, it make sense to get a max_name_len.
    Set max_name_len to <= 0 to disable it.

    If "shortest" is true, then shortest matching flag name and ignore tail.

    return NULL: error and set status
 */
static struct ArgpxFlag *MatchConfLinear_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    char name_start[static 1], size_t max_name_len, bool shortest)
{
    bool tail_limit = max_name_len <= 0 ? false : true;
    // we may need to find the longest match
    struct ArgpxFlag *longest_conf = NULL;
    int longest_len = 0;

    for (int conf_idx = 0; conf_idx < data->conf.count; conf_idx++) {
        struct ArgpxFlag *conf = &data->conf.ptr[conf_idx];
        if (conf->group_idx != grp->idx)
            continue;

        size_t conf_name_len = strlen(conf->name);
        if (tail_limit == true and conf_name_len > max_name_len)
            continue;

        size_t match_len;
        if (shortest != true) {
            match_len = tail_limit == true ? max_name_len : strlen(name_start);
            if (match_len != conf_name_len)
                continue;
        } else {
            match_len = conf_name_len;
        }

        // matching name
        if (strncmp(name_start, conf->name, match_len) != 0)
            continue;

        // if matched, update max length record
        if (longest_len < conf_name_len) {
            longest_len = conf_name_len;
            longest_conf = conf;
        }

        if (shortest == true)
            break;
    }

    if (longest_conf == NULL)
        data->res->status = kArgpxStatusUnknownFlag;
    return longest_conf;
}

/*
    return NULL: error and set status
 */
static struct ArgpxFlag *MatchConfHash_(
    struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1], char name[static 1], size_t name_len)
{
    if (name_len == 0) {
        data->res->status = kArgpxStatusFailure;
        return NULL;
    }

    uint32_t name_hash = ArgpxHashFnv1aB32(name, name_len, ARGPX_HASH_FNV1A_32_INIT);
    struct FlagTableUnit_ *unit = data->conf_table.array + grp->idx * data->conf_table.col_c;
    unit += name_hash % data->conf_table.col_c;

    if (unit->used == false) {
        data->res->status = kArgpxStatusUnknownFlag;
        return NULL;
    }

    while (true) {
        if (strncmp(unit->conf.name, name, name_len) == 0)
            return &unit->conf;

        if (unit->next == NULL) {
            data->res->status = kArgpxStatusUnknownFlag;
            return NULL;
        }

        unit = unit->next;
    }
}

/*
    A collection of MatchConf*() functions.

    TODO don't be ugly
 */
static struct ArgpxFlag *MatchConf_(struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1],
    char name_start[static 1], size_t max_name_len, bool shortest)
{
#ifndef ARGPX_USE_HASH
    return MatchConfLinear_(data, grp, name_start, max_name_len, shortest);
#endif

    if (max_name_len == 0) {
        return MatchConfLinear_(data, grp, name_start, 0, shortest);
    } else {
        return MatchConfHash_(data, grp, name_start, max_name_len);
    }
}

/*
    Return matched symbol item index of sym_v.
    Return negative num is not match.
 */
static int MatchSymbol_(const char *target, const int sym_c, const struct ArgpxSymbol *sym_v)
{
    for (int i = 0; i < sym_c; i++) {
        if (strcmp(target, sym_v[i].str) == 0)
            return i;
    }

    return -1;
}

/*
    Unlike composable mode, independent mode need to know the exact length of the flag name.
    So it must determine in advance if the assignment symbol exist.

    return negative: error and set status
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

    size_t name_len;
    if (assigner_ptr != NULL)
        name_len = assigner_ptr - name_start;
    else
        name_len = strlen(name_start);

    struct ArgpxFlag *conf = MatchConf_(data, grp, name_start, name_len, false);
    // some check
    if (conf == NULL)
        return -1;
    if (assigner_ptr != NULL and ShouldFlagTypeHaveParam_(data, conf) == false) {
        data->res->status = kArgpxStatusParamNoNeeded;
        return -1;
    }
    if (ShouldFlagTypeHaveParam_(data, conf) == true) {
        if (assigner_ptr != NULL and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER) != 0) {
            data->res->status = kArgpxStatusAssignmentDisallowAssigner;
            return -1;
        }
        if (assigner_ptr == NULL and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG) != 0) {
            data->res->status = kArgpxStatusAssignmentDisallowArg;
            return -1;
        }
    }

    char *param_base = assigner_ptr != NULL ? assigner_ptr + grp->assigner_len : NULL;
    // get flag parameters
    switch (conf->action_type) {
    case kArgpxActionParamSingle:
        if (ActionParamSingle_(data, conf, param_base, 0) < 0)
            return -1;
        break;
    case kArgpxActionParamMulti:
        if (ActionParamMulti_(data, grp, conf, param_base, 0) < 0)
            return -1;
        break;
    case kArgpxActionParamList:
        if (ActionParamList_(data, grp, conf, param_base, 0) < 0)
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
    case kArgpxActionCallbackOnly:
        break;
    }

    if (conf->callback != NULL)
        conf->callback(&conf->action_load, conf->callback_param);

    return 0;
}

/*
    return negative: error and errno is set
 */
static int ParseArgumentComposable_(
    struct UnifiedData_ data[static 1], struct UnifiedGroupCache_ grp[static 1], char *arg)
{
    // believe that the prefix exists
    char *base_ptr = arg + grp->prefix_len;
    size_t remaining_len = strlen(arg) - grp->prefix_len;

    while (remaining_len > 0) {
        struct ArgpxFlag *conf = MatchConf_(data, grp, base_ptr, 0, true);
        if (conf == NULL)
            return -1;
        size_t name_len = strlen(conf->name);
        remaining_len -= name_len;

        // some windows style...
        // if group attribute not set, next_prefix will always be NULL
        char *next_prefix = NULL;
        if ((grp->item.attribute & ARGPX_ATTR_COMPOSABLE_NEED_PREFIX) != 0)
            next_prefix = strstr(base_ptr, grp->item.prefix);

        // parameter stuff
        char *param_start = base_ptr + name_len;
        bool conf_have_param = ShouldFlagTypeHaveParam_(data, conf);

        bool assigner_exist;
        // is the assigner exist?
        if (grp->assigner_toggle == true)
            assigner_exist = strncmp(param_start, grp->item.assigner, grp->assigner_len) == 0;
        else
            assigner_exist = false;

        if (assigner_exist == true and conf_have_param == false) {
            data->res->status = kArgpxStatusParamNoNeeded;
            return -1;
        }
        if (assigner_exist == true and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER) != 0) {
            data->res->status = kArgpxStatusAssignmentDisallowAssigner;
            return -1;
        }

        if (assigner_exist == true) {
            param_start += grp->assigner_len;
            remaining_len -= grp->assigner_len;
        }

        size_t param_len = 0;
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
            data->res->status = kArgpxStatusAssignmentDisallowArg;
            return -1;
        }
        if (param_start != NULL and assigner_exist == false
            and (grp->item.attribute & ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING) != 0)
        {
            data->res->status = kArgpxStatusAssignmentDisallowTrailing;
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
        case kArgpxActionCallbackOnly:
            break;
        }

        if (conf->callback != NULL)
            conf->callback(&conf->action_load, conf->callback_param);

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

    If the terminate param is NULL, that's same as {.method = kArgpxTerminateNone} of struct ArgpxTerminateMethod.

    return NULL: result structure can't allocated.
 */
struct ArgpxResult *ArgpxParse(int arg_c, char **arg_v, struct ArgpxStyle *style, struct ArgpxFlagSet *flag,
    struct ArgpxTerminateMethod *terminate)
{
    struct UnifiedData_ data = {
        .arg_c = arg_c,
        .arg_v = arg_v,
        .arg_idx = 0,
        .style = *style,
        .conf = *flag,
    };

    data.res = malloc(sizeof(struct ArgpxResult));
    if (data.res == NULL)
        return NULL;
    *data.res = (struct ArgpxResult){
        .status = kArgpxStatusSuccess,
        .param_c = 0,
        .param_v = NULL,
    };

    if (terminate == NULL)
        data.terminate = (struct ArgpxTerminateMethod){.method = kArgpxTerminateNone};
    else
        data.terminate = *terminate;

#ifdef ARGPX_USE_HASH
    if (FlagTableMake_(&data.style, &data.conf, &data.conf_table) == NULL) {
        data.res->status = kArgpxStatusMemoryError;
        return data.res;
    }
#endif

    bool stop_parsing = false;
    for (; data.arg_idx < data.arg_c; data.arg_idx++) {
        // update index record
        data.res->current_argv_idx = data.arg_idx;
        data.res->current_argv_ptr = data.arg_v[data.arg_idx];

        char *arg = data.arg_v[data.arg_idx];

        if (stop_parsing == true) {
            if (AppendCommandParameter_(&data, arg) < 0)
                goto out;
            continue;
        }

        int symbol_idx = MatchSymbol_(arg, data.style.symbol_c, data.style.symbol_v);
        if (symbol_idx >= 0) {
            struct ArgpxSymbol *sym = &data.style.symbol_v[symbol_idx];
            switch (sym->type) {
            case kArgpxSymbolStopParsing:
                stop_parsing = true;
                continue;
            case kArgpxSymbolTerminateProcessing:
                goto out;
            case kArgpxSymbolCallback:
                sym->callback(sym->callback_param);
                continue;
            }
        }

        struct UnifiedGroupCache_ grp = {0};
        grp.idx = MatchingGroup_(data.style.group_c, data.style.group_v, arg);

        if (grp.idx < 0) {
            if (AppendCommandParameter_(&data, arg) < 0)
                goto out;
            continue;
        }
        grp.item = data.style.group_v[grp.idx];

        if (GroupCacheInit_(&grp) < 0) {
            data.res->status = kArgpxStatusConfigInvalid;
            goto out;
        }

        int ret;
        if ((grp.item.attribute & ARGPX_ATTR_COMPOSABLE) != 0)
            ret = ParseArgumentComposable_(&data, &grp, arg);
        else
            ret = ParseArgumentIndependent_(&data, &grp, arg);

        if (ret < 0)
            goto out;
    }

out:
#ifdef ARGPX_USE_HASH
    FlagTableFree_(&data.conf_table);
#endif
    return data.res;
}
