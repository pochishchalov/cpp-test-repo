#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

static regex QUOTES_INCLUDE_REG(R"/(\s*#\s*include\s*"([^ "]*)"\s*)/");
static regex ARROW_INCLUDE_REG(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories);
bool Preprocess(ifstream& in_f, ofstream& out_f, const path& in_file, const vector<path>& include_directories);

bool FindInIncludeDirectories(const string& include_name, ofstream& out_file, const vector<path>& include_directories);

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return { (istreambuf_iterator<char>(stream)), istreambuf_iterator<char>() };
}

string GetLine(istream& in) {
    string s;
    getline(in, s);
    return s;
}

void Test() {
    error_code err;

    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp"s);
        file << "// this comment before include\n"s
            "#include \"dir1/b.h\"\n"s
            "// text between b.h and c.h\n"s
            "#include \"dir1/d.h\"\n"s
            "\n"s
            "int SayHello() {\n"s
            "    cout << \"hello, world!\" << endl;\n"s
            "#   include<dummy.txt>\n"s
            "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h"s);
        file << "// text from b.h before include\n"s
            "#include \"subdir/c.h\"\n"s
            "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h"s);
        file << "// text from c.h before include\n"s
            "#include <std1.h>\n"s
            "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h"s);
        file << "// text from d.h before include\n"s
            "#include \"lib/std2.h\"\n"s
            "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h"s);
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h"s);
        file << "// std2\n"s;
    }


    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
        { "sources"_p / "include1"_p,"sources"_p / "include2"_p })));

    cout << GetFileContents("sources/a.in"s);

    ostringstream test_out;
    test_out << "// this comment before include\n"s
        "// text from b.h before include\n"s
        "// text from c.h before include\n"s
        "// std1\n"s
        "// text from c.h after include\n"s
        "// text from b.h after include\n"s
        "// text between b.h and c.h\n"s
        "// text from d.h before include\n"s
        "// std2\n"s
        "// text from d.h after include\n"s
        "\n"s
        "int SayHello() {\n"s
        "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    ifstream in_f(in_file);
    if (!in_f) {
        return false;
    }
    ofstream out_f(out_file, ios::app);
    if (!out_f) {
        return false;
    }
    if (!Preprocess(in_f, out_f, in_file, include_directories)) {
        return false;
    }
    return true;
}

bool Preprocess(ifstream& in_f, ofstream& out_f, const path& in_file, const vector<path>& include_directories) {
    size_t line_counter = 1;

    while (in_f.good()) {
        string buffer;
        getline(in_f, buffer);
        smatch m;

        if (regex_match(buffer, m, QUOTES_INCLUDE_REG)) {
            const string include_name = string(m[1]);
            path include_path = in_file.parent_path() / include_name;
            if (filesystem::exists(include_path)) {
                ifstream include_file(include_path);
                if (!Preprocess(include_file, out_f, include_path, include_directories)) {
                    return false;
                };
            }
            else {
                if (!FindInIncludeDirectories(include_name, out_f, include_directories)) {
                    cout << "unknown include file "s << include_name << " at file "s << in_file.string() << " at line " << line_counter << endl;
                    return false;
                };
            }
        }
        else if (regex_match(buffer, m, ARROW_INCLUDE_REG)) {
            const string include_name = string(m[1]);
            if (!FindInIncludeDirectories(include_name, out_f, include_directories)) {
                cout << "unknown include file "s << include_name << " at file "s << in_file.string() << " at line " << line_counter << endl;
                return false;
            };
        }
        else {
            out_f << buffer << endl;
        }
        ++line_counter;
    };

    return true;
}

bool FindInIncludeDirectories(const string& include_name, ofstream& out_file, const vector<path>& include_directories) {
    bool is_found = false;
    for (const auto& include_directory : include_directories) {
        path include_path = include_directory / include_name;
        if (filesystem::exists(include_path)) {
            is_found = true;
            ifstream include_file(include_path);
            if (!Preprocess(include_file, out_file, include_path, include_directories)) {
                return false;
            };
        }
    }
    return is_found;
}