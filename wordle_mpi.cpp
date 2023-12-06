#include <bits/stdc++.h>
#include <mpi.h>
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
    //Initialize the MPI environment.
    MPI_Init(&argc, &argv);

    // Obtain processor id and world size(number of processors)
    int NUM_PROC;
    int ID;

    MPI_Comm_size(MPI_COMM_WORLD, &NUM_PROC);
    MPI_Comm_rank(MPI_COMM_WORLD, &ID);

    //Timer
    MPI_Barrier(MPI_COMM_WORLD);
    double startTime, endTime;
    double startPartTime, endPartTime, totalTime;
    if (ID == 0) {
        startTime = MPI_Wtime();
    }

    double maxCorrectness = 1.0;
    int searchSpaceSize = 0;
    int maxRemoved = 0;
    string maxRemovedWord = "ZZZZZ";
    string answer = "";
    int length = 0;
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

    vector<string> searchSpace;
    unordered_map<string, WordInfo> words;

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
    if (ID == 0) {
        cout << "Answer: " << answer << endl << endl;
    }

    while (!answerFound) {
        int portion_size = searchSpace.size() / NUM_PROC;
        int remainder = searchSpace.size() % NUM_PROC;
        
        int start_index = ID * portion_size + (ID < remainder ? ID : remainder);
        int end_index = start_index + portion_size + (ID < remainder);
        int numWords = end_index - start_index + 1;

        maxRemoved = 0;
        maxRemovedWord = "ZZZZZ";

        // Create an empty map of words
        for (int i = start_index; i < end_index; ++i) {
            words[searchSpace[i]] = {0, 0};
        }

        MPI_Barrier(MPI_COMM_WORLD);
        if (ID == 0) {
            startPartTime = MPI_Wtime();
        }

        // Go through all words in the parition
        for (int i = start_index; i < end_index; ++i) {
            string currWord = searchSpace[i];
            for (int j = 0; j < searchSpace.size(); ++j) {
                string testWord = searchSpace[j];
                int numRemoved = 0;

                //Test Theoretical Max
                int numResponsesLeft = searchSpace.size() - j + 1;
                int theoreticalMax = words[currWord].searchSpaceRemoved + (numResponsesLeft * searchSpace.size() * maxCorrectness);
                if (theoreticalMax < maxRemoved) {
                    //cout << "STOPPED Checking: " << currWord << " at " << j << "/" << searchSpace.size() << endl;
                    break;
                }

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
                        ++numRemoved;
                    }
                }

                // Update the number of words removed
                words[currWord].searchSpaceRemoved += numRemoved;
                words[currWord].numResponsesTested += 1;
            }

            // Update the optimal word if there's a new max
            if (words[currWord].searchSpaceRemoved > maxRemoved ||
                (words[currWord].searchSpaceRemoved == maxRemoved && currWord.compare(maxRemovedWord) < 0)) {
                maxRemoved = words[currWord].searchSpaceRemoved;
                maxRemovedWord = currWord;
            }
        }
        //We have our local max now

        //Communicate to find the global best word 
        MPI_Barrier(MPI_COMM_WORLD);
        //cout << "Guess: " << maxRemovedWord << " max removed = " << maxRemoved << " pid = " << ID << endl;
        int globalMaxRemoved;
        MPI_Allreduce(&maxRemoved, &globalMaxRemoved, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

        int maxCountProcessor;
        int globalMaxProcessorId;
        if (maxRemoved == globalMaxRemoved) {
            maxCountProcessor = ID;
        } else {
            maxCountProcessor = 999999;
        }
        MPI_Allreduce(&maxCountProcessor, &globalMaxProcessorId, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

        char guess_buffer[length];
        if (ID == globalMaxProcessorId) {
            std::strcpy(guess_buffer, maxRemovedWord.c_str());
        }
        MPI_Bcast(guess_buffer, length + 1, MPI_CHAR, globalMaxProcessorId, MPI_COMM_WORLD);
        string guess(guess_buffer);

        //cout << "best word = " << guess << endl;
        MPI_Barrier(MPI_COMM_WORLD);
        if (ID == 0) {
            endPartTime = MPI_Wtime();
            totalTime += (endPartTime - startPartTime);
        }

        //Check the guess and get the response
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
        if (ID == 0){
            cout << "Guess: " << guess << endl;
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
            /*cout << "New search space: " << searchSpace.size() << endl;
            for(string word : searchSpace) {
                cout << word << endl;
            }*/
        }

    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (ID == 0) {
        endTime = MPI_Wtime();
        cout << "Parallel work took " << totalTime << " seconds" << endl;
        cout << "Entire Program took " << endTime-startTime << " seconds" << endl;
    }

    //Finalize MPI
    MPI_Finalize();
    return 0;
}