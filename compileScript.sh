#!/bin/bash
gcc -std=c99 -o slaveProcess slave.c
gcc -std=c99 -o viewProcess view.c
gcc -std=c99 -o mainApplication main.c
