// FastRuleForge source code

#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <omp.h>
#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <memory>

#include "clust_methods.hh"

class args_handler {
public:
    std::string input_filename;
    std::string output_filename;

    std::string method_name = "MDBSCAN";
    unsigned char threshold = 3;

    unsigned char threshold_main = 2;
    unsigned char threshold_sec = 4;
    unsigned char threshold_total = 4;

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

    std::vector<std::string> all_rules = {":", "l", "u", "c", "t", "T", "$", "^", "[", "]", "z", "Z", "D", "i", "o", "s", "}", "{", "r"};

    std::vector<std::string> rules = {":", "l", "u", "c", "t", "T", "$", "^", "[", "]", "z", "Z", "D", "i", "o", "s", "}", "{", "r"};


    std::string kernel_main_function = "DISTANCES";

    void parse_args(int argc, char *argv[]);

    std::unique_ptr<clustering_method> get_method();
};