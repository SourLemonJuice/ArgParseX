#!/usr/bin/env bash

cmd_arg=(
    "param1"
    "--setbool"
    "--setint"
    "param2"
    "-baac"
    "-a"
    "/win1Param1/win2Param2"
    "--"
    "-ba"
    "paramEnd"
)

hyperfine --shell=none -- "./test.out ${cmd_arg[*]}"
