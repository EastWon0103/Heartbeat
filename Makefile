all: sigoutput heat recover-shell
CC = gcc

sigoutput:sigoutput.c
	gcc -o sigoutput sigoutput.c

heat:heat.c
	gcc -o heat heat.c

recover-shell:recover-shell.c 
	gcc -o recover-shell recover-shell.c

clean:
	rm -rf sigoutput heat recover-shell
