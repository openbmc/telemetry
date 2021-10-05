CC=g++
CFLAGS=-I. -std=c++20

example: main.o
	$(CC) -o example main.o

main.o: main.cpp add_report_params.hpp enum_conversion.hpp
	$(CC) $(CFLAGS) -c main.cpp
