
build: main.cpp
	clang++ -Wall -Werror main.cpp -o main.run

clean:
	rm -f ./main.run
