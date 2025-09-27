// FastRuleForge source code

#pragma once
#include <vector>
#include <memory>
#include <random>
#include <string>
#include <algorithm>
#include <queue>
#include <stack>
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

/*
 * LEADER FOLLOWER CLUSTERING METHOD (LEADERS METHOD)
 *
 * This method randomly chooses (if randomize == true) a password, that has not yet been labeled (-1 in result array).
 * This chosen password is proclaimed as Leader and a new cluster is created for it.
 * All password closer than threshold that are unlabeled are added to this newly created cluster.
 * Until there are any unlabeled passwords left, the steps above are performed in a loop.
 * 
 */
class LF : public clustering_method {
public:
    LF(unsigned char threshold, bool randomize) : threshold(threshold), randomize(randomize) {}

    int* calculate() override {
        size_t global_work_size = PASSWORDS_COUNT;

        int* distances;

        //Setting up results array - for each password there will be a cluster number in this array.
        //For now its all -1
        //-1 on index means password with this index has not been assigned to a cluster.
        int* result = new int[PASSWORDS_COUNT];
        #pragma omp parallel for
        for(int i = 0; i < PASSWORDS_COUNT; i++){
            result[i] = -1;
        }

        //Randomising the sequence of passwords
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

        //For all passwords in a randomised sequence.
        for(int i : indexes){
            if(result[i] != -1){
                continue;
            }

            //The password i is a Leader and distances to all other passwords are calculated.
            //It has its own cluster.
            result[i] = i;
            distances = gpu_executor->calculate_distances_to(i, threshold, global_work_size);

            //All passwords closer than threshold are added to Leaders cluster - that is if they are not part of another cluster.
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

/*
 * LEADER FOLLOWER !!!CENTERED!!! CLUSTERING METHOD (LEADERS METHOD)
 * 
 * Modified version of LF that changes the leader to password that is the 'Levenshtein cluster center'.
 * The clusters should be a bit bigger than LF and there should be less change between runs - that is teorethically off course.
 *
 * 1) This method randomly chooses (if randomize == true) a password, that has not yet been labeled (-1 in result array).
 * This chosen password is proclaimed as Leader and a new cluster is created for it.
 * 2) All password closer than threshold that are unlabeled are added to this newly created cluster.
 * Then the new leader of this cluster is calculated - it has the lowest average Lev. distances to others in cluster.
 * If the leader is different, go to step 2), otherwise finnish with this cluster and go to step 1). 
 * 
 */
class LFC : public clustering_method {
public:
    LFC(unsigned char threshold, bool randomize) : threshold(threshold), randomize(randomize) {}

    //This function returns index of a password from this cluster - the new leader.
    //New leader is password with minimal average distance to others in its current cluster.
    int new_leader(std::vector<int> &current_cluster, unsigned char t, size_t global_work_size){
        int min_sum = INT32_MAX;
        int new_leader = current_cluster[0];

        //For every password in cluster.
        for(int i : current_cluster){
            int sum = 0;

            //distances to ALL passwords is calulated - maybe it would be quicker normally.
            int *distances = gpu_executor->calculate_distances_to(i, t, global_work_size);
            for(int e : current_cluster){
                if(i==e){
                    continue;
                }
                sum += distances[e];
            }
            delete[] distances;

            //Minimal average distance with the potential new leader is stored.
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
        // The setup is same as LF.

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

        //For every password.
        for(int i : indexes){
            if(result[i] != -1){
                continue;
            }

            //The current password is proclaimed as leader and is in its own cluster.
            result[i] = i;
            int leader = i;
            std::vector<int> current_cluster;
            current_cluster.push_back(i);

            bool changed = true;

            //Repeat if the current clusters leader has changed.
            while(changed){
                changed = false;

                //All passwords closer than threshold to the current leader are added to current cluster.
                distances = gpu_executor->calculate_distances_to(leader, threshold, global_work_size);
                for(int e=0; e<PASSWORDS_COUNT; e++){
                    if(result[e] == -1 && distances[e] <= threshold){
                        result[e] = i;
                        current_cluster.push_back(e);
                    }
                }
                delete[] distances;

                //If there is more than one password in this cluster - recalculate the leader.
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

/*
 * MODIFIED LEADER FOLLOWER CLUSTERING METHOD (MODIFIED LEADERS METHOD)
 * Method created by myself for this very purpose.
 * 
 * The method tries to eliminate the limitation of LF method - passwords can join the cluster even if they are further from
 * the leader but they must be close enough to a different password already in said cluster.
 *
 * As in the previous methods, unlabeled password is chosen at random (thats the default). - end if there isnt any
 * This password can join a leader that already exists if its at least threshold_main close to it.
 * If this condition is not met - there is another way here:
 * If the password is at least threshold_sec close to an already labeled password and at the same time
 * it is at least threshold_total close to leader of that labeled password, the current password joins this cluster.
 * If both ways of joining fail - the password itself becomes a leader and algorithm chooses another password (step 1 again).
 * 
 * It is important that threshold_total is greater than threshold_main - otherwise it will behave as basic LF.
 */
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

        //For every password.
        for(int i : indexes){

            //Distances to all other passwords are calculated.
            distances = gpu_executor->calculate_distances_to(i, max_threshold, global_work_size);

            bool joined = false;
            //Current password (index i) is checked - if its at least threshold_main close to a leader, it joins this leaders cluster.
            for(int j : leaders){
                if(distances[j] <= threshold_main){
                    result[i] = j;
                    joined = true;
                    break;
                }
            }

            //If current password didnt join a leader yet - there is another possibility to join a leaders cluster. Two conditions have to be met.
            //1) The password must be at least threshold_sec close to a labeled password (already in a cluster).
            //2) It has to be at least threshold_total close to leader of said labeled password in 1).
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

            //If current password didnt join any leader, it becomes one.
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

constexpr double JARO_WEIGHT_STRING_A(1.0/3.0);
constexpr double JARO_WEIGHT_STRING_B(1.0/3.0);
constexpr double JARO_WEIGHT_TRANSPOSITIONS(1.0/3.0);

constexpr int JARO_WINKLER_PREFIX_SIZE(4);
constexpr double JARO_WINKLER_SCALING_FACTOR(0.1);
constexpr double JARO_WINKLER_BOOST_THRESHOLD(0.7);

#define JARO_WINKLER_USE_EARLY_RETURN_PREFIX
#define JARO_WINKLER_USE_THRESHOLD

class MDBSCAN : public clustering_method{
public:
    MDBSCAN(unsigned char eps_1, double eps_2, int minPts, bool randomize) : eps_1(eps_1), eps_2(eps_2), minPts(minPts), randomize(randomize) {}

    /*
    * This implementation of the Jaro-Winkler distance is based on code by YumaTheCompanion:
    * https://github.com/YumaTheCompanion/Jaro-Winkler/
    * Licensed under the MIT License.
    */
    double jaro_winkler_distance(const std::string &str1, const std::string &str2) {
        // Register strings length.
        const int str1Length(str1.size());
        const int str2Length(str2.size());
        
        // If one string has null length, we return 1.
        if (str1Length == 0 || str2Length == 0)
        {
            return 1.0;
        }
        
        // Calculate max length range.
        const int maxRange(std::max(0, std::max(str1Length, str2Length) / 2 - 1));
        
        // Creates 2 vectors of integers.
        std::vector<bool> str1Match(str1Length, false);
        std::vector<bool> str2Match(str2Length, false);
        
        // Calculate matching characters.
        int matchingCharacters(0);
        for (int str1Index(0); str1Index < str1Length; ++str1Index)
        {
            // Calculate window test limits (limit inferior to 0 and superior to str2Length).
            const int minIndex(std::max(str1Index - maxRange, 0));
            const int maxIndex(std::min(str1Index + maxRange + 1, str2Length));
            
            if (minIndex >= maxIndex)
            {
                // No more common character because we don't have characters in b to test with characters in a.
                break;
            }
            
            for (int str2Index(minIndex); str2Index < maxIndex; ++str2Index)
            {
                if (!str2Match.at(str2Index) && str1.at(str1Index) == str2.at(str2Index))
                {
                    // Found some new match.
                    str1Match[str1Index] = true;
                    str2Match[str2Index] = true;
                    ++matchingCharacters;
                    break;
                }
            }
        }
        
        // If no matching characters, we return 1.
        if (matchingCharacters == 0)
        {
            return 1.0;
        }
        
        // Calculate character transpositions.
        std::vector<int> str1Position(matchingCharacters, 0);
        std::vector<int> str2Position(matchingCharacters, 0);
        
        for (int str1Index(0), positionIndex(0); str1Index < str1Length; ++str1Index)
        {
            if (str1Match.at(str1Index))
            {
                str1Position[positionIndex] = str1Index;
                ++positionIndex;
            }
        }
        
        for (int str2Index(0), positionIndex(0); str2Index < str2Length; ++str2Index)
        {
            if (str2Match.at(str2Index))
            {
                str2Position[positionIndex] = str2Index;
                ++positionIndex;
            }
        }
        
        // Counting half-transpositions.
        int transpositions(0);
        for (int index(0); index < matchingCharacters; ++index)
        {
            if (str1.at(str1Position.at(index)) != str2.at(str2Position.at(index)))
            {
                ++transpositions;
            }
        }
        
        // Calculate Jaro distance.
        double distance = (
            JARO_WEIGHT_STRING_A * matchingCharacters / str1Length +
            JARO_WEIGHT_STRING_B * matchingCharacters / str2Length +
            JARO_WEIGHT_TRANSPOSITIONS * ((matchingCharacters - transpositions) / 2.0) / matchingCharacters
        );

        #ifdef JARO_WINKLER_USE_THRESHOLD
        if (distance > JARO_WINKLER_BOOST_THRESHOLD)
        {
        #endif
            // Calculate common string prefix.
            int commonPrefix(0);
            for (int index(0), indexEnd(std::min(std::min(str1Length,str2Length), JARO_WINKLER_PREFIX_SIZE)); index < indexEnd; ++index)
            {
                if (str1.at(index) == str2.at(index))
                {
                    ++commonPrefix;
                }
                #ifdef JARO_WINKLER_USE_EARLY_RETURN_PREFIX
                else
                {
                    break;
                }
                #endif
            }
            
            // Calculate Jaro-Winkler distance.
            distance += JARO_WINKLER_SCALING_FACTOR * commonPrefix * (1.0 - distance);
        #ifdef JARO_WINKLER_USE_THRESHOLD
        }
        #endif
        
        return 1.0 - distance;
    
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
