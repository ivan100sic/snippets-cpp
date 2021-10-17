#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <deque>

#include "split.h"

const std::string SnippetBegin = "/*snippet-begin*/";
const std::string SnippetEnd = "/*snippet-end*/";
const std::string SnippetArg = "SNIPPET_ARG";
const std::string DefaultTarget = "vscode";
const int DefaultTabWidth = 4;

#ifdef _WIN32
const char PathSeparator = '\\';
#else
const char PathSeparator = '/';
#endif

std::string read_file(const std::string& path) {
    std::string result;
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        std::cout << "Couldn't open file: " << path << '\n';
        exit(1);
    }
    stream.seekg(0, std::ios::end);
    size_t size = stream.tellg();
    std::string buffer(size, 0);
    stream.seekg(0);
    stream.read(&buffer[0], size);
    return buffer;
}

std::string trim(const std::string& str) {
    if (str.empty()) {
        return str;
    }

    size_t first = 0, last = str.size() - 1;
    while (first < str.size() && isspace(str[first])) {
        first++;
    }

    if (first == str.size()) {
        return std::string();
    }

    while (isspace(str[last])) last--;

    return str.substr(first, last - first + 1);
}

class Snippet {
public:
    struct Placeholder {
        std::string name;
        int id;
    };

    struct Element {
        bool is_placeholder;
        Placeholder placeholder;
        std::string text;
    };

    static Element make_placeholder(const std::string& arg_string) {
        Element result;
        result.is_placeholder = true;
        auto comma_split = split(arg_string, ',');
        result.placeholder.id = arg_string[0] - '0';
        if (comma_split.size() >= 2) {
            result.placeholder.name = trim(comma_split[1]);
        }
        return result;
    }

    static Element make_text(const std::string& str) {
        Element result;
        result.is_placeholder = false;
        result.text = str;
        return result;
    }

private:
    std::string m_name;
    std::string m_shortcut;
    int m_standard;
    std::vector<Element> m_elements;

public:
    const std::string& name() const { return m_name; }
    const std::string& shortcut() const { return m_shortcut; }
    int standard() const { return m_standard; }

    std::vector<Element>& elements() { return m_elements; }
    const std::vector<Element>& elements() const { return m_elements; }

    static Snippet from_file(const std::string& path) {
        Snippet result;
        auto data = read_file(path);
        auto filename = split(path, PathSeparator).back();
        auto filename_split = split(filename, '.');
        if (filename_split.size() != 3) {
            std::cout << "Wrong file format: " << filename << '\n';
            std::cout << "Expected: shortcut.cppVersion.cpp\n";
            std::cout << "Example: sortall.11.cpp\n";
            exit(1);
        }

        result.m_shortcut = filename_split[0];
        result.m_standard = std::stoi(filename_split[1]);
        {
            auto file_lines = split(data, '\n');
            auto first_line = file_lines[0];
            if (first_line.size() < 2 || first_line[0] != '/' || first_line[1] != '/') {
                std::cout << "First line of file doesn't begin with //";
                exit(1);
            }

            result.m_name = trim(first_line.substr(2));
        }

        // Find the snippet
        auto pos_first = data.find(SnippetBegin);
        auto pos_last = data.find(SnippetEnd);

        if (pos_first == std::string::npos || pos_last == std::string::npos) {
            std::cout << "Error reading snippet " << path << '\n';
            std::cout << "/*snippet-begin*/ or /*snippet-end*/ not found\n";
            exit(1);
        }

        pos_first += SnippetBegin.size();

        if (pos_first >= pos_last) {
            std::cout << "snippet-end is before snippet-begin\n";
            exit(1);
        }

        data = data.substr(pos_first, pos_last - pos_first);

        // Parse the snippet
        size_t parsed = 0;

        while (parsed < data.size()) {
            auto find_result = data.find(SnippetArg, parsed);
            if (find_result != data.npos) {
                result.m_elements.push_back(make_text(data.substr(parsed, find_result - parsed)));
                find_result += SnippetArg.size();
                if (find_result < data.size() && data[find_result] == '(') {
                    // find closing parenthesis
                    auto closing = data.find(')', find_result + 1);
                    if (closing != data.npos) {
                        result.m_elements.push_back(
                            make_placeholder(
                                data.substr(find_result + 1, closing - find_result - 1)
                            )
                        );

                        parsed = closing + 1;
                    } else {
                        std::cout << "Error, SNIPPET_ARG closing parenthesis not found\n";
                        exit(1);
                    }
                } else {
                    std::cout << "Error, SNIPPET_ARG not followed by opening parenthesis\n";
                    exit(1);
                }

            } else {
                result.m_elements.push_back(make_text(data.substr(parsed)));
                parsed = data.size();
            }
        }

        return result;
    }
};

// Doesn't add quotes
std::string json_escape(const std::string str) {
    std::string result;

    for (char c : str) {
        if (c == '\\') {
            result += "\\\\";
        } else if (c == '"') {
            result += "\\\"";
        } else if (c == '\t') {
            result += "\\t";
        } else if (c == '\n') {
            result += "\\n";
        } else if (c == '\r') {
            result += "\\r";
        } else {
            result += c;
        }
    }

    return result;
}

std::string vscode_elements_as_json_string(const std::vector<Snippet::Element>& elements) {
    std::string result;
    for (const auto& element : elements) {
        if (element.is_placeholder) {
            result += '$';
            if (element.placeholder.name.size()) {
                result += '{';
                result += std::to_string(element.placeholder.id);
                result += ':';
                result += element.placeholder.name;
                result += '}';
            } else {
                result += std::to_string(element.placeholder.id);
            }
        } else {
            result += json_escape(element.text);
        }
    }

    return result;
}

std::string export_vscode(const std::vector<Snippet>& snips) {
    if (snips.empty()) {
        return "{}\n";
    }

    std::string result = "{";
    for (auto& snip : snips) {
        result += "\n\t\"";
        result += json_escape(snip.name());
        result += "\": {\n\t\t\"scope\": \"cpp\",\n\t\t\"prefix\": \"";
        result += snip.shortcut();
        result += "\",\n\t\t\"body\": \"";
        result += vscode_elements_as_json_string(snip.elements());
        result += "\"\n\t},\n";
    }

    result.pop_back();
    result.pop_back();

    result += "\n}\n";
    return result;
}

void remove_common_indentation(Snippet& snip, int tab_width) {
    struct Line {
        std::deque<Snippet::Element> elements;
        int spaces; 
    };

    std::vector<Line> lines;
    lines.emplace_back();

    for (auto& elem : snip.elements()) {
        if (!elem.is_placeholder) {
            auto text_lines = split(elem.text, '\n');
            // Add the first element to the end of the current line
            lines.back().elements.push_back(Snippet::make_text(text_lines[0]));
            // Add the rest as new lines
            for (size_t i = 1; i < text_lines.size(); i++) {
                lines.emplace_back();
                lines.back().elements.push_back(Snippet::make_text(text_lines[i]));
            }
        } else {
            // Just add this placeholder into the current line
            lines.back().elements.push_back(elem);
        }
    }

    int common_spaces = 1 << 30;

    // Determine leading space for each line
    for (auto& line : lines) {
        line.spaces = 0;
        while (line.elements.size()) {
            auto& elem = line.elements.front();
            if (elem.is_placeholder) {
                break;
            } else {
                size_t pos = 0;
                while (pos < elem.text.size()) {
                    if (elem.text[pos] == ' ') {
                        pos++;
                        line.spaces++;
                    } else if (elem.text[pos] == '\t') {
                        pos++;
                        line.spaces += tab_width;
                    } else if (elem.text[pos] == '\r') {
                        pos++;
                    } else {
                        break;
                    }
                }

                if (pos == elem.text.size()) {
                    line.elements.pop_front();
                } else {
                    elem.text.erase(0, pos);
                    break;
                }
            }
        }

        // If there is nothing left, ignore this line
        if (line.elements.empty()) {
            line.spaces = -1;
        } else {
            common_spaces = std::min(common_spaces, line.spaces);
        }
    }

    // Reinsert spaces into lines that need them
    auto& result = snip.elements();
    result.clear();
    size_t id = 0;
    for (auto& line : lines) {
        if (line.spaces == -1) {
            // Empty line, add it as such, unless it's the first line, then ignore it
            if (id > 0) {
                result.push_back(Snippet::make_text("\n"));
            }
        } else {
            result.push_back(Snippet::make_text(std::string(line.spaces - common_spaces, ' ')));
            for (auto& elem : line.elements) {
                result.push_back(elem);
            }
            result.push_back(Snippet::make_text("\n"));
        }
        id++;
    }

    // Remove last newline
    result.pop_back();
}

void enforce_tab_width(Snippet& snip, int tab_width) {
    remove_common_indentation(snip, tab_width);

    for (auto& element : snip.elements()) {
        if (!element.is_placeholder) {
            auto lines = split(element.text, '\n');
            auto& result = element.text;
            result = "";
            for (auto& line : lines) {
                size_t spaces = 0;
                while (spaces < line.size() && line[spaces] == ' ') {
                    spaces++;
                }

                size_t tabs = spaces / tab_width;
                result += std::string(tabs, '\t');
                result += line.substr(tabs * tab_width);
                result += '\n';
            }

            result.pop_back(); // remove last newline
        }
    }
}

int main(int argc, char* argv[]) {
    std::string target = DefaultTarget;
    std::string tab_width = std::to_string(DefaultTabWidth);
    std::vector<std::string> input_files;
    std::pair<std::string*, std::string> supported_options[] = {
        {&target, "--target="},
        {&tab_width, "--tab-width="},
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

    std::vector<Snippet> snippets;
    for (const auto& file : input_files) {
        snippets.push_back(Snippet::from_file(file));
    }

    for (auto& snip : snippets) {
        enforce_tab_width(snip, stoi(tab_width));
    }

    if (target == "vscode") {
        std::cout << export_vscode(snippets);
    } else {
        std::cout << "Error, unknown target " << target << '\n';
        return 1;
    }
}
