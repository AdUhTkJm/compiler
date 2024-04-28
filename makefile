CPPFLAGS = -std=c++17 -static
SRCS = $(wildcard *.cpp)

compiler: $(SRCS)
	g++ -g -o com $(SRCS)

try: assem.s
	g++ -g -o tmp assem.s