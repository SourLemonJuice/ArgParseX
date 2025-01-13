#include <argpx_rs.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    hello();
    Style *style = ArgpxStyle_New();
    const long groupId = ArgpxStyle_AppendGroup(style, (struct ArgpxStyleGroup){
        .prefix = "--",
        .assigner = "=",
        .delimiter = ","
    });
    if (groupId == -1) abort();
    Parser *parser = ArgpxParser_New(&style);
    bool bool1 = true;
    if (ArgpxParser_AppendFlag(parser, (struct ArgpxFlag){
        .name = "ciallo",
        .group_id = groupId,
        .action = ArgpxAction_NewSetBool(&bool1)
    })) {
        abort();
    }
    struct ArgpxParseResult parsed;
    if (ArgpxParser_Parse(parser, argc, argv, 0, &parsed)) {
        abort();
    }
    printf("args[0]: \t%s\n", ArgpxString_Read(&parsed.args));
    printf("args.len(): \t%lu\n", parsed.args_count);
    return 0;
}
