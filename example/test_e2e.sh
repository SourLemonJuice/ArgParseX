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

    echo ">>> $*"
    command -- $@
    if [[ $? -ne 0 ]]; then
        echo "[Failure] Assert failed, expected Success(0) -- $@"
        exit 1
    fi
}

function AssertFailure {
    separator - 3

    echo ">>> $*"
    command -- $@
    if [[ $? -eq 0 ]]; then
        separator - 3
        echo "[Failure] Assert failed, expected Failure(1) -- $@"
        exit 1
    fi
}

out='./test.out'

AssertSuccess $out param1 param2 paramEnd
AssertFailure $out --ffff
AssertFailure $out -ffff
AssertSuccess $out --setbool --setint paramEnd
# AssertSuccess $out --test=testStr1,testStr2
# AssertSuccess $out ++test2~str1-str2
AssertSuccess $out -baac -a -- -ba paramEnd
AssertSuccess $out /win1Param1/win2Param2 paramEnd
AssertSuccess $out --samename ++samename

separator - 3
echo "[Success] All test passed(only the return code is detected)"
