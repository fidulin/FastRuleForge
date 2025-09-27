// FastRuleForge source code

#pragma once

#include "GPU_executor.hh"
#include "utils.hh"

#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <climits>


struct rule_a_distance{
  std::string rule = "";
  int distance = -1;
};

class rule_generator{
public:
  std::vector<char> lta_leet = {'4', '3', '1', '0', '7', '5', '$', '@', '8'};
  std::vector<char> lta_alpha = {'a', 'e', 'i', 'o', 't', 's', 's', 'a', 'b'};

  std::vector<std::string> rules;

  void generate_rules(std::vector<int> *cluster, std::vector<std::string> *passwords, std::vector<std::string> *resulted_rules, std::string &representative);

  //returns new distance after rule succsessfully applied and -1 if the rule is not applicable
  rule_a_distance apply_rule(std::string rule, std::string *representative, std::string *password, int &distance);

  std::string find_representative_levenshtein(std::vector<int> *cluster, std::vector<std::string> *all_passwords);

  std::string find_representative_levenshtein_big(std::vector<int> *cluster, GPU_executor *executor);

  std::string find_representative_substring(std::vector<int> *cluster, std::vector<std::string> *all_passwords);
};