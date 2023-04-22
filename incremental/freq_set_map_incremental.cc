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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <queue>
#include <chrono>
#include <sys/time.h>
#include <unistd.h>


#include "/home_nfs/peizhi/zizhong/set_selectivity/utils.h"
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;


std::string pad = "ZZZ";

bool comp(std::pair<std::string,int> &a, std::pair<std::string,int> &b) {
    return a.second > b.second;
}

int binary_search(std::vector<int> &arr, int l, int r, int &x) {
  if (r >= l) {
    int mid = l + (r - l) / 2;
    if (arr[mid] == x) {
      return mid;
    } else if (arr[mid] > x) {
      return binary_search(arr, l, mid - 1, x);
    } else {
      return binary_search(arr, mid + 1, r, x);
    }
  }
  return l;
}

void settrie_insert(TreeNode &root, std::vector<int> &set_key, int freq) {
  auto *curr_node = &root;
  for (auto &element : set_key) {
    curr_node->total_freq += freq;
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
  curr_node->freq += freq;
  curr_node->total_freq += freq;
}

void dfs_addvalue(TreeNode &curr_node, Setvector &set_vector, int &set_value, std::vector<int> &curr_set) {
  if (curr_node.last_flag == true) {
    //if (set_map.find(curr_set) != set_map.end()) {
    //  auto set_value = set_map[curr_set];
    //results.push_back(curr_node.last_flag);
    //}
    curr_node.range.first = set_value;
    set_value++;
    set_vector.push_back(curr_set);
  }
  for (auto &next_node : curr_node.children) {
    //auto next_set = curr_set;
    //next_set.push_back(next_node.id);
    auto next_set = curr_set;
    next_set.push_back(next_node.id);
    dfs_addvalue(next_node, set_vector, set_value, next_set);
  }
  curr_node.range.second = set_value;
}



void dfs(TreeNode &curr_node, std::vector<int> &results, std::vector<int> &curr_set, Vectormap &set_map) {
  if (curr_node.last_flag > -1) {
    //if (set_map.find(curr_set) != set_map.end()) {
    //  auto set_value = set_map[curr_set];
    results.push_back(curr_node.last_flag);
    //}
  }
  for (auto &next_node : curr_node.children) {
    auto next_set = curr_set;
    next_set.push_back(next_node.id);
    dfs(next_node, results, next_set, set_map);
  }
}





void dfs_getfreq(TreeNode &curr_node, std::vector<int> &freq, int level, std::unordered_map<int, int> &level_state) {
  curr_node.level = level;
  if (level_state.find(level) != level_state.end()) {
    level_state[level]++;
  } else {
    level_state[level] = 1;
  }
  if (curr_node.level > 1) {
    freq.push_back(curr_node.total_freq);
  }
  for (auto &next_node : curr_node.children) {
    dfs_getfreq(next_node, freq, level + 1, level_state);
  }
}


// For low frequency nodes, set the corresponding setid as their "root" nodes; Record the node frequency on their "root" nodes.
void add_percentage(TreeNode &curr_node, std::unordered_map<int, double> &percentage, Vectormap &setmap, std::vector<int> &curr_set, int &set_value) {
  // Reset range as (-1, -1)
  curr_node.range = std::make_pair(-1, -1);
  // Also add the histogram frequency if curr_node has
  for (auto &k_v : curr_node.percentage) {
    auto &k = k_v.first;
    auto &v = k_v.second;
    curr_node.percentage[k] = v * curr_node.hist_freq;
    if (percentage.find(k) != percentage.end()) {
      percentage[k] += curr_node.percentage[k];
    } else {
      percentage[k] = curr_node.percentage[k];
    }
  }
  // Add frequency of current and sub nodes.
  if (percentage.find(curr_node.id) != percentage.end()) {
    percentage[curr_node.id] += curr_node.total_freq;
  } else {
    percentage[curr_node.id] = curr_node.total_freq;
  }
  if (curr_node.last_flag == true) {
    setmap[curr_set] = set_value;
  }
  for (auto &next_node : curr_node.children) {
    auto new_set = curr_set;
    new_set.push_back(next_node.id);
    add_percentage(next_node, percentage, setmap, new_set, set_value);
  }

}	


// For high frequency nodes, set value with dfs. For edge frequency nodes, calculate subtree frequency.
void filter_nodes(TreeNode &curr_node, int &min_freq, int &set_value, Vectormap &setmap, std::vector<int> &curr_set,
	         	int &node_num, std::vector<std::pair<int, int>> &lenth, int &pad_id) {
  curr_node.range.first = set_value;
  node_num++;
  if (curr_node.last_flag == true) {
    setmap[curr_set] = set_value;
    set_value++;
  }

  // Destruct curr_node.percentage to elements' frequency.
  for (auto &k_v : curr_node.percentage) {
    auto &k = k_v.first;
    auto &v = k_v.second;
    curr_node.percentage[k] = v * curr_node.hist_freq;
  }
  //curr_node.hist_freq = 0;
  for (auto &next_node : curr_node.children) {
    auto new_set = curr_set;
    new_set.push_back(next_node.id);
    if (next_node.total_freq <= min_freq && curr_node.level > 0) {
      curr_node.hist_freq += next_node.total_freq;
      add_percentage(next_node, curr_node.percentage, setmap, new_set, set_value);
    }
  }
  if (curr_node.percentage.size() > 0) {
    curr_node.hist_val = set_value;
    set_value++;
  }
  for (auto &next_node : curr_node.children) {
    if (next_node.total_freq > min_freq  || curr_node.level == 0) {
      //set_value++;
      auto new_set = curr_set;
      new_set.push_back(next_node.id);
      filter_nodes(next_node, min_freq, set_value, setmap, new_set, node_num, lenth, pad_id);
    }
  }
  curr_node.range.second = set_value;
  int one_num = 0;
  std::vector<int> to_erase;
  for (auto &k_v : curr_node.percentage) {
    auto &key = k_v.first;
    auto &value = k_v.second;
    curr_node.percentage[key] = value / curr_node.hist_freq;
    if (curr_node.percentage[key] > 1 || curr_node.percentage[key] < 0) {
      std::cout << "ERROR: " << value << " " << curr_node.hist_freq << " " << curr_node.percentage[key] << "\n";
      exit(0);
    }
    //if (curr_node.percentage[key] > 0.999) {
    //  one_num++;
    //  curr_node.full_nodes.push_back(key);
    //  to_erase.push_back(key);
    //} else if (key != pad_id) {
      curr_node.all_null *= (1 - curr_node.percentage[key]);
    //}
  }
  //for (auto &key : to_erase) {
  //  curr_node.percentage.erase(key);
  //}
  if (curr_node.percentage.size() > 0) {
    lenth.push_back(std::make_pair(curr_node.percentage.size(), one_num));
  }
}


void serialize(TreeNode &curr_node, std::ofstream &file) {
  if (curr_node.range.first > -1 ) {
    file.write((char*)(&curr_node.level), sizeof(int));
    file.write((char*)(&curr_node.id), sizeof(int));
    file.write((char*)(&curr_node.last_flag), sizeof(bool));
    file.write((char*)(&curr_node.freq), sizeof(int));
    file.write((char*)(&curr_node.total_freq), sizeof(int));
    file.write((char*)(&curr_node.hist_freq), sizeof(int));
    file.write((char*)(&curr_node.range.first), sizeof(int));
    file.write((char*)(&curr_node.range.second), sizeof(int));
    file.write((char*)(&curr_node.hist_val), sizeof(int));
    size_t mapsize = curr_node.percentage.size();
    file.write((char*)(&mapsize), sizeof(size_t));
    for (auto &k_v : curr_node.percentage) {
      auto &key = k_v.first;
      auto &value = k_v.second;
      file.write((char*)(&key), sizeof(int));
      file.write((char*)(&value), sizeof(double));
    }
    size_t vectorsize = curr_node.full_nodes.size();
    file.write((char*)(&vectorsize), sizeof(size_t));
    for (auto &v : curr_node.full_nodes) {
      file.write((char*)(&v), sizeof(int));
    }
    file.write((char*)(&curr_node.all_null), sizeof(double));
  }
  for (auto &next_node : curr_node.children) {
    serialize(next_node, file);
  }
}

void deserialize_node(TreeNode* curr_node_pt, std::ifstream &file, int &level) {
  TreeNode new_node;
  new_node.level = level;
  file.read((char*)(&new_node.id), sizeof(int));
  file.read((char*)(&new_node.last_flag), sizeof(bool));
  file.read((char*)(&new_node.freq), sizeof(int));
  file.read((char*)(&new_node.total_freq), sizeof(int));
  file.read((char*)(&new_node.hist_freq), sizeof(int));
  file.read((char*)(&new_node.range.first), sizeof(int));
  file.read((char*)(&new_node.range.second), sizeof(int));
  file.read((char*)(&new_node.hist_val), sizeof(int));
  size_t hist_size;
  int key;
  double value;
  file.read((char*)(&hist_size), sizeof(size_t));
  for (int j = 0; j < hist_size; j++){
    file.read((char*)(&key), sizeof(int));
    file.read((char*)(&value), sizeof(double));
    new_node.percentage[key] = value;
  }
  size_t vec_size;
  int nodeid;
  file.read((char*)(&vec_size), sizeof(size_t));
  for (int j = 0; j < vec_size; j++) {
    file.read((char*)(&nodeid), sizeof(int));
    new_node.full_nodes.push_back(nodeid);
  }
  file.read((char*)(&new_node.all_null), sizeof(double));
  curr_node_pt->children.emplace_back(new_node);
}

void deserialize_root(TreeNode &root, std::ifstream &file) {
  file.read((char*)(&root.level), sizeof(int));
  file.read((char*)(&root.id), sizeof(int));
  file.read((char*)(&root.last_flag), sizeof(bool));
  file.read((char*)(&root.freq), sizeof(int));
  file.read((char*)(&root.total_freq), sizeof(int));
  file.read((char*)(&root.hist_freq), sizeof(int));
  file.read((char*)(&root.range.first), sizeof(int));
  file.read((char*)(&root.range.second), sizeof(int));
  file.read((char*)(&root.hist_val), sizeof(int));
  size_t hist_size;
  int key;
  double value;
  file.read((char*)(&hist_size), sizeof(size_t));
  std::cout << "hist_size: " << hist_size << "\n";
  for (int j = 0; j < hist_size; j++){
    file.read((char*)(&key), sizeof(int));
    file.read((char*)(&value), sizeof(double));
    root.percentage[key] = value;
  }
  size_t vec_size;
  int nodeid;
  file.read((char*)(&vec_size), sizeof(size_t));
  std::cout << "vec_size: " << vec_size << "\n";
  for (int j = 0; j < vec_size; j++) {
    file.read((char*)(&nodeid), sizeof(int));
    root.full_nodes.push_back(nodeid);
  }
  file.read((char*)(&root.all_null), sizeof(double));
}

void deserialize(TreeNode &root, std::ifstream &file, int &node_num) {
  auto curr_node_pt = &root;
  int curr_level = 0;
  std::cout << "node_num " << node_num << "\n";
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
  int keep_num = atoi(argv[2]);
  int currid = atoi(argv[3]);
  int nextid = atoi(argv[4]);
  std::string folder_name = "./partition_" + std::to_string(partition_num) + "_" + std::to_string(keep_num);
  std::ifstream inidxfile(folder_name + "/idx" + std::to_string(currid) + ".csv");
  std::vector<std::string> idx2word;
  std::unordered_map<std::string, int> word2idx;
  int index;
  std::string line;
  std::string word;
  while (getline(inidxfile, line)) {
    std::stringstream s(line);
    s >> word >> index;
    idx2word.push_back(word);
    word2idx.emplace(word, index);
  }
  int current_vnum = idx2word.size();
  printf("Number of keyword: %lu\n", idx2word.size());
  // frequency is just to filter elements. Dont need it in practical
  std::unordered_map<std::string, int> frequency;
  std::ifstream freqinfile(folder_name + "/frequency" + std::to_string(currid) + ".csv");
  int freq;
  std::string s;
  while (freqinfile >> s >> freq) {
    frequency[s] = freq;
  }

  std::unordered_map<std::string, int> new_keywords;
  std::vector<std::vector<std::string>> texts;
  std::ifstream infile("./geotweet_tags" + std::to_string(nextid) + ".csv");
  getline(infile, s);
  char delimiter = ',';
  int tmpidx = 0;
  while (getline(infile, s)) {
    std::string text = s;
    if (text[0] == '\"') {
      text = text.substr(1, s.size() - 2);
    }
    std::string keyword;
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

  /* Index of new elelments */
  int new_vnum = new_keywords.size();
  int lb = 10;
  std::vector<std::pair<std::string, int>> set_freq;
  for (auto &k_v : new_keywords) {
    if (frequency[k_v.first] >= lb) {
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

  std::string partfilename = folder_name +  "/part" + std::to_string(currid) + ".txt";
  std::ifstream partfile(partfilename);
  std::vector<int> partition(total_vnum);
  int partid;
  for (int i = 0; i < current_vnum; i++) {
    partfile >> partid;
    partition[i] = partid;
  }
  srand(time(NULL));
  /* Randomly allocate partid for new elements */
  for (int i = current_vnum; i < total_vnum; i++) {
    partid = rand() % partition_num;
    partition[i] = partid;
  }

  /*Load current settrie */
  std::cout << "cp1\n"; 
  std::vector<int> dis_num(partition_num, 0);
  std::vector<TreeNode> settrie(partition_num);
  std::string treefilename = folder_name + "/settrie" + std::to_string(currid) + ".txt";
  std::ifstream treefile(treefilename, std::ios::in|std::ios::binary);
  for (int i = 0; i < partition_num; i++) {
    int node_num;
    treefile.read((char*)(&node_num), sizeof(int));
    dis_num[i] = node_num;
    std::cout << node_num << "\n";
    deserialize(settrie[i], treefile, node_num);
  }
  std::cout << "Finish Loading\n"; 
  /* Adjust pad idx in the settrie */ 
  for (int i = 0; i < partition_num; i++) {
    auto &root = settrie[i];
    int pad_pos = root.children.size() - 1;
    if (root.children[pad_pos].id == current_vnum) {
      root.children[pad_pos].id = word2idx[pad];
    }
  } 
  std::cout << "Finish adjusting pad index\n";

  /* Get distinct new set */
  std::vector<Vectormap> set_map(partition_num, Vectormap());
  std::vector<std::vector<int>> set_frequency(partition_num, std::vector<int>());
  for (auto &idxs : text_idx) {
    std::vector<std::vector<int>> part_idx(partition_num, std::vector<int>());
    for (auto &idx : idxs) {
      partid = partition[idx];
      part_idx[partid].push_back(idx);
    }
    for (int i = 0; i < partition_num; i++) {
      if (part_idx[i].size() == 0) {
        part_idx[i].push_back(word2idx[pad]);
      }
      if (part_idx[i].size() != 0) {
        std::sort(part_idx[i].begin(), part_idx[i].end());
        if (set_map[i].find(part_idx[i]) == set_map[i].end()) {
          set_map[i][part_idx[i]] = set_map[i].size();
	        dis_num[i]++;
          set_frequency[i].push_back(1);
        } else {
          auto &setid = set_map[i][part_idx[i]];
          set_frequency[i][setid]++;
        }
      }
    }
  } 
  for (int i = 0; i < partition_num; i++) {
    std::cout << i << " " << dis_num[i] << "\n";
  }

  std::cout << "cp2\n";
  std::vector<Vectormap> setmap(partition_num, Vectormap()); 
  std::vector<int> node_num(partition_num, 0);
  std::vector<std::pair<int, int>> lenth;
  for (int i = 0; i < partition_num; i++) {
    std::cout << "start\n";
    //std::vector<int> pad_key = {word2idx[pad]};
    //set_map[i][pad_key] = set_map[i].size();
    //frequency[i].push_back(2e20);
    int sindex = 0;
    for (auto &s_v : set_map[i]) {
      sindex++;
      auto set_key = s_v.first;
      auto set_id = s_v.second;
      auto &freq = set_frequency[i][set_id];
      // First, insert set and frequency into the tree
      settrie_insert(settrie[i], set_key, freq);
    }
    std::vector<int> all_nodes_freq;
    // Second, calculate total_freq for all nodes
    std::cout << "cp2.1\n";
    std::unordered_map<int, int> level_state;
    dfs_getfreq(settrie[i], all_nodes_freq, 0, level_state);
    std::sort(all_nodes_freq.begin(), all_nodes_freq.end());
    int min_freq;
    if (keep_num == 1) {
      min_freq = 2e30;
    } else if (keep_num < all_nodes_freq.size()) {
      min_freq = all_nodes_freq[all_nodes_freq.size() - keep_num];
    } else {
      min_freq = 0;
    }
    int exist_num = 0;
    for (int i = all_nodes_freq.size() - 1; i >= 0; i--) {
      if (all_nodes_freq[i] == min_freq) {
        exist_num = keep_num - (all_nodes_freq.size() - i);
        break;
      }
    }
    std::cout << "Min freq: " << min_freq << "\n";
    std::cout << "root freq: " << all_nodes_freq[all_nodes_freq.size() - 1] << "\n";
    std::cout << "exist num: " << exist_num << "\n";
    for (int j = 0; j < 20; j++) {
      if (level_state.find(j) != level_state.end()) {
        std::cout << j << " " << level_state[j] << "\n";
      }
    }
    int init_value = 0;
    std::vector<int> curr_set;
    // Third, add set_value to high freq nodes; construct histogram for border node.
    std::cout << "cp2.2\n";
    filter_nodes(settrie[i], min_freq, init_value, setmap[i], curr_set, node_num[i], lenth, word2idx[pad]);
    std::cout << "nodes_num: " << node_num[i] << "\n";
    std::cout << "dis value num: " <<  init_value << "\n";
  }
  std::cout << "cp3\n"; 
  std::string output_filename = folder_name + "/dis_set_map" + std::to_string(nextid) + ".txt";
  std::ofstream output_file(output_filename);
  for (int i = 0; i < partition_num; i++) {
    output_file << i << " " << setmap[i].size() << "\n";
    for (auto &k_v : setmap[i]) {
      auto &dis_set = k_v.first;
      auto &dis_value = k_v.second;
      output_file << dis_set.size() << " ";
      for (auto idx : dis_set) {
        output_file << idx << " ";
      }
      output_file << dis_value << "\n";
    }
  }


  // Store the tree
  std::string output_treename = folder_name + "/settrie" + std::to_string(nextid) + ".txt";
  std::ofstream output_tree(output_treename, std::ios::out | std::ios::binary);
  for (int i = 0; i < partition_num; i++) {
    output_tree.write((char*)(&node_num[i]), sizeof(int));
    serialize(settrie[i], output_tree);
  }
  output_tree.close();
  //std::sort(lenth.begin(), lenth.end());
  std::ofstream state("state.txt");
  for (auto &l : lenth) {
    state << l.first << "\t" << l.second << "\n";
  }
  state.close();
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
  auto end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  std::cout << "Incremental settrie construction time: " << end_time - start_time  << "ms\n";
  return 0;
}
