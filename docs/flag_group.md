# 标志组/Flag group

标志组是所有标志的最上层的抽象，它类似于不同的命令行风格的配置文件

它定义了每个标志的前缀、赋值符号、分隔符以及一些特殊属性（大小为 `uint16_t attribute`）。\
设想中，配合属性组应该能描述出大多常见的甚至近乎所有的命令行参数风格和行为。\
当然目前不是如此，它还需要实现很多的属性才能做到这一点

每一个标志认领组的过程只是检测了前缀是否匹配，如果匹配到了一个组那么接下来所有的解析行为都会受到该组中配置的约束

组不会定义获取参数后的 action，这是每个标志自己的事

## 定义方式

文档是个放细节的地方，明显的事情还是懒得写了（咕咕咕）

参见 [example/test.c](../example/test.c) 中关于 `struct ArgpxFlagGroup` 的部分

## 赋值方式/Assignment

宏前缀：`ARGPX_ATTR_ASSIGNMENT`

默认情况下会启用所有的复制方式，并用组属性的方式分别禁用：

|含义|宏名称|例子|检查错误返回码|
|--|--|--|--|
|禁用赋值符号|`ARGPX_ATTR_ASSIGNMENT_DISABLE_ASSIGNER`|`--test=param`|`kArgpxStatusAssignmentDisallowAssigner`|
|禁用尾随字符串（仅可组合模式会使用）|`ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING`|`-Tparam`|`kArgpxStatusAssignmentDisallowTrailing`|
|禁用使用下一个 arg 作为参数|`ARGPX_ATTR_ASSIGNMENT_DISABLE_ARG`|`-test param`|`kArgpxStatusAssignmentDisallowArg`|

## 参数分割方式/Parameter partition

宏前缀：`ARGPX_ATTR_PARAM`

同样的，默认会启用所有方式，并使用组属性分别禁用：

|含义|宏名称|例子|检查错误返回码|
|--|--|--|--|
|禁止使用分隔符分割|`ARGPX_ATTR_PARAM_DISABLE_DELIMITER`|`--test=param1,param2`|`kArgpxStatusParamDisallowDelimiter`|
|禁止使用 arg 分割|`ARGPX_ATTR_PARAM_DISABLE_ARG`|`--test=param1 param2`|`kArgpxStatusParamDisallowArg`|

之前所说的所有赋值方式都会尊重分割方式的配置\
（特指尾随字符串）（这是可以被允许的： `--test=param1 param2`）

## 可组合化/Composable

它表示了 Unix 命令中类似 `-abc` 的行为

目前它作为一个组属性 `ARGPX_ATTR_COMPOSABLE` 提供

默认情况下如果遇到了有参数的标志，则会按照其它组属性的意愿判断是否合规。\
如果标志名称后有剩余的字符串，则会根据 `ARGPX_ATTR_ASSIGNMENT_DISABLE_TRAILING` 的存在与否来决定将其作为参数解析。\
反之则抛出对应的返回码

### 需要前缀/Need prefix

可组合化模式中有一个特殊的附加属性：需要前缀（`ARGPX_ATTR_COMPOSABLE_NEED_PREFIX`）。\
它需要可组合化已经启用的情况下才会被尊重，但不然呢，我为什么想说这个

该模式用来模仿某些 DOS/Windows 风格的参数。\
也就是：`/A/B/C`

仅此而已，不过还有些参数范围上的小事情。\
假设现在有一个 arg 长这样：`/Astr/B`，那么只有 str 是 `/A` 的参数，而不是 str/B

另外这里的参数仍然尊重分割方式：`/Astr1,str2/B`。\
但如果分割方式为 arg 比如 `/Astr1 str2/B`，那第二个 arg(`str2/B`) 会被视为一个整体当成参数

## 内置组/Built-in group

ArgParseX 打算提供一些常见的组配置作为默认选项，不过如果配置与起对应的选项风格的细节对不上可不好。\
所以这里目前只有这位开发者熟悉的：`GNU` 与 `Unix`

它们分别以这样的方式提供：

|风格名称|宏|例子|
|--|--|--|
|GNU|`ARGPX_GROUP_GNU`|`--test=param1,param2`|
|Unix|`ARGPX_GROUP_UNIX`|`-A -B=str1,str2 -ABstr1,str2`|

它们都会被扩展为一个 `struct ArgpxFlagGroup` 结构体（不是指针），这是我所想的使用方法：

```c
struct ArgpxFlagGroup group[] = {
    ARGPX_GROUP_GNU,
    {
        .attribute = 0,
        .prefix = "else",
        .assigner = NULL,
        .delimiter = NULL,
    },
};
```
