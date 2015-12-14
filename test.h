template <typename A, typename B>
void _test_equal(const A& a, const B& b, const char* file, int line) {
  if (a != b) {
    std::cerr << " * FAIL " << file << ':' << std::dec << line << ": (" << std::hex << a << ") != (" << std::hex << b << ")" << std::endl;
  } else {
    std::cerr << "   PASS " << file << ':' << std::dec << line << ": (" << std::hex << a << ")" << std::endl;
  }
}
#define TEST_EQ(a, b) _test_equal(a, b, __FILE__, __LINE__)
