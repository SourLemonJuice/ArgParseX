#!/usr/bin/env bash
# Generate config.mk for the main Makefile.

sh_name="configure.sh"
config_file="./config.mk"

> "${config_file}"

for arg in "$@"; do
    case "$arg" in
    --help | -h)
        echo "NULL == NULL == 0x0 != awa != QwQ != qvq"
        exit 0
        ;;
    --enable-hash)
        cat <<EOF >> "${config_file}"
enable_hash=true
EOF
        ;;
    --enable-batch-alloc)
        cat <<EOF >> "${config_file}"
enable_batch_alloc=true
EOF
        ;;
    haoye | 好耶)
        echo "好耶"
        # for safety, return non-zero code
        exit 1
        ;;
    *)
        echo "${sh_name}: Unknown argument '${arg}'"
        exit 1
        ;;
    esac
done
