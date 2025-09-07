// FastRuleForge source code

#include <algorithm>

#include "args_handler.hh"

void print_help(){
    std::cout << "Available arguments: --i, --o, --LF, --MLF, --DBSCAN, --MDBSCAN, --HAC, --AP" << std::endl;
    std::cout << "Use --N to set the number of passwords" << std::endl;
    std::cout << "Use --no-randomize to disable randomization" << std::endl;
    std::cout << "Use --verbose or --v for verbose output" << std::endl;
    exit(0);
}

bool args_handler::is_method_selected(const char* method) const{
    return std::find(method_names.begin(),method_names.end(),method) != method_names.end();
}

void args_handler::parse_args(int argc, char *argv[]){
    std::vector<std::string> args(argv, argv + argc);
    if(argc < 5){
        std::cout << "Not enough arguments provided" << std::endl;
        print_help();
        exit(1);
    }
    bool input_set = false;
    bool output_set = false;
    bool method_set = false;

    for(int i = 1; i<argc; i++){
        if(args[i] == "--i"){
            if(i+1 < argc){
                input_filename = args[i+1];
                i++;
                input_set = true;
                continue;
            }
            else{
                throw std::runtime_error("No input file provided");
            }
        }
        else if(args[i] == "--o"){
            if(i+1 < argc){
                output_filename = args[i+1];
                i++;
                output_set = true;
                continue;
            }
            else{
                throw std::runtime_error("No output file provided");
            }
        }
        else if(args[i] == "--LF"){
            if (is_method_selected("LF")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("LF");
            method_set = true;
            if(i+1 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    if(number1 < 0 || number1 > 255){
                        throw std::out_of_range("Threshold value out of range");
                    }
                    threshold = static_cast<unsigned char>(number1);
                    i += 1;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--LFC"){
            if (is_method_selected("LFC")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("LFC");
            method_set = true;
            if(i+1 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    if(number1 < 0 || number1 > 255){
                        throw std::out_of_range("Threshold value out of range");
                    }
                    threshold = static_cast<unsigned char>(number1);
                    i += 1;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--MLF"){
            if (is_method_selected("MLF")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("MLF");
            method_set = true;
            if(i+3 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    int number2 = std::stoi(args[i+2]);
                    int number3 = std::stoi(args[i+3]);
                    if(number1 < 0 || number1 > 255 || number2 < 0 || number2 > 255 || number3 < 0 || number3 > 255){
                        throw std::out_of_range("Threshold value out of range");
                    }
                    threshold_main = static_cast<unsigned char>(number1);
                    threshold_sec = static_cast<unsigned char>(number2);
                    threshold_total = static_cast<unsigned char>(number3);
                    i += 3;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--DBSCAN"){
            if (is_method_selected("DBSCAN")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("DBSCAN");
            method_set = true;
            if(i+2 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    int number2 = std::stoi(args[i+2]);
                    if(number1 < 0 || number1 > 255){
                        throw std::out_of_range("Threshold value out of range");
                    }
                    eps_1 = static_cast<unsigned char>(number1);
                    minPts = number2;
                    i += 2;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--MDBSCAN"){
            if (is_method_selected("MDBSCAN")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("MDBSCAN");
            method_set = true;
            if(i+3 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    double number2 = std::stod(args[i+2]);
                    int number3 = std::stoi(args[i+3]);
                    if(number1 < 0 || number1 > 255){
                        throw std::out_of_range("Threshold value out of range");
                    }
                    eps_1 = static_cast<unsigned char>(number1);
                    eps_2 = number2;
                    minPts = number3;
                    i += 3;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--HACFAST"){
            if (is_method_selected("HACFAST")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("HACFAST");
            method_set = true;
            if(i+1 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    if(number1 < 0 || number1 > 255){
                        throw std::out_of_range("Threshold value out of range");
                    }
                    threshold = static_cast<unsigned char>(number1);
                    i += 1;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--HAC"){
            if (is_method_selected("HAC")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("HAC");
            method_set = true;
            if(i+1 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    if(number1 < 0 || number1 > 255){
                        throw std::out_of_range("Threshold value out of range");
                    }
                    threshold = static_cast<unsigned char>(number1);
                    i += 1;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--AP"){
            if (is_method_selected("AP")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("AP");
            method_set = true;
            if(i+2 < argc){
                try{
                    int number1 = std::stoi(args[i+1]);
                    float number2 = std::stof(args[i+2]);
                    if(number1 < 0){
                        throw std::out_of_range("Iter value out of range");
                    }
                    iter = number1;
                    lambda = number2;
                    i += 2;
                    continue;
                }
                catch(...){}
            }
        }
        else if(args[i] == "--RANDOM"){
            if (is_method_selected("RANDOM")){
                throw std::runtime_error("Cannot use method more than once.");
            }
            method_names.push_back("RANDOM");
            method_set = true;
        }
        else if(args[i] == "--N"){
            if(i+1 < argc){
                int number1 = std::stoi(args[i+1]);
                if(number1 < 0){
                    throw std::out_of_range("N value out of range");
                }
                N = number1;
                i += 1;
                continue;
            }
        }
        else if(args[i] == "--no-randomize"){
            randomize = false;
        }
        else if(args[i] == "--verbose" || args[i] == "--v"){
            verbose = true;
        }
        else if(args[i] == "--levenshtein"){
            levenshtein = true;
            substring = false;
        }
        else if(args[i] == "--substring"){
            levenshtein = false;
            substring = true;
        }
        else if(args[i] == "--combo"){
            levenshtein = true;
            substring = true;
        }
        else if(args[i] == "--help" || args[i] == "-h"){
            print_help();
            exit(0);
        }
        else if(args[i] == "--set-rules"){
            if(i+1 < argc){
                std::string raw_rules = args[i+1];
                int lenght = raw_rules.length();
                std::vector<std::string> new_rules;
                for(int e=0; e<lenght && e != all_rules.size()*3+2; e++){
                    if(std::isprint(raw_rules[e])){
                        for(std::string rule : all_rules){
                            if(raw_rules[e] == rule[0]){
                                new_rules.push_back(rule);
                                break;
                            }
                        }
                    }
                }
                std::cout << "Rule priority set to: [";
                for(std::string rule : new_rules){
                    std::cout << rule;
                }
                std::cout << "]" << std::endl;
                rules = new_rules;
                i += 1;
            }
        }
        else{
            std::cout << "Unknown argument: " << args[i] << std::endl;
            print_help();
            exit(1);
        }
    }

    if(!input_set){
        std::cout << "Input file not set. Use --i to set the input file." << std::endl;
        exit(1);
    }
    if(!output_set){
        std::cout << "Output file not set. Use --o to set the output file." << std::endl;
        exit(1);
    }
    
    return;
}

std::unique_ptr<clustering_method> args_handler::get_method(int index){
    if (index < 0 || index >= get_method_count()){
        throw std::runtime_error("Selected method index is out of range.");
    }
    
    std::string method_name = method_names[index];
    
    if(method_name == "LF"){
        return std::make_unique<LF>(threshold, randomize);
    }
    else if(method_name == "LFC"){
        return std::make_unique<LFC>(threshold, randomize);
    }
    else if(method_name == "MLF"){
        return std::make_unique<MLF>(threshold_main, threshold_sec, threshold_total, randomize);
    }
    else if(method_name == "DBSCAN"){
        return std::make_unique<DBSCAN>(eps_1, minPts, randomize);
    }
    else if(method_name == "MDBSCAN"){
        return std::make_unique<MDBSCAN>(eps_1, eps_2, minPts, randomize);
    }
    else if(method_name == "HACFAST"){
        return std::make_unique<HACFAST>(threshold);
    }
    else if(method_name == "HAC"){
        return std::make_unique<HAC>(threshold);
    }
    else if(method_name == "AP"){
        return std::make_unique<AP>(iter, lambda);
    }
    else if(method_name == "RANDOM"){
        return std::make_unique<RANDOM>();
    }
    throw std::runtime_error("Unknown clustering method: " + method_name);
  }

std::string args_handler::get_kernel_main_function_for_method(int index) const{
    if (index < 0 || index >= get_method_count()){
        throw std::runtime_error("Selected method index is out of range.");
    }
    
    std::string method_name = method_names[index];

    if(method_name == "LF"){
        return "DISTANCES";
    }
    else if(method_name == "LFC"){
        return "DISTANCES";
    }
    else if(method_name == "MLF"){
        return "DISTANCES";
    }
    else if(method_name == "DBSCAN"){
        return "DISTANCES";
    }
    else if(method_name == "MDBSCAN"){
        return "DISTANCES";
    }
    else if(method_name == "HACFAST"){
        return "HAC";
    }
    else if(method_name == "HAC"){
        return "DISTANCES";
    }
    else if(method_name == "AP"){
        return "AP";
    }
    else if(method_name == "RANDOM"){
        return "DISTANCES";
    }
    throw std::runtime_error("Unknown clustering method: " + method_name);
}

int args_handler::get_method_count() const {
    return method_names.size();
}