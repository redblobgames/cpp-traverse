TESTOUTPUT=/tmp/test-traverse
WARNINGS=-Wall -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wno-unused-function -Wno-unused-parameter
CXXFLAGS=-g -std=c++14 $(WARNINGS)
TEST=$(CXX) $(CXXFLAGS) -o $(TESTOUTPUT)

all: tests

tests:
	$(TEST) test-traverse.cpp test-link.cpp			&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-int-encoding.cpp				&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-picojson.cpp				&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-rapidjson.cpp -I rapidjson/include		&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-variant.cpp -I variant/include		&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-picojson-variant.cpp -I variant/include	&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-lua.cpp -l lua				&& $(TESTOUTPUT) >/dev/null

# Run fuzz tests using AFL
fuzz-tests: build/fuzz-test
	mkdir -p fuzz-input fuzz-output
	$(TEST) fuzz-gen.cpp && $(TESTOUTPUT)
	afl-fuzz -m 5 -i fuzz-input -o fuzz-output build/fuzz-test

# Check all potential hangs reported by AFL to see if they're false alarms
run-fuzz-hangs:
	for hang in fuzz-output/hangs/*; do ( ulimit -Sv 1000000; build/fuzz-test < $$hang ) done

build/fuzz-test: fuzz-test.cpp $(wildcard *.h) Makefile
	mkdir -p build
	AFL_HARDEN=1 afl-clang++ $(CXXFLAGS) fuzz-test.cpp -o build/fuzz-test

clean:
	rm -rf build fuzz-input fuzz-output
