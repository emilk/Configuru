#pragma once

#include <functional>
#include <iostream>
#include <string>

#define LOGURU_WITH_STREAMS 1
#include <loguru.hpp>

const std::string PASS_STRING = std::string(loguru::terminal_green()) + "PASS: " + loguru::terminal_reset();
const std::string FAIL_STRING = std::string(loguru::terminal_red())   + "FAIL: " + loguru::terminal_reset();

class Tester
{
public:
	void on_test(bool did_pass, const char* filename, unsigned line, const std::string& test_name, const std::string& extra = "")
	{
		if (!did_pass)
		{
			std::cout << std::endl << filename << ":" << line << "  " << (did_pass ? PASS_STRING : FAIL_STRING) << test_name;
			if (extra != "")
			{
				std::cout << ": " << extra;
			}
			std::cout << std::endl << std::endl;
		}

		_num_run += 1;

		if (!did_pass)
		{
			_num_failed += 1;
		}
	}

	void print_results_and_exit() __attribute__((noreturn))
	{
		if (_num_failed == 0) {
			printf("%s%lu/%lu tests passed!%s\n", loguru::terminal_green(), _num_run, _num_run, loguru::terminal_reset());
		} else {
			printf("%s%lu/%lu tests failed.%s\n", loguru::terminal_red(), _num_failed, _num_run, loguru::terminal_reset());
		}
		printf("\n\n");
		fflush(stdout);
		std::exit(_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
	}

private:
	size_t _num_run    = 0;
	size_t _num_failed = 0;
};

static Tester s_tester;

inline void test_code(const char* filename, unsigned line, const std::string& test_name, bool should_pass, std::function<void()> code)
{
	try {
		code();

		if (should_pass) {
			s_tester.on_test(true, filename, line, test_name);
		} else {
			s_tester.on_test(false, filename, line, test_name, "Should not have parsed");
		}
	} catch (std::exception& e) {
		if (should_pass) {
			s_tester.on_test(false, filename, line, test_name, e.what());
		} else {
			s_tester.on_test(true, filename, line, test_name, e.what());
		}
	}
}

#define TEST_PASS(message) s_tester.on_test(true, __FILE__, __LINE__, message)
#define TEST_FAIL(message) s_tester.on_test(false, __FILE__, __LINE__, message)
#define TEST_FAIL2(message, extra) s_tester.on_test(false, __FILE__, __LINE__, message, extra)

#define TEST(expr)                             \
	do {                                       \
		if (expr) {                            \
			s_tester.on_test(true, __FILE__, __LINE__, #expr);        \
		} else {                               \
			s_tester.on_test(false, __FILE__, __LINE__, #expr);        \
		}                                      \
	} while (0)

#define TEST_EQ(a, b) TEST((a) == (b))

#define TEST_NOTHROW(expr)                     \
	do {                                       \
		try {                                  \
			expr;                              \
			s_tester.on_test(true, __FILE__, __LINE__, #expr);                  \
		} catch (std::exception& e) {          \
			s_tester.on_test(false, __FILE__, __LINE__, #expr, e.what());       \
		} catch (...) {                        \
			s_tester.on_test(false, __FILE__, __LINE__, #expr);                  \
		}                                      \
	} while (0)


#define TEST_THROW(expr, exception_type)       \
	do {                                       \
		try {                                  \
			expr;                              \
			s_tester.on_test(false, __FILE__, __LINE__, #expr);                  \
		} catch (exception_type& e) {          \
			s_tester.on_test(true, __FILE__, __LINE__, #expr, e.what());         \
		} catch (...) {                        \
			s_tester.on_test(false, __FILE__, __LINE__, #expr);                  \
		}                                      \
	} while (0)
