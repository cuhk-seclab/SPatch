#!/bin/bash
cd $3
/home/anonym/juxta/llvm/tools/clang/tools/scan-build/fss-build --use-analyzer=/home/anonym/juxta/bin/llvm/bin/clang --fss-output-dir=$1 CC=/home/anonym/juxta/bin/llvm/bin/clang $2
