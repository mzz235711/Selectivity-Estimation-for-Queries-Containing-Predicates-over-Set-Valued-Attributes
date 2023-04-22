#include <boost/functional/hash.hpp>
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
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "/home_nfs/peizhi/zizhong/set_selectivity/utils.h"

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;


bool comp(std::pair<std::string,int> &a, std::pair<std::string,int> &b) {
    return a.second > b.second;
}

int BinarySearch(std::vector<int> &arr, int l, int r, int &x) {
  if (r >= l) {
    int mid = l + (r - l) / 2;
    if (arr[mid] == x) {
      return 1;
    } else if (arr[mid] > x) {
      return BinarySearch(arr, l, mid - 1, x);
    } else {
      return BinarySearch(arr, mid + 1, r, x);
    }
  }
  return 0;
}

void settrie_insert(TreeNode &root, std::vector<int> &set_key) {
  auto *curr_node = &root;
  for (auto &element : set_key) {
    std::vector<TreeNode>::iterator it;
    for (it = curr_node->children.begin(); it != curr_node->children.end(); it++) {
      if (it->id == element) {
	auto &new_node = *it;
	curr_node = &new_node;
	break;
      } else if (it->id > element) {
        TreeNode new_node;
	new_node.id = element;
	int index = it - curr_node->children.begin();
	curr_node->children.insert(it, new_node);
	curr_node = &(curr_node->children[index]);
	break;
      }
    }
    if (it == curr_node->children.end()) {
      TreeNode new_node;
      new_node.id = element;
      curr_node->children.push_back(new_node);
      curr_node = &(curr_node->children[curr_node->children.size() - 1]);
    }
  }
  curr_node->last_flag = true;
}

void dfs_addvalue(TreeNode &curr_node, int &set_value, Vectormap &setmap, std::vector<int> &curr_set, int &node_num, int level) {
  curr_node.range.first = set_value;
  curr_node.level = level;
  node_num++;
  if (curr_node.last_flag == true) {
    //if (set_map.find(curr_set) != set_map.end()) {
    //  auto set_value = set_map[curr_set];
    //results.push_back(curr_node.last_flag);
    //}
    setmap[curr_set] = set_value;
    set_value++;
  }
  for (auto &next_node : curr_node.children) {
    //auto next_set = curr_set;
    //next_set.push_back(next_node.id);
    auto next_set = curr_set;
    next_set.push_back(next_node.id);
    dfs_addvalue(next_node, set_value, setmap, next_set, node_num, level + 1);
  }
  curr_node.range.second = set_value;
}

void serialize(TreeNode &curr_node, std::ofstream &file) {
  file.write((char*)(&curr_node.level), sizeof(int));
  file.write((char*)(&curr_node.id), sizeof(int));
  file.write((char*)(&curr_node.last_flag), sizeof(bool));
  file.write((char*)(&curr_node.range.first), sizeof(int));
  file.write((char*)(&curr_node.range.second), sizeof(int));
  for (auto &next_node : curr_node.children) {
    serialize(next_node, file);
  }
}

void deserialize_node(TreeNode* curr_node_pt, std::ifstream &file, int &level) {
  TreeNode new_node;
  new_node.level = level;
  file.read((char*)(&new_node.id), sizeof(int));
  file.read((char*)(&new_node.last_flag), sizeof(bool));
  file.read((char*)(&new_node.range.first), sizeof(int));
  file.read((char*)(&new_node.range.second), sizeof(int));
  curr_node_pt->children.emplace_back(new_node);
}

void deserialize_root(TreeNode &root, std::ifstream &file) {
  file.read((char*)(&root.level), sizeof(int));
  file.read((char*)(&root.id), sizeof(int));
  file.read((char*)(&root.last_flag), sizeof(bool));
  file.read((char*)(&root.range.first), sizeof(int));
  file.read((char*)(&root.range.second), sizeof(int));
}

void deserialize(TreeNode &root, std::ifstream &file, int &node_num) {
  auto curr_node_pt = &root;
  int curr_level = 0;
  deserialize_root(root, file);
  for (int i = 0; i < node_num - 1; i++) {
    int level;
    file.read((char*)(&level), sizeof(int));
    if (level == curr_level + 1) {
      deserialize_node(curr_node_pt, file, level);
      curr_level++;
      curr_node_pt = &(curr_node_pt->children[curr_node_pt->children.size() - 1]);
    } else {
      curr_node_pt = &root;
      for (curr_level = 0; curr_level < level - 1; curr_level++) {
        curr_node_pt = &(curr_node_pt->children[curr_node_pt->children.size() - 1]);
      }
      deserialize_node(curr_node_pt, file, level);
    }
  }
}




int main(int argc, char** argv) {
  auto start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  int partition_num = atoi(argv[1]);
  int currid = atoi(argv[2]);
  int nextid = atoi(argv[3]);
  std::string folder_name = "color_partition_" + std::to_string(partition_num);
  /* Load existing element index */
  std::vector<std::string> idx2word;
  std::unordered_map<std::string, int> word2idx;
  int index = 0;
  std::string s;
  std::ifstream idxinfile(folder_name + "/idx" + std::to_string(currid) + ".csv");
  while (idxinfile >> s >> index) {
    word2idx.emplace(s, index);
    idx2word.push_back(s);
    if (s == "abcxyz") {
      std::cout << s << "\t" << index << "\t" << idx2word.size() << "\n";
    }
  }
  int current_vnum = idx2word.size();
  printf("Number of keyword: %lu\n", idx2word.size());

  /* Load existing element frequency */
  std::unordered_map<std::string, int> frequency;
  std::ifstream freqinfile(folder_name + "/frequency" + std::to_string(currid) + ".csv");
  int freq;
  while (freqinfile >> s >> freq) {
    frequency[s] = freq;
  }

  /* Load increamental data */
  std::ifstream infile("./geotweet_tags" + std::to_string(nextid) + ".csv");
  getline(infile, s);
  char delimiter = ',';
  std::regex e("[a-z]+");
  std::unordered_map<std::string, int> new_keywords;
  std::vector<std::vector<std::string>> texts;
  std::string pad = "ZZZ";
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
        if (word2idx.find(keyword) == word2idx.end()) {
          if (new_keywords.find(keyword) == new_keywords.end()){
            new_keywords.emplace(keyword, 1);
          } else {
            new_keywords[keyword] += 1;
	        }
        }
        if (frequency.find(keyword) == frequency.end()) {
          frequency[keyword] = 1;
        } else {
          frequency[keyword] += 1;
        }
        new_text.push_back(keyword);
      }
    }
    texts.push_back(new_text);
  }
  printf("file length %lu\n", texts.size());

  /* Index of new elements */
  int new_vnum = new_keywords.size();
  int lb = 10;
  std::vector<std::pair<std::string, int>> set_freq;
  for (auto &k_v : new_keywords) {
    if (frequency[k_v.first] >= 10) {
      set_freq.push_back(k_v);
    }
  }
  std::sort(set_freq.begin(), set_freq.end(), comp);
  for (auto &k_v : set_freq) {
    auto &k = k_v.first;
    word2idx[k] = word2idx.size();
    idx2word.push_back(k);
  }
  int total_vnum = idx2word.size();
  word2idx[pad] = idx2word.size();
  idx2word.push_back(pad);
  
  /* Transfer incremental data */
  std::vector<std::vector<int>> text_idx;
  for (auto &text : texts) {
    std::vector<int> new_text;
    for (auto t: text) {
      if (word2idx.find(t) != word2idx.end()) {
        new_text.push_back(word2idx[t]);
      }
    }
    std::sort(new_text.begin(), new_text.end());
    text_idx.push_back(new_text);
  }


  //std::string folder_name = "./graph_color";
  //std::string partfilename = folder_name + "/approx_clique_part_" + std::to_string(partition_num) + ".txt";
  std::string partfilename = folder_name + "/part" + std::to_string(currid) + ".txt";
  std::ifstream partfile(partfilename);
  std::vector<int> partition(total_vnum);
  int partid;
  int maxpartid = 0;
  for (int i = 0; i < current_vnum; i++) {
    partfile >> partid;
    partition[i] = partid;
    if (partid > maxpartid) {
      maxpartid = partid;
    }
  }
  srand(time(NULL));
  /* Randomly allocate partid for new elements */
  for (int i = current_vnum; i < total_vnum; i++) {
    partid = rand() % partition_num;
    partition[i] = partid;
  }

  /* Load current settrie */
  int u, v;
  int set_size;
  std::vector<int> max_length(partition_num, 0);
  std::vector<TreeNode> settrie(partition_num);
  std::string treefilename = folder_name + "/settrie" + std::to_string(currid) + ".txt";
  std::ifstream treefile(treefilename, std::ios::in|std::ios::binary);
  std::vector<int> dis_num(partition_num, 0);
  for (int i = 0; i < partition_num; i++) {
    int node_num;
    treefile.read((char*)(&node_num), sizeof(int));
    dis_num[i] = node_num;
    std::cout << node_num << "\n";
    deserialize(settrie[i], treefile, node_num);
  }
  
  /* Get distinct new set */
  std::cout << "cp1\n"; 
  std::vector<Vectormap> setmap(partition_num, Vectormap());
  for (auto &idxs : text_idx) {
    std::vector<std::vector<int>> part_idx(partition_num, std::vector<int>());
    for (auto &idx : idxs) {
      partid = partition[idx];
      part_idx[partid].push_back(idx);
    }
    for (int i = 0; i < partition_num; i++) {
      if (part_idx[i].size() != 0) {
        std::sort(part_idx[i].begin(), part_idx[i].end());
        if (setmap[i].find(part_idx[i]) == setmap[i].end()) {
          setmap[i][part_idx[i]] = dis_num[i];
	        dis_num[i]++;
        }
      }
    }
  } 

  /* Adjust pad idx in the settrie */
  for (auto &root : settrie) {
    int pad_pos = root.children.size() - 1;
    if (root.children[pad_pos].id == current_vnum) {
      root.children[pad_pos].id = word2idx[pad];
    }
  }
  
  /* Add new set into the settrie and re-calculte the set value */
  std::vector<Vectormap> set_map(partition_num, Vectormap());
  std::string output_treename = folder_name + "/settrie" + std::to_string(nextid) + ".txt";
  std::ofstream output_tree(output_treename, std::ios::out | std::ios::binary);
  for (int i = 0; i < partition_num; i++) {
    int node_num = 0;
    for (auto &k_v : setmap[i]) {
      auto set_key = k_v.first;
      settrie_insert(settrie[i], set_key);
    }
    int init_value = 0;
    std::vector<int> curr_set;
    dfs_addvalue(settrie[i], init_value, set_map[i], curr_set, node_num, 0);
    output_tree.write((char*)(&node_num), sizeof(int));
    std::cout << "node num " << i << " " << node_num << "\n";
    serialize(settrie[i], output_tree);
  }
  output_tree.close();
  std::cout << "cp2\n"; 
  std::string output_filename = folder_name + "/dis_set_map" + std::to_string(nextid) + ".txt";
  std::ofstream output_file(output_filename);
  for (int i = 0; i < partition_num; i++) {
    output_file << i << " " << set_map[i].size() << "\n";
    for (auto &k_v : set_map[i]) {
      auto &dis_set = k_v.first;
      auto dis_value = k_v.second;
      output_file << dis_set.size() << " ";
      for (auto idx : dis_set) {
        output_file << idx << " ";
      }
      output_file << dis_value << "\n";
    }
  }

  /* Write down new updated index, frequency and partition ids */
  std::ofstream outidxfile(folder_name + "/idx" + std::to_string(nextid) + ".csv");
  std::ofstream outfreqfile(folder_name + "/frequency" + std::to_string(nextid) + ".csv");
  std::ofstream outpartfile(folder_name + "/part" + std::to_string(nextid) + ".txt");
  // excluding pad index
  for (int i = 0; i < idx2word.size() - 1; i++) {
    outidxfile << idx2word[i] << "\t" << i << "\n";
  }
  for (auto &k_v : frequency) {
    outfreqfile << k_v.first << "\t" << k_v.second << "\n";
  }
  for (int i = 0; i < partition.size() - 1; i++) {
    outpartfile << partition[i] << "\n";
  }
  outidxfile.close();
  outfreqfile.close();
  outpartfile.close();
  auto end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  std::cout << "Incremental settrie construction time: " << end_time - start_time  << "ms\n";
  return 0;
}
