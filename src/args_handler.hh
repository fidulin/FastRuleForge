// FastRuleForge source code

#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <memory>

#include "clust_methods.hh"

class args_handler {
public:
    std::string input_filename;
    std::string output_filename;

    std::vector<std::string> method_names;
    unsigned char threshold = 3;

    unsigned char threshold_main = 2;
    unsigned char threshold_sec = 4;
    unsigned char threshold_total = 4;

    double threshold_total_jw = 0.25;

    int N = -1;
    
    unsigned char eps_1 = 2;
    double eps_2 = 0.25;
    int minPts = 3;

    bool randomize = true;
    bool verbose = false;

    bool levenshtein = true;
    bool substring = true;

    int iter = 15;
    float lambda = 0.9;

    std::vector<std::string> all_rules = {":", "l", "u", "c", "t", "T", "$", "^", "[", "]", "z", "Z", "D", "i", "o", "s", "}", "{", "r", "Y", "\'", "y", ",", ".", "*"};

    std::vector<std::string> rules = {":", "l", "u", "c", "t", "T", "$", "^", "[", "]", "z", "Z", "D", "i", "o", "s", "}", "{", "r", "Y", "\'", "y", ",", ".", "*"};

    void parse_args(int argc, char *argv[]);

    std::unique_ptr<clustering_method> get_method(int);
    std::string get_method_name(int) const;
    int get_method_count() const;
    std::string get_kernel_main_function_for_method(int) const;

    bool is_method_selected(const char *) const;
};