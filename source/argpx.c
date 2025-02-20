/*
    Compiler macros(passing them with -D<macro> flag):
    #define ARGPX_ENABLE_HASH           // when searching flags, use hash as much as possible
    #define ARGPX_ENABLE_BATCH_ALLOC    // reduce system calls during configuration
 */
#include "argpx/argpx.h"

#include <assert.h>
#include <inttypes.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARGPX_ENABLE_HASH
#include "argpx_hash.h"
#define ARGPX_FLAG_TABLE_LOADFACTOR 0.75
#endif

struct FlagTableUnit_ {
    // only check if the root key is used
    bool root_used;
    struct ArgpxFlag *conf;
    // if not NULL, then it is a linked list
    struct FlagTableUnit_ *next;
};

struct FlagTable_ {
    int count; // calculated by TABLE_LOADFACTOR dynamics
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
    assert(haystack_in != NULL);
    assert(needle_in != NULL);

    // return memmem(haystack, len, needle, strlen(needle));
    // from GNU libc
    // this won't make benchmark faster, the haystack is too short, not even now

    const char *haystack = haystack_in;
    const char *needle = needle_in;

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
    Grow 1 slot for given array. If batch alloc enabled, it will allocate many items at a time.
    Return the array base pointer.

    return NULL: error
 */
static void *ArrGrowOneSlot_(void *base, size_t unit_size, unsigned int current_c, unsigned int batch)
{
    assert(unit_size > 0);
    assert(batch > 0);

#ifdef ARGPX_ENABLE_BATCH_ALLOC
    if (base == NULL) {
        base = malloc(unit_size * batch);
    } else if (current_c % batch == 0) {
        // is full
        base = realloc(base, unit_size * (current_c + batch));
    }
#else
    base = realloc(base, unit_size * (current_c + 1));
#endif

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
        return "Invalid group config, perhaps empty string";
    case kArgpxStatusUnknownFlag:
        return "Unknown flag name, but group identified";
    case kArgpxStatusActionUnavailable:
        return "Flag action unavailable. Not implemented or configuration conflict";
    case kArgpxStatusParamExcess:
        return "Flag gets excess parameters";
    case kArgpxStatusParamNoNeeded:
        return "Flag doesn't need parameters, but a value is being assigned";
    case kArgpxStatusAssignmentDisallowAssigner:
        return "Detected assignment mode is Assigner, but unavailable";
    case kArgpxStatusAssignmentDisallowTrailing:
        return "Detected assignment mode is Trailing, but unavailable";
    case kArgpxStatusAssignmentDisallowArg:
        return "Detected assignment mode is Arg, but unavailable";
    case kArgpxStatusParamInsufficient:
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
    If batch alloc enabled, expand 3 slots at once.

    return negative: error
 */
int ArgpxGroupAppend(struct ArgpxStyle *style, const struct ArgpxGroup *new)
{
    assert(style != NULL);
    assert(new != NULL);

    style->group_v = ArrGrowOneSlot_(style->group_v, sizeof(struct ArgpxGroup), style->group_c, 3);
    if (style->group_v == NULL)
        return -1;

    style->group_c += 1;
    int new_idx = style->group_c - 1;
    style->group_v[new_idx] = *new;

    return style->group_c - 1;
}

/*
    Like ArgpxGroupAppend(), return the new symbol index.
    If batch alloc enabled, expand 3 slots at once.

    Return negative: error
 */
int ArgpxSymbolAppend(struct ArgpxStyle *style, const struct ArgpxSymbol *new)
{
    assert(style != NULL);
    assert(new != NULL);

    style->symbol_v = ArrGrowOneSlot_(style->symbol_v, sizeof(struct ArgpxSymbol), style->symbol_c, 3);
    if (style->symbol_v == NULL)
        return -1;

    style->symbol_c += 1;
    int new_idx = style->symbol_c - 1;
    style->symbol_v[new_idx] = *new;

    return new_idx;
}

void ArgpxStyleFree(struct ArgpxStyle *style)
{
    assert(style != NULL);

    free(style->group_v);
    free(style->symbol_v);
}

/*
    Like ArgpxGroupAppend(), return the new flag index.
    If batch alloc enabled, expand 16 slots at once.

    Return negative: error
 */
int ArgpxFlagAppend(struct ArgpxFlagSet *set, const struct ArgpxFlag *new)
{
    assert(set != NULL);
    assert(new != NULL);

    set->ptr = ArrGrowOneSlot_(set->ptr, sizeof(struct ArgpxFlag), set->count, 16);
    if (set->ptr == NULL)
        return -1;

    set->count += 1;
    int new_idx = set->count - 1;
    set->ptr[new_idx] = *new;

    return new_idx;
}

void ArgpxFlagFree(struct ArgpxFlagSet *set)
{
    assert(set != NULL);

    free(set->ptr);
}

void ArgpxResultFree(struct ArgpxResult *res)
{
    assert(res != NULL);

    free(res->param_v);
}

#ifdef ARGPX_ENABLE_HASH

/*
    Call this function if root_used is true. Otherwise use root entry directly.

    return NULL: error
 */
static struct FlagTableUnit_ *FlagTableEnterUnit_(struct FlagTableUnit_ *unit)
{
    assert(unit != NULL);

    while (true) {
        if (unit->next != NULL) {
            unit = unit->next;
            continue;
        }
        unit->next = malloc(sizeof(struct FlagTableUnit_));
        if (unit->next == NULL)
            return NULL;

        return unit->next;
    }
}

/*
    Initialize the incoming table. That should be on the stack.

    return the "table" parameter self.
    return NULL: error
 */
static struct FlagTable_ *FlagTableMake_(
    struct ArgpxStyle *style, struct ArgpxFlagSet *flagset, struct FlagTable_ *table)
{
    assert(style != NULL);
    assert(flagset != NULL);
    assert(table != NULL);

    table->count = flagset->count / ARGPX_FLAG_TABLE_LOADFACTOR;
    table->array = malloc(sizeof(struct FlagTableUnit_) * table->count);
    if (table == NULL) {
        return NULL;
    }

    for (int i = 0; i < table->count; i++) {
        table->array[i] = (struct FlagTableUnit_){.root_used = false};
    }

    // init finished

    for (int i = 0; i < flagset->count; i++) {
        struct ArgpxFlag *conf = &flagset->ptr[i];

        uint32_t name_hash = ArgpxHashFnv1aB32(conf->name, strlen(conf->name), ARGPX_HASH_FNV1A_32_INIT);
        // TODO bad dispersion
        struct FlagTableUnit_ *unit = table->array + (name_hash + conf->group_idx) % table->count;

        if (unit->root_used == true) {
            unit = FlagTableEnterUnit_(unit);
            if (unit == NULL) {
                free(table->array);
                return NULL;
            }
        }

        *unit = (struct FlagTableUnit_){.root_used = true, .conf = conf, .next = NULL};
    }

    return table;
}

/*
    Call this function, if "unit->next" linked list is exist.

    Note: it won't free up the parameter "unit", that's managed by table.array.
    Note: this function won't check .used element of root unit.
 */
static void FlagTableFreeRecursiveUnit_(struct FlagTableUnit_ *unit)
{
    assert(unit != NULL);

    unit = unit->next;
    do {
        struct FlagTableUnit_ *ahead_unit = unit->next;
        free(unit);
        unit = ahead_unit;
    } while (unit != NULL);
}

static void FlagTableFree_(struct FlagTable_ *table)
{
    assert(table != NULL);

    for (int i = 0; i < table->count; i++) {
        struct FlagTableUnit_ *unit = &table->array[i];
        if (unit->root_used == true and unit->next != NULL)
            FlagTableFreeRecursiveUnit_(unit);
    }
    free(table->array);
}

#endif

/*
    Using the offset shift arguments, it will be safe.
    Return a pointer to the new argument.

    return NULL: error
 */
static char *ShiftArguments_(struct UnifiedData_ *data, const int offset)
{
    assert(data != NULL);
    assert(offset > 0);

    if (data->arg_idx + offset >= data->arg_c)
        return NULL;
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
static int AppendCommandParameter_(struct UnifiedData_ *data, char *str)
{
    assert(data != NULL);
    assert(str != NULL);
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
    assert(string != NULL);
    if (length == 0)
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

    return negative: error
 */
static int StringToType_(const char *source_str, const size_t max_len, const enum ArgpxVarType type, void *ptr)
{
    assert(source_str != NULL);
    assert(ptr != NULL);
    if (max_len == 0)
        return -1;

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
        return 0; // dont't free up
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
    return 0;
}

static size_t TypeToSize_(const enum ArgpxVarType type)
{
    switch (type) {
    case kArgpxVarString:
        return sizeof(char *);
    case kArgpxVarInt:
        return sizeof(int);
    case kArgpxVarBool:
        return sizeof(bool);
    case kArgpxVarFloat:
        return sizeof(float);
    case kArgpxVarDouble:
        return sizeof(double);
    }
}

/*
    If param_len <= 0 then no limit.
    If param_start is NULL, shift to the next argument.

    return negative: error and set status
 */
static int ActionParamSingle_(
    struct UnifiedData_ *data, struct ArgpxFlag *conf, bool ondemand, char *param_start, size_t param_len)
{
    assert(data != NULL);
    assert(conf != NULL);

    struct ArgpxOutParamSingle *unit = &conf->action_load.param_single;
    if (ondemand == true)
        unit->var_ptr = malloc(TypeToSize_(unit->type));
    if (unit->var_ptr == NULL) {
        data->res->status = kArgpxStatusMemoryError;
        return -1;
    }

    if (param_start == NULL)
        param_start = ShiftArguments_(data, 1);
    if (param_start == NULL) {
        data->res->status = kArgpxStatusParamInsufficient;
        return -1;
    }

    if (param_len == 0)
        param_len = strlen(param_start);

    if (param_len == 0) {
        data->res->status = kArgpxStatusParamInsufficient;
        return -1;
    }

    if (StringToType_(param_start, param_len, unit->type, unit->var_ptr) < 0) {
        data->res->status = kArgpxStatusFailure;
        return -1;
    }

    return 0;
}

void ArgpxOutParamSingleFree(struct ArgpxOutParamSingle *out)
{
    assert(out != NULL);

    free(out->var_ptr);
}

/*
    Append a new item into a ParamList action. It can only be attached to the tail.
    The last_idx acts as both the counter(index + 1) and new item index.

    If batch alloc enabled, allocate 3 slots of the list at a time.

    return negative: error(memory error)
 */
static int ParamListAppend_(struct ArgpxOutParamList *out, const int last_idx, const char *str, const size_t str_len)
{
    assert(out != NULL);
    assert(last_idx >= 0);
    assert(str != NULL);
    if (str_len == 0)
        return -1;

    out->out_count = last_idx + 1;

    out->out_list = ArrGrowOneSlot_(out->out_list, sizeof(char *), out->out_count, 3);
    if (out->out_list == NULL)
        return -1;

    char *new_str = malloc(sizeof(char) * str_len + 1);
    if (new_str == NULL)
        return -1;

    memcpy(new_str, str, str_len);
    new_str[str_len] = '\0';

    out->out_list[last_idx] = new_str;

    return 0;
}

/*
    If max_param_len == 0 then no limit.

    return negative: error and set status
 */
static int ActionParamList_(struct UnifiedData_ *data, struct UnifiedGroupCache_ *grp, struct ArgpxFlag *conf,
    char *param_start_ptr, size_t max_param_len)
{
    assert(data != NULL);
    assert(grp != NULL);
    assert(conf != NULL);
    assert(param_start_ptr != NULL);
    struct ArgpxOutParamList *out = &conf->action_load.param_list;

    char *param_now = param_start_ptr != NULL ? param_start_ptr : ShiftArguments_(data, 1);
    if (param_now == NULL) {
        data->res->status = kArgpxStatusParamInsufficient;
        return -1;
    }
    int remaining_len = max_param_len > 0 ? max_param_len : strlen(param_now);

    char *delimiter_ptr;
    for (int param_idx = 0; remaining_len > 0; param_idx++) {
        if (out->max > 0 and param_idx + 1 > out->max) {
            data->res->status = kArgpxStatusParamExcess;
            return -1;
        }

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

        if (ParamListAppend_(out, param_idx, param_now, param_len) < 0) {
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
    Clean up the struct ArgpxOutParamList.
 */
void ArgpxOutParamListFree(struct ArgpxOutParamList *out)
{
    assert(out != NULL);

    for (int i = 0; i < out->out_count; i++)
        free(out->out_list[i]);
    free(out->out_list);
}

/*
    kArgpxActionSetMemory
 */
static void ActionSetMemory_(struct UnifiedData_ *data, struct ArgpxFlag *conf_ptr)
{
    assert(data != NULL);
    assert(conf_ptr != NULL);

    struct ArgpxOutSetMemory *ptr = &conf_ptr->action_load.set_memory;
    memcpy(ptr->target_ptr, ptr->source_ptr, ptr->size);
}

/*
    kArgpxActionSetBool
 */
static void ActionSetBool_(struct UnifiedData_ *data, struct ArgpxFlag *conf_ptr)
{
    assert(data != NULL);
    assert(conf_ptr != NULL);

    struct ArgpxOutSetBool *ptr = &conf_ptr->action_load.set_bool;
    *ptr->target_ptr = ptr->source;
}

/*
    kArgpxActionSetInt
 */
static void ActionSetInt_(struct UnifiedData_ *data, struct ArgpxFlag *conf_ptr)
{
    assert(data != NULL);
    assert(conf_ptr != NULL);

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
static int MatchingGroup_(int group_c, struct ArgpxGroup *group_v, char *arg)
{
    assert(group_c > 0);
    assert(group_v != NULL);
    assert(arg != NULL);

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
static int GroupCacheInit_(struct UnifiedGroupCache_ *grp)
{
    assert(grp != NULL);

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
static bool ShouldFlagTypeHaveParam_(struct UnifiedData_ *data, const struct ArgpxFlag *conf_ptr)
{
    assert(data != NULL);
    assert(conf_ptr != NULL);

    switch (conf_ptr->action_type) {
    case kArgpxActionSetMemory:
    case kArgpxActionSetBool:
    case kArgpxActionSetInt:
    case kArgpxActionCallbackOnly:
        return false;
    case kArgpxActionParamSingle:
    case kArgpxActionParamSingleOnDemand:
    case kArgpxActionParamList:
        return true;
    }

    return false; // make compiler happy... noreturn is baster then this
}

/*
    Matching a name in all flag configs.
    It will find the conf with the highest match length in name_start.
    But if shortest == true, it will find the first matching one:

    e.g. Find str "TestTail" or str "Test" in "TestTail"
    false: -> "TestTail"
    true:  -> "Test"

    And flag may have assignment symbol, I think the caller should already known the names range.
    In this case, it make sense to get a max_name_len.
    Set max_name_len to == 0 to disable it.

    If "shortest" is true, then shortest matching flag name and ignore tail.

    return NULL: error and set status
 */
static struct ArgpxFlag *MatchConfLinear_(
    struct UnifiedData_ *data, struct UnifiedGroupCache_ *grp, char *name_start, size_t max_name_len, bool shortest)
{
    assert(data != NULL);
    assert(grp != NULL);
    assert(name_start != NULL);

    bool tail_limit = max_name_len == 0 ? false : true;
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

#ifdef ARGPX_ENABLE_HASH

/*
    return NULL: error and set status
 */
static struct ArgpxFlag *MatchConfHash_(
    struct UnifiedData_ *data, struct UnifiedGroupCache_ *grp, char *name, size_t name_len)
{
    assert(data != NULL);
    assert(grp != NULL);
    assert(name != NULL);

    if (name_len == 0) {
        data->res->status = kArgpxStatusFailure;
        return NULL;
    }

    uint32_t name_hash = ArgpxHashFnv1aB32(name, name_len, ARGPX_HASH_FNV1A_32_INIT);
    struct FlagTableUnit_ *unit = data->conf_table.array + (name_hash + grp->idx) % data->conf_table.count;

    if (unit->root_used == false) {
        data->res->status = kArgpxStatusUnknownFlag;
        return NULL;
    }

    while (true) {
        if (unit->conf->group_idx == grp->idx and strncmp(unit->conf->name, name, name_len) == 0)
            return unit->conf;

        if (unit->next == NULL) {
            data->res->status = kArgpxStatusUnknownFlag;
            return NULL;
        }

        unit = unit->next;
    }
}

#endif

/*
    A wrapper of MatchConf*() functions.

    If name_len is 0, then the shortest matching name is implied(the "shortest" of MatchConfLinear_() function).
    Because hash mode can only be used when the length fixed, so even the HASH mode enabled, when length is unknown(like
    ARGPX_ATTR_COMPOSABLE mode), linear algorithm still be use.
 */
static struct ArgpxFlag *MatchConf_(
    struct UnifiedData_ *data, struct UnifiedGroupCache_ *grp, char *name_start, size_t name_len)
{
    assert(data != NULL);
    assert(grp != NULL);
    assert(name_start != NULL);

#ifndef ARGPX_ENABLE_HASH
    return MatchConfLinear_(data, grp, name_start, name_len, name_len == 0 ? true : false);
#else
    if (name_len == 0) {
        return MatchConfLinear_(data, grp, name_start, 0, true);
    } else {
        return MatchConfHash_(data, grp, name_start, name_len);
    }
#endif
}

/*
    Return matched symbol item index of sym_v.
    Return negative num is not match.
 */
static int MatchSymbol_(const char *target, const int sym_c, const struct ArgpxSymbol *sym_v)
{
    assert(target != NULL);
    assert(sym_c > 0);
    assert(sym_v != NULL);

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
static int ParseArgumentIndependent_(struct UnifiedData_ *data, struct UnifiedGroupCache_ *grp, char *arg)
{
    assert(data != NULL);
    assert(grp != NULL);
    assert(arg != NULL);

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

    struct ArgpxFlag *conf = MatchConf_(data, grp, name_start, name_len);
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
        if (ActionParamSingle_(data, conf, false, param_base, 0) < 0)
            return -1;
        break;
    case kArgpxActionParamSingleOnDemand:
        if (ActionParamSingle_(data, conf, true, param_base, 0) < 0)
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

#include <stdio.h>
/*
    return negative: error and status is set
 */
static int ParseArgumentComposable_(struct UnifiedData_ *data, struct UnifiedGroupCache_ *grp, char *arg)
{
    assert(data != NULL);
    assert(grp != NULL);
    assert(arg != NULL);

    // believe that the prefix exists
    char *base_ptr = arg + grp->prefix_len;
    size_t remaining_len = strlen(arg) - grp->prefix_len;

    while (remaining_len > 0) {
        struct ArgpxFlag *conf = MatchConf_(data, grp, base_ptr, 0);
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
            if (ActionParamSingle_(data, conf, false, param_start, param_len) < 0)
                return -1;
            break;
        case kArgpxActionParamSingleOnDemand:
            if (ActionParamSingle_(data, conf, true, param_start, param_len) < 0)
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
    The caller needs to prepare the memory of struct ArgpxResult.

    If the terminate param is NULL, that's same as {.method = kArgpxTerminateNone} of struct ArgpxTerminateMethod.

    return the result.status enum(ArgpxStatus) code.
    Only 0(kArgpxStatusSuccess) is success.
 */
int ArgpxParse(struct ArgpxResult *in_result, int in_arg_c, char **in_arg_v, struct ArgpxStyle *in_style,
    struct ArgpxFlagSet *in_flag, struct ArgpxTerminateMethod *in_terminate)
{
    assert(in_result != NULL);
    assert(in_arg_v != NULL);
    assert(in_style != NULL);
    assert(in_flag != NULL);

    struct UnifiedData_ data = {
        .res = in_result,
        .arg_c = in_arg_c,
        .arg_v = in_arg_v,
        .arg_idx = 0,
        .style = *in_style,
        .conf = *in_flag,
    };

    *data.res = (struct ArgpxResult){
        .status = kArgpxStatusSuccess,
        .current_argv_idx = 0,
        .current_argv_ptr = NULL,
        .param_c = 0,
        .param_v = NULL,
    };

    if (data.arg_c < 0) {
        data.res->status = kArgpxStatusFailure; // TODO change name
        return data.res->status;
    } else if (data.arg_c == 0) {
        return data.res->status;
    }

    if (in_terminate == NULL)
        data.terminate = (struct ArgpxTerminateMethod){.method = kArgpxTerminateNone};
    else
        data.terminate = *in_terminate;

#ifdef ARGPX_ENABLE_HASH
    if (FlagTableMake_(&data.style, &data.conf, &data.conf_table) == NULL) {
        data.res->status = kArgpxStatusMemoryError;
        return data.res->status;
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
#ifdef ARGPX_ENABLE_HASH
    FlagTableFree_(&data.conf_table);
#endif
    return data.res->status;
}
