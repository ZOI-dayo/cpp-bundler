#include <string>
#include <set>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cassert>
#include <deque>
#include <unistd.h>

using namespace std;
namespace fs = filesystem;
using Path = fs::path;

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
  throw runtime_error(format("{}の処理中、{}というヘッダが見つかりませんでした。", current_file.string(), include_file.string()));
}

void read_file(Path file, const deque<Path>& include_path, set<Path>& skip_target) {
  ifstream src_file_reader(file);
  if (!src_file_reader) {
    throw runtime_error("入力ファイルが見つかりませんでした。");
  }

  int num = 1;
  while (!src_file_reader.eof()) {
    string s;
    getline(src_file_reader, s);
    const string include_token = "#include \"";
    if(s.starts_with(include_token)) {
      const Path include_file = lookup_header(s.substr(include_token.size(), s.size()-(include_token.size()+1)), file, include_path);
      if(skip_target.count(include_file)) {
        cout << "\n";
      } else {
        cout << format("#line 1 \"{}\"\n", include_file.string());
        read_file(include_file, include_path, skip_target);
        cout << format("#line {} \"{}\"\n", num+1, file.string());
      }
    } else {
      if(s.starts_with("#pragma once")) {
        skip_target.insert(file);
        cout << "\n";
      } else {
        cout << s << "\n";
      }
    }
    num++;
  }
  cout << flush;
}

int main(int argc, char** argv) {
  optional<Path> lib_path;

  {
    int opt;
    while((opt = getopt(argc, argv, "I:")) != -1) {
      switch(opt) {
        case 'I':
          lib_path = optarg;
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
  read_file(src_file, include_path, skip_target);
}
