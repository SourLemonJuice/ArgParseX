#ifndef ARGPX_H_
#define ARGPX_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// the interface versions
#define ARGPX_VERSION_MAJOR 0
#define ARGPX_VERSION_MINOR 4
// a continuously growing integer, +1 for each release
#define ARGPX_VERSION_REVISION 4

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
    // like SetBool, maybe enum need it.
    // note: this action just uses the "int" type
    kArgpxActionSetInt,
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

struct ArgpxStyle {
    int group_c;
    struct ArgpxGroupItem *group_v;
    // stop parsing symbol count
    // like "--" or "-"
    int stop_c;
    // stop parsing symbol array
    char **stop_v;
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
struct ArgpxParamUnit {
    enum ArgpxVarType type;
    // a list of pointer of actual variable
    void *ptr;
};

struct ArgpxHidden_OutcomeParamMulti {
    int count;
    // format units array
    struct ArgpxParamUnit *units;
};

struct ArgpxHidden_OutcomeParamList {
    int *count;
    char ***params; // pointer to a pointer list to a string list...
};

struct ArgpxHidden_OutcomeSetMemory {
    size_t size;
    void *source_ptr;
    void *target_ptr;
};

struct ArgpxHidden_OutcomeSetBool {
    bool source;
    bool *target_ptr;
};

struct ArgpxHidden_OutcomeSetInt {
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
        struct ArgpxParamUnit param_single;
        struct ArgpxHidden_OutcomeParamMulti param_multi;
        struct ArgpxHidden_OutcomeParamList param_list;
        struct ArgpxHidden_OutcomeSetMemory set_memory;
        struct ArgpxHidden_OutcomeSetBool set_bool;
        struct ArgpxHidden_OutcomeSetInt set_int;
    } action_load;
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
    int param_count;
    // an array of command parameters
    char **paramv;
};

enum ArgpxHidden_BuiltinGroup {
    kArgpxHidden_BuiltinGroupGnu,
    kArgpxHidden_BuiltinGroupUnix,
    kArgpxHidden_BuiltinGroupCount,
};

extern const struct ArgpxGroupItem argpx_hidden_builtin_group[kArgpxHidden_BuiltinGroupCount];

#define ARGPX_BUILTIN_GROUP_GNU &argpx_hidden_builtin_group[kArgpxHidden_BuiltinGroupGnu]
#define ARGPX_BUILTIN_GROUP_UNIX &argpx_hidden_builtin_group[kArgpxHidden_BuiltinGroupUnix]

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
void ArgpxAppendStopSymbol(struct ArgpxStyle style[static 1], char symbol[static 1]);
void ArgpxAppendFlag(struct ArgpxFlagSet set[static 1], const struct ArgpxFlagItem new[static 1]);
struct ArgpxResult *ArgpxMain(struct ArgpxMainOption *func);

#endif
