#include <algorithm>
#include <cassert>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <unistd.h>

using namespace std;
namespace fs = filesystem;
using Path = fs::path;

// ヘッダファイルの絶対パスを探す
Path lookup_header(const Path include_file, const Path current_file, const deque<Path>& include_path) {
  assert(include_file.is_relative());
  assert(current_file.is_absolute());
  assert(all_of(include_path.begin(), include_path.end(), [](auto&& p) { return p.is_absolute(); }));
  auto check_exist = [&](Path p) {
    cerr << p / include_file << endl;
    if(fs::exists(p / include_file)) {
      return optional((p / include_file).lexically_normal());
    }
    return optional<Path>();
  };
  if(auto res = check_exist(current_file.parent_path()); res) return *res;
  for(auto&& p : include_path) {
    if(auto res = check_exist(p); res) return *res;
  }
  throw runtime_error(current_file.string() + "の処理中、" + include_file.string() + "というヘッダが見つかりませんでした。");
}

void read_file(Path file, const deque<Path>& include_path, set<Path>& skip_target, const set<string>& default_define_set) {
  ifstream src_file_reader(file);
  if (!src_file_reader) {
    throw runtime_error("入力ファイルが見つかりませんでした。");
  }

  int line = 1;

  const string include_token = "#include \"";
  const string define_token = "#define ";
  const string undef_token = "#undef ";
  const string if_token = "#if ";
  const string ifdef_token = "#ifdef ";
  const string ifndef_token = "#ifndef ";
  const string endif_token = "#endif";
  const string else_token = "#else";
  set<string> define_set(default_define_set);
  stack<bool> if_stack;

  while (!src_file_reader.eof()) {
    string s;
    getline(src_file_reader, s);

    if(s.starts_with(if_token)) {
      if_stack.push(true);
      cout << s << "\n";
      continue;
    } else if(s.starts_with(ifdef_token)) {
      if_stack.push(define_set.count(s.substr(ifdef_token.size())));
      cout << s << "\n";
      continue;
    } else if(s.starts_with(ifndef_token)) {
      if_stack.push(!define_set.count(s.substr(ifndef_token.size())));
      cout << s << "\n";
      continue;
    } else if(s.starts_with(endif_token)) {
      if(if_stack.empty()) {
        throw runtime_error(file.string() + "の処理中、" + endif_token + "が余分に存在します。");
      }
      if_stack.pop();
      cout << s << "\n";
      continue;
    } else if(s.starts_with(else_token)) {
      if(if_stack.empty()) {
        throw runtime_error(file.string() + "{}の処理中、" + else_token + "が余分に存在します。");
      }
      if_stack.top() = !if_stack.top();
      cout << s << "\n";
      continue;
    }

    if(!if_stack.empty() && !if_stack.top()) {
      cout << "\n";
      continue;
    }

    if(s.starts_with(include_token)) {
      const Path include_file = lookup_header(s.substr(include_token.size(), s.size()-(include_token.size()+1)), file, include_path);
      if(skip_target.count(include_file)) {
        cout << "\n";
      } else {
        cout << "#line 1 \"" << include_file.string() << "\"\n";
        read_file(include_file, include_path, skip_target, default_define_set);
        cout << "#line " << line + 1 << " \"" << file.string() << "\"\n";
      }
    } else if(s.starts_with(define_token)) {
      define_set.insert(s.substr(define_token.size()));
      cout << s << "\n";
    } else if(s.starts_with(undef_token)) {
      define_set.erase(s.substr(undef_token.size()));
      cout << s << "\n";
    } else {
      if(s.starts_with("#pragma once")) {
        skip_target.insert(file);
        cout << "\n";
      } else {
        cout << s << "\n";
      }
    }
    line++;
  }
  cout << flush;
}

int main(int argc, char** argv) {
  optional<Path> lib_path;
  set<string> default_define_set;

  {
    int opt;
    while((opt = getopt(argc, argv, "I:D:")) != -1) {
      switch(opt) {
        case 'I':
          lib_path = optarg;
          break;
        case 'D':
          default_define_set.insert(optarg);
          break;
        default:
          throw runtime_error("不明なオプションが指定されました。");
      }
    }
  }

  const deque<Path> include_path = lib_path ? deque<Path>{*lib_path} : deque<Path>();

  if(argc <= optind) {
    throw runtime_error("入力ファイルが指定されていません。");
  }
  // ファイル読み込み
  const Path src_file_name = argv[optind];
  const Path src_file = src_file_name.is_absolute() ? src_file_name : fs::current_path() / src_file_name;
  ifstream src_file_reader(src_file);
  if (!src_file_reader) {
    throw runtime_error("入力ファイルが見つかりませんでした。");
    return 1;
  }

  set<Path> skip_target;
  read_file(src_file, include_path, skip_target, default_define_set);
}
