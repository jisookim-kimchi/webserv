#pragma once

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

inline std::string projectRoot() {
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer)) == nullptr)
        return ".";
    return buffer;
}

inline void testFail(const std::string& message) {
    throw std::runtime_error(message);
}

inline void expectTrue(bool condition, const std::string& message) {
    if (!condition)
        testFail(message);
}

template <typename A, typename B>
void expectEq(const A& actual, const B& expected, const std::string& message) {
    if (!(actual == expected)) {
        std::ostringstream oss;
        oss << message << " (expected=" << expected << ", actual=" << actual << ')';
        testFail(oss.str());
    }
}

inline void expectContains(const std::string& haystack, const std::string& needle,
                           const std::string& message) {
    if (haystack.find(needle) == std::string::npos)
        testFail(message + " (missing `" + needle + "`)");
}

#define EXPECT_TRUE(cond) expectTrue(static_cast<bool>(cond), std::string(#cond) + " is false")

#define EXPECT_EQ(actual, expected) \
    expectEq((actual), (expected), std::string(#actual) + " != " + #expected)

#define EXPECT_CONTAINS(haystack, needle) \
    expectContains((haystack), (needle), std::string(#haystack) + " missing " + #needle)

inline int runTest(const char* name, void (*fn)()) {
    try {
        fn();
        std::cout << "  PASS  " << name << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cout << "  FAIL  " << name << "\n         " << ex.what() << '\n';
        return 1;
    }
}
