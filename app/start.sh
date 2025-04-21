#!/bin/bash
gcc main.c lib/juneper.c lib/junerver.c -Ilib -o main && ./main
