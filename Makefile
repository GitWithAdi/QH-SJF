CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?= -lm

SERVER_BIN = bin/qhsjf-server
TEST_BIN   = bin/test_runner

# Standalone tests
TP  = bin/test_process
TPO = bin/test_policies
TSI = bin/test_simulator
TM  = bin/test_metrics
TQ  = bin/test_qhsjf
TSM = bin/test_sim

all: $(SERVER_BIN) $(TEST_BIN)

$(SERVER_BIN): src/server.c src/sim.c src/workload.c src/tests.c include/sim.h include/workload.h include/tests.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ src/server.c src/sim.c src/workload.c src/tests.c $(LDFLAGS)

$(TEST_BIN): src/main_test.c src/tests.c src/sim.c src/workload.c include/sim.h include/workload.h include/tests.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ src/main_test.c src/tests.c src/sim.c src/workload.c $(LDFLAGS)

tests-standalone: $(TP) $(TPO) $(TSI) $(TM) $(TQ) $(TSM)
	./$(TP)
	./$(TPO)
	./$(TSI)
	./$(TM)
	./$(TQ)
	./$(TSM)

$(TP): tests/test_process.c src/sim.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TPO): tests/test_policies.c src/sim.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TSI): tests/test_simulator.c src/sim.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TM): tests/test_metrics.c src/sim.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TQ): tests/test_qhsjf.c src/sim.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TSM): tests/test_sim.c src/sim.c src/workload.c
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_BIN) tests-standalone
	./$(TEST_BIN)

run: $(SERVER_BIN)
	./$(SERVER_BIN) 5173

clean:
	rm -rf bin *.o *.exe

.PHONY: all test tests-standalone run clean
