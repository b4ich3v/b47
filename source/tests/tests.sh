#!/usr/bin/env bash

set -Euo pipefail
shopt -s lastpipe

script_dir=$(cd "$(dirname "$0")" && pwd)
BIN=${BIN:-}
[[ -z $BIN && -x "$script_dir/../b47" ]] && BIN="$script_dir/../b47"
[[ -z $BIN && -x "$(pwd)/b47" ]]          && BIN="$(pwd)/b47"
if [[ -z ${BIN:-} || ! -x $BIN ]]; then
  echo "error: shell binary 'b47' not found – run 'make' or set BIN=/path" >&2
  exit 1
fi

pass=0
fail=0

strip() 
{
 
  sed -E $'s/\x1B\[[0-9;]*[A-Za-z]//g' |
  sed -E '/^[[:space:]]*[[:alnum:]_]+>[[:space:]]*/d' |
  sed -E '/^[[:space:]]*exit[[:space:]]*$/d'

}

run()
{

  local id="$1" cmd="$2" expect="$3" tmp
  tmp=$(mktemp)
  printf '%s\nexit\n' "$cmd" | "$BIN" 2>/dev/null | strip >"$tmp" || true

  if diff -u <(printf '%s\n' "$expect") "$tmp" >/dev/null; then
    printf '\e[32m[PASS]\e[0m %s\n' "$id"; ((pass++))
  else
    printf '\e[31m[FAIL]\e[0m %s\n' "$id"; ((fail++))
    echo "  expected -> $expect"
    echo -n "  got      -> "; tr -d '\n' <"$tmp"; echo
  fi

  rm -f "$tmp"

}

run echo_simple           'echo hello'                                'hello'
abs_tmp="$(pwd)/tests_tmp_dir"
rm -rf "$abs_tmp"; mkdir -p "$abs_tmp"
run builtin_cd_pwd        "cd '$abs_tmp' && pwd"                      "$abs_tmp"
rm -rf "$abs_tmp"
run pipeline_simple       "printf 'foo\\nbar\\n' | grep bar"          'bar'
run pipeline_wc           "printf 'a b c\\n' | tr ' ' '\n' | wc -l"  '3'
run redir_output          'echo 42 > out.tmp && cat out.tmp && rm out.tmp' '42'
run redir_append          'echo one > file.tmp && echo two >> file.tmp && cat file.tmp && rm file.tmp' $'one\ntwo'
run redir_input           "printf '1 2 3' > in.tmp && wc -w < in.tmp && rm in.tmp" '3'
run logical_and           '[ 5 -eq 5 ] && echo ok'                    'ok'
run logical_or            '[ 5 -eq 4 ] || echo fail'                  'fail'
run quote_spaces          "echo 'a  b   c'"                           'a  b   c'
run subshell_ret          '(echo inner)'                              'inner'
run tilde_expansion       'echo ~'                                    "$HOME"
run command_substitution  'echo "Today is: $(echo Monday)"'           'Today is: Monday'
run chaining_commands     'false; echo a; true && echo b'             $'a\nb'
run test_brackets_eq      '[ 10 -eq 10 ] && echo match'               'match'
run test_brackets_neq     '[ 10 -ne 5 ] && echo true'                 'true'
run escape_characters     'echo \"escaped\"'                          '"escaped"'
run printf_newline        'printf "line1\\nline2\\n"'            $'line1\nline2'
run printf_tabbed         'printf "A\\tB\\tC\\n"'                 $'A\tB\tC'
run sed_usage             "echo 'foo bar baz' | sed 's/bar/BOX/'"     'foo BOX baz'
run grep_case_insensitive "echo Hello | grep -i hello"                'Hello'
run find_and_count "mkdir -p tmp && touch tmp/a.conf tmp/b.conf && find tmp -name '*.conf' | wc -l && rm -r tmp" '2'
run multiple_redir        "echo 123 > f && cat < f > g && cat g && rm f g" '123'
run logical_or_chain   '[ 0 -ne 0 ] || echo or_true' 'or_true'
run nested_subshell    '(echo outer; (echo inner))' $'outer\ninner'
run quote_double       'echo "Variable is $HOME"' "Variable is $HOME"
run quote_single       "echo 'Variable is $HOME'" 'Variable is /home/s0600328'
run command_subs       'echo $(echo $(echo nested))' 'nested'
run pipeline_complex   "echo 'a b c d' | tr ' ' '\n' | grep c | wc -l" '1'
run append_redirection 'echo first > file && echo second >> file && cat file' $'first\nsecond'
run logical_and_chain '[ 1 -eq 1 ] && echo yes && echo still_yes' $'yes\nstill_yes'
run command_substitution_multiple 'echo $(date +%Y)' "$(date +%Y)"
run test_exit_code 'false || echo failed' 'failed'
run string_comparison '[ "abc" = "abc" ] && echo match' 'match'
run command_not_found 'notacommand || echo command_failed' 'command_failed'
run echo_empty           'echo'                           ''
run echo_123             'echo 123'                      '123'
run command_chain        'true && echo ok'                'ok'
run command_chain_fail   'false && echo fail || echo success' 'success'

printf '\n--------------------------------------------------\n'
printf 'Total: %d  Pass: %d  Fail: %d\n' $((pass+fail)) $pass $fail
exit $((fail!=0))
