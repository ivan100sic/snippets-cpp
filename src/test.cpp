// Linux only.

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>

#include "split.h"

class ThreadPool {
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_shutdown;

    void worker() {
        while (1) {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [&]() { return m_shutdown || !m_tasks.empty(); });

            if (m_tasks.empty()) {
                return;
            }

            auto task = std::move(m_tasks.front());
            m_tasks.pop();
            lock.unlock();

            task();
        }
    }

public:
    ThreadPool(size_t n) : m_threads(n), m_shutdown(false) {
        for (auto& t : m_threads) {
            t = std::thread([&]() { worker(); });
        }
    }
    
    void push(std::function<void()> task) {
        m_mutex .lock();
        m_tasks.push(task);
        m_mutex.unlock();
        m_cv.notify_all();
    }

    void wait() {
        m_mutex.lock();
        m_shutdown = true;
        m_mutex.unlock();
        m_cv.notify_all();
        for (auto& t : m_threads) {
            t.join();
        }
    }
};

const std::string DefaultCompilerPath = "/usr/bin/g++";
const int DefaultThreads = 4;

std::string two_digits(int x) {
    std::string a(2, 0);
    a[0] = x / 10 + '0';
    a[1] = x % 10 + '0';
    return a;
}

std::string exe_name(const std::string& source_path, int cpp_version) {
    std::hash<std::string> hasher;
    return "tmp/" + std::to_string(hasher(source_path + "@" + std::to_string(cpp_version))) + ".exe";
}

std::string make_cmd(const std::string& compiler_path,
    int cpp_version, const std::string& source_path, bool allow_warnings)
{
    std::string cmd = compiler_path;
    cmd += " -pedantic-errors";
    if (!allow_warnings) {
        cmd += " -Wall -Werror";
    }
    cmd += " -o ";
    cmd += exe_name(source_path, cpp_version);
    cmd += " -std=c++";
    cmd += two_digits(cpp_version);
    cmd += ' ';
    cmd += source_path;
    cmd += " 2>/dev/null";
    return cmd;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> input_files;

    std::string compiler_path = DefaultCompilerPath;
    std::string threads = std::to_string(DefaultThreads);
    std::pair<std::string*, std::string> supported_options[] = {
        {&compiler_path, "--compiler-path="},
        {&threads, "--threads="},
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

    ThreadPool executor(stoi(threads));
    std::atomic<int> warnings = 0, errors = 0;

    system("rm -rf tmp");
    system("mkdir tmp");

    for (auto& path : input_files) {
        auto filename = split(path, '/').back();
        int file_cpp_ver = stoi(split(filename, '.')[1]);
        for (int version : {3, 11, 14, 17, 20}) {
            if (version < file_cpp_ver) {
                executor.push([version, compiler_path, path, &warnings]() {
                    auto cmd = make_cmd(compiler_path, version, path, true);
                    int status = system(cmd.c_str());
                    if (status == 0) {
                        std::cout << "Warning: " << path << " compiles with lower cpp version " << two_digits(version) << '\n';
                        warnings++;
                    } else {
                        std::cout << "OK: " << path << ' ' << two_digits(version) << '\n';
                    }
                });
            } else {
                executor.push([version, compiler_path, path, &errors]() {
                    auto cmd = make_cmd(compiler_path, version, path, false);
                    int status = system(cmd.c_str());
                    if (status != 0) {
                        std::cout << "Error: " << path << " does not compile with cpp version " << two_digits(version) << '\n';
                        errors++;
                    } else {
                        auto cmd = "./" + exe_name(path, version) + " >/dev/null";
                        int status = system(cmd.c_str());
                        if (status != 0) {
                            std::cout << "Error: " << path << " failed test with cpp version " << two_digits(version) << '\n';
                            errors++;
                        } else {
                            std::cout << "OK: " << path << ' ' << two_digits(version) << '\n';
                        }
                    }
                });
            }
        }
    }

    executor.wait();
    system("rm -rf tmp");
    std::cout << "Finished with " << warnings << " warnings and " << errors << " errors.\n";
}
