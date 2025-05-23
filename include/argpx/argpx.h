#ifndef ARGPX_H_
#define ARGPX_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// details are in ArgpxStatusString()
enum ArgpxStatus {
    kArgpxStatusSuccess = 0,
    kArgpxStatusFailure,
    kArgpxStatusMemoryError,

    kArgpxStatusConfigInvalid,
    kArgpxStatusUnknownFlag,
    kArgpxStatusActionUnavailable,

    kArgpxStatusParamNoNeeded,
    kArgpxStatusParamInsufficient,
    kArgpxStatusParamExcess,

    kArgpxStatusAssignmentDisallowAssigner,
    kArgpxStatusAssignmentDisallowTrailing,
    kArgpxStatusAssignmentDisallowArg,
    kArgpxStatusBizarreFormat,
};

enum ArgpxActionType {
    // only run callback, no any outcome(struct ArgpxOut*)
    // for safety, it's 0(default after zero init)
    kArgpxActionCallbackOnly = 0,
    // get a single flag parameter, but can still convert it's data type
    kArgpxActionParamSingle,
    // same as ParamSingle, but allocation output memory with itself
    kArgpxActionParamSingleOnDemand,
    // get flag parameters raw string array, the array size is dynamic
    // it will allocation output memory with itself
    kArgpxActionParamList,
    // if need some custom structure or the other data type
    kArgpxActionSetMemory,
    // the most common operation on the command line
    kArgpxActionSetBool,
    // like SetBool, maybe enum need it
    // note: this action just uses the "int" type
    kArgpxActionSetInt,
};

#define ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER 1 << 0
// independent flag won't use trailing mode
#define ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING 1 << 1
#define ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG 1 << 2

#define ARGPX_ATTR_COMPOSABLE 1 << 3
#define ARGPX_ATTR_COMPOSABLE_NEED_PREFIX 1 << 4

struct ArgpxGroup {
    // all group attribute
    uint16_t attribute;
    // prefix of flag, like the "--" of "--flag". it's a string
    // all group prefixes cannot be duplicated, including ""(single \0) also
    char *prefix;
    // flag parameter assignment symbol(string)
    // NULL: disable
    // the empty string: ""(single \0), is an error
    char *assigner;
    // flag parameter delimiter(string)
    // NULL: disable
    // the empty string: ""(single \0), is an error
    char *delimiter;
};

enum ArgpxSymbolType {
    kArgpxSymbolStopParsing,
    kArgpxSymbolTerminateProcessing,
    kArgpxSymbolCallback,
};

struct ArgpxSymbol {
    // key symbol array
    char *str;
    // key symbol type array
    enum ArgpxSymbolType type;
    void (*callback)(void *param);
    void *callback_param;
};

struct ArgpxStyle {
    int group_c;
    struct ArgpxGroup *group_v;
    // key symbol count
    // for example, StopParsing symbol may like "--" or "-"
    int symbol_c;
    struct ArgpxSymbol *symbol_v;
};

#define ARGPX_STYLE_INIT \
    (struct ArgpxStyle) \
    { \
        .group_c = 0, .group_v = NULL, .symbol_c = 0, .symbol_v = NULL \
    }

enum ArgpxVarType {
    // string type will return a manually alloced full string(have \0)
    kArgpxVarString,
    kArgpxVarInt,
    kArgpxVarBool,
    kArgpxVarFloat,
    kArgpxVarDouble,
};

// Convert a string in flag's parameter
struct ArgpxOutParamSingle {
    enum ArgpxVarType type;
    // a pointer to the actual variable
    void *var_ptr;
};

struct ArgpxOutParamList {
    int out_count;
    char **out_list;
    // if <= 0, no limit
    int max;
};

struct ArgpxOutSetMemory {
    size_t size;
    void *source_ptr;
    void *target_ptr;
};

struct ArgpxOutSetBool {
    bool source;
    bool *target_ptr;
};

struct ArgpxOutSetInt {
    int source;
    int *target_ptr;
};

// in library source code it is called "conf/config"
struct ArgpxFlag {
    // It's an index not an id
    // emm... I trust the programer can figure out the array index in their hands
    // then there is no need for a new hash table here
    int group_idx;
    // name of flag, like the "flagName" of "--flagName"
    char *name;

    // one flag only have one action, but one action may need to define mutiple structures.
    enum ArgpxActionType action_type;
    union {
        struct ArgpxOutParamSingle param_single;
        struct ArgpxOutParamList param_list;
        struct ArgpxOutSetMemory set_memory;
        struct ArgpxOutSetBool set_bool;
        struct ArgpxOutSetInt set_int;
        // no practical use, set to anything include NULL
        // it's also will be passed to the callback function as the first param
        void *callback_only;
    } action_load;

    // if NULL, skip callback
    void (*callback)(void *action_load, void *param);
    void *callback_param;
};

struct ArgpxFlagSet {
    int count;
    struct ArgpxFlag *ptr;
};

#define ARGPX_FLAGSET_INIT \
    (struct ArgpxFlagSet) \
    { \
        .count = 0, .ptr = NULL \
    }

struct ArgpxParseOption {
    int max_cmdparam;
    bool use_hash;
};

#define ARGPX_PARSE_OPTION_INIT \
    (struct ArgpxParseOption) \
    { \
        .max_cmdparam = 0, .use_hash = false \
    }

struct ArgpxResult {
    enum ArgpxStatus status;
    // index to the last parsed argument, processing maybe finished or maybe wrong
    int current_argv_idx;
    // pretty much the same as current_argv_idx, but it's directly available string
    char *current_argv_ptr;
    // parameter here is non-flag command "argument"
    int param_c;
    // an array of command parameters
    char **param_v;
};

char *ArgpxStatusString(enum ArgpxStatus status);

int ArgpxGroupAppend(struct ArgpxStyle *style, const struct ArgpxGroup *new);
int ArgpxSymbolAppend(struct ArgpxStyle *style, const struct ArgpxSymbol *new);
void ArgpxStyleFree(struct ArgpxStyle *style);

int ArgpxFlagAppend(struct ArgpxFlagSet *set, const struct ArgpxFlag *new);
void ArgpxFlagFree(struct ArgpxFlagSet *set);

void ArgpxResultFree(struct ArgpxResult *res);
void ArgpxOutParamSingleFree(struct ArgpxOutParamSingle *out);
void ArgpxOutParamListFree(struct ArgpxOutParamList *out);

int ArgpxParse(struct ArgpxResult *in_result, int in_arg_c, char **in_arg_v, struct ArgpxStyle *in_style,
    struct ArgpxFlagSet *in_flag, struct ArgpxParseOption *in_option);

// clang-format off

#define ARGPX_GROUP_GNU &(struct ArgpxGroup){ \
        .prefix = "--", \
        .assigner = "=", \
        .delimiter = ",", \
        .attribute = 0, \
    }

#define ARGPX_GROUP_UNIX &(struct ArgpxGroup){ \
        .prefix = "-", \
        .assigner = "=", \
        .delimiter = ",", \
        .attribute = ARGPX_ATTR_COMPOSABLE | ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG, \
    }

#define ARGPX_SYMBOL_STOP_PARSING(str_in) \
    &(struct ArgpxSymbol){ \
        .str = str_in, \
        .type = kArgpxSymbolStopParsing, \
    }

// clang-format on

#endif
