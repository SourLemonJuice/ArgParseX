# ArgParseX

> [!WARNING]
> Still under development.

Comprehensive and configurable command line arguments parser library.\
Written for C language.

## Usage flow

The goal of ArgParseX is not to be simple use, which also make the interface look less nice. Some actual usage at here: [example/test.c](example/test.c)

In short, the user need create a `struct ArgpxStyle` and append it with `ArgpxAppendGroup()` and `struct ArgpxGroupItem`.

```c
struct ArgpxStyle style = {0};
ArgpxAppendGroup(&style, &(struct ArgpxGroupItem){
    .prefix = "--",
    .assigner = "=",
    .delimiter = ",",
});
```

Although the built-in `item` is usable enough.

```c
ArgpxAppendGroup(&style, ARGPX_GROUP_GNU);
```

And add some stop parsing symbols:

```c
ArgpxAppendSymbol(&style, ARGPX_SYMBOL_STOP_PARSING("--"));
ArgpxAppendSymbol(&style, ARGPX_SYMBOL_STOP_PARSING("-"));
```

Finally, the most important thing to define the content of flags:

```c
struct ArgpxFlagSet flag = {0};
ArgpxAppendFlag(&flag, &(struct ArgpxFlagItem){
    .group_idx = 0, // the first flag group that was be added(here is GNU)
    .name = "setbool",
    .action_type = kArgpxActionSetBool,
    .action_load.set_bool = {.source = true, .target_ptr = &test_bool},
});
```

It means, if `--setbool` detected, `test_bool` will be set to `true`.

After the configuration, call the main parser:

```c
struct ArgpxResult *res = ArgpxMain(&(struct ArgpxMainOption){
    // skip first arg
    .argc = argc - 1,
    .argv = argv + 1,
    .style = &style,
    .flag = &flag,
    .terminate.method = kArgpxTerminateNone,
    // .terminate.method = kArgpxTerminateAtNumberOfCommandParam,
    // .terminate.load.num_of_cmd_param.limit = 2,
});
```

This seems tedious, but the interfaces are still being adjusted.

## Documents

At here(Simplified Chinese): [docs/](./docs/)

## C standard

Compatibility whit C99 is the main thing.\
But i think it might be possible to implement a new interface for C23 with macro switch.\
There has `auto` and `typeof()`... that could make things easier.

## Todo

- 时间复杂度可能有点高的说，等最后肯定会去用哈希表什么的啦...
- 重复匹配一个标志的后果？

## See also

- [命令行界面程序的各种参数设计模式与风格 | SourLemonJuice-blog](https://sourlemonjuice.github.io/SourLemonJuice-blog/posts2/2024/09/command-line-style)

## License

Published under MIT license.
