
build: main.cpp
	clang++ -Wall -Werror -std=c++11 main.cpp -o main.run

clean:
	rm -f ./main.run
