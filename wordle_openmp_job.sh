#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)

# Set up batch job settings
#SBATCH --job-name=wordle
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=36
#SBATCH --exclusive
#SBATCH --time=00:05:00
#SBATCH --account=eecs587f23_class
#SBATCH --partition=standard

g++ -std=c++11 -O0 -fopenmp -o wordle_openMP wordle_openMP.cpp

./wordle_openMP 512 5 MINCE > wordle_openMP_out.txt