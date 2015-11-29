#include <iostream>

#include <boost/filesystem.hpp>

#define CONFIGURU_IMPLEMENTATION 1
#include "../configuru.hpp"

#define LOGURU_IMPLEMENTATION 1
#include "loguru.hpp"

namespace fs = boost::filesystem;

std::vector<fs::path> list_files(fs::path directory, std::string extension)
{
    std::vector<fs::path> result;
    fs::directory_iterator it(directory);
    fs::directory_iterator end;
    for (; it != end; ++it) {
        if (fs::is_regular_file(it->status()) && fs::extension(*it) == extension) {
            result.push_back(it->path());
        }
    }
    return result;
}

void test(ParseOptions options, bool should_pass, fs::path dir, std::string extension, size_t& num_run, size_t& num_failed)
{
	for (auto path : list_files(dir, extension))
	{
		try {
			auto config = configuru::parse_config_file(path.string(), options);
			if (should_pass) {
				std::cout << loguru::terminal_green() << "PASS: " << loguru::terminal_reset() << path.filename() << std::endl;
			} else {
				std::cout <<loguru::terminal_red() <<  "SHOULD NOT HAVE PARSED: " << loguru::terminal_reset() << path.filename() << std::endl;
				num_failed += 1;
			}
		} catch (std::exception& e) {
			if (should_pass) {
				std::cout << loguru::terminal_red() << "FAILED: " << loguru::terminal_reset() << path.filename() << ": " << e.what() << std::endl << std::endl;
				num_failed += 1;
			} else {
				std::cout << loguru::terminal_green() << "PASS: " << loguru::terminal_reset() << path.filename() << ": " << e.what() << std::endl << std::endl;
			}
		}
		num_run += 1;
	}
}

const char* TEST_CFG = R"(
pi:   3.14,
list: [1 2 3 4]
obj {
	nested_value: 42
}
)";

int main()
{
	size_t num_run = 0;
	size_t num_failed = 0;
	std::cout << "JSON expected to pass:" << std::endl;;
	test(configuru::JSON, true, "../../test_suite/json_pass",      ".json", num_run, num_failed);
	test(configuru::JSON, true, "../../test_suite/json_only_pass", ".json", num_run, num_failed);
	test(configuru::JSON, true, "../../test_suite/cfg_only_fail",  ".cfg", num_run, num_failed);

	std::cout << std::endl << "JSON expected to fail:" << std::endl;;
	test(configuru::JSON, false, "../../test_suite/json_fail", ".json", num_run, num_failed);
	test(configuru::JSON, false, "../../test_suite/cfg_pass",  ".cfg", num_run, num_failed);
	test(configuru::JSON, false, "../../test_suite/cfg_fail",  ".cfg", num_run, num_failed);

	std::cout << std::endl << "CFG expected to pass:" << std::endl;;
	test(configuru::CFG, true,  "../../test_suite/json_pass", ".json", num_run, num_failed);
	test(configuru::CFG, true,  "../../test_suite/cfg_pass",  ".cfg", num_run, num_failed);

	std::cout << std::endl << "CFG expected to fail:" << std::endl;;
	test(configuru::CFG, false, "../../test_suite/json_only_pass", ".json", num_run, num_failed);
	test(configuru::CFG, false, "../../test_suite/json_fail",      ".json", num_run, num_failed);
	test(configuru::CFG, false, "../../test_suite/cfg_fail",       ".cfg", num_run, num_failed);
	test(configuru::CFG, false, "../../test_suite/cfg_only_fail",  ".cfg", num_run, num_failed);
	std::cout << std::endl << std::endl;

	if (num_failed == 0) {
		printf("%s%lu/%lu tests passed!%s\n", loguru::terminal_green(), num_run, num_run, loguru::terminal_reset());
	} else {
		printf("%s%lu/%lu tests failed.%s\n", loguru::terminal_red(), num_failed, num_run, loguru::terminal_reset());
	}
	printf("\n\n");
	fflush(stdout);

	auto cfg = configuru::parse_config(TEST_CFG, ParseOptions(), "test_cfg");
	std::cout << "pi: " << (float)cfg["pi"] << std::endl;
	cfg.check_dangling();

	std::cout << std::endl;
	std::cout << "// sjson:" << std::endl;
	configuru::FormatOptions options;
	std::cout << configuru::write_config(cfg, options);

	std::cout << std::endl;
	std::cout << "// Json:" << std::endl;
	options.json = true;
	std::cout << configuru::write_config(cfg, options);
}
