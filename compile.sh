#! /bin/bash
gcc -fsanitize=address -g ./src/main.c ./src/alloc.c -o main
./main