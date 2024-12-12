# 标志/Flag

主要参见 [example/test.c](../example/test.c) 中与 `ArgpxAppendFlag()` 函数相关的部分

## 动作/Action

触发选项后执行的动作

目前可以使用的动作参见 [argpx.h](../example/test.c) 中的 `enum ArgpxActionType` 及其每个条目的注释。\
`action_load` 联合体所需要的参数可以需要参考标头中中以 `ArgpxOut*` 开头的结构体定义

> 下文的所有代码例子都是在设定一个 `struct ArgpxFlag` 结构体的参数

### 单个参数(kArgpxActionParamSingle)

获取由赋值方式定义的参数范围内的一整个字符串，并将其作为参数。\
需要定义一个 `struct ArgpxOutParamSingle` 来确定该字符串的转换方式

这个例子会让 `--test=DemoString` 中等号后的部分（也就是 DemoString）赋值到代码中的 `(char *)demo_string` 里

```c
{
    ...
    .action_type = kArgpxActionParamSingle,
    .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &demo_string},
}
```

单个参数模式完全不会关心可能的分隔符，比如该模式下 `--test=a,b` 中的 `a,b` 会被识别为一个完整的参数

### 多个参数(kArgpxActionParamMulti)

相比于单个参数，多了检测中间的分隔符的能力

这个例子可以将 `--test=Str1,128` 中的 `"Str1"` 与 `"128"` 分别转换到 `(char *)demo1` 与 `(int)demo2` 中

```c
{
    ...
    .action_load.param_multi.count = 2,
    .action_load.param_multi.unit_v =
        (struct ArgpxOutParamSingle[]){
            {.type = kArgpxVarString, .var_ptr = &demo1},
            {.type = kArgpxVarInt, .var_ptr = &demo2},
        },
}
```

### 参数列表(kArgpxActionParamList)

获取由分隔符分割的参数的原始字符串列表，但列表的长度是无法限制的。\
整个列表长度会根据实际输入动态增加，这也要求了调用者需要同时提供一个 int 变量来储存列表长度

比如：

```c
{
    ...
    .action_type = kArgpxActionParamList,
    .action_load.param_list = {
        .count_ptr = &param_list_count,
        .list_ptr = &param_list,
    }
}
```

在这个例子中，如果输入为 `--test=a,b,c` 则 `(char **)param_list` 会被赋值为储存了三个字符串的的列表的指针，而 `(int)param_list_count` 会被设定为该列表的大小（也就是3）

### kArgpxActionSet* 系列

这一部分动作是为了不需要参数的标志创建的，比如：`--setbool`。\
它们通常会将一个目标地址中的内容设定成预先配置的内容，比如把一个栈上的 bool 设定为 true

SetInt 可以用于枚举，SetMemory 可以用于自定义的结构体

### 回调函数

参见 [callback.md](./callback.md)

`kArgpxActionCallbackOnly` 动作就是为了只执行回调而出现的

### 参数末尾的分隔符

参数末尾不允许存在分隔符：`--test=a,b,`\
这将直接返回错误 `kArgpxStatusBizarreFormat`
