# Benchmark

In speed there is no advantage of ArgParseX over the classic GNU getopt. But the difference is less then double in the best case.\
Here are the benchmark results on my PC:

Test sample:

```c
#define ARGPX_DEV_BM_SAMPLE_ARR \
    (char *[]){ \
        "--config=./a_file.txt", \
        "-abcParamOfC",\
        "--verbose", \
        "param1", \
        "--log-level=1", \
        "param2", \
        "--retry=7", \
    }
```

Compiler `clang version 18.1.8, Target: x86_64-pc-linux-gnu`.\
Count of loop: `10 * 1000 * 1000`\
Source code located in: [benchmark/](./benchmark/)

|Library|Time|
|:--|:--|
|GNU getopt|0m1.456s|
|ArgParseX(-O3)|0m2.580s|
|ArgParseX(-O0)|0m4.619s|
