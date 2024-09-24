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
};

enum ArgpxActionType {
    kArgpxActionParamMulti,
    kArgpxActionParamSingle,
    kArgpxActionParamList,   // TODO
    kArgpxActionSetMemory,   // TODO
    kArgpxActionSetBool,
    kArgpxActionSetInt,      // TODO
};

enum ArgpxVarType {
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

// TODO implement '\0'
struct ArgpxFlagGroup {
    // all group attribute
    uint16_t attribute;
    // prefix of flag, like the "--" of "--flag". it's a string.
    // all group prefixes cannot be duplicated, including “”(single \0) also
    char *prefix;
    // parameter assignment symbol(string)
    char *assigner;
    // parameter delimiter(string)
    char *delimiter;
};

/*
    Convert a string in flag's parameter
 */
struct ArgpxParamUnit {
    enum ArgpxVarType type;
    // a list of secondary pointer of actual variable
    void *ptr;
};

/*
    If need some custom structure or the other data type
 */
struct ArgpxHidden_OutcomeSetMemory {
    size_t size;
    void *source_ptr;
    void *target_ptr;
};

/*
    The most common operation on the command line
 */
struct ArgpxHidden_OutcomeSetBool {
    bool source;
    bool *target_ptr;
};

/*
    Maybe enum need it
 */
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
    // It's an index not an id.
    // emm... I trust the programer can figure out the array index in their hands.
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

char *ArgpxStatusToString(enum ArgpxStatus status);
struct ArgpxResult *ArgpxMain(int argc, int last_arg, char *argv[], struct ArgpxFlagGroup *groups, int group_count,
    struct ArgpxFlag *opts, int opt_count, void (*ErrorCallback)(struct ArgpxResult *));

#endif
