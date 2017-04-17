// For using CHECK_F for CONFIGURU_ASSERT:
#define LOGURU_IMPLEMENTATION 1
#include <loguru.hpp>

// With visit_struct.hpp (from https://github.com/cbeck88/visit_struct) we get semi-automatic
// (de)serialization of structs.
// Must be included before configuru.hpp
#include <visit_struct/visit_struct.hpp>

// ----------------------------------------------------------------------------

// #define CONFIGURU_ASSERT(test) TEST(test)
#define CONFIGURU_ASSERT(test) CHECK_F(test)

// CONFIGURU_IMPLICIT_CONVERSIONS and CONFIGURU_VALUE_SEMANTICS set by build system

#define CONFIGURU_IMPLEMENTATION 1
#include <../configuru.hpp>

// ----------------------------------------------------------------------------

#include "simple_test.hpp"

#include <iostream>

#include <boost/filesystem.hpp>

#include <json.hpp>

namespace fs = boost::filesystem;
using namespace configuru;

// ----------------------------------------------------------------------------

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
	std::sort(result.begin(), result.end());
	return result;
}

void test_parse(FormatOptions options, bool should_pass, fs::path path)
{
	test_code(__FILE__, __LINE__, path.filename().string(), should_pass, [&](){
		parse_file(path.string(), options);
	});
}

void test_all_in(FormatOptions options, bool should_pass, fs::path dir, std::string extension)
{
	for (auto path : list_files(dir, extension)) {
		test_parse(options, should_pass, path.string());
	}
}

template<typename T>
void test_roundtrip(FormatOptions options, T value)
{
	std::string serialized = dump_string(Config(value), options);
	auto parsed_config = parse_string(serialized.c_str(), options, "roundtrip");
	T parsed_value = (T)parsed_config;
	if (value == parsed_value) {
		TEST_PASS(serialized);
	} else {
		TEST_FAIL2("round-trip", serialized);
	}
}

template<typename T>
void test_writer(FormatOptions options, std::string name, T value, const std::string& expected)
{
	std::string serialized = dump_string(Config(value), options);
	if (serialized[serialized.size() - 1] == '\n') {
		serialized.resize(serialized.size() - 1);
	}
	if (serialized == expected) {
		TEST_PASS(name);
	} else {
		TEST_FAIL2(name, "Expected: '" + expected + "', got: '" + serialized + "'");
	}
}

void test_special()
{
	auto format = JSON;
	format.enforce_indentation = true;
	format.indentation = "\t";
	test_parse(format, false, "../../test_suite/special/two_spaces_indentation.json");

	format.indentation = "    ";
	test_parse(format, false, "../../test_suite/special/two_spaces_indentation.json");

	format.indentation = "  ";
	test_parse(format, true, "../../test_suite/special/two_spaces_indentation.json");

	test_roundtrip(JSON, 0.1);
	test_roundtrip(JSON, 0.1f);
	test_roundtrip(JSON, 3.14);
	test_roundtrip(JSON, 3.14f);
	test_roundtrip(JSON, 3.14000010490417);
	test_roundtrip(JSON, 1234567890123456ll);

	test_writer(JSON, "3.14 (double)", 3.14,  "3.14");
	test_writer(JSON, "3.14f (float)", 3.14f, "3.14");
}

void test_roundtrip_string()
{
	auto test_roundtrip = [&](const std::string& json)
	{
		Config cfg = parse_string(json.c_str(), JSON, "roundtrip");
		std::string serialized = dump_string(cfg, JSON);
		if (serialized[serialized.size() - 1] == '\n') {
			serialized.resize(serialized.size() - 1);
		}
		if (json == serialized) {
			TEST_PASS(json);
		} else {
			TEST_FAIL2("round-trip", "Expected: '" + json + "', got: '" + serialized + "'");
		}
	};

	test_roundtrip("42");
	test_roundtrip("-42");
	test_roundtrip("9223372036854775807");
	test_roundtrip("-9223372036854775808");
	test_roundtrip("0.0");
	test_roundtrip("-0.0");
	test_roundtrip("1.0");
	test_roundtrip("-1.0");
	test_roundtrip("5e-324");
	test_roundtrip("2.225073858507201e-308");
	test_roundtrip("2.2250738585072014e-308");
	test_roundtrip("1.7976931348623157e+308");
	test_roundtrip("3.14");
}

void test_string(const std::string& json, const char* str, size_t len = 0)
{
	if (len == 0) { len = strlen(str); }
	std::string expected(str, str + len);
	std::string output = parse_string(json.c_str(), JSON, "string").as_string();
	if (output == expected) {
		TEST_PASS(expected);
	} else {
		TEST_FAIL2(json, "Got: '" + output + ", expected: '" + expected + "'");
	}
}


void test_strings()
{
	// Tests from https://github.com/miloyip/nativejson-benchmark
	// Test UTF-8:
	test_string("\"\"",                         "");
	test_string("\"Hello\"",                    "Hello");
	test_string("\"Hello\\nWorld\"",            "Hello\nWorld");
	test_string("\"Hello\\u0000World\"",        "Hello\0World", 11);
	test_string("\"\\\"\\\\/\\b\\f\\n\\r\\t\"", "\"\\/\b\f\n\r\t");
	test_string("\"\\u0024\"",                  "\x24");             // Dollar sign U+0024
	test_string("\"\\u00A2\"",                  "\xC2\xA2");         // Cents sign U+00A2
	test_string("\"\\u20AC\"",                  "\xE2\x82\xAC");     // Euro sign U+20AC
	test_string("\"\\uD834\\uDD1E\"",           "\xF0\x9D\x84\x9E"); // G clef sign U+1D11E
}

void test_doubles()
{
	auto test_double = [&](const std::string& json, const double expected)
	{
		double output = (double)parse_string(json.c_str(), JSON, "string");
		if (output == expected) {
			TEST_PASS(json);
		} else {
			TEST_FAIL2(json, std::to_string(output) + " != " + std::to_string(expected));
		}
	};

	// Tests from https://github.com/miloyip/nativejson-benchmark
	test_double("0.0", 0.0);
	test_double("-0.0", -0.0);
	test_double("1.0", 1.0);
	test_double("-1.0", -1.0);
	test_double("1.5", 1.5);
	test_double("-1.5", -1.5);
	test_double("3.1416", 3.1416);
	test_double("1E10", 1E10);
	test_double("1e10", 1e10);
	test_double("1E+10", 1E+10);
	test_double("1E-10", 1E-10);
	test_double("-1E10", -1E10);
	test_double("-1e10", -1e10);
	test_double("-1E+10", -1E+10);
	test_double("-1E-10", -1E-10);
	test_double("1.234E+10", 1.234E+10);
	test_double("1.234E-10", 1.234E-10);
	test_double("1.79769e+308", 1.79769e+308);
	test_double("2.22507e-308", 2.22507e-308);
	test_double("-1.79769e+308", -1.79769e+308);
	test_double("-2.22507e-308", -2.22507e-308);
	test_double("4.9406564584124654e-324", 4.9406564584124654e-324); // minimum denormal
	test_double("2.2250738585072009e-308", 2.2250738585072009e-308); // Max subnormal double
	test_double("2.2250738585072014e-308", 2.2250738585072014e-308); // Min normal positive double
	test_double("1.7976931348623157e+308", 1.7976931348623157e+308); // Max double
	test_double("1e-10000", 0.0);                                   // must underflow
	test_double("18446744073709551616", 18446744073709551616.0);    // 2^64 (max of uint64_t + 1, force to use double)
	test_double("-9223372036854775809", -9223372036854775809.0);    // -2^63 - 1(min of int64_t + 1, force to use double)
	test_double("0.9868011474609375", 0.9868011474609375);          // https://github.com/miloyip/rapidjson/issues/120
	test_double("123e34", 123e34);                                  // Fast Path Cases In Disguise
	test_double("45913141877270640000.0", 45913141877270640000.0);
	test_double("2.2250738585072011e-308", 2.2250738585072011e-308); // http://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
	test_double("1e-00011111111111", 0.0);
	test_double("-1e-00011111111111", -0.0);
	test_double("1e-214748363", 0.0);
	test_double("1e-214748364", 0.0);
	test_double("1e-21474836311", 0.0);
	test_double("0.017976931348623157e+310", 1.7976931348623157e+308); // Max double in another form

	// Since
	// abs((2^-1022 - 2^-1074) - 2.2250738585072012e-308) = 3.109754131239141401123495768877590405345064751974375599... ¡Á 10^-324
	// abs((2^-1022) - 2.2250738585072012e-308) = 1.830902327173324040642192159804623318305533274168872044... ¡Á 10 ^ -324
	// So 2.2250738585072012e-308 should round to 2^-1022 = 2.2250738585072014e-308
	test_double("2.2250738585072012e-308", 2.2250738585072014e-308); // http://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/

	// More closer to normal/subnormal boundary
	// boundary = 2^-1022 - 2^-1075 = 2.225073858507201136057409796709131975934819546351645648... ¡Á 10^-308
	test_double("2.22507385850720113605740979670913197593481954635164564e-308", 2.2250738585072009e-308);
	test_double("2.22507385850720113605740979670913197593481954635164565e-308", 2.2250738585072014e-308);

	// 1.0 is in (1.0 - 2^-54, 1.0 + 2^-53)
	// 1.0 - 2^-54 = 0.999999999999999944488848768742172978818416595458984375
	test_double("0.999999999999999944488848768742172978818416595458984375", 1.0); // round to even
	test_double("0.999999999999999944488848768742172978818416595458984374", 0.99999999999999989); // previous double
	test_double("0.999999999999999944488848768742172978818416595458984376", 1.0); // next double
	// 1.0 + 2^-53 = 1.00000000000000011102230246251565404236316680908203125
	test_double("1.00000000000000011102230246251565404236316680908203125", 1.0); // round to even
	test_double("1.00000000000000011102230246251565404236316680908203124", 1.0); // previous double
	test_double("1.00000000000000011102230246251565404236316680908203126", 1.00000000000000022); // next double

	// Numbers from https://github.com/floitsch/double-conversion/blob/master/test/cctest/test-strtod.cc

	test_double("72057594037927928.0", 72057594037927928.0);
	test_double("72057594037927936.0", 72057594037927936.0);
	test_double("72057594037927932.0", 72057594037927936.0);
	test_double("7205759403792793199999e-5", 72057594037927928.0);
	test_double("7205759403792793200001e-5", 72057594037927936.0);

	test_double("9223372036854774784.0", 9223372036854774784.0);
	test_double("9223372036854775808.0", 9223372036854775808.0);
	test_double("9223372036854775296.0", 9223372036854775808.0);
	test_double("922337203685477529599999e-5", 9223372036854774784.0);
	test_double("922337203685477529600001e-5", 9223372036854775808.0);

	test_double("10141204801825834086073718800384", 10141204801825834086073718800384.0);
	test_double("10141204801825835211973625643008", 10141204801825835211973625643008.0);
	test_double("10141204801825834649023672221696", 10141204801825835211973625643008.0);
	test_double("1014120480182583464902367222169599999e-5", 10141204801825834086073718800384.0);
	test_double("1014120480182583464902367222169600001e-5", 10141204801825835211973625643008.0);

	test_double("5708990770823838890407843763683279797179383808", 5708990770823838890407843763683279797179383808.0);
	test_double("5708990770823839524233143877797980545530986496", 5708990770823839524233143877797980545530986496.0);
	test_double("5708990770823839207320493820740630171355185152", 5708990770823839524233143877797980545530986496.0);
	test_double("5708990770823839207320493820740630171355185151999e-3", 5708990770823838890407843763683279797179383808.0);
	test_double("5708990770823839207320493820740630171355185152001e-3", 5708990770823839524233143877797980545530986496.0);

	{
		char n1e308[310];   // '1' followed by 308 '0'
		n1e308[0] = '1';
		for (int j = 1; j < 309; j++)
			n1e308[j] = '0';
		n1e308[309] = '\0';
		test_double(n1e308, 1E308);
	}

	// Cover trimming
	test_double(
		"2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508"
		"7914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012"
		"9811224514518898490572223072852551331557550159143974763979834118019993239625482890171070818506906306"
		"6665599493827577257201576306269066333264756530000924588831643303777979186961204949739037782970490505"
		"1080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621"
		"5722898802581825451803257070188608721131280795122334262883686223215037756666225039825343359745688844"
		"2390026549819838548794829220689472168983109969836584681402285424333066033985088644580400103493397042"
		"7567186443383770486037861622771738545623065874679014086723327636718751234567890123456789012345678901"
		"e-308",
		2.2250738585072014e-308);
}

void test_bad_usage()
{
	auto config = parse_file("../../test_suite/special/config.json", JSON);
	test_code(__FILE__, __LINE__, "access_float_as_float", true, [&]{
		auto b = (float)config["pi"];
		(void)b;
	});
	test_code(__FILE__, __LINE__, "access_float_bool", false, [&]{
		auto f = (bool)config["pi"];
		(void)f;
	});
	test_code(__FILE__, __LINE__, "key_not_found", false, [&]{
		std::cout << (float)config["obj"]["does_not_exist"];
	});
	test_code(__FILE__, __LINE__, "indexing_non_array", false, [&]{
		std::cout << (float)config["pi"][5];
	});
	test_code(__FILE__, __LINE__, "out_of_bounds", false, [&]{
		std::cout << (float)config["array"][5];
	});

	test_code(__FILE__, __LINE__, "assign_to_non_object", false, []{
		Config cfg;
		cfg["hello"] = 42;
	});

	test_code(__FILE__, __LINE__, "read_from_non_object", false, []{
		const Config cfg;
		std::cout << cfg["hello"];
	});

	test_code(__FILE__, __LINE__, "assign_to_non_array", false, []{
		Config cfg;
		cfg.push_back("hello");
	});
}

void run_unit_tests()
{
	// JSON expected to pass:
	test_all_in(JSON, true,  "../../test_suite/json_pass",      ".json");
	test_all_in(JSON, true,  "../../test_suite/json_only_pass", ".json");

	// JSON expected to fail:
	test_all_in(JSON, false, "../../test_suite/json_fail",      ".json");
	test_all_in(JSON, false, "../../test_suite/cfg_pass",       ".cfg");
	test_all_in(JSON, false, "../../test_suite/cfg_fail",       ".cfg");

	// CFG expected to pass:
	test_all_in(CFG,  true,  "../../test_suite/json_pass",      ".json");
	test_all_in(CFG,  true,  "../../test_suite/cfg_pass",       ".cfg");

	// CFG expected to fail:
	test_all_in(CFG,  false, "../../test_suite/json_only_pass", ".json");
	test_all_in(CFG,  false, "../../test_suite/json_fail",      ".json");
	test_all_in(CFG,  false, "../../test_suite/cfg_fail",       ".cfg");

	test_special();
	test_bad_usage();
	test_strings();
	test_doubles();
	test_roundtrip_string();
}

static const char* TEST_CFG = R"(
pi:    3.14,
array: [1 2 3 4]
obj:   {
	// A comment
	nested_value: 42
}
)";

void parse_and_print()
{
	std::cout << "----- parse_and_print ---------------------------------------" << std::endl;
	auto cfg = parse_string(TEST_CFG, CFG, "test_cfg");
	std::cout << "pi: " << cfg["pi"] << std::endl;
	cfg.visit_dangling([](const std::string& key, const Config& value){
		std::cout << value.where() << "Key '" << key << "' never accessed" << std::endl;
	});

	std::cout << std::endl;
	std::cout << "// CFG:" << std::endl;
	std::cout << dump_string(cfg, CFG);

	std::cout << std::endl;
	std::cout << "// JSON with tabs:" << std::endl;
	std::cout << dump_string(cfg, JSON);

	std::cout << std::endl;
	std::cout << "// JSON with two spaces:" << std::endl;
	auto format = JSON;
	format.indentation = "  ";
	std::cout << dump_string(cfg, format);

	std::cout << std::endl;
	std::cout << "// JSON with keys sorted lexicographically:" << std::endl;
	format.sort_keys = true;
	std::cout << dump_string(cfg, format);

	std::cout << std::endl;
	std::cout << "// Compact JSON:" << std::endl;
	format = JSON;
	format.indentation = "";
	std::cout << dump_string(cfg, format);

	std::cout << std::endl;
	std::cout << "-------------------------------------------------------------" << std::endl;
}

void create()
{
	/*
	Based on https://github.com/nlohmann/json#examples

	Taget JSON:

	{
		"pi":      3.141,
		"happy":   true,
		"name":    "Emil",
		"nothing": null,
		"answer":  {
			"everything": 42
		},
		"array":   [1, 0, 2],
		"object": {
			"currency": "USD",
			"value":    42.99
		}
	}
	*/

	// Create the config as an object:
	auto cfg = Config::object();

	// add a number that is stored as double
	cfg["pi"] = 3.141;

	// add a Boolean that is stored as bool
	cfg["happy"] = true;

	// add a string that is stored as std::string
	cfg["name"] = "Emil";

	// add another null entry by passing nullptr
	cfg["nothing"] = nullptr;

	// add an object inside the object
	cfg["answer"] = Config::object();
	cfg["answer"]["everything"] = 42;

	// add an array that is stored as std::vector
	cfg["array"] = Config::array({ 1, 0, 2 });

	// add another object (using an initializer_list of pairs)
	cfg["object"] = {
		{ "currency", "USD" },
		{ "value",    42.99 }
	};

	// instead, you could also write (which looks very similar to the JSON above)
	const Config cfg2 = {
		{ "pi",      3.141   },
		{ "happy",   true    },
		{ "name",    "Emil"  },
		{ "nothing", nullptr },
		{ "answer",  {
			{ "everything", 42 }
		} },
		{ "array",   Config::array({ 1, 0, 2 }) },
		{ "object",  {
			{ "currency", "USD" },
			{ "value",    42.99 }
		} }
	};

	// std::cout << "cfg:\n"  << cfg  << std::endl;
	// std::cout << "cfg2:\n" << cfg2 << std::endl;
}

void test_check_dangling()
{
	const char* TEST_CFG_2 = R"(
	{
		"value":  3.14,
		"array":  ["array_0", "array_1"],
		"object": {
			"key_0": 0,
			"key_1": 1
	}
	})";

	{
		const auto const_cfg = parse_string(TEST_CFG_2, JSON, "test_cfg_2");

		try {
			const_cfg.check_dangling();
			TEST_FAIL("Should have thrown");
		} catch (std::exception& e) {
			std::string msg = e.what();
			TEST(msg.find("'value'")  != std::string::npos);
			TEST(msg.find("'array'")  != std::string::npos);
			TEST(msg.find("'object'") != std::string::npos);
			TEST(msg.find("'key_0'")  == std::string::npos);
			TEST(msg.find("'key_1'")  == std::string::npos);
		}

		std::stringstream ss;
		ss << const_cfg;
		TEST_THROW(const_cfg.check_dangling(), std::exception);

		const_cfg.mark_accessed(true);
		TEST_NOTHROW(const_cfg.check_dangling());
		const_cfg.mark_accessed(false);
		TEST_THROW(const_cfg.check_dangling(), std::exception);

		#if CONFIGURU_VALUE_SEMANTICS
			const_cfg.mark_accessed(false);
			TEST_THROW(const_cfg.check_dangling(), std::exception);
			const auto copy = const_cfg;
			TEST_NOTHROW(const_cfg.check_dangling());
			TEST_THROW(copy.check_dangling(), std::exception);
		#endif

		const_cfg.mark_accessed(false);
		dump_string(const_cfg, JSON);
		TEST_NOTHROW(const_cfg.check_dangling());

		const_cfg.mark_accessed(false);
		TEST_THROW(const_cfg.check_dangling(), std::exception);

		std::cout << "object contents: " << std::endl;
		for (const auto& p : const_cfg.as_object())
		{
			std::cout << p.key() << ": " << p.value() << std::endl;
		}

		try {
			const_cfg.check_dangling();
			TEST_FAIL("Should have thrown");
		} catch (std::exception& e) {
			std::string msg = e.what();
			TEST(msg.find("'value'")  == std::string::npos);
			TEST(msg.find("'array'")  == std::string::npos);
			TEST(msg.find("'object'") == std::string::npos);
			TEST(msg.find("'key_0'")  != std::string::npos);
			TEST(msg.find("'key_1'")  != std::string::npos);
		}
	}

	{
		auto mut_cfg = parse_string(TEST_CFG_2, JSON, "test_cfg_2");
		for (auto& p : mut_cfg.as_object())
		{
			p.value() = p.key();
		}
		TEST_NOTHROW(mut_cfg.check_dangling());
		TEST(mut_cfg["value"]  == "value");
		TEST(mut_cfg["array"]  == "array");
		TEST(mut_cfg["object"] == "object");
	}
}

void test_comments()
{
	auto in_path  = "../../test_suite/comments_in.cfg";
	auto out_path = "../../test_suite/comments_out.cfg";
	auto out_2_path = "../../test_suite/comments_out_2.cfg";
	auto data = parse_file(in_path, CFG);
	dump_file(out_path, data, CFG);

	data["number"] = 42;
	data["array"].push_back("new value");
	data["object"]["new_key"] = true;

	Config rearranged = data;
	rearranged["indent"] = {
		{ "array",  data["array"],  },
		{ "object", data["object"], },
	};
	rearranged.erase("object");
	rearranged.erase("array");
	dump_file(out_2_path, rearranged, CFG);
}

void test_conversions()
{
	Config cfg = {
		{ "bool",        true     },
		{ "int",         42       },
		{ "float",        2.75f   },
		{ "double",       3.14    },
		{ "string",      "Hello!" },
		{ "mixed_array", Config::array({ nullptr, 1, "two" }) }
	};

	auto explicit_bool = (bool)cfg["bool"];
	TEST_EQ(explicit_bool, true);
	auto explicit_int = (int)cfg["int"];
	TEST_EQ(explicit_int, 42);
	auto explicit_float = (float)cfg["float"];
	TEST_EQ(explicit_float, 2.75f);
	auto explicit_double = (double)cfg["double"];
	TEST_EQ(explicit_double, 3.14);
	auto explicit_string = (std::string)cfg["string"];
	TEST_EQ(explicit_string, "Hello!");
#if !CONFIGURU_IMPLICIT_CONVERSIONS
	auto explicit_mixed_array = (std::vector<Config>)cfg["mixed_array"];
	TEST_EQ(explicit_mixed_array[0], nullptr);
	TEST_EQ(explicit_mixed_array[1], 1);
	TEST_EQ(explicit_mixed_array[2], "two");
#endif

#if CONFIGURU_IMPLICIT_CONVERSIONS
	bool implicit_bool; implicit_bool = cfg["bool"];
	TEST_EQ(implicit_bool, true);
	int implicit_int; implicit_int = cfg["int"];
	TEST_EQ(implicit_int, 42);
	float implicit_float; implicit_float = cfg["float"];
	TEST_EQ(implicit_float, 2.75f);
	double implicit_double; implicit_double = cfg["double"];
	TEST_EQ(implicit_double, 3.14);
	// std::string implicit_string; implicit_string = cfg["string"];
	// TEST_EQ(implicit_string, "Hello!");
	std::vector<Config> implicit_mixed_array; implicit_mixed_array = cfg["mixed_array"];
	TEST_EQ(implicit_mixed_array[0], nullptr);
	// TEST_EQ(implicit_mixed_array[1], 1);
	TEST_EQ(implicit_mixed_array[2], "two");
#endif

#if !CONFIGURU_IMPLICIT_CONVERSIONS
	auto parse_json = [](const std::string& json) {
		return parse_string(json.c_str(), JSON, "");
	};

	auto strings = (std::vector<std::string>)parse_json(R"(["hello", "you"])");
	TEST_EQ(strings.size(), 2u);
	TEST_EQ(strings[0], "hello");
	TEST_EQ(strings[1], "you");

	auto array = (std::array<int, 2>)parse_json(R"([32, 20])");
	TEST_EQ(array[0], 32);
	TEST_EQ(array[1], 20);

	auto ints = (std::vector<int>)parse_json(R"([0,1,2])");
	TEST_EQ(ints.size(), 3u);
	TEST_EQ(ints[0], 0);
	TEST_EQ(ints[1], 1);
	TEST_EQ(ints[2], 2);

	auto pairs = (std::vector<std::pair<std::string, float>>)parse_json(R"([["1", 2.2], ["3", 4.4]])");
	TEST_EQ(pairs.size(), 2u);
	TEST_EQ(pairs[0].first, "1");
	TEST_EQ(pairs[0].second, 2.2f);
	TEST_EQ(pairs[1].first, "3");
	TEST_EQ(pairs[1].second, 4.4f);
#endif
}

void test_copy_semantics()
{
	Config original{
		{ "key", "original_value" }
	};
	TEST_EQ(original["key"], "original_value");
	Config copy = original;
	TEST_EQ(copy["key"], "original_value");
	copy["key"] = "new_value";
	TEST_EQ(copy["key"], "new_value");
#if CONFIGURU_VALUE_SEMANTICS
	TEST_EQ(original["key"], "original_value");
#else
	TEST_EQ(original["key"], "new_value");
#endif
}

void test_swap()
{
	Config a{{ "message", "hello" }};
	Config b{{ "salute", "goodbye" }};
	a.swap(b);
	TEST_EQ(b["message"], "hello");
	TEST_EQ(a["salute"], "goodbye");
	std::swap(a, b);
	TEST_EQ(a["message"], "hello");
	TEST_EQ(b["salute"], "goodbye");
}

void test_get_or()
{
	const Config cfg = parse_string(R"({
	"a": {
		"b": {
			"c": {
				"key": 42
			}
		}
	}
})", JSON, "test_get_or");

	TEST_EQ(cfg.get_or({"a", "b", "c", "key"}, 0), 42);
	TEST_EQ(cfg.get_or({"a", "x", "c", "key"}, 3.14), 3.14);
	TEST_EQ(cfg.get_or({"a", "b", "c", "not_key"}, "hello"), "hello");
	try {
		cfg.get_or({"a", "b", "c", "key", "not_ok"}, 0);
		TEST_FAIL("Should have thrown");
	} catch (std::exception& e) {
		TEST_EQ(std::string(e.what()), std::string("test_get_or:5: Expected object, got integer"));
	}
}

// ----------------------------------------------------------------------------

struct TestStruct
{
	std::string some_string = "hello";
	int         some_int    = 42;
};
bool operator==(const TestStruct& a, const TestStruct& b)
{
	return
		a.some_string == b.some_string &&
		a.some_int    == b.some_int;
}
VISITABLE_STRUCT(TestStruct, some_int, some_string);

void test_serialize_deserialize()
{
	std::vector<std::string> errors;
	auto store_errors = [&errors](const std::string& error) { errors.push_back(error); };

	TestStruct test_struct;
	configuru::deserialize(&test_struct, configuru::parse_string("{}", CFG, ""), store_errors);
	TEST(errors.empty());
	TEST_EQ(test_struct.some_string, "hello");

	TEST_EQ(test_struct.some_string, "hello");
	TEST_EQ(test_struct.some_int,    42);

	configuru::deserialize(&test_struct, configuru::parse_string("{some_int: 17}", CFG, ""), store_errors);
	TEST(errors.empty());
	TEST_EQ(test_struct.some_string, "hello");

	TEST_EQ(test_struct.some_string, "hello");
	TEST_EQ(test_struct.some_int,    17);

	const std::vector<TestStruct> before{{"seven", 7}, {"eight", 8}};
	std::vector<TestStruct> after;
	configuru::deserialize(&after, configuru::serialize(before), store_errors);
	TEST(errors.empty());
	TEST_EQ(before, after);
}

// ----------------------------------------------------------------------------

void configuru_vs_nlohmann()
{
	nlohmann::json nlohmann_json {
		{"float",       3.14f},
		{"double",      3.14},
		{"short_array", {1, 2, 3}},
		{"long_array",  {
			"one",
			{"two", "things"},
			"three",
		}},
	};

	Config configuru_cfg {
		{"float",       3.14f},
		{"double",      3.14},
		{"short_array", Config::array({1, 2, 3})},
		{"long_array",  Config::array({
			"one",
			Config::array({"two", "things"}),
			"three",
		})},
	};

	std::cout << "---- configuru_vs_nlohmann ----------------------------------" << std::endl;
	std::cout << "nlohmann: \n"       << nlohmann::json(nlohmann_json).dump(4) << std::endl;
	std::cout << "configuru JSON: \n" << dump_string(configuru_cfg, JSON) << std::endl;
	std::cout << "configuru CFG: \n"  << dump_string(configuru_cfg, CFG)  << std::endl;
	std::cout << "-------------------------------------------------------------" << std::endl;
}

int main()
{
	printf("CONFIGURU_VALUE_SEMANTICS:      %s\n", CONFIGURU_VALUE_SEMANTICS      ? "ON" : "OFF");
	printf("CONFIGURU_IMPLICIT_CONVERSIONS: %s\n", CONFIGURU_IMPLICIT_CONVERSIONS ? "ON" : "OFF");

	parse_and_print();
	configuru_vs_nlohmann();
	create();
	test_check_dangling();
	test_comments();
	test_conversions();
	run_unit_tests();
	test_copy_semantics();
	test_swap();
	test_get_or();
	test_serialize_deserialize();

	// ------------------------------------------------------------------------

	s_tester.print_results_and_exit();
}
