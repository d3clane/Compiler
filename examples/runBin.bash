#!/bin/bash

input_file=$1

if [ -z "$input_file" ]; then
    echo "Usage: $0 <input file>"
    exit 1
fi

./bin/preprocessor $input_file bin/after_processing.txt

./bin/frontEnd bin/after_processing.txt bin/ParseTree.txt

./bin/middleEnd bin/ParseTree.txt bin/SimplifiedTree.txt

./bin/backEnd bin/SimplifiedTree.txt bin/Out.bin

chmod +x bin/Out.bin

rm -rf bin/*.html
rm -rf bin/ParseTree.txt
rm -rf bin/SimplifiedTree.txt