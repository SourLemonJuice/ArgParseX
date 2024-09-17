#ifndef ARG_PARSE_X_H_
#define ARG_PARSE_X_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum ArgpxStatus {
    kArgpxStatusSuccess = 0,
    kArgpxStatusFailure,
    kArgpxStatusActionUnavailable,
    kArgpxStatusShiftingArg,
    kArgpxStatusUnknownFlag,
    kArgpxStatusMissingAssigner,
    kArgpxStatusMissingDelimiter,
    kArgpxStatusFlagParamDeficiency,
};

enum ArgpxActionType {
    kArgpxActionParamMulti,
    kArgpxActionParamSingle, // TODO
    kArgpxActionParamList,   // TODO
    kArgpxActionSetMemory,   // TODO
    kArgpxActionSetBool,
    kArgpxActionSetInt,      // TODO
};

enum ArgpxVarType {
    kArgpxVarTypeString,
    // TODO
    kArgpxVarTypeInt,
    kArgpxVarTypeBool,
    kArgpxVarTypeFloat,
    kArgpxVarTypeDouble,
};

// flag's range is uint16_t
#define ARGPX_GROUP_MANDATORY_ASSIGNER 0b1 << 0
#define ARGPX_GROUP_MANDATORY_DELIMITER 0b1 << 1
#define ARGPX_GROUP_FLAG_GROUPABLE 0b1 << 2

// TODO implement '\0'
struct ArgpxFlagGroup {
    uint16_t flag;
    // prefix of flag, like the "--" of "--flag"
    char *prefix;
    // parameter assignment symbol
    char assigner;
    // parameter delimiter
    char delimiter;
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

// TODO TBD
// give user a macro but not enum's name?
#define ARG_PARSER_VAR_TYPE enum ArgpxVarType

char *ArgpxStatusToString(enum ArgpxStatus status);
struct ArgpxResult *ArgpxMain(int argc, int last_arg, char *argv[], struct ArgpxFlagGroup *groups, int group_count,
                              struct ArgpxFlag *opts, int opt_count, void (*ErrorCallback)(struct ArgpxResult *));

#endif
