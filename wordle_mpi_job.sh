#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)
# Set up batch job settings
#SBATCH --job-name=mpi_matrix
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=32
#SBATCH --mem-per-cpu=1g
#SBATCH --time=00:05:00
#SBATCH --account=eecs587f23_class
#SBATCH --partition=standard
mpirun -np 32 --bind-to core:overload-allowed ./wordle_mpi 2000 5 STORE > wordle_mpi_out.txt
