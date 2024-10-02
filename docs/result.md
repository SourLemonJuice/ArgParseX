# 返回结果

## 如果解析正常

如果一切正常 `ArgpxMain` 会返回一个指向充满有效值的 `struct ArgpxResult` 的指针

其中将包含：

|含义|成员名|
|--|--|
|永远为 `kArgpxStatusSuccess` 的返回码|`.status`|
|当前的 arg 在 argv 中的索引|`.current_argv_idx`|
|指向当前 arg 字符串的指针|`.current_argv_ptr`|
|命令参数数量|`.param_count`|
|命令参数 array|`.paramv`|
|当初喂给 `ArgpxMain` 的那个 arg 计数器|`.argc`|
|`.argv` 记的就是它的数量对吧|`.argv`|

## 错误返回

比起常规返回，错误返回会很不一样。它不会直接体现到返回码（`.status`）上，而是会调用注册的回调函数

回调函数需要能接受一个 `struct ArgpxResult *` 的参数并通过它访问当前出错的 arg 是什么，又是由于什么出错的。\
这是 [example/test.c](../example/test.c) 中的一个例子：

```c
static void Error_(struct ArgpxResult *res)
{
    printf("Error, parser status: %d\n", res->status);
    printf("%s\n", ArgpxStatusToString(res->status));
    printf("Problematic argument(index %d): %s\n", res->current_argv_idx, res->current_argv_ptr);
    exit(EXIT_FAILURE);
}
```

`.status` 看起来不易理解，不过 `ArgpxStatusToString()` 可以将其转换成该枚举所对应的英文解释，一个 `strerror()` 的仿制品啦

另外，如果当初没有设定回调函数，解析还出了错，则会直接在 argpx.c 中调用 `exit(EXIT_FAILURE)` 结束整个程序
