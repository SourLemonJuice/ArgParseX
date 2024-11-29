# 预期支持目标

## 前缀

- `--test`
- `+=)))test`
- `-test`
- `test`

没有前缀简直就是异端（烦）

## 命名

- `--test`
- `-t`
- `-abcd`
- `-a-b-c-d`
- `/a/b/c/d`

## 标志参数

`--test` 与 `-t` 为标志名，`str` 是它们的负载，`str2` 是多参数示例中的第二参数

- `--test=str`
- `--test str`
- `--test===str`
- `--teststr`
- `-t=str`
- `-t str`
- `-abct=str`
- `-abct str`
- `-tstr`
- `/a/t=str/b`
- `/a/tstr/b`

## 特殊符号

比如 `--` 用来停止解析，甚至类似 `cd` 命令的 `-`（跳转到上次执行 `cd` 以外命令的位置）
