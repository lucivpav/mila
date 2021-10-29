#!/bin/bash

if [ ! -d llvm-src ]; then
  mkdir -p llvm-src
  mkdir -p llvm-obj
  git clone git@github.com:llvm/llvm-project.git llvm-src
  cd llvm-src
  git checkout 00bd79d794b1
  cd ..
  ln -s $(pwd)/src llvm-src/llvm/projects/mila
fi

cd llvm-obj
../llvm-src/llvm/configure --enable-optimized
make -j5
