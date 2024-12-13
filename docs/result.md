# 返回结果

## 如果解析正常

如果一切正常 `ArgpxMain` 会返回一个指向充满有效值的 `struct ArgpxResult` 的指针

## 识别状态码的含义

`.status` 看起来不易理解，不过 `ArgpxStatusString()` 可以将其转换成该枚举所对应的英文解释，一个 `strerror()` 的仿制品啦
