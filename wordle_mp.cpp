#include <omp.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <queue>

int main(int argc, char** argv) {
    // based off of user input, load [input] length words into set and map (initialize map value to 0)
    std::unordered_set<std::string> words();
    std::unordered_map<std::string, int> word_to_removed();

    // run word search space algorithm until certain convergence
    while (words.size() > 1){
        // initially filled with words from words unordered set
        std::queue<std::string> tasks();

        // curr_max used for early stopping
        double curr_max = 0;

        // run search space reduction algorithm
        while (!tasks.empty()) {}

        // make choice, reduce search space
    }
    
    return 0;
}