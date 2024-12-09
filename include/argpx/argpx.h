#ifndef ARGPX_H_
#define ARGPX_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// details are in ArgpxStatusToString()
enum ArgpxStatus {
    kArgpxStatusSuccess = 0,
    kArgpxStatusFailure,
    kArgpxStatusGroupConfigInvalid,
    kArgpxStatusUnknownFlag,
    kArgpxStatusActionUnavailable,
    kArgpxStatusArgumentsDeficiency,
    kArgpxStatusParamNoNeeded,
    kArgpxStatusAssignmentDisallowAssigner,
    kArgpxStatusAssignmentDisallowTrailing,
    kArgpxStatusAssignmentDisallowArg,
    kArgpxStatusParamDeficiency,
    kArgpxStatusParamBizarreFormat,
};

enum ArgpxActionType {
    // get a single flag parameter, but can still convert it's data type
    kArgpxActionParamSingle,
    // get multiple flag parameters with different data type
    kArgpxActionParamMulti,
    // get flag parameters raw string array, the array size is dynamic
    kArgpxActionParamList,
    // if need some custom structure or the other data type
    kArgpxActionSetMemory,
    // the most common operation on the command line
    kArgpxActionSetBool,
    // like SetBool, maybe enum need it
    // note: this action just uses the "int" type
    kArgpxActionSetInt,
    // only run callback, no any outcome
    kArgpxActionCallbackOnly,
};

#define ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER 0b1 << 0
// independent flag won't use trailing mode
#define ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING 0b1 << 1
#define ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG 0b1 << 2

#define ARGPX_ATTR_COMPOSABLE 0b1 << 3
#define ARGPX_ATTR_COMPOSABLE_NEED_PREFIX 0b1 << 4

struct ArgpxGroupItem {
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

struct ArgpxSymbolItem {
    // key symbol array
    char *str;
    // key symbol type array
    enum ArgpxSymbolType type;
    void (*callback)(void *param);
    void *callback_param;
};

struct ArgpxStyle {
    int group_c;
    struct ArgpxGroupItem *group_v;
    // key symbol count
    // for example, StopParsing symbol may like "--" or "-"
    int symbol_c;
    struct ArgpxSymbolItem *symbol_v;
};

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
    void *value;
};

struct ArgpxOutParamMulti {
    int count;
    // parameter format units array
    struct ArgpxOutParamSingle *unit_v;
};

struct ArgpxOutParamList {
    int *count_ptr;
    char ***list_ptr; // pointer to a pointer list to a string list...
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
struct ArgpxFlagItem {
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
        struct ArgpxOutParamMulti param_multi;
        struct ArgpxOutParamList param_list;
        struct ArgpxOutSetMemory set_memory;
        struct ArgpxOutSetBool set_bool;
        struct ArgpxOutSetInt set_int;
        // no practical use, set to anything include NULL
        // it's also will be passed to the callback function as the first param
        void *callback_only;
    } action_load;

    void (*callback)(void *action_load, void *param);
    void *callback_param;
};

struct ArgpxFlagSet {
    int count;
    struct ArgpxFlagItem *ptr;
};

struct ArgpxResult {
    enum ArgpxStatus status;
    // index to the last parsed argument, processing maybe finished or maybe wrong
    int current_argv_idx;
    // pretty much the same as current_argv_idx, but it's directly useable string
    char *current_argv_ptr;
    // parameter here is non-flag command "argument"
    int param_c;
    // an array of command parameters
    char **param_v;
};

// TODO aaa... it's works
enum ArgpxHidden_TerminateMethod {
    kArgpxTerminateNone,
    kArgpxTerminateAtNumberOfCommandParam,
};

struct ArgpxHidden_TerminateAtNumberOfCommandParam {
    int limit;
};

struct ArgpxTerminateMethod {
    enum ArgpxHidden_TerminateMethod method;
    union {
        struct ArgpxHidden_TerminateAtNumberOfCommandParam num_of_cmd_param;
    } load;
};

/*
    The function parameter of ArgpxMain()
 */
struct ArgpxMainOption {
    int argc;
    char **argv;
    struct ArgpxStyle *style;
    struct ArgpxFlagSet *flag;
    struct ArgpxTerminateMethod terminate;
};

char *ArgpxStatusToString(enum ArgpxStatus status);
void ArgpxAppendGroup(struct ArgpxStyle set[static 1], const struct ArgpxGroupItem new[static 1]);
void ArgpxAppendSymbol(struct ArgpxStyle style[static 1], struct ArgpxSymbolItem new[static 1]);
void ArgpxAppendFlag(struct ArgpxFlagSet set[static 1], const struct ArgpxFlagItem new[static 1]);
struct ArgpxResult *ArgpxMain(struct ArgpxMainOption *func);

/*
    Shortcuts
 */

// clang-format off

enum ArgpxHidden_BuiltinGroup {
    kArgpxHidden_BuiltinGroupGnu,
    kArgpxHidden_BuiltinGroupUnix,
    kArgpxHidden_BuiltinGroupCount,
};

#define ARGPX_GROUP_GNU &(const struct ArgpxGroupItem){ \
        .prefix = "--", \
        .assigner = "=", \
        .delimiter = ",", \
        .attribute = 0, \
    }

#define ARGPX_GROUP_UNIX &(const struct ArgpxGroupItem){ \
        .prefix = "-", \
        .assigner = "=", \
        .delimiter = ",", \
        .attribute = ARGPX_ATTR_COMPOSABLE | ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG, \
    }

#define ARGPX_SYMBOL_STOP_PARSING(str_in) \
    &(struct ArgpxSymbolItem){ \
        .str = str_in, \
        .type = kArgpxSymbolStopParsing, \
    }

// clang-format on

#endif
