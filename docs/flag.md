# 标志/Flag

主要参见 [example/test.c](../example/test.c) 中与 `ArgpxAppendFlag()` 函数相关的部分

## 动作/Action

触发选项后执行的动作

目前可以使用的动作参见 [argpx.h](../example/test.c) 中的 `enum ArgpxActionType` 及其每个条目的注释。\
`action_load` 联合体所需要的参数可以需要参考标头中中以 `ArgpxOut*` 开头的结构体定义

### 定义方式

在以前的版本中（< v1.0）配置项（`struct ArgpxFlag`）被写到了一个大大的 array(`struct ArgpxFlagSet[]`) 中在编译时定义，现在（>= v1.0）它们都使用一个函数进行添加（append）。\
不过添加的函数也有变动，在 v1.0 及以后使用的函数名是 `ArgpxAppendFlag()` 在大于 v1.1 的版本使用的是 `ArgpxFlagAppend()`

以下示例都只包含 `ArgpxFlag` 本身

### 单个参数(kArgpxActionParamSingle)

获取由赋值方式定义的参数范围内的一整个字符串，并将其作为参数。\
需要定义一个 `struct ArgpxOutParamSingle` 来确定该字符串的转换方式

这个例子会让 `--test=DemoString` 中等号后的部分（也就是 DemoString）赋值到代码中的 `(char *)demo_string` 里

```c
{
    .group_idx = 0,
    .name = "paramsingle",
    .action_type = kArgpxActionParamSingle,
    .action_load.param_single = {.type = kArgpxVarString, .var_ptr = &demo_string},
}
```

单个参数模式完全不会关心可能的分隔符，比如该模式下 `--test=a,b` 中的 `a,b` 会被识别为一个完整的参数

### 单个参数-按需(kArgpxActionParamSingleOnDemand)

该 action 是 ParamSingle 的变种，同一个 out 结构体 `struct ArgpxOutParamSingle`，但其中的 `.var_ptr` 会在被触发时指向在 ArgParseX 内部申请的内存。\
配合 callback 就可以不用让调用者预先保留一块内存，再在 ArgParseX 处理结束后在检测是否为空

在回调函数中需要使用 `ArgpxOutParamSingleFree()` 释放刚才申请的内存

### 多个参数(kArgpxActionParamMulti)

> 过于没用，已经在大于 v1.1 的版本里被删掉啦

相比于单个参数，多了检测中间的分隔符的能力

这个例子可以将 `--test=Str1,128` 中的 `"Str1"` 与 `"128"` 分别转换到 `(char *)demo1` 与 `(int)demo2` 中

```c
{
    .group_idx = 0,
    .name = "parammulti",
    .action_load.param_multi.count = 2,
    .action_load.param_multi.unit_v =
        (struct ArgpxOutParamSingle[]){
            {.type = kArgpxVarString, .var_ptr = &demo1},
            {.type = kArgpxVarInt, .var_ptr = &demo2},
        },
}
```

### 参数列表(kArgpxActionParamList)(<= v1.1)

获取由分隔符分割的参数的原始字符串列表，但列表的长度是无法限制的。\
整个列表长度会根据实际输入动态增加，这也要求了调用者需要同时提供一个 int 变量来储存列表长度

比如：

```c
{
    .group_idx = 0,
    .name = "paramlist",
    .action_type = kArgpxActionParamList,
    .action_load.param_list = {
        .count_ptr = &param_list_count,
        .list_ptr = &param_list,
    }
}
```

在这个例子中，如果输入为 `--test=a,b,c` 则 `(char **)param_list` 会被赋值为储存了三个字符串的的列表的指针，而 `(int)param_list_count` 会被设定为该列表的大小（也就是3）

注意：\
调用者需要在事后调用 `ArgpxParamListFree()` 释放列表所吃掉的内存

### 参数列表(kArgpxActionParamList)(> v1.1)

与以前的版本类似，但现在所有输出都记录在 `.action_load` 联合体对应的结构体中而非使用 `*_ptr` 的指针指向调用者处理的外部内存。\
此外，还可以使用 `.max = 1` 的形式控制列表最大可以写入多少，小于等于0为无限制

```c
{
    .group_idx = 0,
    .name = "paramlist",
    .action_type = kArgpxActionParamList,
    .action_load.param_list = {.max = 3},
    .callback = CallbackFunction_,
});
```

### kArgpxActionSet* 系列 action

这一部分动作是为了不需要参数的标志创建的，比如：`--setbool`。\
它们通常会将一个目标地址中的内容设定成预先配置的内容，比如把一个栈上的 bool 设定为 true

`SetInt` `可以用于枚举，SetMemory` 可以用于自定义的结构体

### 回调函数

参见 [callback.md](./callback.md)

`kArgpxActionCallbackOnly` 动作就是为了只执行回调而出现的

### 参数末尾的分隔符

参数末尾不允许存在分隔符：`--test=a,b,`\
这将直接返回错误 `kArgpxStatusBizarreFormat`
