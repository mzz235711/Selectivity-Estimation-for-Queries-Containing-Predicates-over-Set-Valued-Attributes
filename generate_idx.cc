#include <vector>
#include <string>
#include <fstream>
#include <cctype>
#include <unordered_map>
#include <ctime>
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <limits>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>

#include "utils.h"


using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

bool comp(std::pair<std::string,int> &a, std::pair<std::string,int> &b) {
    return a.second > b.second;
}

/* Generate indices and frequencies from the set-valued column */
int main(int argc, char** argv) {
  std::ifstream infile("dataset/geotweet/geotweet_tags.csv");
  std::string s;
  getline(infile, s);
  char delimiter = ',';
  std::unordered_map<std::string, int> all_keywords;
  std::vector<std::vector<std::string>> texts;
  int tmpidx = 0;
  while (getline(infile, s)) {
    //size_t pos = s.find(delimiter);
    //auto t = pos + delimiter.length();
    //size_t pos2 = s.find(delimiter, t);
    //std::string text = s.substr(pos + 1, pos2 - pos - 1);
    std::string text = s;
    if (s[0] == '"') {
      text = text.substr(1, text.size() - 2);
    }
    std::string keyword;
    //for(unsigned i = 0; i < text.size(); i++) {
    //  text[i] = std::tolower(text[i]);
    //}
    std::stringstream ss(text);
    std::vector<std::string> new_text;
    while (std::getline(ss, keyword, delimiter)) {
      if (std::find(new_text.begin(), new_text.end(), keyword) == new_text.end()) {
        if (all_keywords.find(keyword) == all_keywords.end()){
          all_keywords.emplace(keyword, 1);
        } else {
          all_keywords[keyword] += 1;
	      }
        new_text.push_back(keyword);
      }
    }
    texts.push_back(new_text);
  }
  printf("file length %lu\n", texts.size());
  auto start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  int vnum = all_keywords.size();
  std::cout << "origin_vnum: " << vnum << "\n";
  int lb = 10;
  std::vector<std::pair<std::string, int>> set_freq;
  for (auto &k_v : all_keywords) {
    if (k_v.second >= lb) {
      set_freq.push_back(k_v);
    }
  }
  std::cout << "current_vnum: " << set_freq.size() << "\n";
  std::sort(set_freq.begin(), set_freq.end(), comp);
  std::ofstream idxfile("idx.csv");
  std::ofstream freqfile("frequency.csv");
  int idx = 0;
  for (auto &k_v : set_freq) {
    auto &k = k_v.first;
    auto &v = k_v.second;
    idxfile << k << "\t" << idx << "\n";
    freqfile << k << "\t" << v << "\n";
    idx++;
  }
  auto end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  std::cout << "Idx time: " << end_time - start_time << "\n"; 
  idxfile.close();
  freqfile.close() ;
  return 0;
}
