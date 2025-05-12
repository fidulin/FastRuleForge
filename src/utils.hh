// FastRuleForge source code

#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>

void narrow_down(std::vector<std::string>* resulted_rules, int N, std::vector<std::string> rules);

int output_rules(std::vector<std::string>* resulted_rules, std::string filename);

int print_clusters(std::vector<std::vector<int>> clusters, int PASSWORDS_COUNT, std::vector<std::string> passwords, bool print_singles);
  
void convert_clusters(int* result, std::vector<std::vector<int>>& clusters, int PASSWORDS_COUNT);

int levenshtein_distance(const std::string &str_x_string, const std::string &str_y_string);