# This is outdated, do not use.
read -p "C++ standard: " STD
ROOT=`git rev-parse --show-toplevel`
$ROOT/build/generate `find $ROOT/snippets/ -name "*.cpp"` --cpp-standard=$STD > $ROOT/vscode-cpp$STD.code-snippets
