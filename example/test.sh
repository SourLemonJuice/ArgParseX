#!/usr/bin/env bash

# lemon-bash-lib | separator | v3
# 分隔线函数
# options:
# $1 (optional,default == '=')
#   option's value == $display_char
#
# $2 (optional,default similar to '1')
#   it's used for control $spearator_width
#   expression: $[$shell_width/$2]
function separator {
    local display_char
    if [[ -z $1 ]] || [[ ! ${#1} -eq 1 ]]; then
        display_char='='
    else
        display_char=$1
    fi

    # 获取终端宽度
    local shell_width=$(stty size | awk '{print $2}')
    # 用输入的信息计算出要打出多长的分割线
    local spearator_width
    if [[ -z $2 ]]; then
        # 如果没有输入则直接用终端宽度
        spearator_width=$shell_width
    else
        spearator_width=$(($shell_width / $2))
    fi
    # 输出
    yes "$display_char" | sed "${spearator_width}q" | tr -d "\n" && echo ""
}

function AssertSuccess {
    separator - 3

    command -- $@
    if [[ $? -ne 0 ]]; then
        echo "> Assert failed, expected Success(0) -- $@"
        exit 1
    fi
}

function AssertFailure {
    separator - 3

    command -- $@
    if [[ $? -eq 0 ]]; then
        echo "> Assert failed, expected Failure(1) -- $@"
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
echo "All test passed(only the return code is detected)"
