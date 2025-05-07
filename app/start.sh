#!/bin/bash
gcc -static main.c lib/juneper.c lib/junerver.c api/post.c -Ilib -Iapi -o main && ./main
