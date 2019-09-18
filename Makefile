myshell: myshell.o
	gcc -std=c99 -o myshell myshell.o
myshell.o: myshell.c
	gcc -std=c99 -c myshell.c
clean: 
	rm -f myshell *.o core *~
