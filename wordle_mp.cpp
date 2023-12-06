#include <bits/stdc++.h>
#include <omp.h>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>

using std::unordered_set;
using std::unordered_map;
using std::string;
using std::ifstream;
using std::queue;
using std::vector;
using std::cout;
using std::endl;
using std::stoi;

struct Task {
    int testWordIndex;
    int testWordLayer;
    string currWord;
};

struct WordInfo {
    int searchSpaceRemoved;
    int numResponsesTested;
};

int main(int argc, char** argv) {
    double start; 
    double end; 
    double total_time = 0;
    double START = omp_get_wtime();
    double END;

    double maxCorrectness = 1.0;
    int maxRemoved = 0;
    string maxRemovedWord = "";
    string answer = "";
    string lastGuess = "";
    int length = 0;
    int count = 0;
    int listSize = 0;
    bool answerFound = false;

    // Input validation
    if (argc < 3 || argc > 4) {
        cout << "Usage: ./wordle_mp <list_size> <num_letters> <secret_word (optional)>" << endl;
        return -1;
    }
    listSize = stoi(argv[1]);
    length = stoi(argv[2]);
    if (argc == 4) {
        answer = argv[3];
    }
    if (length < 4 || length > 9 || answer.length() != length) {
        cout << "argument num_letters must be between [4, 9] and argument 'answer' must be num_letters long" << endl;
        return -1;
    }

    // Create map of the entire search space
    vector<string> searchSpace;
    unordered_map<string, WordInfo> words;
    queue<Task> tasks;

    //Read in from file
    ifstream wordFile;
    wordFile.open("words_final_" + std::to_string(listSize) + "/" + std::to_string(length) + "_letter_words.txt");
    string nextWord = "";
    srand(time(0)); 
    int randWord = (rand() % listSize) + 1;
    int inc = 0;
    while (!wordFile.eof()) {
        wordFile >> nextWord;
        searchSpace.push_back(nextWord);
        ++inc;
        if (answer == "" && inc == randWord) {
            answer = nextWord; //Change from last word in the list to a random one
        }
    }
    searchSpace.pop_back();
    wordFile.close();
    cout << "Answer: " << answer << endl << endl;

    int numIterations = 0;

    while (!answerFound) {
        ++numIterations;
        if (numIterations >= 7) {
            break;
        }

        //Add all words to the queue
        for (string word: searchSpace) {
            maxRemoved = 0;
            maxRemovedWord = "ZZZZZ";
            words[word] = {0, 0};
            tasks.push({0, 0, word});
        }

        int maxLayer  = (int)log2(searchSpace.size());
        double logNormal = log2(searchSpace.size());
        if (logNormal - maxLayer > 0.0000001) {
            ++maxLayer;
        }

        unordered_set<string> stopChecking;
        while (!tasks.empty()) {
            int numTasks = tasks.size();

            start = omp_get_wtime();
            #pragma omp parallel for
            for (int i = 0; i < numTasks; ++i) {
                Task currTask;
                string currWord;
                bool stop;
                #pragma omp critical
                {
                    currTask = tasks.front();
                    tasks.pop();
                    currWord = currTask.currWord;
                    stop = (stopChecking.find(currWord) != stopChecking.end());
                }
                if (stop) {
                    continue;
                }
                
                // Calculate thoretical max and compare to global max
                #pragma omp critical
                {
                    WordInfo currWordInfo = words[currWord];
                    int numResponsesLeft = searchSpace.size() - currWordInfo.numResponsesTested;
                    int theoreticalMax = currWordInfo.searchSpaceRemoved + (numResponsesLeft * searchSpace.size() * maxCorrectness);
                    
                    if (theoreticalMax < maxRemoved) {
                        //cout << "STOPPED Checking: " << currWord << " at " << currWordInfo.numResponsesTested << "/" << searchSpace.size() << endl;
                        stopChecking.insert(currWord);
                        stop = true;
                    }
                }


                // Do the calculation
                string testWord = searchSpace[currTask.testWordIndex];
                int testWordLayer = currTask.testWordLayer;
                if (testWordLayer == maxLayer && currTask.testWordIndex < searchSpace.size()) {
                    int numWordsRemoved = 0;

                    //Get the response for the currWord (answer) + testWord (guess) combo
                    int combo[length];
                    vector<bool> matched(length, false); //Keeps track of matched letter in the answer
                    for (int i = 0; i < testWord.length(); ++i) {
                        int index = -1;
                        for (int j = 0; j < currWord.length(); ++j) {
                            if (testWord[i] == currWord[j] && !matched[j]) {
                                index = j;
                                break;
                            }
                        }

                        if (index == -1) {
                            combo[i] = 0;
                        }
                        else if (index != i) {
                            combo[i] = 1;
                            matched[index] = true;
                        }
                        else {
                            combo[i] = 2;
                            matched[index] = true;
                        }
                    }

                    //Go through each word in the search space and count how many would be invalid had we guessed the testWord
                    for (const auto &word : searchSpace) {
                        bool valid = true;

                        // Check if a word meets the response
                        vector <bool> matched(length, false);
                        for (int i = 0; i < testWord.length(); ++i) {
                            int index = -1;
                            for (int j = 0; j < word.length(); ++j) {
                                if (testWord[i] == word[j] && !matched[j]) {
                                    index = j;
                                    break;
                                }
                            }

                            if (combo[i] == 0) { // Grey
                                if (index != -1) {
                                    valid  = false;
                                    break;
                                }
                            }
                            else if (combo[i] == 1) { // Yellow
                                if (index == i || index == -1) {
                                    valid  = false;
                                    break;
                                }
                                matched[index] = true;
                            }
                            else { // Green
                                if (index != i) {
                                    valid = false;
                                    break;
                                }
                                matched[index] = true;
                            }
                        }

                        if (!valid) {
                            ++numWordsRemoved;
                        }
                    }

                    // Update the number of words removed
                    #pragma omp critical
                    {
                        words[currWord].searchSpaceRemoved += numWordsRemoved;
                        words[currWord].numResponsesTested += 1;

                        // Update the optimal word if there's a new max.
                        // If tie, choose the one that comes first alphabetically
                        if (words[currWord].searchSpaceRemoved > maxRemoved ||
                            (words[currWord].searchSpaceRemoved == maxRemoved && currWord.compare(maxRemovedWord) < 0)) {
                            maxRemoved = words[currWord].searchSpaceRemoved;
                            maxRemovedWord = currWord;
                        }
                    }
                }


                // Add to task queue
                #pragma omp critical
                {
                    if (testWordLayer < maxLayer) {
                        tasks.push({(currTask.testWordIndex * 2) + 0, testWordLayer+1, currWord});
                        tasks.push({(currTask.testWordIndex * 2) + 1, testWordLayer+1, currWord});
                    }
                }
                
            }
            end = omp_get_wtime();
            total_time += (end - start);
        }

        // Make guess + capture response
        string guess = maxRemovedWord;
        if (guess == lastGuess) {
            cout << "Answer was not in the file, please choose a word from the following file: words_final_" << listSize << "/" << length << "_letter_words.txt" << endl;
            return -1;
        }
        lastGuess = guess;
        int comboResponse[length];
        vector<bool> matched(length, false); //Keeps track of matched letter in the answer
        for (int i = 0; i < guess.length(); ++i) {
            int index = -1;
            for (int j = 0; j < answer.length(); ++j) {
                if (guess[i] == answer[j] && !matched[j]) {
                    index = j;
                    break;
                }
            }

            if (index == -1) {
                comboResponse[i] = 0;
            }
            else if (index != i) {
                comboResponse[i] = 1;
                matched[index] = true;
            }
            else {
                comboResponse[i] = 2;
                matched[index] = true;
            }
        }

        // Check if it's the answer
        bool isAnswer = true;
        for (int i = 0; i < length; ++i) {
            if (comboResponse[i] != 2) {
                isAnswer = false;
                break;
            }
        }
        if (isAnswer) {
            answerFound = true;
        }

        // Update search space
        //Go through each word in the searchSpace
        int numRemoved = 0;
        unordered_set <string> toRemove;
        for (const auto &word : searchSpace) {
            bool valid = true;
            
            // Check if a word meets the response
            vector <bool> matched(length, false);
            for (int i = 0; i < guess.length(); ++i) {
                int index = -1;
                for (int j = 0; j < word.length(); ++j) {
                    if (guess[i] == word[j] && !matched[j]) {
                        index = j;
                        break;
                    }
                }

                if (comboResponse[i] == 0) {
                    if (index != -1) {
                        valid  = false;
                        break;
                    }
                }
                else if (comboResponse[i] == 1) {
                    if (index == i || index == -1) {
                        valid  = false;
                        break;
                    }
                    matched[index] = true;
                }
                else {
                    if (index != i) {
                        valid = false;
                        break;
                    }
                    matched[index] = true;
                }
            }

            if (!valid) {
                toRemove.insert(word);
            }
        }
        toRemove.insert(guess);

        //Remove words that don't work
        vector<string> newSearchSpace;
        for (string word : searchSpace) {
            if (toRemove.find(word) == toRemove.end()) {
                newSearchSpace.push_back(word);
            }
        }
        searchSpace = newSearchSpace;
        cout << "Guess #" << numIterations << ": " << guess << endl;
        cout << "Response: ";
        for (int i = 0; i < length; ++i) {
            if (comboResponse[i] == 0) {
                cout << "B";
            }
            else if (comboResponse[i] == 1) {
                cout << "Y";
            }
            else {
                cout << "G";
            }
        }
        cout << endl;
        cout << "Num Words Removed: " << toRemove.size() << endl << endl;
    }

    END = omp_get_wtime(); 
    printf("Parallel work took %f seconds\n", total_time);
    printf("Entire Program took %f seconds\n", END-START);


    return 0;
}