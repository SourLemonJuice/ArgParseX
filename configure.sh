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
    --debug)
        cat <<EOF >> "${config_file}"
debug=true
EOF
        ;;
    --enable-hash)
        echo "[Warning] Flag '--enable-hash' is deprecated. Set .use_hash element of struct ArgpxParseOption in runtime to instead it."
        ;;
    --enable-batch-alloc)
        echo "[Warning] Flag '--enable-batch-alloc' is deprecated. Batch alloc is now always on."
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
