#/usr/bin/env bash

# for scan-build(Clang Static Analyzer)
CC="clang"

src_code=(
    "../source/argpx*.c"
    "./test.c"
)

cflag=(
    "-std=c99"
    "-I../include/"
    "-Wall"
    "-Wvla"
    "-Og"
    "-g"
    # for gprof
    # "-pg"
)

# https://unix.stackexchange.com/a/193660
function run_echo
{
    local PS4='running command ->| '
    local -
    set -o xtrace

    $@
}

run_echo \
"${CC}" "${src_code[@]}" "${cflag[@]}" \
    --output test.out

echo ""
echo "the demo for ./test.out is in ./test.sh"
