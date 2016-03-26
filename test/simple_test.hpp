#pragma once

#include <functional>
#include <iostream>
#include <string>

#include <loguru.hpp>

const std::string PASS_STRING = std::string(loguru::terminal_green()) + "PASS: " + loguru::terminal_reset();
const std::string FAIL_STRING = std::string(loguru::terminal_red())   + "FAIL: " + loguru::terminal_reset();

class Tester
{
public:
	void print_pass(const std::string& test_name)
	{
		// std::cout << PASS_STRING << test_name << std::endl;
		(void)test_name;
		_num_run += 1;
	}

	void print_pass(const std::string& test_name, const std::string& extra)
	{
		// std::cout << PASS_STRING << test_name << ": " << extra << std::endl << std::endl;
		(void)test_name; (void)extra;
		_num_run += 1;
	}

	void print_fail(const std::string& test_name)
	{
		std::cout << FAIL_STRING << test_name << std::endl;
		_num_run += 1;
		_num_failed += 1;
	}

	void print_fail(const std::string& test_name, const std::string& extra)
	{
		std::cout << FAIL_STRING << test_name << ": " << extra << std::endl << std::endl;
		_num_run += 1;
		_num_failed += 1;
	}

	void print_results()
	{
		if (_num_failed == 0) {
			printf("%s%lu/%lu tests passed!%s\n", loguru::terminal_green(), _num_run, _num_run, loguru::terminal_reset());
		} else {
			printf("%s%lu/%lu tests failed.%s\n", loguru::terminal_red(), _num_failed, _num_run, loguru::terminal_reset());
		}
		printf("\n\n");
		fflush(stdout);
	}

private:
	size_t _num_run    = 0;
	size_t _num_failed = 0;
};

static Tester s_tester;

inline void test_code(const std::string& test_name, bool should_pass, std::function<void()> code)
{
	try {
		code();

		if (should_pass) {
			s_tester.print_pass(test_name);
		} else {
			s_tester.print_fail(test_name, "Should not have parsed");
		}
	} catch (std::exception& e) {
		if (should_pass) {
			s_tester.print_fail(test_name, e.what());
		} else {
			s_tester.print_pass(test_name, e.what());
		}
	}
}

#define TEST_PASS(message) s_tester.print_pass(message)
#define TEST_FAIL(message) s_tester.print_fail(message)
#define TEST_FAIL2(message, extra) s_tester.print_fail(message, extra)

#define TEST(expr)                             \
	do {                                       \
		if (expr) {                            \
			s_tester.print_pass(#expr);        \
		} else {                               \
			s_tester.print_fail(#expr);        \
		}                                      \
	} while (0)

#define TEST_EQ(a, b) TEST((a) == (b))

#define TEST_NOTHROW(expr)                     \
	do {                                       \
		try {                                  \
			expr;                              \
			TEST_PASS(#expr);                  \
		} catch (std::exception& e) {          \
			TEST_FAIL2(#expr, e.what());       \
		} catch (...) {                        \
			TEST_FAIL(#expr);                  \
		}                                      \
	} while (0)
