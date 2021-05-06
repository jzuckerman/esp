run the gen.sh shell script, it will ask the number of rows for the matrix that you want to generate.
It then generates the input.txt and output.txt files which are then used in Testbench for simulation.
gen.sh invokes a python script "datagen.py" which generates a random matrix of "user input rows" and then performs cholesky factorization and generates the golden output file.
