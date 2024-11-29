# ArgParseX 文档

- [标志组/Flag group](./flag_group.md)
- [标志/Flag](./flag.md)
- [返回结果](./result.md)
- [处理流程](./flow.md)
- [命名](./naming.md)
- [风格](./style.md)
- [预期支持目标](./support_goal.md)

## 版本

> 已被删除

库文件的版本在 argpx.h 中有定义，宏分别为：

|宏|例子|解释|
|--|--|--|
|ARGPX_VERSION_MAJOR|v1.0 -> 1|主要版本号，包含重要接口变化（我不会把它变成永远是 1 的摆设的）|
|ARGPX_VERSION_MINOR|v1.0 -> 0|次要版本号，最多只会有极小的接口变化|
|ARGPX_VERSION_REVISION||一个每次发布都会加一的计数器，如果想固定检查某个版本，这会很有用的|
