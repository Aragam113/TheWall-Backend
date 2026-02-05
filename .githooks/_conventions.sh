#!/usr/bin/env sh

# Shared helpers for git hooks. Keep POSIX-compatible.

get_current_branch() {
  git rev-parse --abbrev-ref HEAD 2>/dev/null
}

is_main_branch() {
  case "$1" in
    main|master|develop) return 0 ;;
    *) return 1 ;;
  esac
}

is_valid_branch_description_simple() {
  # lowercase letters/numbers, hyphen-separated words
  echo "$1" | grep -Eq '^[a-z0-9]+(-[a-z0-9]+)*$'
}

is_valid_branch_description_release() {
  # lowercase letters/numbers with hyphen or dot separators
  echo "$1" | grep -Eq '^[a-z0-9]+([-.][a-z0-9]+)*$'
}

is_valid_branch_name() {
  branch="$1"

  if is_main_branch "$branch"; then
    return 0
  fi

  # type/description
  case "$branch" in
    */*)
      type="${branch%%/*}"
      desc="${branch#*/}"
      ;;
    *)
      return 1
      ;;
  esac

  case "$type" in
    feature|feat|bugfix|fix|hotfix|release|chore) ;;
    *) return 1 ;;
  esac

  if [ -z "$desc" ]; then
    return 1
  fi

  if [ "$type" = "release" ]; then
    is_valid_branch_description_release "$desc"
    return $?
  fi

  is_valid_branch_description_simple "$desc"
  return $?
}

branch_error_message() {
  cat <<'EOF'
Invalid branch name. Use conventional-branch format:
  - main, master, develop
  - <type>/<description>
    type: feature|feat|bugfix|fix|hotfix|release|chore
    description: lowercase words separated by hyphens
    release description may also use dots for versions (e.g. release/v1.2.0)
Examples:
  feature/add-login
  bugfix/fix-null-crash
  release/v1.2.0
EOF
}