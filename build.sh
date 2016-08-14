#!/bin/bash

if [ ! -d llvm-src ]; then

  mkdir -p llvm-src
  mkdir -p llvm-obj

  svn checkout https://llvm.org/svn/llvm-project/llvm/trunk@229311 llvm-src
  mv mila llvm-src/projects/mila

fi

cd llvm-obj
../llvm-src/configure CC=gcc CXX=g++
make -j5
