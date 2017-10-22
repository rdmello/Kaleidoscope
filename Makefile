
build: main.cpp
	clang++ -Wall -Werror -std=c++11 main.cpp `../llvm_install/bin/llvm-config --cxxflags --ldflags --system-libs --libs core` -o main.run 

clean:
	rm -f ./main.run
