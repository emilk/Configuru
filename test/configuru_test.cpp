#include <iostream>

#define CONFIGURU_IMPLEMENTATION 1
#include "../configuru.hpp"

const char* TEST_CFG = R"(
pi:   3.14,
list: [1 2 3 4]
obj {
	nested_value = 42
}
)";

int main()
{
	auto cfg = configuru::parse_config(TEST_CFG, "test_cfg");
	std::cout << "pi: " << (float)cfg["pi"] << std::endl;
	cfg.check_dangling();

	std::cout << "sjson:" << std::endl;
	configuru::FormatOptions options;
	std::cout << configuru::write_config(cfg, options);

	std::cout << "Json:" << std::endl;
	options.json = true;
	std::cout << configuru::write_config(cfg, options);
}
