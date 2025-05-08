#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

// Include grammar: #include <filename>
const std::string matching = "#include";


std::string readAll(std::string name) {
    std::cout << "Trying reading - " << name << "\n";
    std::ifstream in{name};
    assert(in);

    std::stringstream ss;
    ss << in.rdbuf();

    return ss.str();
}

std::string_view fetchIncludeFilename(std::string_view include_view) {
    assert(include_view.find(matching) == 0);

    auto filename_begin = include_view.find("<");
    assert(filename_begin != std::string_view::npos && "invalid #include syntax");
    ++filename_begin; // shifting to filename begin

    auto filename_end = include_view.find(">");
    assert(filename_begin != std::string_view::npos && "invalid #include syntax");
    assert(filename_end == include_view.size() - 1);

    size_t filename_size = filename_end - filename_begin;
    std::string_view filename{include_view.data() + filename_begin, filename_size};

    return filename;
}

std::string::size_type 
processInclude(std::string& str, std::string::size_type include_begin_pos) {
    auto include_end = str.find(">", include_begin_pos);
    assert(include_end != std::string::npos && "invalid #include syntax");
    ++include_end;

    std::string_view include_view{str.data() + include_begin_pos, include_end};
    std::string_view filename_view = fetchIncludeFilename(include_view);
    std::string filename{filename_view};

    std::string file_input = readAll(filename);

    str.erase(include_begin_pos, include_end - include_begin_pos);
    str.insert(include_begin_pos, file_input);

    return include_begin_pos + file_input.size();
}


int main(const int argc, const char* argv[]) {
    assert(argc > 0);
    if (argc < 3) {
        printf("Usage: %s <input code file> <output code file>", argv[0]);
        return EXIT_FAILURE;
    }
    
    auto input = readAll(argv[1]);
    auto output = input;

    auto curr_searching_begin = 0;
    while (true) {
        auto pos = output.find(matching, curr_searching_begin);
        if (pos == std::string::npos) {
            break;
        }

        curr_searching_begin = processInclude(output, pos);
    }

    std::ofstream out{argv[2]};
    assert(out);

    out << output;
}