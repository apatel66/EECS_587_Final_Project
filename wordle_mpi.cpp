#include <bits/stdc++.h>
#include <mpi.h>
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
using std::endl:
using std::stoi;
using std::vector;

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
    unordered_set<string> searchSpace;
    unordered_map<string, WordInfo> words;
    vector<Task> tasks;

    // Input validation
    if (argc < 2 || argc > 3) {
        cout << "Usage: ./wordle_mpi <num_letters> <secret_word (optional)>" << endl;
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

    //Read in from file
    ifstream wordFile;
    wordFile.open("words_final/" + to_string(length) + "_letter_words.txt");
    string nextWord = "";
    while (!wordFile.eof()) {
        wordFile >> nextWord;
        words[nextWord] = {false, 0, 0};
        searchSpace.insert(nextWord);
        ++searchSpaceSize;
        answer = nextWord; //Change from last word in the list to a random one
    }
    wordFile.close();

    for (const auto &word : searchSpace) {
        tasks.push_back({0, 0, word});
    }

    int portion_size = tasks.size() / num_proc;
    int remainder = tasks.size() % num_proc;

    int start_index = id * portion_size + (id < remainder ? id : remainder);
    int end_index = start_index + portion_size + (id < remainder);

    // queue for enumerating all possible response tasks
    queue<Task> create_tasks;

    vector<Task> final_tasks;

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
        Task currTask = create_tasks.front()
        string currWord = currTask.currWord;

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

    portion_size = final_tasks.size() / num_proc;
    remainder = final_tasks.size() % num_proc;

    start_index = id * portion_size + (id < remainder ? id : remainder);
    end_index = start_index + portion_size + (id < remainder);

    // local max removed + local best word
    int maxRemoved = 0;
    string maxRemovedWord = "";

    for (int i = start_index; i < end_index; ++i) {
        Task currTask = tasks[i];
        string currWord = currTask.currWord;
        currWordInfo = words[currWord];

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
        for (int i = len-1; i >= 0; —i) {
            combo[i] = responseNumCopy / pow(3, i);
            responseNumCopy %= pow(3, i);
        }

        for (const auto &word : searchSpace) {
            bool valid = true;
            vector<bool> matched(length, false);

            for (int i = 0; i < currWord.length(); ++i) {
                if (currWord[i] == word[j] && !matched[i]) {
                    index = j;
                    break;
                }
            }

            // if curr tile is black and its in the word
            // then not valid
            if (combo[i] == 0) {
                if (index != -1) {
                    valid  = false;
                    break;
                }
            }

            // if curr tile is yellow and its in the word
            // if index is currently at where its yellow = not valid
            else if (combo[i] == 1) {
                if (index == i) {
                    valid  = false;
                    break;
                }
                matched[i] = true;
            }

            // if green and not in correct position
            // not valid
            else {
                if (index != i) {
                    valid = false;
                    break;
                }
                matched[i] = true;
            }
        }

        if (!valid) ++numWordsRemoved;

        words[currWord].searchSpaceRemoved += numWordsRemoved;
        words[currWord].numResponsesTested += 1;
        if (words[currWord].searchSpaceRemoved > maxRemoved) {
            maxRemoved = words[currWord].searchSpaceRemoved;
            maxRemovedWord = currWord;
        }
    }

    int globalMaxRemoved;
    MPI_Allreduce(&maxRemoved, &globalMaxRemoved, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    const char* char_array = maxRemovedWord.c_str();

    if (maxRemoved == globalMaxRemoved) {
        MPI_Bcast(const_cast<char*>(char_array), maxRemovedWord.size() + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    string guess(char_array);
    int comboResponse[length];
    vector<bool> matched(length, 0);
    for (int i = 0; i < guess.length(); ++i) {
        int index = -1;
        for (int j = 0; j < answer.length; ++j) {
            if (guess[i] == answer[j] && !matched[j]) {
                index = i;
                break;
            }
        }

        if (index == -1) {
            comboResponse[i] = 0;
        }
        else if (index != i) {
            comboResponse[i] = 1;
            matched[i] = true;
        }
        else {
            comboResponse[i] == 2;
            matched[i] = true;
        }
    }

    for (const auto &word : searchSpace) {
        bool valid = true;

        vector <bool> matched(length, false);
        for (int i = 0; i < currWord.length(); ++i) {
            int index = -1;
            for (int j = 0; j < word.length(); ++j) {
                if (currWord[i] == word[j] && !matched[i]) {
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

        if (!valid) {
            searchSpace.erase(word);
        }
    }

    //Finalize MPI
    MPI_Finalize();
    return 0;
}