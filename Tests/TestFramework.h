#pragma once

// Minimal dependency-free test harness for AERIFORM.
//   AERIFORM_TEST(name) { ... CHECK(cond); CHECK_NEAR(a, b, tol); ... }

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace aeriform::test
{
struct TestCase
{
    const char* name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar
{
    Registrar (const char* name, std::function<void()> fn) { registry().push_back ({ name, std::move (fn) }); }
};

struct Context
{
    static int& failures() { static int f = 0; return f; }
    static int& checks()   { static int c = 0; return c; }
    static const char*& current() { static const char* n = ""; return n; }
};

inline void reportFailure (const char* expr, const char* file, int line, const std::string& detail = {})
{
    ++Context::failures();
    std::printf ("    FAIL [%s] %s:%d: %s %s\n", Context::current(), file, line, expr, detail.c_str());
}
} // namespace aeriform::test

#define AERIFORM_TEST(name)                                                              \
    static void aeriform_test_##name();                                                  \
    static aeriform::test::Registrar aeriform_registrar_##name (#name, aeriform_test_##name); \
    static void aeriform_test_##name()

#define CHECK(cond)                                                                      \
    do { ++aeriform::test::Context::checks();                                            \
         if (! (cond)) aeriform::test::reportFailure (#cond, __FILE__, __LINE__); } while (0)

#define CHECK_MSG(cond, msg)                                                             \
    do { ++aeriform::test::Context::checks();                                            \
         if (! (cond)) aeriform::test::reportFailure (#cond, __FILE__, __LINE__, std::string ("(") + (msg) + ")"); } while (0)

#define CHECK_NEAR(a, b, tol)                                                            \
    do { ++aeriform::test::Context::checks();                                            \
         const double va_ = (double) (a); const double vb_ = (double) (b);               \
         if (! (std::fabs (va_ - vb_) <= (double) (tol)))                                \
             aeriform::test::reportFailure (#a " ~= " #b, __FILE__, __LINE__,            \
                 "(" + std::to_string (va_) + " vs " + std::to_string (vb_) + ", tol " + std::to_string ((double) (tol)) + ")"); } while (0)
