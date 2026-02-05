#!/usr/bin/env sh

set -eu

git config core.hooksPath .githooks
printf '%s\n' 'core.hooksPath set to .githooks'