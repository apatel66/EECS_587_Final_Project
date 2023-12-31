import random
import sys
import os

word_list = []
rand_words = []
MAX_WORDS = 3300

if len(sys.argv) > 1:
    MAX_WORDS = int(sys.argv[1])

for i in range(4, 10):
    word_list = []
    rand_words = []

    input_filename = "words_raw/" + str(i) + "_letter_words.txt"
    with open(input_filename, "r") as in_file:
        for line in in_file:
            for word in line.split():
                word_list.append(word)

    rand_words = random.sample(word_list, MAX_WORDS)
    rand_words.sort()

    path_name = "words_final_" + str(MAX_WORDS) + "/"
    if not os.path.isdir(path_name):
        os.mkdir(path_name, 0o777)

    output_filename = path_name + str(i) + "_letter_words.txt"
    with open(output_filename, "w") as out_file:
        for word in rand_words:
            out_file.write(word.upper() + "\n")
