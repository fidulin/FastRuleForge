// FastRuleForge source code

#pragma once
#include <vector>
#include <memory>
#include <random>
#include <string>
#include <algorithm>
#include <queue>
#include <stack>
#include <omp.h>
#include <atomic>
#include "GPU_executor.hh"
#include "utils.hh"

class clustering_method {
public:
    virtual ~clustering_method() = default;

    virtual void set_data(GPU_executor *executor) {
        this->gpu_executor = executor;
        this->PASSWORDS_COUNT = executor->PASSWORDS_COUNT;
    }

    virtual int* calculate() = 0;

    GPU_executor* gpu_executor;
    int PASSWORDS_COUNT;
};

class LF : public clustering_method {
public:
    LF(unsigned char threshold, bool randomize) : threshold(threshold), randomize(randomize) {}

    int* calculate() override {
        size_t global_work_size = PASSWORDS_COUNT;

        int* distances;

        int* result = new int[PASSWORDS_COUNT];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            result[i] = -1;
        }

        std::vector<int> indexes(PASSWORDS_COUNT);
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            indexes[i] = i;
        }

        if(randomize){
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indexes.begin(), indexes.end(), g);
        }

        for(int i : indexes){
            if(result[i] != -1){
                continue;
            }
            result[i] = i;
            distances = gpu_executor->calculate_distances_to(i, threshold, global_work_size);
            
            #pragma omp parallel for
            for(int e=0; e<PASSWORDS_COUNT; e++){
                if(result[e] == -1 && distances[e] <= threshold){
                    result[e] = i;
                }
            }

            delete[] distances;
        }
        return result;
    }


private:
    unsigned char threshold;
    bool randomize;
};

//like LF but moves leadership - possible improvement
class LFC : public clustering_method {
public:
    LFC(unsigned char threshold, bool randomize) : threshold(threshold), randomize(randomize) {}

    int new_leader(std::vector<int> &current_cluster, unsigned char t, size_t global_work_size){
        int min_sum = INT32_MAX;
        int new_leader = current_cluster[0];

        for(int i : current_cluster){
            int sum = 0;
            
            int *distances = gpu_executor->calculate_distances_to(i, t, global_work_size);
            for(int e : current_cluster){
                if(i==e){
                    continue;
                }
                sum += distances[e];
            }
            delete[] distances;
            
            if(sum < min_sum){
                new_leader = i;
                min_sum = sum;
            }
        }
        return new_leader;
    }

    int* calculate() override {
        size_t global_work_size = PASSWORDS_COUNT;

        int* distances;

        int* result = new int[PASSWORDS_COUNT];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            result[i] = -1;
        }

        std::vector<int> indexes(PASSWORDS_COUNT);
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            indexes[i] = i;
        }

        if(randomize){
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indexes.begin(), indexes.end(), g);
        }

        for(int i : indexes){
            if(result[i] != -1){
                continue;
            }
            result[i] = i;

            int leader = i;
            std::vector<int> current_cluster;
            current_cluster.push_back(i);

            bool changed = true;

            while(changed){
                changed = false;

                distances = gpu_executor->calculate_distances_to(leader, threshold, global_work_size);
                for(int e=0; e<PASSWORDS_COUNT; e++){
                    if(result[e] == -1 && distances[e] <= threshold){
                        result[e] = i;
                        current_cluster.push_back(e);
                    }
                }
                delete[] distances;

                if(current_cluster.size() > 1){
                    int old_leader = leader;
                    leader = new_leader(current_cluster, threshold, global_work_size);
                    if(old_leader != leader){
                        changed = true;
                    }
                }
            }
        }
        return result;
    }

private:
    unsigned char threshold;
    bool randomize;
};


class MLF : public clustering_method {
public:
    MLF(unsigned char t1, unsigned char t2, unsigned char t3, bool randomize) : 
    threshold_main(t1), threshold_sec(t2), threshold_total(t3), randomize(randomize) {}

    int* calculate() override {
        size_t global_work_size = PASSWORDS_COUNT;

        std::vector<int> leaders;
        int* distances;

        int* result = new int[PASSWORDS_COUNT];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            result[i] = -1;
        }

        std::vector<int> indexes(PASSWORDS_COUNT);
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            indexes[i] = i;
        }

        if(randomize){
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indexes.begin(), indexes.end(), g);
        }

        unsigned char max_threshold = std::max(std::max(threshold_main, threshold_sec), threshold_total);

        for(int i : indexes){
            
            distances = gpu_executor->calculate_distances_to(i, max_threshold, global_work_size);

            bool joined = false;
            for(int j : leaders){
                if(distances[j] <= threshold_main){
                    result[i] = j;
                    joined = true;
                    break;
                }
            }

            if(!joined){
            for(int j = 0; j < PASSWORDS_COUNT; j++){
                //joins if distance to non leader is <= threshold_sec and distance to leader of its cluster is <= threshold_total
                if(distances[j] <= threshold_sec && result[j] != -1 && distances[result[j]] <= threshold_total && j != i){
                result[i] = result[j];
                joined = true;
                break;
                }
            }
            }

            if(!joined){
                leaders.push_back(i);
                result[i] = i;
            }
            delete[] distances;
        }

        return result;
    }

private:
    unsigned char threshold_main;
    unsigned char threshold_sec;
    unsigned char threshold_total;
    bool randomize;
};

class HACFAST : public clustering_method {
public:
    HACFAST(unsigned char threshold) : threshold(threshold) {}

    int* calculate() override {
        size_t local_work_size = 1024;
        size_t global_work_size = PASSWORDS_COUNT;
        return gpu_executor->HAC_calculate(threshold, local_work_size, global_work_size);
    }

private:
    unsigned char threshold;
};

class HAC : public clustering_method {
public:
    HAC(unsigned char threshold) : threshold(threshold) {}

    void join_groups(int* result, int pswd1, int pswd2){ //join group of pswd2 into pswd1
        int group_to_move = result[pswd2];
        int group_to_join = result[pswd1];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            if(result[i] == group_to_move){
                result[i] = group_to_join;
            }
        }
    }

    int* calculate() override {
        size_t global_work_size = PASSWORDS_COUNT;

        std::vector<int> leaders;
        int* distances;

        int* result = new int[PASSWORDS_COUNT];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            result[i] = i;
        }

        for(int i=0; i<PASSWORDS_COUNT; i++){
            distances = gpu_executor->calculate_distances_to(i, threshold, global_work_size);

            for(int e=0; e<PASSWORDS_COUNT; e++){
                if(distances[e] <= threshold && e != i){
                    join_groups(result, i, e);
                }
            }

            delete[] distances;
        }
        return result;
    }

private:
    unsigned char threshold;
};

class DBSCAN : public clustering_method {
public:
    DBSCAN(unsigned char eps_1, int minPts, bool randomize) : eps_1(eps_1), minPts(minPts), randomize(randomize) {}


    int* calculate() override {
        size_t global_work_size = PASSWORDS_COUNT;

        int* distances;
        std::stack<int> stack;

        int* result = new int[PASSWORDS_COUNT];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            result[i] = -1;
        }

        std::vector<int> indexes(PASSWORDS_COUNT);
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            indexes[i] = i;
        }

        if(randomize){
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indexes.begin(), indexes.end(), g);
        }

        int cluster_index = 0;
        for(int i : indexes){
            if(result[i] != -1){
                continue;
            }

            distances = gpu_executor->calculate_distances_to(i, eps_1, global_work_size);
            
            std::vector<int> init_neighbourhood;
            #pragma omp parallel
            {
                std::vector<int> local_buffer;

                #pragma omp for nowait
                for (int j = 0; j < PASSWORDS_COUNT; j++) {
                    if (distances[j] <= eps_1) {
                        local_buffer.push_back(j);
                    }
                }

                #pragma omp critical
                init_neighbourhood.insert(init_neighbourhood.end(), local_buffer.begin(), local_buffer.end());
            }

            delete[] distances;

            if(init_neighbourhood.size() < minPts){
                continue;
            }

            stack.push(i);

            while(!stack.empty()){
                int current = stack.top();
                stack.pop();

                if(result[current] == -1){
                    result[current] = cluster_index;

                    std::vector<int> neighbourhood;
                    if(i!=current){
                        distances = gpu_executor->calculate_distances_to(current, eps_1, global_work_size);

                        #pragma omp parallel
                        {
                            std::vector<int> local_buffer;

                            #pragma omp for nowait
                            for (int j = 0; j < PASSWORDS_COUNT; j++) {
                                if (distances[j] <= eps_1) {
                                    local_buffer.push_back(j);
                                }
                            }

                            #pragma omp critical
                            neighbourhood.insert(neighbourhood.end(), local_buffer.begin(), local_buffer.end());
                        }

                        delete[] distances;
                    }
                    else{
                        neighbourhood = init_neighbourhood;
                    }
                    
                    if(neighbourhood.size() >= minPts){
                        for(int j : neighbourhood){
                            if(result[j] == -1){
                                stack.push(j);
                            }
                        }
                    }

                }
            }
            cluster_index++;
        }

        return result;
    }

private:
    unsigned char eps_1;
    int minPts;
    bool randomize;
};

class MDBSCAN : public clustering_method{
public:
    MDBSCAN(unsigned char eps_1, double eps_2, int minPts, bool randomize) : eps_1(eps_1), eps_2(eps_2), minPts(minPts), randomize(randomize) {}

    /*
    * This implementation of the Jaro-Winkler distance is based on code from Rosetta Code:
    * https://rosettacode.org/wiki/Jaro-Winkler_distance
    * Licensed under the GNU Free Documentation License 1.3
    */
    double jaro_winkler_distance(std::string &str1, std::string &str2) {
        size_t len1 = str1.size();
        size_t len2 = str2.size();
        if (len1 < len2) {
            std::swap(str1, str2);
            std::swap(len1, len2);
        }
        if (len2 == 0)
            return len1 == 0 ? 0.0 : 1.0;
        size_t delta = std::max(size_t(1), len1/2) - 1;
        std::vector<bool> flag(len2, false);
        std::vector<char> ch1_match;
        ch1_match.reserve(len1);
        for (size_t idx1 = 0; idx1 < len1; ++idx1) {
            char ch1 = str1[idx1];
            for (size_t idx2 = 0; idx2 < len2; ++idx2) {
                char ch2 = str2[idx2];
                if (idx2 <= idx1 + delta && idx2 + delta >= idx1
                    && ch1 == ch2 && !flag[idx2]) {
                    flag[idx2] = true;
                    ch1_match.push_back(ch1);
                    break;
                }
            }
        }
        size_t matches = ch1_match.size();
        if (matches == 0)
            return 1.0;
        size_t transpositions = 0;
        for (size_t idx1 = 0, idx2 = 0; idx2 < len2; ++idx2) {
            if (flag[idx2]) {
                if (str2[idx2] != ch1_match[idx1])
                    ++transpositions;
                ++idx1;
            }
        }
        double m = matches;
        double jaro = (m/len1 + m/len2 + (m - transpositions/2.0)/m)/3.0;
        size_t common_prefix = 0;
        len2 = std::min(size_t(4), len2);
        for (size_t i = 0; i < len2; ++i) {
            if (str1[i] == str2[i])
                ++common_prefix;
        }
        return 1.0 - (jaro + common_prefix * 0.1 * (1.0 - jaro));
    }

    int* calculate() override{
        size_t global_work_size = PASSWORDS_COUNT;

        int* distances;

        int* result = new int[PASSWORDS_COUNT];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            result[i] = -1;
        }

        std::vector<int> indexes(PASSWORDS_COUNT);
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            indexes[i] = i;
        }

        if(randomize){
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(indexes.begin(), indexes.end(), g);
        }

        int cluster_index = 0;
        for(int i : indexes){
            if(result[i] != -1){
                continue;
            }

            distances = gpu_executor->calculate_distances_to(i, eps_1, global_work_size);
            
            std::vector<int> init_neighbourhood;
            #pragma omp parallel
            {
                std::vector<int> local_buffer;

                #pragma omp for nowait
                for (int j = 0; j < PASSWORDS_COUNT; j++) {
                    if (distances[j] <= eps_1) {
                        local_buffer.push_back(j);
                    }
                }

                #pragma omp critical
                init_neighbourhood.insert(init_neighbourhood.end(), local_buffer.begin(), local_buffer.end());
            }

            delete[] distances;

            if(init_neighbourhood.size() < minPts){
                continue;
            }

            std::queue<int> queue;
            queue.push(i);
            

            while(!queue.empty()){
                int current = queue.front();
                queue.pop();

                if(result[current] == -1 && jaro_winkler_distance(gpu_executor->passwords[current], gpu_executor->passwords[i]) <= eps_2){
                    result[current] = cluster_index;

                    if(i!=current){
                        std::vector<int> neighbourhood;
                        distances = gpu_executor->calculate_distances_to(current, eps_1, global_work_size);

                        #pragma omp parallel
                        {
                            std::vector<int> local_buffer;

                            #pragma omp for nowait
                            for (int j = 0; j < PASSWORDS_COUNT; j++) {
                                if (distances[j] <= eps_1) {
                                    local_buffer.push_back(j);
                                }
                            }

                            #pragma omp critical
                            neighbourhood.insert(neighbourhood.end(), local_buffer.begin(), local_buffer.end());
                        }

                        delete[] distances;

                        if(neighbourhood.size() >= minPts){
                            for(int j : neighbourhood){
                                if(result[j] == -1){
                                    queue.push(j);
                                }
                            }
                        }
                    }
                    else{
                        if(init_neighbourhood.size() >= minPts){
                            for(int j : init_neighbourhood){
                                if(result[j] == -1){
                                    queue.push(j);
                                }
                            }
                        }
                    }
                    
                    

                }
            }
            cluster_index++;
        }

        return result;
    }


private:
    unsigned char eps_1;
    double eps_2;
    int minPts;
    bool randomize;
};

class AP : public clustering_method {
public:
    AP(int iter, float lambda) : iter(iter), lambda(lambda) {}
    int* calculate() override{
        return gpu_executor->AP_calculate(iter, lambda);
    }
private:
    int iter;
    float lambda;
};

class RANDOM : public clustering_method {
    public:
        int* calculate() override{
            int* result = new int[PASSWORDS_COUNT];
            #pragma omp parallel for
            for(int i = 0; i < PASSWORDS_COUNT; i++){
                result[i] = 0;
            }
            return result;
        }
    };