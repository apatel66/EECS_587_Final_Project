import random
import sys
import os

for i in range(4, 10):
    word_list = []
    input_filename = "words_raw/" + str(i) + "_letter_words.txt"
    with open(input_filename, "r") as in_file:
        for line in in_file:
            for word in line.split():
                word_list.append(word)

    rand_words = []
    list_sizes = [125, 128, 250, 256, 500, 512, 1000, 1024, 2000, 2048, 3000, 3300]

    for size in list_sizes:
        # Keep adding to rand_words
        while len(rand_words) < size:
            next_word = random.choice(word_list)
            if next_word not in rand_words:
                rand_words.append(next_word)
                word_list.remove(next_word)

        # Sort rand words
        rand_words.sort()

        # output to file
        path_name = "words_final_" + str(size) + "/"
        if not os.path.isdir(path_name):
            os.mkdir(path_name, 0o777)

        output_filename = path_name + str(i) + "_letter_words.txt"
        with open(output_filename, "w") as out_file:
            for word in rand_words:
                out_file.write(word.upper() + "\n")
