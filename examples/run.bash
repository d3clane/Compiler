#!/bin/bash

input_file=$1
arch_name=$2

if [ -z "$input_file" ] || [ -z "$arch_name" ]; then
    echo "Usage: $0 [input_file] [-march=name]"
    exit 1
fi

case "$arch_name" in
  "-march=elf64")
    ./runBin.bash "$input_file"
    ;;
  "-march=spu57")
    ./runSpu.bash "$input_file"
    ;;
  *)
    echo "Undefined arch name"
    echo "Possible arch names:"
    echo "- elf64"
    echo "- spu57"
    exit 1
    ;;
esac
