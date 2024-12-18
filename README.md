# ArgParseX

Comprehensive and configurable command line arguments parser library.\
Written for C language.

## Usage flow

The goal of ArgParseX is not to be simple use, which also make the interface look less nice. Some actual usage at here: [example/test.c](example/test.c)

In short, the user need create a `struct ArgpxStyle` and append it with `ArgpxAppendGroup()` and `struct ArgpxGroup`.

```c
struct ArgpxStyle style = {0}; // zero initialization is required
ArgpxAppendGroup(&style, &(struct ArgpxGroup){
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
struct ArgpxFlagSet flag = {0}; // zero initialization is required
ArgpxAppendFlag(&flag, &(struct ArgpxFlag){
    .group_idx = 0, // the first flag group that was be added(here is GNU)
    .name = "setbool",
    .action_type = kArgpxActionSetBool,
    .action_load.set_bool = {.source = true, .target_ptr = &test_bool},
});
```

It means, if `--setbool` detected, `test_bool` will be set to `true`.

After the configuration, call the main parser:

```c
// skip the first arg, that's the exec command name
struct ArgpxResult *res = ArgpxParse(argc - 1, argv + 1, &style, &flag, NULL);
```

## Documents

At here(Simplified Chinese): [docs/](./docs/)

All the functions do not use static variables, so they're thread safe.\
However, the config data structures can't be modified at the same times.

## C standard

Compatibility whit C99 is the main thing.\
But i think it might be possible to implement a new interface for C23 with macro switch.\
There has `auto` and `typeof()`... that could make things easier.

Uses some `malloc()`, otherwise? Why not use heap memory?\
And `-Wvla`(no Variable-length array).

Code style follows [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), and clang-format config.\
The "namespace" of identifiers is `argpx_xxx`.The base pointer of the array is named `xxx_v`, its counter named `xxx_c`.

## Todo

- 时间复杂度可能有点高的说，等最后肯定会去用哈希表什么的啦...
- 重复匹配一个标志的后果？

## See also

- [命令行界面程序的各种参数设计模式与风格 | SourLemonJuice-blog](https://sourlemonjuice.github.io/SourLemonJuice-blog/posts2/2024/09/command-line-style)

## License

Published under MIT license.
