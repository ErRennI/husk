#!/bin/sh

HUSK=$1
TEST_DIR=${2:-tests}
TIMEOUT_SECS=5

if [ -z "$HUSK" ] || [ ! -x "$HUSK" ]; then
    echo "usage: $0 path/to/husk [tests-dir]" >&2
    exit 2
fi

# Absolute paths, because each test runs inside its own temp folder.
HUSK=$(cd "$(dirname "$HUSK")" && pwd)/$(basename "$HUSK")
TEST_DIR=$(cd "$TEST_DIR" && pwd)

# Stop a hanging test instead of letting CI hang. `timeout` is part of
# GNU coreutils (Linux); on macOS it may be missing, so it's optional.
if command -v timeout >/dev/null 2>&1; then
    LIMIT="timeout $TIMEOUT_SECS"
else
    LIMIT=""
fi

pass=0
fail=0

for input in "$TEST_DIR"/*.sh; do
    name=$(basename "$input" .sh)
    expected="$TEST_DIR/$name.expected"

    if [ ! -f "$expected" ]; then
        echo "SKIP $name (no $name.expected)"
        continue
    fi

    work=$(mktemp -d)
    actual="$work/.actual"
    errors="$work/.stderr"

    (cd "$work" && HUSK_TEST=hello $LIMIT "$HUSK" < "$input" > "$actual" 2> "$errors")
    status=$?

    if [ "$status" -eq 124 ]; then
        echo "FAIL $name (timed out after ${TIMEOUT_SECS}s)"
        fail=$((fail + 1))
    elif diff -u "$expected" "$actual" > "$work/.diff"; then
        echo "PASS $name"
        pass=$((pass + 1))
    else
        echo "FAIL $name"
        sed 's/^/    /' "$work/.diff"
        if [ -s "$errors" ]; then
            echo "    --- stderr:"
            sed 's/^/    /' "$errors"
        fi
        fail=$((fail + 1))
    fi

    rm -rf "$work"
done

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
