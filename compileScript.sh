#!/bin/bash
gcc -std=c99 -Wall -o slaveProcess ./slave.c
gcc -std=c99 -Wall -o viewProcess ./view.c
gcc -std=c99 -Wall -o mainApplication ./main.c
