TESTOUTPUT=/tmp/a.out
WARNINGS=-Wall -Wextra -pedantic -Wpointer-arith -Wshadow -Wfloat-conversion -Wno-unused-function -Wno-unused-parameter
CXXFLAGS=-g -std=c++14 $(WARNINGS)
TEST=$(CXX) $(CXXFLAGS) -o $(TESTOUTPUT)

all: tests

tests:
	$(TEST) test-traverse.cpp test-link.cpp		&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-int-encoding.cpp			&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-picojson.cpp			&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-rapidjson.cpp -I rapidjson/include	&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-variant.cpp -I variant/include	&& $(TESTOUTPUT) >/dev/null
	$(TEST) test-lua.cpp -l lua			&& $(TESTOUTPUT) >/dev/null
