#include <iostream>

#include <boost/filesystem.hpp>

#define CONFIGURU_IMPLEMENTATION 1
#include "../configuru.hpp"

// #define LOGURU_IMPLEMENTATION 1
// #include "loguru.hpp"

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

void test_json_pass()
{
	for (auto path : list_files("../../json_test_suite/pass", ".json"))
	{
		try {
			auto config = configuru::parse_config_file(path.string(), configuru::JSON);
			std::cout << "PASS: " << path.filename() << std::endl;
		} catch (std::exception& e) {
			std::cout << "FAILED: " << path.filename() << ": " << e.what() << std::endl;
		}
	}
}

void test_json_fail()
{
	for (auto path : list_files("../../json_test_suite/fail", ".json"))
	{
		try {
			auto config = configuru::parse_config_file(path.string(), configuru::JSON);
			std::cout << "SHOULD NOT HAVE SUCCEEDED: " << path.filename() << std::endl;
		} catch (std::exception& e) {
			std::cout << "PASS: " << path.filename() << ": " << e.what() << std::endl;
		}
	}
}

const char* TEST_CFG = R"(
pi:   3.14,
list: [1 2 3 4]
obj {
	nested_value = 42
}
)";

int main()
{
	test_json_pass();
	test_json_fail();

	auto cfg = configuru::parse_config(TEST_CFG, ParseOptions(), "test_cfg");
	std::cout << "pi: " << (float)cfg["pi"] << std::endl;
	cfg.check_dangling();

	std::cout << "sjson:" << std::endl;
	configuru::FormatOptions options;
	std::cout << configuru::write_config(cfg, options);

	std::cout << "Json:" << std::endl;
	options.json = true;
	std::cout << configuru::write_config(cfg, options);
}
