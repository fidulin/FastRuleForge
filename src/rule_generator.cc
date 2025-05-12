// FastRuleForge source code

#include "rule_generator.hh"

rule_a_distance rule_generator::apply_rule(std::string rule, std::string *representative, std::string *password, int &distance)
{
  if(rule == "l") //lowercase whole representative
  {
    std::string tmp_representative = *representative;

    std::transform(tmp_representative.begin(), tmp_representative.end(), tmp_representative.begin(), [](unsigned char c)
      { return std::tolower(c); });

    int new_distance = levenshtein_distance(tmp_representative, *password);
    rule_a_distance rad;
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }
    else{
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
  }
  else if(rule == "u") //uppercase whole representative
  {
    std::string tmp_representative = *representative;
    
    std::transform(tmp_representative.begin(), tmp_representative.end(), tmp_representative.begin(), [](unsigned char c)
    { return std::toupper(c); });
    
    int new_distance = levenshtein_distance(tmp_representative, *password);
    rule_a_distance rad;
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }
    else{
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
  }
  else if(rule == "c") //lowercase whole representative, then uppercase the first character 
  {
    std::string tmp_representative = *representative;

    std::transform(tmp_representative.begin(), tmp_representative.end(), tmp_representative.begin(), [](unsigned char c)
      { return std::tolower(c); });
      rule_a_distance rad;
    if(tmp_representative.length() == 0){
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
    tmp_representative.at(0) = std::toupper(tmp_representative.at(0));

    int new_distance = levenshtein_distance(tmp_representative, *password);
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }
    else{
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
  }
  else if(rule == "t") //toggle case in whole representative
  {
    std::string tmp_representative = *representative;

    std::transform(tmp_representative.begin(), tmp_representative.end(), tmp_representative.begin(), [](unsigned char c)
      { return std::islower(c) ? std::toupper(c) : std::tolower(c); });

    int new_distance = levenshtein_distance(tmp_representative, *password);
    rule_a_distance rad;
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }
    else{
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
  }
  else if(rule == "T") //toggle case on a character
  {
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    for(int i=0; i<tmp_representative.length(); i++){
      if(std::isalpha(tmp_representative[i])){
        tmp_representative[i] = std::islower(tmp_representative[i]) ? std::toupper(tmp_representative[i]) : std::tolower(tmp_representative[i]);
        int new_distance = levenshtein_distance(tmp_representative, *password);
        if(new_distance < distance){
          *representative = tmp_representative;

          if(i < 10){ //0-9
            rad.distance = new_distance;
            rad.rule = rule+std::to_string(i);
            return rad;
          }
          else if(i < 36){ //A-Z
            rad.distance = new_distance;
            rad.rule = rule+(char)(i+55);
            return rad;
          }
          else{
            rad.rule = "";
            rad.distance = -1;
            return rad;
          }
        }
        else{
          tmp_representative = *representative;
        }
      }
    }
    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "z"){ //duplicate first character N times
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    int lenght = tmp_representative.length();
    int new_distance;
    int old_distance = 255;
    for(int i=0; i<lenght; i++){
      tmp_representative.insert(0, 1, tmp_representative[0]);
      new_distance = levenshtein_distance(tmp_representative, *password);
      if(new_distance > old_distance && old_distance < distance){
        tmp_representative.erase(0, 1);
        *representative = tmp_representative;
        rad.rule = rule + std::to_string(i);
        rad.distance = old_distance;
        return rad;
      }
      else{
        old_distance = new_distance;
      }
    }


    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "Z"){ //duplicate last character N times
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    int lenght = tmp_representative.length();
    int new_distance;
    int old_distance = 255;
    for(int i=0; i<lenght; i++){
      tmp_representative.insert(tmp_representative.length(), 1, tmp_representative[tmp_representative.length()-1]);
      new_distance = levenshtein_distance(tmp_representative, *password);
      if(new_distance > old_distance && old_distance < distance){
        tmp_representative.erase(tmp_representative.length()-1, 1);
        *representative = tmp_representative;
        rad.rule = rule + std::to_string(i);
        rad.distance = old_distance;
        return rad;
      }
      else{
        old_distance = new_distance;
      }
    }
    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "d"){ //dupliate the whole representative once again
    std::string tmp_representative = *representative;
    tmp_representative += tmp_representative;

    int new_distance = levenshtein_distance(tmp_representative, *password);
    rule_a_distance rad;
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }
    else{
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
  }
  else if(rule == "{" || rule == "}"){ //rotate the representative left or right
    std::string tmp_representative = *representative;

    if(rule == "}"){ //rotate right
      std::rotate(tmp_representative.begin(), tmp_representative.end() - 1, tmp_representative.end());
    }
    else{ //rotate left
      std::rotate(tmp_representative.begin(), tmp_representative.begin() + 1, tmp_representative.end());
    }

    int new_distance = levenshtein_distance(tmp_representative, *password);
    rule_a_distance rad;
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }
    else{
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
  }
  else if(rule == "$"){ //add a character at the end
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    for(char c : *password){
      if(std::isprint(c)){
        tmp_representative.push_back(c);
        int new_distance = levenshtein_distance(tmp_representative, *password);
        if(new_distance < distance){
          *representative = tmp_representative;
          rad.rule = rule + c;
          rad.distance = new_distance;
          return rad;
        }
        else{
          tmp_representative.pop_back();
        }
      }
    }

    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "^"){ //add a character at the beginning
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    for(char c : *password){
      if(std::isprint(c)){
        tmp_representative.insert(tmp_representative.begin(), c);
        int new_distance = levenshtein_distance(tmp_representative, *password);
        if(new_distance < distance){
          *representative = tmp_representative;
          rad.rule = rule + c;
          rad.distance = new_distance;
          return rad;
        }
        else{
          tmp_representative = *representative;
        }
      }
    }

    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "D"){ //delete a character
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    for(int i=0; i<tmp_representative.length(); i++){
      tmp_representative.erase(i, 1);
      int new_distance = levenshtein_distance(tmp_representative, *password);

      if(new_distance < distance){
        *representative = tmp_representative;

        if(i < 10){ //0-9
          rad.rule = rule+std::to_string(i);
          rad.distance = new_distance;
          return rad;
        }
        else if(i < 36){ //A-Z
          rad.rule = rule+(char)(i+55);
          rad.distance = new_distance;
          return rad;
        }
        else{
          break;
        }
      }
      else{
        tmp_representative = *representative;
      }
    }
    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "i"){ //insert a character X at position N (ouptut == iNX)
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    for(char c : *password){
      for(int i=0; i<=tmp_representative.length(); i++){
        tmp_representative.insert(i, 1, c);
        int new_distance = levenshtein_distance(tmp_representative, *password);
        if(new_distance < distance){
          *representative = tmp_representative;

          if(i < 10){ //0-9
            rad.rule = rule+std::to_string(i)+c;
            rad.distance = new_distance;
            return rad;
          }
          else if(i < 36){ //A-Z
            rad.rule = rule+(char)(i+55)+c;
            rad.distance = new_distance;
            return rad;
          }
          else{
            break;
          }
        }
        else{
          tmp_representative = *representative;
        }
      }
    }

    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "o"){ //overwrite with character X at position N (ouptut == iNX)
    std::string tmp_representative = *representative;
    rule_a_distance rad;
    
    for(char c : *password){
      for(int i=0; i<tmp_representative.length(); i++){
        tmp_representative.replace(i, 1, 1, c);
        int new_distance = levenshtein_distance(tmp_representative, *password);
        if(new_distance < distance){
          *representative = tmp_representative;

          if(i < 10){ //0-9
            rad.rule = rule+std::to_string(i)+c;
            rad.distance = new_distance;
            return rad;
          }
          else if(i < 36){ //A-Z
            rad.rule = rule+(char)(i+55)+c;
            rad.distance = new_distance;
            return rad;
          }
          else{
            break;
          }
        }
        else{
          tmp_representative = *representative;
        }
      }
    }

    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "[" || rule == "]"){ //delete firs or last character
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    if(rule == "]"){ //delete last character
      tmp_representative.pop_back();
    }
    else{ //delete first character
      tmp_representative.erase(0, 1);
    }
    int new_distance = levenshtein_distance(tmp_representative, *password);
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }
    else{
      rad.rule = "";
      rad.distance = -1;
      return rad;
    }
  }
  else if(rule == "s"){ //replace all specific characters with a different character
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    for(char c1 : *password){
      for(char c2 : tmp_representative){

        std::replace(tmp_representative.begin(), tmp_representative.end(), c2, c1);
        int new_distance = levenshtein_distance(tmp_representative, *password);
        
        if(new_distance < distance){
          *representative = tmp_representative;
          rad.distance = new_distance;
          rad.rule = rule + c2 + c1;
          return rad;
        }
        else{
          tmp_representative = *representative;
        }
      }
    }

    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else if(rule == "r"){ //reverse the representative
    std::string tmp_representative = *representative;
    rule_a_distance rad;

    std::reverse(tmp_representative.begin(), tmp_representative.end());
    int new_distance = levenshtein_distance(tmp_representative, *password);
    if(new_distance < distance){
      *representative = tmp_representative;
      rad.rule = rule;
      rad.distance = new_distance;
      return rad;
    }

    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
  else{
    rule_a_distance rad;
    rad.rule = "";
    rad.distance = -1;
    return rad;
  }
}

std::string rule_generator::find_representative_levenshtein(std::vector<int> *cluster, std::vector<std::string> *all_passwords)
{
  int representative_index = (*cluster)[0];
  int min_distance = INT_MAX;
  for (int password_index : *cluster)
  {
    int distance = 0;
    for (int i = 0; i < (*cluster).size(); i++)
    {
      distance += levenshtein_distance((*all_passwords)[password_index], (*all_passwords)[(*cluster)[i]]);

      if(distance > min_distance){
        break;
      }
    }
    if (distance < min_distance)
    {
      min_distance = distance;
      representative_index = password_index;
    }
  }
  return (*all_passwords)[representative_index];
}

std::string rule_generator::find_representative_levenshtein_big(std::vector<int> *cluster, GPU_executor *executor)
{
  int *distances;

  size_t global_work_size = executor->PASSWORDS_COUNT;
  
  int min_distance = INT_MAX;

  int representative_index = (*cluster)[0];

  for(int i=0; i<cluster->size(); i++){
    distances = executor->calculate_distances_to((*cluster)[i], 255, global_work_size);
    int distance = 0;
    for(int password_index : *cluster){
      distance += distances[password_index];
    }
    if(distance < min_distance){
      min_distance = distance;
      representative_index = (*cluster)[i];
    }
    delete[] distances;
  }
  return (executor->passwords)[representative_index];
}

std::string rule_generator::find_representative_substring(std::vector<int> *cluster, std::vector<std::string> *all_passwords)
{
  std::vector<std::string> alpha_passwords;
  for(int index : *cluster){
    std::string password = (*all_passwords)[index];

    for (int i = 0; i < lta_leet.size(); i++)
    {
      std::replace(password.begin(), password.end(), lta_leet[i], lta_alpha[i]);
    }
    alpha_passwords.push_back(password);
  }
  
  //find the longest substring
  std::string base = alpha_passwords[0];
  std::string longest;

  for (int i = 0; i < base.length(); ++i) {
    for (int j = i + 1; j <= base.length(); ++j) {
      std::string sub = base.substr(i, j - i);
      if (sub.length() <= longest.length()) continue;

      bool is_common = true;
      for (int k = 1; k < alpha_passwords.size(); ++k) {
        if (alpha_passwords[k].find(sub) == std::string::npos) {
          is_common = false;
          break;
        }
      }

      if (is_common) {
        longest = sub;
      }
    }
  }

  //std::cout << "Longest common substring: " << longest << std::endl;
  return longest;
}

void rule_generator::generate_rules(std::vector<int> *cluster, std::vector<std::string> *all_passwords, std::vector<std::string> *resulted_rules, std::string &representative)
{
  //for each password in the cluster
  #pragma omp parallel for
  for (int password_index : *cluster)
  {
    std::string password = (*all_passwords)[password_index];
    int distance = levenshtein_distance(representative, password);
    if(distance == 0){
      continue;
    }

    std::string tmp_representative = representative;
    std::vector<std::string> applied_rules_for_password;
    bool rule_applied = false;


    size_t rule_index = 0;
    
    while (rule_index < this->rules.size()) {
      std::string rule = this->rules[rule_index];
      rule_a_distance rad = apply_rule(rule, &tmp_representative, &password, distance);
      
      if (rad.distance == -1) {
        rule_index++; // Move to the next rule
        continue;
      }

      applied_rules_for_password.push_back(rad.rule);
      rule_applied = true;
      distance = rad.distance;

      if (distance == 0) {
        if (!applied_rules_for_password.empty()) {
          std::string applied_rules_sequence = "";
          for(std::string rule : applied_rules_for_password){
            #pragma omp critical
            resulted_rules->push_back(rule);
            applied_rules_sequence += rule + " ";
          }
          applied_rules_sequence.pop_back();
          #pragma omp critical
          resulted_rules->push_back(applied_rules_sequence);
          //std::cout << representative << "->" << password << " : " << applied_rules_sequence << std::endl;
        }
        break;
      }

    }
    rule_index = 0;

    
  }
}