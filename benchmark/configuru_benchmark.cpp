#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#define CONFIGURU_VALUE_SEMANTICS 1
#define CONFIGURU_IMPLEMENTATION 1
#include <../configuru.hpp>

#include <boost/filesystem.hpp>

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

int main()
{
	const fs::path in_dir  = "../../test_suite/huge/in";
	const fs::path out_dir = "../../test_suite/huge/out";
	for (const auto& filename : list_file_names(in_dir, ".json")) {
		const auto in_path  = in_dir  / filename;
		const auto out_path = out_dir / filename;
		const auto cfg = configuru::parse_file(in_path.string(), configuru::JSON);

		auto compact_json = configuru::JSON;
		compact_json.indentation = "";
		configuru::dump_file(out_path.string(), cfg, compact_json);
	}
}
