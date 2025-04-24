#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

# Check if the first argument looks like an option (starts with '-') or is a known command.
# If so, execute the command and arguments as passed.
if [ "$#" -gt 0 ] && ( [ "${1:0:1}" = '-' ] || [ "$1" = 'djpegli' ] || [ "$1" = 'bash' ] || [ "$1" = 'sh' ] || [ "$1" = 'cjpegli' ] ); then
  exec "$@"
else
  # Otherwise, assume the user wants to run cjpegli. Prepend 'cjpegli' to the arguments.
  exec cjpegli "$@"
fi 