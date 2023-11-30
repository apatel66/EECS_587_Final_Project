#include <bits/stdc++.h>
#include <omp.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <queue>

using std::unordered_set;
using std::unordered_map;
using std::string;
using std::ifstream;
using std::queue;
using std::cout;
using std::endl:
using std::stoi;

struct Task {
    int response;
    int responseLength:
    string currWord;
};

struct WordInfo {
    bool inSearchSpace;
    int searchSpaceRemoved;
    int numResponsesTested;
};

int main(int argc, char** argv) {
    int searchSpaceSize = 0;
    int maxRemoved = 0;
    string maxRemovedWord = "";

    int length = 0;
    string answer = "";

    // Input validation
    if (argc < 2 || argc > 3) {
        cout << "Usage: ./wordle_mp <num_letters> <secret_word (optional)>" << endl;
        return -1;
    }
    length = stoi(argv[1]);
    if (argc == 3) {
        answer = argv[2];
    }
    if (length < 4 || length > 9 || answer.length() != length) {
        "argument num_letters must be between [4, 9] and argument 'answer' must be num_letters long" << endl;
        return -1;
    }

    // Create map of the entire search space
    int maxResponseNum = pow(3, length);
    unordered_set<string> searchSpace;
    unordered_map<string, WordInfo> words;
    queue<Task> tasks;

    //Read in from file
    ifstream wordFile;
    wordFile.open("words_final/" + to_string(length) + "_letter_words.txt");
    string nextWord = "";
    while (!wordFile.eof()) {
        wordFile >> nextWord;
        words[nextWord] = {false, 0, 0};
        searchSpace.insert(nextWord);
        ++searchSpaceSize;
    }
    wordFile.close();

    //Add all words to the queue
    for (const auto &word : searchSpace) {
        tasks.push({0, 0, word});
    }

    while (!tasks.empty()) {
        int numTasks = tasks.size();

        #pragma omp parallel for
        for (int i = 0; i < numTasks; ++i) {
            Task currTask;

            #pragma omp critical
            {
                currTask = tasks.front();
                tasks.pop();
            }

            // Calculate thoretical max and compare to global max
            string currWord = currTask.currWord;
            WordInfo currWordInfo;
            #pragma omp critical
            {
                currWordInfo = words[currWord];
            }
            int numResponsesLeft = maxResponseNum - currWordInfo.numResponsesTested;
            int theoreticalMax = currWordInfo.searchSpaceRemoved + numResponsesLeft * searchSpaceSize;
            
            if (theoreticalMax < maxRemoved) {
                break;
            }

            

            // Do the calculation
            int responseNum = currTask.response;
            int responseLength = currTask.responseLength;
            if (responseLength == length) {
                int numWordsRemoved = 0;
                // Decode response. ResponseNum --> Response

                //Go through each word in the searchSpace
                for (const auto &word : searchSpace) {
                    bool valid = true;

                    int combo[length];
                    int responseNumCopy = responseNum;
                    for (int i = len-1; i >= 0; —i) {
                        combo[i] = responseNumCopy / pow(3, i);
                        responseNumCopy %= pow(3, i);
                    }

                    // TODO: Check if a word is meets the response

                    if (!valid) {
                        ++numWordsRemoved;
                    }
                }

                // Update the number of words removed
                #pragma omp critical
                {
                    words[currWord].searchSpaceRemoved += numWordsRemoved;
                    words[currWord].numResponsesTested += 1;

                    // Update the optimal word if there's a new max
                    if (words[currWord].searchSpaceRemoved > maxRemoved) {
                        maxRemoved = words[currWord].searchSpaceRemoved;
                        maxRemovedWord = currWord;
                    }
                }
            }


            // Add to task queue
            if (responseLength < length) {
                tasks.push({responseNum*3 + 0, responseLength+1, currWord});
                tasks.push({responseNum*3 + 1, responseLength+1, currWord});
                tasks.push({responseNum*3 + 2, responseLength+1, currWord});
            }
        }

    }

    // TODO: make guess
    // TODO: capture response
    // TODO: update search space





    return 0;
}


/*
    // run word search space algorithm until certain convergence
    while (words.size() > 1){
        // initially filled with words from words unordered set
        // somehow update to NOT include words that have been removed 
        // (or just simply remove these words from vector, could be expensive) 
        std::queue<std::string> tasks(words.begin(), words.end());

        // curr_max used for early stopping
        double curr_max = 0;

        // run search space reduction algorithm
        while (!tasks.empty()) {}

        // make choice, reduce search space
    }
    
*/