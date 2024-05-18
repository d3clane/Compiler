#!/bin/bash

input_file=$1

if [ -z "$input_file" ]; then
    echo "Usage: $0 <input file>"
    exit 1
fi

./bin/frontEnd $input_file bin/ParseTree.txt

./bin/middleEnd bin/ParseTree.txt bin/SimplifiedTree.txt

./bin/backEnd bin/ParseTree.txt bin/Out.bin