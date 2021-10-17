// Linux only.

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>

#include "split.h"

const std::string DefaultCompilerPath = "/usr/bin/g++";
const std::string ExeName = "tmp.exe";

std::string two_digits(int x) {
    std::string a(2, 0);
    a[0] = x / 10 + '0';
    a[1] = x % 10 + '0';
    return a;
}

std::string make_cmd(const std::string& compiler_path,
    int cpp_version, const std::string& source, bool allow_warnings)
{
    std::string cmd = compiler_path;
    cmd += " -pedantic-errors";
    if (!allow_warnings) {
        cmd += " -Wall -Werror";
    }
    cmd += " -o ";
    cmd += ExeName;
    cmd += " -std=c++";
    cmd += two_digits(cpp_version);
    cmd += ' ';
    cmd += source;
    cmd += " 2>/dev/null";
    return cmd;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> input_files;

    std::string compiler_path = DefaultCompilerPath;
    std::pair<std::string*, std::string> supported_options[] = {
        {&compiler_path, "--compiler-path="},
    };

    // Parse command line options
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        bool is_option = false;
        for (auto& option : supported_options) {
            if (arg.find(option.second) == 0) {
                *option.first = arg.substr(option.second.size());
                is_option = true;
            }
        }

        if (!is_option) {
            // Then it is a file
            input_files.push_back(arg);
        }
    }

    int warnings = 0, errors = 0;

    for (auto& path : input_files) {
        auto filename = split(path, '/').back();
        int file_cpp_ver = stoi(split(filename, '.')[1]);
        for (int version : {3, 11, 14, 17, 20}) {

            if (version < file_cpp_ver) {
                auto cmd = make_cmd(compiler_path, version, path, true);
                int status = system(cmd.c_str());
                if (status == 0) {
                    std::cout << "Warning: " << path << " compiles with lower cpp version " << two_digits(version) << '\n';
                    warnings++;
                }
            } else {
                auto cmd = make_cmd(compiler_path, version, path, false);
                int status = system(cmd.c_str());
                if (status != 0) {
                    std::cout << "Error: " << path << " does not compile with cpp version " << two_digits(version) << '\n';
                    errors++;
                } else {
                    auto cmd = "./" + ExeName + " >/dev/null";
                    int status = system(cmd.c_str());
                    if (status != 0) {
                        std::cout << "Error: " << path << " failed test with cpp version " << two_digits(version) << '\n';
                        errors++;
                    }
                }
            }
        }
    }

    std::cout << "Finished with " << warnings << " warnings and " << errors << " errors.\n";
}
