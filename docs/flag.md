# 标志/Flag

主要参见 [example/test.c](../example/test.c) 中与 `struct ArgpxFlag` 相关的部分

## 动作/Action

触发选项后的执行的动作

目前可以使用的动作参见 [argpx.h](../example/test.c) 中的 `enum ArgpxActionType` 及其每个条目的注释。\
`action_load` 联合体所需要的参数可以需要参考 .h 中以 `ArgpxHidden_Outcome` 开头的结构体定义

TODO
