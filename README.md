# snippets-cpp
Manage C++ snippets for any editor.

CMake and a C++ compiler supporting at least C++17 are required to compile this project.

## Generate utility

### Example usage
``./generate `find snippets/ -name "*.cpp"` --cpp-standard=20 > vscode-cpp20.code-snippets``

### Options
+ `--cpp-standard=N` sets the target C++ language standard. If there are multiple snippets with the same shortcut, choose the one with the highest language standard that doesn't exceed N. If there are no snippets with some shortcut that satisfy the standard (for example, the target is 11, and you only have snippets with 14 and 17), picks the lowest one.
+ `--tab-width=N` sets the tab width in which the snippets are written. Snippets will be stored using tabs, switching to spaces if needed.
+ `--target=ide` sets the target IDE for which to generate snippets. Currently only VSCode (file.code-snippets) is supported.

All other command line arguments are interpreted as input files.

### Snippets syntax

A snippet is mostly a normal cpp file. It must follow the naming convention `shortcut.std.cpp` where `shortcut` is the word triggering the snippet, and `std` is the C++ language standard. A snippet must contain
exactly two comments `/*snippet-begin*/` and `/*snippet-end*/`, and the first line must be a single line comment describing the snippet. The snippet is the text between these two comments. The text will be automatically unindented. To add placeholders to the snippet, put a macro `SNIPPET_ARG` with one, two, or more arguments. The first argument will be interpreted as the placeholder ID. The second argument will be interpreted as the default text, and all the following arguments are ignored.

`#define SNIPPET_ARG(x, y, z) z`

For example, you can define a snippet for the begin/end iterator pair like this:
```
// Begin-end iterators
/*snippet-begin*/begin(SNIPPET_ARG(1, sequence)), end(SNIPPET_ARG(1, sequence))/*snippet-end*/
```

## Test utility

Use the Test utility to ensure your snippets are correct, and compile using the target language version. It will compile every snippet using every language version from the set (3, 11, 14, 17, 20). If the language version exceeds the snippet language version, the snippet is expected to compile, otherwise, the snippet is expected to fail compilation - in that case, a warning will be raised. After successful
compilation, the snippet is run and its exit code is checked. If it's nonzero, the run failed and an error will be raised. 

### Example usage
`./test --compiler-path=/usr/bin/g++-11 --threads=8 snippets/*.cpp`

### Options
+ `--compiler-path=path` sets the C++ compiler to use. Currently only newer versions of GCC are guaranteed to work.
+ `--threads=N` use up to N threads to run compilation and execution jobs.

All other command line arguments are interpreted as input files.
