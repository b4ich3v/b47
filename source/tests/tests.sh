#!/usr/bin/env bash

set -Eeuo pipefail
shopt -s lastpipe

BIN=${BIN:-./b47}

if [[ ! -x "$BIN" ]]; then
  echo "error: shell binary '$BIN' not found or not executable" >&2
  exit 1
fi

passCounter=0
failCounter=0

stripPrompt() 
{
  
  sed -E $'s/\x1B\[[0-9;]*[A-Za-z]//g' |
  sed -E '/^[[:space:]]*[[:alnum:]_]+>[[:space:]]*/d' |
  sed -E 's/[[:space:]]*[[:alnum:]_]+>[[:space:]]*$//'

}

runTest() 
{

    local name="$1"  
    local commands="$2"
    local expected="$3"

    local out_file
    out_file=$(mktemp)

    printf "%s\nexit\n" "$commands" | "$BIN" 2>/dev/null | stripPrompt | sed '/^$/d' >"$out_file"

    if diff -u <(printf "%s\n" "$expected") "$out_file" >/dev/null; then
        printf '\e[32m[PASS]\e[0m %s\n' "$name"
        ((passCounter++))
    else
        printf '\e[31m[FAIL]\e[0m %s\n' "$name"
        echo "  expected -> $expected"
        echo -n "  got      -> "; cat "$out_file" | tr -d '\n'; echo
        ((failCounter++))
    fi

    rm -f "$out_file"

}

runTest "echo_simple"        "echo hello"                                       "hello"
runTest "builtin_cd_pwd"     "mkdir -p tests_tmp; cd tests_tmp; pwd"            "$(pwd)/tests_tmp"
runTest "variable_assign"    "FOO=bar; echo $FOO"                               "bar"
runTest "pipeline"           "echo -e 'foo\nbar' | grep bar"                   "bar"
runTest "redir_output"       "echo 42 > out.tmp; cat out.tmp"                  "42"
runTest "redir_input"        "echo 'one' > in.tmp; wc -l < in.tmp"             "1"
runTest "background_job"     "sleep 0.1 & echo done"                           "done"

echo "--------------------------------------------------"
echo "Total: $((passCounter+failCounter))  Pass: $passCounter  Fail: $failCounter"
exit $((failCounter!=0))
