# Overview

## A C program that processes a user-provided string with options to:

1. Count words (-c)
2. Reverse the string (-r)
3. Print words and their lengths (-w)
4. Display help (-h)
5. Compilation

## Run the following command to compile:

gcc -o stringfun stringfun.c

## Usage
./stringfun [-h|c|r|w] "string"

## Options

-h: Display help
-c: Count words
-r: Reverse the string
-w: Print words with their lengths

Examples

Help
./stringfun -h
Count Words
./stringfun -c "Sample string"
Output:

Word Count: 2
Buffer:  Sample string....................................
Reverse String
./stringfun -r "Reverse me"
Output:

Reversed String: em esreveR
Print Words
./stringfun -w "Word lengths"
Output:

1. Word (4)
2. lengths (7)
Notes

Buffer size is fixed at 50 characters.
Extra spaces and tabs are compressed.
