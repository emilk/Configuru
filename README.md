Configuru
===============================================================================
Configuru, an experimental JSON config library for C++, by Emil Ernerfeldt.

License
-------------------------------------------------------------------------------
This software is in the public domain. Where that dedication is not
recognized, you are granted a perpetual, irrevocable license to copy
and modify this file as you see fit.

That being said, I would appreciate credit!
If you find this library useful, tweet me at @ernerfeldt mail me at emil.ernerfeldt@gmail.com.

Overview
-------------------------------------------------------------------------------
Configuru is a JSON parser/writer for C++11. Configuru was written for human created/edited config files and therefore prioritizes helpful error messages over parse speed.

Goals
-------------------------------------------------------------------------------
* Debugable:
	* Find typos most config libs miss (like typos in keys).
	* Easily find source of typos with line numbers and helpful error messages.
	* Cleverly help to point out mismatched braces in the right place.
* Configurable:
	* Configure the format to allow relaxed and extended flavors of JSON.
	* Very pretty printing.
	* Extensible with custom conversions.
	* Control how the `configuru::Config` behaves in your code via compile time constants:
		* Override `CONFIGURU_ONERROR` to add additional debug info (like stack traces) on errors.
		* Override `CONFIGURU_ASSERT` to use your own asserts.
		* Set `CONFIGURU_IMPLICIT_CONVERSIONS` to allow things like `float f = some_config;`
		* Set `CONFIGURU_VALUE_SEMANTICS` to have `Config` behave like a value type rather than a reference type.
* Easy to use:
	* Smooth C++11 integration for reading and creating config values.
* JSON compliant:
	* Configuru has one of the highest conformance ratings on [Native JSON Benchmark](https://github.com/miloyip/nativejson-benchmark)

Non-goals
-------------------------------------------------------------------------------
* Low overhead

Error messages
===============================================================================
Configuru prides itself on great error messages both for parse errors and for value errors (expecting one thing and getting another).

Parse errors
-------------------------------------------------------------------------------

	equal_in_object.cfg:1:16: Expected : after object key
	{ "is_this_ok" = true }
	               ^

	bad_escape.json:1:9: Unknown escape character 'x'
	{"42":"\x42"}
	        ^

	no_quotes.json:2:22: Expected value
	   "forgotten_quotes": here
	                       ^

	trucated_key.json:1:2: Unterminated string
	{"X
	 ^

	single_line_comment.cfg:1:4: Single line comments forbidden.
	{} // Blah-bla
	   ^

	unary_plus.cfg:1:1: Prefixing numbers with + is forbidden.
	+42
	^

Note how all errors mention follow the standard `filename:line:column` structure (most errors above happen on line `1` since they are from small unit tests).

Value errors
-------------------------------------------------------------------------------
Similarly, using a parsed Config value in the wrong way produces nice error messages. Take the following file (`config.json`):

	{
		"pi":    3.14,
		"array": [ 1, 2, 3, 4 ],
		"obj":   {
			"nested_value": 42
		}
	}

Here's some use errors and their error messages:

	auto b = (bool)config["pi"];

`config.json:2: Expected bool, got float`. Note that the file:line points to where the value is defined.

	std::cout << (float)config["obj"]["does_not_exist"];

`config.json:4: Failed to find key 'does_not_exist'`. Here the file and line of the owner (`"obj"`) of the missing value is referenced.

	std::cout << (float)config["pi"][5];

`config.json:2: Expected array, got float`.

	std::cout << (float)config["array"][5];

`config.json:3: Array index out of range`

	Config cfg;
	cfg["hello"] = 42;

`Expected object, got uninitialized. Did you forget to call Config::object()?`

	Config cfg;
	cfg.push_back("hello");

`Expected array, got uninitialized. Did you forget to call Config::array()?`

Unused keys
-------------------------------------------------------------------------------
Configuru has a novel mechanism for detecting subtle typos in object keys. Let say you have a Config that looks like this:

	{
		"colour": "red",
		...
	}

Here's how it could be used:

	auto cfg = configuru::parse_file("config.json", configuru::JSON);
	auto color = cfg.get_or("color", DEFAULT_COLOR);
	cfg.check_dangling();

The call to `check_dangling` will print a warning:

	config.json:2: Key 'colour' never accessed

This is very helpful for finding subtle mistakes. The call to `check_dangling` is recursive, so you only need to call it once for every config file. If you want to mute warning for some key (which you may intentionally be ignoring, or saving for later) you can call `cfg.mark_checed(true)`.

Usage
===============================================================================
For using:
	`#include <configuru.hpp>`

And in one .cpp file:

	#define CONFIGURU_IMPLEMENTATION 1
	#include <configuru.hpp>

Usage (parsing)
-------------------------------------------------------------------------------

	Config cfg = configuru::parse_file("input.json", configuru::JSON);
	float alpha = (float)cfg["alpha"];
	if (cfg.has_key("beta")) {
		std::string beta = (std::string)cfg["beta"];
	}
	float pi = cfg.get_or(pi, 3.14f);

	const auto& array = cfg["array"];
	if (cfg["array"].is_array()) {
		std::cout << "First element: " << cfg["array"][0];
		for (const Config& element : cfg["array"].as_array()) {
			std::cout << element << std::endl;
		}
	}

	for (auto& p : cfg["object"].as_object()) {
		std::cout << "Key: "   << p.key() << std::endl;
		std::cout << "Value: " << p.value();
		p.value() = "new value";
	}

	cfg.check_dangling(); // Make sure we haven't forgot reading a key!

	// You can modify the read config:
	cfg["message"] = "goodbye";

	dump_file("output.cfg", cfg, JSON); // Will include comments in the original.

Reference semantics vs value semantics
-------------------------------------------------------------------------------
By default, Config objects acts like reference types, e.g. like a std::shared_ptr:

	auto shallow_copy = cfg;
	cfg["message"] = "changed!";
	std::cout << shallow_copy["message"]; // Will print "changed!";

You can control this behavior with `#define CONFIGURU_VALUE_SEMANTICS 1`:

	#define CONFIGURU_VALUE_SEMANTICS 1
	#include <configuru.hpp>
	...
	Config cfg{{"message", "original"}};
	auto deep_clone = cfg;
	cfg["message"] = "changed!";
	std::cout << deep_clone["message"]; // Will print "original";

Errors
-------------------------------------------------------------------------------
The default behavior of Configuru is to throw a `std::runtime_error` on any error. You can change this behavior by overriding `CONFIGURU_ONERROR`.

Usage (writing):
-------------------------------------------------------------------------------

	Config cfg = Config::new_object();
	cfg["pi"]     = 3.14;
	cfg["array"]  = { 1, 2, 3 };
	cfg["object"] = {
		{ "key1", "value1" },
		{ "key2", "value2" },
	};

	std::string json = dump_string(cfg, JSON);
	dump_file("output.cfg", cfg, JSON);

CFG format
===============================================================================

In addition to JSON, Configuru also has native support for a format I simply call *CFG*. CFG is like JSON, but with simplifications. Example file:

	values: [1 2 3 4 5 6]
	object: {
		nested_key: +inf
	}
	python_style: """This is a string
	                 which spans many lines."""
	"C# style": @"Also nice for \ and stuff"

* Top-level can be key-value pairs (no need for {} surrounding entire document).
* Keys need not be quoted if identifiers.
* Commas optional for arrays and objects.
* Trailing , allowed in arrays and objects.

`"""` starts a verbatim multi-line string

`@"` starts a C# style verbatim string which ends on next quote (except `""` which is a single-quote).

Numbers can be represented in any common form:
`-42`, `1e-32`, `0xCAFE`, `0b1010`

`+inf`, `-inf`, `+NaN` are valid numbers

Indentation is enforced, and must be done with tabs. Tabs anywhere else is not allowed.

You can also allow selective parts of the above extensions to create your own dialect of JSON. Look at the members of `configuru::FormatOptions` for details.

Beautiful output
===============================================================================
One of the great things about JSON is that it is human readable (as opposed to XML). Configuru goes to great lengths to make the output as readable as possible. Here's an example structure (as defined in C++):

	Config cfg{
		{"float",       3.14f},
		{"double",      3.14},
		{"short_array", {1, 2, 3}},
		{"long_array",  {"one", {"two", "things"}, "three"}},
	};

Here's how the output turns out in most JSON encoders (this one produced by the excellent [nlohmann json library](https://github.com/nlohmann/json)):

	{
	    "double": 3.14,
	    "float": 3.14000010490417,
	    "long_array": [
	        "one",
	        [
	            "two",
	            "things"
	        ],
	        "three"
	    ],
	    "short_array": [
	        1,
	        2,
	        3
	    ]
	}

In contrast, here's how the output looks in Configuru:

	{
		"float":       3.14,
		"double":      3.14,
		"short_array": [ 1, 2, 3 ],
		"long_array":  [
			"one",
			[ "two", "things" ],
			"three"
		]
	}

Note how Configuru refrains from unnecessary line breaks on short arrays and does not write superfluous (and ugly!) trailing decimals. Configuru also writes the keys of the objects in the same order as it was given (unless the `sort_keys` option is explicitly set). The aligned values is just a preference of mine, inspired by [how id software does it](http://kotaku.com/5975610/the-exceptional-beauty-of-doom-3s-source-code). Writing the same data in the CFG format makes it turn out like this:

	float:       3.14
	double:      3.14
	short_array: [ 1 2 3 ]
	long_array:  [
		"one"
		[ "two" "things" ]
		"three"
	]


TODO:
===============================================================================
* Consider prettifying how objects are defined in C++ using a `KV` (key-value) function/class:

	configuru::Config cfg{
		KV{"float",       3.14f},
		KV{"double",      3.14},
		KV{"short_array", {1, 2, 3}},
		KV{"long_array",  {"one", {"two", "things"}, "three"}},
	};
