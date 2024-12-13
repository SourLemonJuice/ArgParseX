#!/usr/bin/env bash

# lemon-bash-lib | separator | v4
# print a separator line
# options:
# $1 (optional,default == '=')
#   option's value == $display_char
#
# $2 (optional,default similar to '1')
#   it's used for control $separator_w
#   expression: $[$shell_w/$2]
# note:
# If the shell width not available, then fix width to 2.
# GitHub Action is an example...
function separator {
    local display_char
    if [[ -z "$1" ]] || [[ ! ${#1} -eq 1 ]]; then
        display_char='='
    else
        display_char="$1"
    fi

    # 获取终端宽度
    local shell_w="$(stty size | awk '{print $2}')"
    if [[ -z "$shell_w" ]]; then
        shell_w=2
    fi

    # 用输入的信息计算出要打出多长的分割线
    local separator_w
    if [[ -z "$2" ]]; then
        # 如果没有输入则直接用终端宽度
        separator_w="$shell_w"
    else
        separator_w="$(($shell_w / "$2"))"
    fi

    # 宽度永远不能为零
    if [[ "$separator_w" -eq 0 ]]; then
        separator_w="$shell_w"
    fi

    # 输出
    yes "$display_char" | sed "${separator_w}q" | tr -d "\n" && echo ""
}

function AssertSuccess {
    separator - 3

    command -- $@
    if [[ $? -ne 0 ]]; then
        echo "[Failure] Assert failed, expected Success(0) -- $@"
        exit 1
    fi
}

function AssertFailure {
    separator - 3

    command -- $@
    if [[ $? -eq 0 ]]; then
        echo "[Failure] Assert failed, expected Failure(1) -- $@"
        exit 1
    fi
}

test_exec='./test.out'

AssertSuccess $test_exec param1 param2 paramEnd
AssertFailure $test_exec --ffff
AssertFailure $test_exec -ffff
AssertSuccess $test_exec --setbool --setint paramEnd
AssertSuccess $test_exec --test=testStr1,testStr2
AssertSuccess $test_exec ++test2~str1-str2
AssertSuccess $test_exec -baac -a -- -ba paramEnd
AssertSuccess $test_exec /win1Param1/win2Param2 paramEnd

separator - 3
echo "[Success] All test passed(only the return code is detected)"
