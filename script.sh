#!/bin/bash

module load compilers/gnu-5.4.0
module load libraries/openmpi-2.0.1-gcc-5.4.0
make clean
make $1
time make run-$1
