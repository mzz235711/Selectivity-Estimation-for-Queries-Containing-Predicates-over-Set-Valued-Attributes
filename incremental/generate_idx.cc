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

#include "/home_nfs/peizhi/zizhong/set_selectivity/utils.h"


using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

bool comp(std::pair<std::string,int> &a, std::pair<std::string,int> &b) {
    return a.second > b.second;
}

int main(int argc, char** argv) {
  auto start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  int partnum = atoi(argv[1]);
  int fragnum = atoi(argv[2]);
  std::unordered_map<std::string, int> all_keywords;
  std::vector<std::vector<std::string>> texts;
  std::string s;
  for (int i = 0; i < fragnum; i++) {
    std::ifstream infile("./geotweet_tags" + std::to_string(i) + ".csv");
    getline(infile, s);
    char delimiter = ',';
    int tmpidx = 0;
    while (getline(infile, s)) {
      std::string text = s;
      if (s[0] == '"') {
        text = text.substr(1, text.size() - 2);
      }
      std::string keyword;
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
    infile.close();
  }
  int vnum = all_keywords.size();
  int lb = 10;
  std::vector<std::pair<std::string, int>> set_freq;
  for (auto &k_v : all_keywords) {
    //if (k_v.second >= lb) {
      set_freq.push_back(k_v);
    //}
  }
  std::sort(set_freq.begin(), set_freq.end(), comp);
  std::ifstream oldidxfile("../idx.csv");
  std::ifstream oldpartfile("../graph_color/part.txt");
  std::ifstream oldpartxfile("../color_partition_" + std::to_string(partnum) + "/part.txt");
  std::string idxfilename = "idx";
  std::string freqfilename = "frequency";
  std::string newpartfilename = "graph_color/part";
  std::string newpartxfilename = "color_partition_" + std::to_string(partnum) + "/part";
  for (int i = 0; i < fragnum; i++) {
    idxfilename += std::to_string(i);
    freqfilename += std::to_string(i);
    newpartfilename += std::to_string(i);
    newpartxfilename += std::to_string(i);
  }
  idxfilename += ".csv";
  freqfilename += ".csv";
  newpartfilename += ".txt";
  newpartxfilename += ".txt";
  std::ofstream idxfile(idxfilename);
  std::ofstream freqfile(freqfilename);
  std::ofstream newpartfile(newpartfilename);
  std::ofstream newpartxfile(newpartxfilename);
  std::unordered_map<std::string, int> oldword2part;
  std::unordered_map<std::string, int> oldword2partx;
  int idx = 0, pid = 0;
  while (oldidxfile >> s >> idx) {
    oldpartfile >> pid;
    oldword2part[s] = pid;
    oldpartxfile >> pid;
    oldword2partx[s] = pid;
  }
  idx = 0;
  for (auto &k_v : set_freq) {
    auto &k = k_v.first;
    auto &v = k_v.second;
    if (v >= lb) {
      idxfile << k << "\t" << idx << "\n";
      newpartfile << oldword2part[k] << "\n";
      newpartxfile << oldword2partx[k] << "\n";
      idx++;
    }
    freqfile << k << "\t" << v << "\n";
  }
  idxfile.close();
  freqfile.close() ;
  auto end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  std::cout << "Idx time: " << end_time - start_time << "\n"; 

  return 0;
}