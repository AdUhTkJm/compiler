CPPFLAGS = -std=c++17 -static
SRCS = $(wildcard *.cpp)

compiler: $(SRCS)
	g++ -o com $(SRCS)

try: assem.s
	g++ -o tmp assem.s