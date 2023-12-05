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
    int response;
    int responseLength;
    string currWord;
};

struct WordInfo {
    int searchSpaceRemoved;
    int numResponsesTested;
};

int main(int argc, char** argv) {
    double start; 
    double end; 
    start = omp_get_wtime(); 

    int searchSpaceSize = 0;
    int maxRemoved = 0;
    string maxRemovedWord = "";
    string answer = "";
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
    int maxResponseNum = pow(3, length);
    unordered_set<string> searchSpace;
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
        searchSpace.insert(nextWord);
        ++searchSpaceSize;
        ++inc;
        if (answer == "" && inc == randWord) {
            answer = nextWord; //Change from last word in the list to a random one
        }
    }
    wordFile.close();
    cout << "Answer: " << answer << endl << endl;

    int numIterations = 0;

    while (!answerFound) {
        ++numIterations;
        if (numIterations >= 10) {
            break;
        }

        //Add all words to the queue
        for (const auto &word : searchSpace) {
            maxRemoved = 0;
            maxRemovedWord = "";
            words[word] = {0, 0};
            tasks.push({0, 0, word});
        }

        while (!tasks.empty()) {
            int numTasks = tasks.size();
            bool flag = false;

            #pragma omp parallel for
            for (int i = 0; i < numTasks; ++i) {
                if (flag) {
                    continue;
                }

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
                    flag = true;
                    continue;
                }

                // Do the calculation
                int responseNum = currTask.response;
                int responseLength = currTask.responseLength;
                if (responseLength == length) {
                    int numWordsRemoved = 0;

                    // Decode response. ResponseNum --> Response
                    // 0 -> B, 1 -> Y, 2 -> G
                    int combo[length];
                    int responseNumCopy = responseNum;
                    for (int i = length-1; i >= 0; --i) {
                        combo[i] = responseNumCopy / pow(3, i);
                        responseNumCopy %= (int) pow(3, i);
                    }

                    //Go through each word in the searchSpace
                    for (const auto &word : searchSpace) {
                        bool valid = true;

                        // Check if a word meets the response
                        vector <bool> matched(length, false);
                        for (int i = 0; i < currWord.length(); ++i) {
                            int index = -1;
                            for (int j = 0; j < word.length(); ++j) {
                                if (currWord[i] == word[j] && !matched[j]) {
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

                        // Update the optimal word if there's a new max
                        if (words[currWord].searchSpaceRemoved > maxRemoved) {
                            maxRemoved = words[currWord].searchSpaceRemoved;
                            maxRemovedWord = currWord;
                            //cout << maxRemovedWord << " " << maxRemoved << endl;
                        }
                    }
                }


                // Add to task queue
                #pragma omp critical
                {
                    if (responseLength < length) {
                        tasks.push({responseNum*3 + 0, responseLength+1, currWord});
                        tasks.push({responseNum*3 + 1, responseLength+1, currWord});
                        tasks.push({responseNum*3 + 2, responseLength+1, currWord});
                    }
                }
                
            }

        }

        // Make guess + capture response
        string guess = maxRemovedWord;
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
            //cout << guess[i] << " " << "Position in Guess: " << i << " " << "Position in answer: " << index << endl;

            if (index == -1) {
                //cout << "Black" << endl;
                comboResponse[i] = 0;
            }
            else if (index != i) {
                //cout << "Yellow" << endl;
                comboResponse[i] = 1;
                matched[index] = true;
            }
            else {
                //cout << "Green" << endl;
                comboResponse[i] = 2;
                matched[index] = true;
            }
        }
        cout << "Guess: " << guess << "   Response: ";
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
        vector <string> toRemove;
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
            //cout << "Word: " << word << "  Valid: " << valid << endl;

            if (!valid) {
                toRemove.push_back(word);
            }
        }
        toRemove.push_back(guess);

        //Remove words that don't work
        for (string word : toRemove) {
            if (searchSpace.find(word) != searchSpace.end()) {
                //cout << word << endl;
                searchSpace.erase(word);
                --searchSpaceSize;
                ++numRemoved;  
            }
        }

        cout << "Optimal Word: " << guess << " Num Words Removed: " << numRemoved << endl << endl;
    }


    end = omp_get_wtime(); 
    printf("Work took %f seconds\n", end - start);

    return 0;
}