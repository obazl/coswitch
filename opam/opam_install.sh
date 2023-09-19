#!/usr/bin/env bash

#set -x

# echo "INSTALLER"

TGT=$1
shift
# echo "TGT: $TGT"

## cp .bazel/bin/src/makeheaders to $prefix/bin

# Copy-pasted from the Bazel Bash runfiles library v3.
# https://github.com/bazelbuild/bazel/blob/master/tools/bash/runfiles/runfiles.bash

# --- begin runfiles.bash initialization v3 ---
# Copy-pasted from the Bazel Bash runfiles library v3.
set -uo pipefail; set +e; f=bazel_tools/tools/bash/runfiles/runfiles.bash
source "${RUNFILES_DIR:-/dev/null}/$f" 2>/dev/null || \
    source "$(grep -sm1 "^$f " "${RUNFILES_MANIFEST_FILE:-/dev/null}" | cut -f2- -d' ')" 2>/dev/null || \
    source "$0.runfiles/$f" 2>/dev/null || \
    source "$(grep -sm1 "^$f " "$0.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
    source "$(grep -sm1 "^$f " "$0.exe.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
    { echo>&2 "ERROR: cannot find $f"; exit 1; }; f=; set -e
# --- end runfiles.bash initialization v3 ---

# CRP="$(runfiles_current_repository 1)"
# echo "CRP: $CRP"

X="$(rlocation $TGT)"

# BA="$(rlocation new/templates/ocaml_bigarray.BUILD)"
# echo "BA: $BA"

# echo "BIN: $X"
T=`dirname $X`
echo "LT: `ls $PWD/new`"

mkdir -p $OPAM_SWITCH_PREFIX/share/obazl/templates

CWD=$PWD

cp -RL $CWD/new/templates/ $OPAM_SWITCH_PREFIX/share/obazl/templates

# echo "CWD: $CWD"
# echo "LS CWD: `ls $CWD`"

# mkdir -p $OPAM_SWITCH_PREFIX/lib/coswitch
# touch $OPAM_SWITCH_PREFIX/lib/coswitch/META

install $X $OPAM_SWITCH_PREFIX/bin/coswitch
