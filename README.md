# ArgParseX

Comprehensive and configurable command line arguments parser library.\
Written for C language.

## Documents

At here(Simplified Chinese): [docs/](./docs/)

All the functions do not use static variables, so they're thread-safe.\
However, the config data structures can't be modified at the same times.

Note the documents are always up-to-date, so make sure to check the corresponding git tag.

## Build

Requirement: clang

In the repository root, and run:

- create configuration file: `./configure.sh`
- create build output directory: `mkdir build`
- build archive file(.a) and shared library(.so): `make`
- the library file located in: `build/libargparsex.a` and `build/libargparsex.so`

For debugging, add the `--debug` flag to `./configure.sh`.

NOTE: this is not GNU Automake, there're just some scripts and Makefile written by me.

## Usage

The goal of ArgParseX is not to be simple use, which also make the interface look less nice. Some actual usage at here: [example/test.c](example/test.c)

In short, the user need create a `struct ArgpxStyle` and append it with `ArgpxGroupAppend()` and `struct ArgpxGroup`.

```c
struct ArgpxStyle style = ARGPX_STYLE_INIT;
ArgpxGroupAppend(&style, &(struct ArgpxGroup){
    .prefix = "--",
    .assigner = "=",
    .delimiter = ",",
});
```

Although the built-in `item` is usable enough.

```c
ArgpxGroupAppend(&style, ARGPX_GROUP_GNU);
```

And add some stop parsing symbols:

```c
ArgpxSymbolAppend(&style, ARGPX_SYMBOL_STOP_PARSING("--"));
ArgpxSymbolAppend(&style, ARGPX_SYMBOL_STOP_PARSING("-"));
```

Finally, the most important thing to define the content of flags:

```c
struct ArgpxFlagSet flag = ARGPX_FLAGSET_INIT;
ArgpxFlagAppend(&flag, &(struct ArgpxFlag){
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

## C standard

Compatibility whit C99 is the main thing.\
But i think it might be possible to implement a new interface for C23 with macro switch.\
There has `auto` and `typeof()`... that could make things easier.

Uses some `malloc()`, otherwise? Why not use heap memory?\
And `-Wvla`(no Variable-length array).

Code style follows [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), and clang-format config.\
The "namespace" of identifiers is `argpx_xxx`.The base pointer of the array is named `xxx_v`, its counter named `xxx_c`.

For non-user input(like null pointer), it use `assert()` std macro to check function parameters' validity.

Storage string length as `size_t`.

## Hash table mode

To enable hash table mode, add the `--enable-hash` flag to `./configure.sh` script. Library built this way will use hash tables as much as they can.\
It **would not** make simple task faster, even slower.\
The critical value will be determined in the future.

The hash function used now is FNV-1a 32bit.

## Benchmark

In speed there is no advantage of ArgParseX over the classic GNU getopt. But the difference is less then double in the best case.\
Here are the benchmark results on my PC:

Test sample:

```c
#define ARGPX_DEV_BM_SAMPLE_ARR \
    (char *[]){ \
        "--config=./a_file.txt", \
        "-abcParamOfC",\
        "--verbose", \
        "param1", \
        "--log-level=1", \
        "param2", \
        "--retry=7", \
    }
```

Compiler `clang version 18.1.8, Target: x86_64-pc-linux-gnu`.\
Count of loop: `10 * 1000 * 1000`\
Source code located in: [benchmark/](./benchmark/)

|Library|Time|
|:--|:--|
|GNU getopt|0m1.456s|
|ArgParseX(-O3)|0m2.580s|
|ArgParseX(-O0)|0m4.619s|

## Todo

- 重复匹配一个标志的后果？

## See also

- [命令行界面程序的各种参数设计模式与风格 | SourLemonJuice-blog](https://sourlemonjuice.github.io/SourLemonJuice-blog/posts2/2024/09/command-line-style)

## License

Published under MIT license.
