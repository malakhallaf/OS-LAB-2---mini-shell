.PHONY: cleanup

myshell: command.o tokenizer.o
	g++ command.o tokenizer.o -o myshell

command.o: command.cc command.h
	g++ -c command.cc

tokenizer.o: tokenizer.cc tokenizer.h
	g++ -c tokenizer.cc

cleanup:
	rm -f command.o tokenizer.o	myshell child_log.txt