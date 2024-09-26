#ifndef ARGPX_H_
#define ARGPX_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// the interface versions
#define ARGPX_VERSION_MAJOR 0
#define ARGPX_VERSION_MINOR 2
// a continuously growing integer, +1 for each release
#define ARGPX_VERSION_REVISION 2

// details are in ArgpxStatusToString()
enum ArgpxStatus {
    kArgpxStatusSuccess = 0,
    kArgpxStatusFailure,
    kArgpxStatusUnknownFlag,
    kArgpxStatusActionUnavailable,
    kArgpxStatusNoArgAvailableToShifting,
    kArgpxStatusFlagParamNoNeeded,
    kArgpxStatusAssignmentDisallowAssigner,
    kArgpxStatusAssignmentDisallowTrailing,
    kArgpxStatusAssignmentDisallowArg,
    kArgpxStatusParamDisallowDelimiter,
    kArgpxStatusParamDisallowArg,
    kArgpxStatusFlagParamDeficiency,
    kArgpxStatusGroupConfigEmptyString,
};

enum ArgpxActionType {
    // get multiple flag parameters with different data type
    kArgpxActionParamMulti,
    // get a single flag parameter, but can still convert it's data type
    kArgpxActionParamSingle,
    // TODO get flag parameters raw string array
    kArgpxActionParamList,
    // TODO If need some custom structure or the other data type
    kArgpxActionSetMemory,
    // The most common operation on the command line
    kArgpxActionSetBool,
    // TODO Maybe enum need it
    kArgpxActionSetInt,
};

enum ArgpxVarType {
    // string type will return a manually alloced full string(have \0)
    kArgpxVarString,
    // TODO
    kArgpxVarInt,
    kArgpxVarBool,
    kArgpxVarFloat,
    kArgpxVarDouble,
};

#define ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER 0b1 << 0
// independent flag won't use trailing mode
#define ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING 0b1 << 1
#define ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG 0b1 << 2

#define ARGPX_ATTR_PARAM_DISABLE_DELIMITER 0b1 << 3
#define ARGPX_ATTR_PARAM_DISABLE_ARG 0b1 << 4

#define ARGPX_ATTR_COMPOSABLE 0b1 << 5
#define ARGPX_ATTR_COMPOSABLE_NEED_PREFIX 0b1 << 6

struct ArgpxFlagGroup {
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

// Convert a string in flag's parameter
struct ArgpxParamUnit {
    enum ArgpxVarType type;
    // a list of secondary pointer of actual variable
    void *ptr;
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
    uintmax_t source;
    uintmax_t *target_ptr;
};

struct ArgpxHidden_OutcomeGetMultiParamArray {
    int count;
    // format units array
    struct ArgpxParamUnit *units;
};

struct ArgpxHidden_OutcomeActionList {
    union {
        struct ArgpxParamUnit param_single;
        struct ArgpxHidden_OutcomeGetMultiParamArray param_multi;
        struct ArgpxHidden_OutcomeSetMemory set_memory;
        struct ArgpxHidden_OutcomeSetBool set_bool;
        struct ArgpxHidden_OutcomeSetInt set_int;
    };
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
    struct ArgpxHidden_OutcomeActionList action_load;
};

struct ArgpxResult {
    enum ArgpxStatus status;
    // index to the last parsed argument, processing maybe finished or maybe wrong
    int current_argv_idx;
    // pretty much the same as current_argv_idx, but it's directly useable string
    char *current_argv_ptr;
    // parameter here is non-flag command "argument"
    int params_count;
    // an array of command parameters
    char **params;
    // the command argc
    int argc;
    // the command argv
    char **argv;
};

enum ArgpxHidden_BuiltinGroup {
    kArgpxHidden_BuiltinGroupGnu,
    kArgpxHidden_BuiltinGroupUnix,
    kArgpxHidden_BuiltinGroupCount,
};

extern struct ArgpxFlagGroup argpx_hidden_builtin_group[kArgpxHidden_BuiltinGroupCount];

#define ARGPX_BUILTIN_GROUP_GNU argpx_hidden_builtin_group[kArgpxHidden_BuiltinGroupGnu]
#define ARGPX_BUILTIN_GROUP_UNIX argpx_hidden_builtin_group[kArgpxHidden_BuiltinGroupUnix]

/*
    public functions
 */

char *ArgpxStatusToString(enum ArgpxStatus status);
struct ArgpxResult *ArgpxMain(int argc, int arg_base, char *argv[static argc], int group_count,
    struct ArgpxFlagGroup groups[static group_count], int opt_count, struct ArgpxFlag opts[static opt_count],
    void (*ErrorCallback)(struct ArgpxResult *));

#endif
