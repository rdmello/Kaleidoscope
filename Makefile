
build: main.cpp
	clang++ -Wall -std=c++11 -g main.cpp `../llvm_install/bin/llvm-config --cxxflags --ldflags --system-libs --libs core` -o main.run 

clean:
	rm -f ./main.run
