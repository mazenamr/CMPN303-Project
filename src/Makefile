build:
	[ -d bin ] || mkdir bin
	gcc process_generator.c -o bin/process_generator.out
	gcc clk.c -o bin/clk.out
	gcc scheduler.c -o bin/scheduler.out
	gcc process.c -o bin/process.out
	gcc test_generator.c -o bin/test_generator.out

build-debug:
	[ -d bin ] || mkdir bin
	gcc -g process_generator.c -o bin/process_generator.out
	gcc -g clk.c -o bin/clk.out
	gcc -g scheduler.c -o bin/scheduler.out
	gcc -g process.c -o bin/process.out
	gcc -g test_generator.c -o bin/test_generator.out

clean:
	rm -rf bin

all: clean build

debug:
	./bin/test_generator.out ./bin/processes.txt 16
	gdb ./bin/process_generator.out ./bin/processes.txt 3

run:
	./bin/test_generator.out ./bin/processes.txt 16
	./bin/process_generator.out ./bin/processes.txt 3
