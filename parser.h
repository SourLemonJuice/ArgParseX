#ifndef ARG_PARSE_X_H_
#define ARG_PARSE_X_H_

#include <stdbool.h>
#include <stdint.h>

enum ArgpxStatus {
    kArgpxStatusSuccess,
    kArgpxStatusFailure,
    kArgpxStatusShiftingArg,
    kArgpxStatusNoAssigner,
    kArgpxStatusFlagParamFormatIncorrect,
    kArgpxStatusUnknownFlag,
    kArgpxStatusMethodAvailabilityError,
};

// TODO not implemented all
enum ArgpxVarMethod {
    kArgpxMethodToggleBool,
    kArgpxMethodSetVar, // TODO
    kArgpxMethodSingleParam, // TODO
    kArgpxMethodMultipleParam,
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

// in library source code it is called "conf/config"
struct ArgpxFlag {
    // It's an index not an id.
    // emm... I trust the programer can figure out the array index in their hands.
    // then there is no need for a new hash table here
    int group_idx;
    enum ArgpxVarMethod method;
    // name of flag, like the "flagName" of "--flagName"
    char *name;
    // "toggle bool", "single variable" method use it, made that's simple
    void *single_var_ptr;
    // the count of "var_types" and "var_ptrs"
    int var_count;
    enum ArgpxVarType *var_types;
    // a list of secondary pointer of actual variable
    void **var_ptrs;
};

struct ArgpxResult {
    enum ArgpxStatus status;
    int parsed_argc_index; // Pointer to the last parsed argument
    int params_count;      // Parameters is non-flag argument
    char **parameters;
};

// TODO TBD
// give user a macro but not enum's name?
#define ARG_PARSER_VAR_TYPE enum ArgpxVarType

char *ArgpxStatusToString(enum ArgpxStatus status);
struct ArgpxResult *ArgParser(int argc, int last_arg, char *argv[], struct ArgpxFlagGroup *groups, int group_count,
                              struct ArgpxFlag *opts, int opt_count, void (*ErrorCallback)(struct ArgpxResult *));

#endif
