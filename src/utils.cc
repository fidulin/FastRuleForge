// FastRuleForge source code

#include "utils.hh"

//help function for sorting
bool compare_r(std::pair<std::string, int>& a, std::pair<std::string, int>& b) {
  if (a.second > b.second) {
    return true;
  }
  if (a.second == b.second && a.first < b.first) {
    return true;
  }
  return false;
}

void narrow_down(std::vector<std::string>* resulted_rules, int N, std::vector<std::string> rules){
  std::unordered_map<std::string, int> rule_used;
  for(std::string rule : *resulted_rules){
    rule_used[rule]++;
  }

  //sort the dict
  std::vector<std::pair<std::string, int>> freq_rules(rule_used.begin(), rule_used.end());
  std::sort(freq_rules.begin(), freq_rules.end(), compare_r);

  if(N != -1 && N < freq_rules.size()){
    freq_rules.resize(N); //only the TOP N
  }

  resulted_rules->clear();
  for(std::pair<std::string, int> p : freq_rules) {
    resulted_rules->push_back(p.first);
  }

  //":" allways is there (if its in rules, user can chose for it not to be there)
  for(std::string rule : rules){
    if(rule == ":"){
      resulted_rules->pop_back();
      resulted_rules->push_back(":");
      break;
    }
  }
}

int output_rules(std::vector<std::string>* resulted_rules, std::string filename){
  std::ofstream output_file(filename);
  if (!output_file) {
    std::cerr << "Error opening output file: " << filename << std::endl;
    return -1;
  }
  for(std::string rule : *resulted_rules){
    output_file << rule << std::endl;
  }
  output_file.close();
  return 0;
}

int print_clusters(std::vector<std::vector<int>> clusters, int PASSWORDS_COUNT, std::vector<std::string> passwords, bool print_singles){
    for(int i=0; i<clusters.size(); i++){
      if(clusters[i].size() <= 1 && !print_singles){
        continue;
      }
      if(clusters[i].size() > 0) {
        std::cout << "[";
      }
  
      for(int e=0; e<clusters[i].size(); e++){
        std::cout << passwords[clusters[i][e]] << " ";
      }
  
      if(clusters[i].size() > 0) {
        std::cout << "]";
        std::cout << std::endl;
      }
    }
    return 0;
  }
  
void convert_clusters(int* result, std::vector<std::vector<int>>& clusters, int PASSWORDS_COUNT){
    clusters.resize(PASSWORDS_COUNT);

    for (int i = 0; i < PASSWORDS_COUNT; i++){
        if(result[i] >= 0){
            clusters[result[i]].push_back(i);
        }
    }
    return;
}

int levenshtein_distance(const std::string &str_x_string, const std::string &str_y_string)
{
  unsigned char len_x = str_x_string.length();
  unsigned char len_y = str_y_string.length();
  char *str_x = (char *)str_x_string.c_str();
  char *str_y = (char *)str_y_string.c_str();

  if(len_x > len_y){
    std::swap(str_x, str_y);
    std::swap(len_x, len_y);
  }

  unsigned char v0_array[len_x + 1];
  unsigned char v1_array[len_y + 1];
  unsigned char *v0 = v0_array;
  unsigned char *v1 = v1_array;

    for (unsigned char j = 0; j <= len_y; ++j) {
      v0[j] = j;
    }

    for (unsigned char i = 0; i < len_x; ++i) {
        v1[0] = i + 1;

        for (unsigned char j = 0; j < len_y; ++j) {
            unsigned char deletion_cost = v0[j + 1] + 1;
            unsigned char insertion_cost = v1[j] + 1;
            unsigned char substitution_cost = v0[j] + (str_x[i] != str_y[j]);

            v1[j + 1] = std::min(deletion_cost, std::min(insertion_cost, substitution_cost));
        }
        unsigned char *temp = v0;
        v0 = v1;
        v1 = temp;
    }

    return v0[len_y];
}