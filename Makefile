TESTOUTPUT=/tmp/test-traverse
WARNINGS=-Wall -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wno-unused-function -Wno-unused-parameter
# SANITIZE=-fsanitize=address
# SANITIZE=-fsanitize=undefined
CXXFLAGS=-g -std=c++17 $(shell pkg-config --cflags lua) $(WARNINGS) $(SANITIZE)
TEST=$(CXX) $(CXXFLAGS) -o $(TESTOUTPUT) $(shell pkg-config --libs lua)

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
fuzz-tests: /tmp/fuzz-test
	mkdir -p /tmp/fuzz-input /tmp/fuzz-output
	$(TEST) fuzz-gen.cpp && $(TESTOUTPUT)
	AFL_ALLOW_TMP=1 afl-cmin -i /tmp/fuzz-input -o /tmp/fuzz-input-min -- /tmp/fuzz-test
	afl-fuzz -m 20 -i /tmp/fuzz-input-min -o /tmp/fuzz-output /tmp/fuzz-test

# Check all potential hangs reported by AFL to see if they're false alarms
run-fuzz-hangs: /tmp/fuzz-test
	for hang in /tmp/fuzz-output/hangs/*; do ( ulimit -Sv 1000000; /tmp/fuzz-test < $$hang ) done

/tmp/fuzz-test: fuzz-test.cpp $(wildcard *.h) Makefile
	AFL_HARDEN=1 afl-clang++ $(CXXFLAGS) fuzz-test.cpp -o $@
