// FastRuleForge source code

#include "clust_methods.hh"
#include "GPU_executor.hh"
#include "rule_generator.hh"
#include "args_handler.hh"
#include "utils.hh"

#include <ostream>
#include <vector>
#include <algorithm>
#include <omp.h>
#include <set>
#include <string>


int main(int argc, char *argv[]) {
  //------------------------SETTING UP------------------------
  args_handler args;
  args.parse_args(argc, argv);

  GPU_executor executor;
  executor.process_input(args.input_filename, args.verbose);
  if(args.verbose) {std::cout << "Input file [" << args.input_filename << "] containing [" << executor.PASSWORDS_COUNT << "] passwords" << std::endl;}
  executor.setup(args.kernel_main_function, args.verbose);
  
  //------------------------CLUSTERING------------------------
  auto method = args.get_method();
  method->set_data(&executor);
  if(args.verbose){std::cout << "Using clustering method [" << args.method_name << "]" << std::endl;}
  int* result = method->calculate();
  executor.clean();
  
  convert_clusters(result, executor.clusters, executor.PASSWORDS_COUNT);
  
  if(args.verbose){
    print_clusters(executor.clusters, executor.PASSWORDS_COUNT, executor.passwords, false);
  }
  
  if(args.verbose){std::cout << "CLUSTERS GENERATED" << std::endl;}

  //------------------------CHOOSING REPRESENTATIVES------------------------
  rule_generator Rule_generator;
  Rule_generator.rules = args.rules;
  std::vector<std::string> resulted_rules;

  std::vector<std::string> lev_representatives;
  std::vector<std::string> sub_representatives;
  
  // LEVENSTHTEIN METHOD
  if(args.levenshtein){
    lev_representatives.resize(executor.clusters.size());

    executor.setup("DISTANCES", false);
    #pragma omp parallel for schedule(dynamic)
    for(int i=0; i< executor.clusters.size(); i++){
      if(executor.clusters[i].size() <= 1) continue;
      if(executor.clusters[i].size() > 1500){
        lev_representatives[i] = Rule_generator.find_representative_levenshtein_big(&executor.clusters[i], &executor);
      }
      else{
        lev_representatives[i] = Rule_generator.find_representative_levenshtein(&executor.clusters[i], &executor.passwords);
      }
    }
    executor.clean();
  }

  // SUBSTRING METHOD
  if(args.substring){
    sub_representatives.resize(executor.clusters.size());

    #pragma omp parallel for
    for(int i=0; i<executor.clusters.size(); i++){
      if(executor.clusters[i].size() <= 1) continue;
      sub_representatives[i] = Rule_generator.find_representative_substring(&executor.clusters[i], &executor.passwords);
    }
  }
  if(args.verbose) {std::cout << "REPRESENTATIVES CHOSEN" << std::endl;}


  //------------------------GENERATING RULES------------------------
  for(int i=0; i<executor.clusters.size(); i++){
    if(executor.clusters[i].size() <= 1) continue;

    if(args.levenshtein){
      Rule_generator.generate_rules(&executor.clusters[i], &executor.passwords, &resulted_rules, lev_representatives[i]);
    }
    if(args.substring){
      Rule_generator.generate_rules(&executor.clusters[i], &executor.passwords, &resulted_rules, sub_representatives[i]);
    }
  }
  if(resulted_rules.size() == 0){
    std::cout << "No rules generated" << std::endl;
    return 0;
  }
  narrow_down(&resulted_rules, args.N, args.rules);
  
  if(args.verbose){std::cout << "GENERATED [" << resulted_rules.size() << "] RULE SETS" << std::endl;} 
  output_rules(&resulted_rules, args.output_filename);
  
  delete[] result;
  return 0;
}
