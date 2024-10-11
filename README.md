# ArgParseX

> [!WARNING]
> Still under development

Comprehensive and configurable command line arguments parser library.\
Written for C language

## Documents

At here(Simplified Chinese): [docs/](./docs/)

## C standard

Compatibility whit C99 is the main thing.\
But i think it might be possible to implement a new interface for C23 with macro switch.\
There has `auto` and `typeof()`... that could make things easier

## Todo

- 时间复杂度可能有点高的说，等最后肯定会去用哈希表什么的啦...
- 停止解析符
- 允许设置些能直接停止解析的方式，比如遇到了第一个非标志参数之类的（给子命令这种东西用）
- 重复匹配一个标志的后果？

## See also

- [命令行界面程序的各种参数设计模式与风格 | SourLemonJuice-blog](https://sourlemonjuice.github.io/SourLemonJuice-blog/posts2/2024/09/command-line-style)
- If I finish it, maybe another post will appear here

## License

Published under MIT license
