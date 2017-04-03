.c.o:
	g++ -std=c++11 -g -c $?

# compile client and server
all: predictors

# compile server program
predictors: clean
	g++ -std=c++11 -g -o predictors p1.cpp

clean:
	rm -f *.o predictors
