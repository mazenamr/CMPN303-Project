COUNT?=4
SCH?=1
MEM?=1

build:
	gcc process_generator.c -o scheduler.o
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

build-debug:
	gcc -g process_generator.c -o scheduler.o
	gcc -g clk.c -o clk.out
	gcc -g scheduler.c -o scheduler.out
	gcc -g process.c -o process.out
	gcc -g test_generator.c -o test_generator.out

clean:
	rm -rf *.out
	rm -rf *.o

all: clean build

debug:
	./test_generator.out ./processes.txt $(COUNT)
	gdb ./scheduler.o ./processes.txt $(SCH) $(MEM)

run:
	./test_generator.out ./processes.txt $(COUNT)
	./scheduler.o ./processes.txt $(SCH) $(MEM)

run-no-gen:
	./scheduler.o ./processes.txt $(SCH) $(MEM)
