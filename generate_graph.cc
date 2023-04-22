#include <vector>
#include <string>
#include <fstream>
#include <cctype>
#include <regex>
#include <unordered_map>
#include <ctime>
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <limits>
#include <sstream>

int main(int argc, char** argv) {
  std::ifstream infile("./dataset/geotweet/geotweet_tags.csv");
  std::string s;
  getline(infile, s);
  char delimiter = ',';
  std::regex e("[a-z]+");
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
  std::vector<std::string> idx2word;
  std::unordered_map<std::string, int> word2idx;
  int index = 0;
  std::ifstream idxinfile("idx.csv");
  while (idxinfile >> s >> index) {
    word2idx.emplace(s, index);
    idx2word.push_back(s);
    if (s == "lumpur") {
      std::cout << s << "\t" << index << "\t" << idx2word.size() << "\n";
    }
  }
  
  /*
  for (auto &w : all_keywords) {
    if (w.second >= 10) {
      idx2word.push_back(w.first);
      word2idx.emplace(w.first, index);
      index++;
    }
  }
  
  std::ofstream idxoutfile("idx.csv");
  for (int i = 0; i < idx2word.size(); i++) {
    idxoutfile << idx2word[i] << " " << i << "\n";
  }
  idxoutfile.close();
  */
  printf("Number of keyword: %lu\n", idx2word.size());
  std::vector<std::vector<int>> text_idx;
  tmpidx = 0;
  for (auto &text : texts) {
    std::vector<int> new_text;
    for (auto t: text) {
      if (word2idx.find(t) != word2idx.end()) {
        new_text.push_back(word2idx[t]);
      }
    }
    text_idx.push_back(new_text);
  }

  /***
  std::ifstream misfile("MIS_part.csv");
  std::vector<int> part(idx2word.size());
  for (int i = 0; i < idx2word.size(); i++) {
    misfile >> part[i];
  }
  */

  int vnum = idx2word.size();
  int ednum = 0;
  std::vector<std::vector<int>> graph(idx2word.size(), std::vector<int>());
  std::vector<std::vector<int>> edge_weight(idx2word.size(), std::vector<int>());
  for (size_t i = 0; i < idx2word.size(); i++) {
    graph[i] = std::vector<int>();
  }
  for (auto &text : text_idx) {
    for (size_t i = 0; i < text.size(); i++) {
      for (size_t j = i + 1; j < text.size(); j++) {
        auto &w1 = (text[i] > text[j]) ? text[i] : text[j];
	auto &w2 = (text[i] > text[j]) ? text[j] : text[i];
	auto it = std::find(graph[w1].begin(), graph[w1].end(), w2);
	if (it == graph[w1].end()) {
	  graph[w1].push_back(w2);
	  edge_weight[w1].push_back(1);
	  ednum++;
	  /*
	  if (part[w1] == part[w2]) {
	    std::cout << "Wrong MIS " << std::to_string(w1) << " " << std::to_string(w2) << "\n";
	    return 0;
	  }
	  */
	} else {
	  int index = it - graph[w1].begin();
	  edge_weight[w1][index]++;
	}
      }
    }
  }

  std::ofstream graphfile("geotweet.mtx");
  graphfile << "\%\%MatrixMarket matrix coordinate pattern symmetric\n";
  graphfile << std::to_string(vnum) << " " << std::to_string(vnum) << " " << std::to_string(ednum) << "\n";
  for (int i = 0; i < vnum; i++) {
    for (int j = 0; j < graph[i].size(); j++) {
      graphfile << i << " " << graph[i][j] << " " << edge_weight[i][j] << "\n";
    }
  }
  graphfile.close();
  return 0;
}


