.c.o:
	g++ -std=c++11 -g -c $?

# compile client and server
all: predictors

# compile server program
# i wish i knew how to make a makefile so this clean wouldnt have to be here
predictors: clean
	g++ -std=c++11 -g -o predictors p1.cpp

clean:
	rm -f *.o predictors
