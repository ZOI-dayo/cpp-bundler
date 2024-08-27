#include <string>
#include <list>
#include <set>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cassert>
#include <deque>

using namespace std;
namespace fs = filesystem;
using Path = fs::path;

// ../や./を解決する
string resolve_path(string path) {
  vector<string> dirs;
  string dir;
  for(auto c : path) {
    if(c == '/') {
      if(dir == "..") {
        if(!dirs.empty()) dirs.pop_back();
      } else if(dir != ".") {
        dirs.push_back(dir);
      }
      dir = "";
    } else {
      dir += c;
    }
  }
  if(dir == "..") {
    if(!dirs.empty()) dirs.pop_back();
  } else if(dir != ".") {
    dirs.push_back(dir);
  }
  string res;
  for(auto d : dirs) {
    res += d + "/";
  }
  res.pop_back();
  return res;
}

Path lookup_header(Path include_file, Path current_file, deque<Path> include_path) {
  assert(include_file.is_relative());
  assert(current_file.is_absolute());
  assert(all_of(include_path.begin(), include_path.end(), [](auto&& p) { return p.is_absolute(); }));
  include_path.push_front(current_file.parent_path());
  for(auto&& p : include_path) {
    if(fs::exists(p / include_file)) {
      return resolve_path(p / include_file);
    }
  }
  throw runtime_error(include_file.string() + "というヘッダは見つかりませんでした。");
}


int main(int argc, char** argv) {
  assert(argc == 2);

  const Path lib_path = "/home/zoi/ghq/github.com/ZOI-dayo/atcoder-library/";
  const deque<Path> include_path = {lib_path};
  // ファイル読み込み
  const Path src_file = argv[1];
  ifstream src_file_reader(src_file);
  if (!src_file_reader) {
    throw runtime_error("入力ファイルが見つかりませんでした。");
    return 1;
  }

  // 処理状態(1行ごとに保持)
  struct FileLine {
    // 1行の文字列
    string line;
    // この行が含まれるファイルのパス
    Path file;
    int line_number;
    
    FileLine(string line, Path file, int line_number) : line(line), file(file), line_number(line_number) {
      if(!file.is_absolute()) {
        throw runtime_error(file.string() + "は絶対パスである必要があります。");
      }
      if(!file.has_filename()) {
        throw runtime_error(file.string() + "はファイルである必要があります。");
      }
      assert(line.find('\n') == string::npos);
    }
  };

  // 改行で区切ってキューに入れる
  list<FileLine> lines;
  {
    int num = 1;
    while (!src_file_reader.eof()) {
      string s;
      getline(src_file_reader, s);
      lines.emplace_back(s, src_file, num);
      num++;
    }
  }

  // 書き込み
  set<Path> skip_target;
  while (!lines.empty()) {
    auto [line, file, num] = lines.front(); lines.pop_front();
    // もし#include "から始まっていれば、そのファイルを読み込んでqueueに追加し、continue
    const string include_token = "#include \"";
    // 新規ファイルの場合、インクルードし、その行の出力はしない
    if(line.starts_with(include_token)) {
      Path include_file = lookup_header(line.substr(include_token.size(), line.size()-(include_token.size()+1)), file, include_path);
      if(skip_target.count(include_file)) continue;
      ifstream header_reader(include_file);
      if (!header_reader) {
        throw runtime_error(include_file.string() + "というヘッダを読み込むことができませんでした。権限が不足している可能性があります。");
      }
      list<FileLine> tmp;
      {
        int num = 1;
        while (!header_reader.eof()) {
          string l;
          getline(header_reader, l);
          tmp.emplace_back(l, include_file, num);
        }
      }
      tmp.emplace_back("#line " + to_string(num) + " \"" + file.filename().string() + "\"", file, num+1);
      lines.splice(lines.begin(), tmp);
      cout << "#line 1 \"" << include_file << "\"" << endl;
      continue;
    }
    // pragma onceの場合、記録をして空行を出力
    if(line.starts_with("#pragma once")) {
      skip_target.insert(file);
      cout << endl;
      continue;
    }
    cout << line << endl;
  }
}
