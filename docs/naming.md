# 命名

公开标识符的命名是设计接口时最头疼的问题，非全局链接的函数随便写就好，但公开接口是要为此承担可能的历史遗留问题的

## 代码风格

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) 的大部分内容
- 但仍然以 .clang-format 配置为准

## 术语

|缩写|全称|常用中文|意义|
|--|--|--|--|
|arg|argument|NULL|特指程序接收到的每一个参数的完整字符串|
|group|flag group|组/标志组|就是这个东西 [flag_group.md](./flag_group.md)|
|NULL|flag|标志|对每个类似以 `--` 开头的可被组匹配的 arg 的称呼|
|param|parameter|标志参数|标志或者程序本身所接受的参数字符串，比如 `--flag=paramStr` 或 `cmd.out paramStr`|
|NULL|action|标志动作/动作|一系列标志被识别后可以被触发的动作，每个标志只能触发一个动作|
|NULL|outcome|NULL|[基本弃用] action 的前命名，当时想了半天怎么安排接口来着呢|

## 命名空间

可能类似于这些

`ArgPX_Name`/`argpx_name`/`ArgpxName`/`ARGPX_`/`ARG_PARSE_X_`

几乎没有库用到 `argpx` 这个缩写，所以... 我觉得还好

另外 `ArgpxHidden_` 是公开的隐藏定义，它们虽然出于某种原因被放到了标头中，但... 最好别用，名字很可能会改

## 基本原则

基本上所有的命名都尽可能的使用全名，就像 .clang-format 文件设定的最大宽度120列那样，我不希望因为屏幕宽度而牺牲标识符的可理解性

但同样的，公开接口中的标识符需要保证简洁和一致性，特殊的词语需要尽量减少，长度也要尽可能的缩短。\
一个例子就是：

```c
// 这是一个标志配置结构体的一部分
.action_load.param_multi.units =
    (struct ArgpxParamUnit[]){
        {.type = kArgpxVarTypeString, .ptr = &test_str1},
        {.type = kArgpxVarTypeString, .ptr = &test_str2},
    },
```

其中的 `.ptr` 为了留出更多的空间给参数放弃了 pointer 转而使用了缩写

...其实也没那么严肃，但我真的很想很想给自己的第一个“库”留下一个不会有太多历史遗留的接口ww
