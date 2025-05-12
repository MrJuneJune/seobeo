#!/bin/bash
# gcc -O2 -static main.c lib/helper.c lib/server.c lib/router.c -Ilib -o main && ./main
gcc -I../include main.c ../src/*.c -o example_server && ./example_server
