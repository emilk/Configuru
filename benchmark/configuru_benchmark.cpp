#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#define CONFIGURU_VALUE_SEMANTICS 1
#define CONFIGURU_IMPLEMENTATION 1
#include <../configuru.hpp>

#include <fstream>
#include <iostream>

#include <boost/filesystem.hpp>

#include <json.hpp>

namespace fs = boost::filesystem;
using namespace configuru;

// ----------------------------------------------------------------------------

std::vector<std::string> list_file_names(fs::path directory, std::string extension)
{
    std::vector<std::string> result;
    fs::directory_iterator it(directory);
    fs::directory_iterator end;
    for (; it != end; ++it) {
        if (fs::is_regular_file(it->status()) && fs::extension(*it) == extension) {
            result.push_back(it->path().filename().string());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

int main(int argc, char* argv[])
{
	bool use_configuru = (argc == 1 || strcmp(argv[1], "configuru") == 0);
	std::cout << "Using " << (use_configuru ? "configuru" : "nlohmann::json") << std::endl;

	const fs::path in_dir  = "../../test_suite/huge/in";
	const fs::path out_dir = "../../test_suite/huge/out";

	for (const auto& filename : list_file_names(in_dir, ".json")) {
		const auto in_path  = in_dir  / filename;
		const auto out_path = out_dir / filename;

		if (use_configuru) {
			const auto cfg = configuru::parse_file(in_path.string(), configuru::JSON);
			auto compact_json = configuru::JSON;
			compact_json.indentation = "";
			configuru::dump_file(out_path.string(), cfg, compact_json);
		} else {
			std::ifstream in_file(in_path.string());
			nlohmann::json j(in_file);
			std::ofstream(out_path.string()) << j;
		}
	}
}
