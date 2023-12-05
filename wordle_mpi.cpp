#include <bits/stdc++.h>
#include <mpi.h>
#include <ctime>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <vector>

using std::unordered_set;
using std::unordered_map;
using std::string;
using std::ifstream;
using std::queue;
using std::cout;
using std::endl;
using std::stoi;
using std::vector;

struct Task {
    int response;
    int responseLength;
    string currWord;
};

struct WordInfo {
    bool inSearchSpace;
    int searchSpaceRemoved;
    int numResponsesTested;
};

int main(int argc, char** argv) {
    //Initialize the MPI environment.
    MPI_Init(&argc, &argv);

    // Obtain processor id and world size(number of processors)
    int num_proc;
    int id;

    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    int searchSpaceSize = 0;
    string answer;
    int length;
    int maxResponseNum;
    int listSize;
    unordered_set<string> searchSpace;
    unordered_map<string, WordInfo> words;
    vector<Task> tasks;

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
    if (id == 0) {
        cout << "Answer: " << answer << endl << endl;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    // queue for enumerating all possible response tasks
    queue<Task> create_tasks;
    vector<Task> final_tasks;

    int maxRemoved = 0;
    string maxRemovedWord = "";

    for (const auto &word : searchSpace) {
        maxRemoved = 0;
        maxRemovedWord = "";
        words[word] = {0, 0};
        tasks.push_back({0, 0, word});
    }

    int portion_size = tasks.size() / num_proc;
    int remainder = tasks.size() % num_proc;

    int start_index = id * portion_size + (id < remainder ? id : remainder);
    int end_index = start_index + portion_size + (id < remainder);

    for (int i = start_index; i < end_index; ++i) {
        Task currTask = tasks[i];
        string currWord = currTask.currWord;

        int responseNum = currTask.response;
        int responseLength = currTask.responseLength;
        if (responseLength < length) {
            create_tasks.push({responseNum*3 + 0, responseLength+1, currWord});
            create_tasks.push({responseNum*3 + 1, responseLength+1, currWord});
            create_tasks.push({responseNum*3 + 2, responseLength+1, currWord});
        }
    }

    while (!create_tasks.empty()) {
        Task currTask = create_tasks.front();
        string currWord = currTask.currWord;

        create_tasks.pop();

        int responseNum = currTask.response;
        int responseLength = currTask.responseLength;
        if (responseLength < length) {
            create_tasks.push({responseNum*3 + 0, responseLength+1, currWord});
            create_tasks.push({responseNum*3 + 1, responseLength+1, currWord});
            create_tasks.push({responseNum*3 + 2, responseLength+1, currWord});
        } else if (responseLength == length) {
            final_tasks.push_back(currTask);
        }
    }

    for (int i = 0; i < final_tasks.size(); ++i) {
        Task currTask = final_tasks[i];
        string currWord = currTask.currWord;
        WordInfo currWordInfo = words[currWord];

        int numResponsesLeft = maxResponseNum - currWordInfo.numResponsesTested;
        int theoreticalMax = currWordInfo.searchSpaceRemoved + numResponsesLeft * searchSpaceSize;
        
        if (theoreticalMax < maxRemoved) {
            continue;
        }

        int responseNum = currTask.response;
        int responseLength = currTask.responseLength;
        
        int numWordsRemoved = 0;
        int combo[length];
        int responseNumCopy = responseNum;
        for (int i = length-1; i >= 0; --i) {
            combo[i] = responseNumCopy / pow(3, i);
            responseNumCopy %= (int) pow(3, i);
        }

        for (const auto &word : searchSpace) {
            bool valid = true;
            vector<bool> matched(length, false);

            for (int i = 0; i < currWord.length(); ++i) {
                int index = -1;
                for (int j = 0; j < word.length(); ++j) {
                    if (currWord[i] == word[j] && !matched[j]) {
                        index = j;
                        break;
                    }
                }
                if (combo[i] == 0) {
                    if (index != -1) {
                        valid  = false;
                        break;
                    }
                }

                else if (combo[i] == 1) {
                    if (index == i) {
                        valid  = false;
                        break;
                    }
                    matched[i] = true;
                }

                else {
                    if (index != i) {
                        valid = false;
                        break;
                    }
                    matched[i] = true;
                }
            }
            if (!valid) ++numWordsRemoved;
        }

        words[currWord].searchSpaceRemoved += numWordsRemoved;
        words[currWord].numResponsesTested += 1;
        if (words[currWord].searchSpaceRemoved > maxRemoved) {
            maxRemoved = words[currWord].searchSpaceRemoved;
            maxRemovedWord = currWord;
        }
    }

    cout << "Guess: " << maxRemovedWord << " max removed = " << maxRemoved << " pid = " << id << endl;

    int globalMaxRemoved;
    MPI_Allreduce(&maxRemoved, &globalMaxRemoved, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    if (maxRemoved == globalMaxRemoved) {
        MPI_Bcast(const_cast<char*>(maxRemovedWord.c_str()), maxRemovedWord.size() + 1, MPI_CHAR, id, MPI_COMM_WORLD);
    } else {
        MPI_Bcast(nullptr, 0, MPI_CHAR, MPI_ROOT, MPI_COMM_WORLD);
        char* receivedWordBuffer = new char[maxRemovedWord.size() + 1];
        MPI_Bcast(receivedWordBuffer, maxRemovedWord.size() + 1, MPI_CHAR, MPI_ROOT, MPI_COMM_WORLD);
        std::string receivedWord(receivedWordBuffer);
        delete[] receivedWordBuffer;
        maxRemovedWord = receivedWord;
    }

    // string guess(char_array);

    cout << maxRemovedWord << endl;

    // int comboResponse[length];
    // vector<bool> matched(length, false);
    // for (int i = 0; i < guess.length(); ++i) {
    //     int index = -1;
    //     for (int j = 0; j < answer.length(); ++j) {
    //         if (guess[i] == answer[j] && !matched[j]) {
    //             index = i;
    //             break;
    //         }
    //     }

    //     if (index == -1) {
    //         comboResponse[i] = 0;
    //     }
    //     else if (index != i) {
    //         comboResponse[i] = 1;
    //         matched[i] = true;
    //     }
    //     else {
    //         comboResponse[i] == 2;
    //         matched[i] = true;
    //     }
    // }

    // if (id == 0) {
    //     cout << "Guess: " << guess << "   Response: ";
    //     for (int i = 0; i < length; ++i) {
    //         if (comboResponse[i] == 0) {
    //             cout << "B";
    //         }
    //         else if (comboResponse[i] == 1) {
    //             cout << "Y";
    //         }
    //         else {
    //             cout << "G";
    //         }
    //     }
    //     cout << endl;
    // }

    // MPI_Barrier(MPI_COMM_WORLD);

    // for (const auto &word : searchSpace) {
    //     bool valid = true;

    //     vector <bool> matched(length, false);
    //     for (int i = 0; i < guess.length(); ++i) {
    //         int index = -1;
    //         for (int j = 0; j < word.length(); ++j) {
    //             if (guess[i] == word[j] && !matched[i]) {
    //                 index = j;
    //                 break;
    //             }
    //         }

    //         if (comboResponse[i] == 0) {
    //             if (index != -1) {
    //                 valid  = false;
    //                 break;
    //             }
    //         }
    //         else if (comboResponse[i] == 1) {
    //             if (index == i) {
    //                 valid  = false;
    //                 break;
    //             }
    //             matched[i] = true;
    //         }
    //         else {
    //             if (index != i) {
    //                 valid = false;
    //                 break;
    //             }
    //             matched[i] = true;
    //         }
    //     }

    //     if (!valid) {
    //         searchSpace.erase(word);
    //     }
    // }

    //Finalize MPI
    MPI_Finalize();
    return 0;
}