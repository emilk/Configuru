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

void test(ParseOptions options, bool should_pass, fs::path dir, std::string extension)
{
	for (auto path : list_files(dir, extension))
	{
		try {
			auto config = configuru::parse_config_file(path.string(), options);
			if (should_pass) {
				std::cout << "PASS: " << path.filename() << std::endl;
			} else {
				std::cout << "SHOULD NOT HAVE PARSED: " << path.filename() << std::endl;
			}
		} catch (std::exception& e) {
			if (should_pass) {
				std::cout << "FAILED: " << path.filename() << ": " << e.what() << std::endl;
			} else {
				std::cout << "PASS: " << path.filename() << ": " << e.what() << std::endl;
			}
		}
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
	std::cout << "JSON expected to pass:" << std::endl;;
	test(configuru::JSON, true, "../../test_suite/json_pass",      ".json");
	test(configuru::JSON, true, "../../test_suite/json_only_pass", ".json");
	test(configuru::JSON, true, "../../test_suite/cfg_only_fail",  ".cfg");

	std::cout << std::endl << "JSON expected to fail:" << std::endl;;
	test(configuru::JSON, false, "../../test_suite/json_fail", ".json");
	test(configuru::JSON, false, "../../test_suite/cfg_pass",  ".cfg");
	test(configuru::JSON, false, "../../test_suite/cfg_fail",  ".cfg");

	std::cout << std::endl << "CFG expected to pass:" << std::endl;;
	test(configuru::CFG, true,  "../../test_suite/json_pass", ".json");
	test(configuru::CFG, true,  "../../test_suite/cfg_pass",  ".cfg");

	std::cout << std::endl << "CFG expected to fail:" << std::endl;;
	test(configuru::CFG, false, "../../test_suite/json_only_pass", ".json");
	test(configuru::CFG, false, "../../test_suite/json_fail",      ".json");
	test(configuru::CFG, false, "../../test_suite/cfg_fail",       ".cfg");
	test(configuru::CFG, false, "../../test_suite/cfg_only_fail",  ".cfg");
	std::cout << std::endl << std::endl;;

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
