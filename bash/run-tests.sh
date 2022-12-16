# This is outdated, do not use.
read -p "Threads: " THR
read -p "Compiler path: " CXX
ROOT=`git rev-parse --show-toplevel`
$ROOT/build/test --threads=$THR --compiler-path=$CXX `find $ROOT/snippets/ -name "*.cpp"`
