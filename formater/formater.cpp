#define LOGURU_IMPLEMENTATION 1
#include "loguru.hpp"

#define CONFIGURU_ASSERT(test) CHECK_F(test)

#define CONFIGURU_IMPLEMENTATION 1
#include "../configuru.hpp"

#include <boost/filesystem.hpp>

using namespace configuru;
namespace fs = boost::filesystem;

void format(const fs::path& path, const FormatOptions& parse_format, const FormatOptions& dump_format)
{
	if (fs::is_directory(path)) {
		LOG_F(1, "Formating every file in %s (recursively)", path.c_str());
		fs::directory_iterator end_iter;
		for (fs::directory_iterator dir_iter(path); dir_iter != end_iter; ++dir_iter) {
			format(dir_iter->path(), parse_format, dump_format);
		}
	} else {
		auto extension = fs::extension(path.filename());
		if (extension == ".cfg" || extension == ".json") {
			try {
				auto parsed = parse_file(path.c_str(), parse_format);
				dump_file(path.c_str(), parsed, dump_format);
				LOG_F(2, "Formated %s", path.c_str());
			} catch (std::exception& e) {
				LOG_F(ERROR, "Failed to format %s: %s", path.c_str(), e.what());
			}
		}
	}
}

int main(int argc, char* argv[])
{
	loguru::init(argc, argv);

	if (argc < 2) {
		std::cout << "Recursively parses and formats existing config files to a pretty JSON format." << std::endl;
		std::cout << "Usage: " << argv[0] << " [file or directory]" << std::endl;
		return 1;
	}

	const auto parse_format = configuru::FORGIVING;
	const auto dump_format  = configuru::JSON;

	for (int i = 1; i < argc; ++i) {
		format(argv[i], parse_format, dump_format);
	}

	return 0;
}
