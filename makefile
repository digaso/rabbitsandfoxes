CC=gcc
ARGS=-Wall
LINKS=-lpthread
OUTPUT=ecosystem

all:
	$(CC) $(ARGS) main.c matrix_utils.c movements.c entities.c output.c rabbitsandfoxes.c threads.c linkedlist.c -o $(OUTPUT) $(LINKS)

test-5x5: $(OUTPUT)
	@echo "=== Testing 5x5 input ==="
	@echo "Sequential:"
	@./$(OUTPUT) 0 < ecosystem_examples/input5x5 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_seq_5x5.out
	@if diff -q test_seq_5x5.out ecosystem_examples/output5x5 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_seq_5x5.out ecosystem_examples/output5x5; fi
	@echo "1 thread:"
	@./$(OUTPUT) 1 < ecosystem_examples/input5x5 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_1t_5x5.out
	@if diff -q test_1t_5x5.out ecosystem_examples/output5x5 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_1t_5x5.out ecosystem_examples/output5x5; fi
	@echo "2 threads:"
	@./$(OUTPUT) 2 < ecosystem_examples/input5x5 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_2t_5x5.out
	@if diff -q test_2t_5x5.out ecosystem_examples/output5x5 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_2t_5x5.out ecosystem_examples/output5x5; fi
	@echo "4 threads:"
	@./$(OUTPUT) 4 < ecosystem_examples/input5x5 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_4t_5x5.out
	@if diff -q test_4t_5x5.out ecosystem_examples/output5x5 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_4t_5x5.out ecosystem_examples/output5x5; fi

test-10x10: $(OUTPUT)
	@echo "=== Testing 10x10 input ==="
	@echo "Sequential:"
	@./$(OUTPUT) 0 < ecosystem_examples/input10x10 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_seq_10x10.out
	@if diff -q test_seq_10x10.out ecosystem_examples/output10x10 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_seq_10x10.out ecosystem_examples/output10x10; fi
	@echo "1 thread:"
	@./$(OUTPUT) 1 < ecosystem_examples/input10x10 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_1t_10x10.out
	@if diff -q test_1t_10x10.out ecosystem_examples/output10x10 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_1t_10x10.out ecosystem_examples/output10x10; fi
	@echo "2 threads:"
	@./$(OUTPUT) 2 < ecosystem_examples/input10x10 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_2t_10x10.out
	@if diff -q test_2t_10x10.out ecosystem_examples/output10x10 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_2t_10x10.out ecosystem_examples/output10x10; fi
	@echo "4 threads:"
	@./$(OUTPUT) 4 < ecosystem_examples/input10x10 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_4t_10x10.out
	@if diff -q test_4t_10x10.out ecosystem_examples/output10x10 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_4t_10x10.out ecosystem_examples/output10x10; fi
	@echo "8 threads:"
	@./$(OUTPUT) 8 < ecosystem_examples/input10x10 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_8t_10x10.out
	@if diff -q test_8t_10x10.out ecosystem_examples/output10x10 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_8t_10x10.out ecosystem_examples/output10x10; fi

test-20x20: $(OUTPUT)
	@echo "=== Testing 20x20 input ==="
	@echo "Sequential:"
	@./$(OUTPUT) 0 < ecosystem_examples/input20x20 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_seq_20x20.out
	@if diff -q test_seq_20x20.out ecosystem_examples/output20x20 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_seq_20x20.out ecosystem_examples/output20x20; fi
	@echo "1 thread:"
	@./$(OUTPUT) 1 < ecosystem_examples/input20x20 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_1t_20x20.out
	@if diff -q test_1t_20x20.out ecosystem_examples/output20x20 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_1t_20x20.out ecosystem_examples/output20x20; fi
	@echo "2 threads:"
	@./$(OUTPUT) 2 < ecosystem_examples/input20x20 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_2t_20x20.out
	@if diff -q test_2t_20x20.out ecosystem_examples/output20x20 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_2t_20x20.out ecosystem_examples/output20x20; fi
	@echo "4 threads:"
	@./$(OUTPUT) 4 < ecosystem_examples/input20x20 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_4t_20x20.out
	@if diff -q test_4t_20x20.out ecosystem_examples/output20x20 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_4t_20x20.out ecosystem_examples/output20x20; fi
	@echo "8 threads:"
	@./$(OUTPUT) 8 < ecosystem_examples/input20x20 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_8t_20x20.out
	@if diff -q test_8t_20x20.out ecosystem_examples/output20x20 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_8t_20x20.out ecosystem_examples/output20x20; fi
	@echo "16 threads:"
	@./$(OUTPUT) 16 < ecosystem_examples/input20x20 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_16t_20x20.out
	@if diff -q test_16t_20x20.out ecosystem_examples/output20x20 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_16t_20x20.out ecosystem_examples/output20x20; fi

test-100x100: $(OUTPUT)
	@echo "=== Testing 100x100 input ==="
	@echo "Sequential:"
	@./$(OUTPUT) 0 < ecosystem_examples/input100x100 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_seq_100x100.out
	@if diff -q test_seq_100x100.out ecosystem_examples/output100x100 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_seq_100x100.out ecosystem_examples/output100x100; fi
	@echo "1 thread:"
	@./$(OUTPUT) 1 < ecosystem_examples/input100x100 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_1t_100x100.out
	@if diff -q test_1t_100x100.out ecosystem_examples/output100x100 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_1t_100x100.out ecosystem_examples/output100x100; fi
	@echo "2 threads:"
	@./$(OUTPUT) 2 < ecosystem_examples/input100x100 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_2t_100x100.out
	@if diff -q test_2t_100x100.out ecosystem_examples/output100x100 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_2t_100x100.out ecosystem_examples/output100x100; fi
	@echo "4 threads:"
	@./$(OUTPUT) 4 < ecosystem_examples/input100x100 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_4t_100x100.out
	@if diff -q test_4t_100x100.out ecosystem_examples/output100x100 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_4t_100x100.out ecosystem_examples/output100x100; fi
	@echo "8 threads:"
	@./$(OUTPUT) 8 < ecosystem_examples/input100x100 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_8t_100x100.out
	@if diff -q test_8t_100x100.out ecosystem_examples/output100x100 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_8t_100x100.out ecosystem_examples/output100x100; fi
	@echo "16 threads:"
	@./$(OUTPUT) 16 < ecosystem_examples/input100x100 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_16t_100x100.out
	@if diff -q test_16t_100x100.out ecosystem_examples/output100x100 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_16t_100x100.out ecosystem_examples/output100x100; fi
	@echo "25 threads:"
	@./$(OUTPUT) 25 < ecosystem_examples/input100x100 | grep -v "Initial population:\|Initializing thread\|RESULTS:\|Took.*microseconds" > test_25t_100x100.out
	@if diff -q test_25t_100x100.out ecosystem_examples/output100x100 > /dev/null; then echo "PASSED"; else echo "FAILED"; diff test_25t_100x100.out ecosystem_examples/output100x100; fi

test: test-5x5 test-10x10 test-20x20 test-100x100
	@rm -f test_*.out

clean:
	rm -f *.o $(OUTPUT)