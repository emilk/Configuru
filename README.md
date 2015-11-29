Configuru
=========
Configuru, an experimental config library for C++, by Emil Ernerfeldt.

License
-------
This software is in the public domain. Where that dedication is not
recognized, you are granted a perpetual, irrevocable license to copy
and modify this file as you see fit.

That being said, I would appreciate credit!
If you find this library useful, tweet me at @ernerfeldt mail me at emil.ernerfeldt@gmail.com.

Overview
--------
Config read/write. The format is a form of simplified JSON.
This config library is unique in a few ways:

* Indentation/style must be correct in input.
* Round-trip parse/write of comments.
* Novel(?) method for finding typos in config files:
	When reading a config, "forgotten" keys are warned about. For instance:

		auto cfg = configuru::parse_string("colour = [1,1,1]");
		auto color = cfg.get_or("color", Color(0, 0, 0));
		cfg.check_dangling(); // Will warn about how we never read "colour",

Error messages
--------------
Configuru prides itself on great error messages both for parse errors and for value errors (expecting one thing and getting another). Example parse errors:

	equal_in_map.cfg:1:16: Expected : after object key
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

Similarily, using a parsed Config value in the wrong way produces nice error messages:

Accessing a bool beleiving it to be a float: `config.json:2: Expected float, got bool`. Note that the line of the file where they value is defined is mentioned.

Accessing a key which has not been set: `config.json:1: Failed to find key 'does_not_exist'`. Again, the line where the object is defined at is mentioned.




Usage (general):
----------------
For using:
	`#include <configuru.hpp>`

And in one .cpp file:

	#define CONFIGURU_IMPLEMENTATION 1
	#include <configuru.hpp>

Usage (parsing):
----------------

	Config cfg = configuru::parse_config_file("input.json");
	float alpha = (float)cfg["alpha"];
	if (cfg.has_key("beta")) {
		std::string beta = (std::string)cfg["beta"];
	}
	float pi = cfg.get_or(pi, 3.14f);

	const auto& array = cfg["array"];
	if (cfg["array"].is_list()) {
		std::cout << "First element: " << cfg["array"][0u];
		for (const Config& element : cfg["array"].as_array()) {
		}
	}

	for (auto& p : cfg["object"].as_object()) {
		std::cout << "Key: "   << p.first << std::endl;
		std::cout << "Value: " << (float)p.second;
	}

	cfg.check_dangling(); // Make sure we haven't forgot reading a key!

	// You can modify the read config:
	cfg["message"] = "goodbye";

	write_config_file("output.cfg", cfg); // Will include comments in the original.

Usage (writing):
----------------

	Config cfg = Config::new_object();
	cfg["pi"]     = 3.14;
	cfg["array"]  = { 1, 2, 3 };
	cfg["object"] = {
		{ "key1", "value1" },
		{ "key2", "value2" },
	};
	write_config_file("output.cfg", cfg);

Goals:
========
* Debugable:
	* Find typos most config libs miss (typos in keys).
	* Cleverly help to point out mismatched braces in the right place.
	* Easily find source of typos with line numbers and helpful error messages.
* Configurable:
* Allow comments in configs.
* Pretty printing.
* Extensible.

# Non-goals:
* Low overhead

# TODO:
* Code cleanup.
* Dissallow having commas for just some entries in an array or object.


Config format
-------------

Like JSON, but with simplifications. Example file:

	values: [1 2 3 4 5 6]
	object {
		nested_key = +inf
	}
	python_style: """This is a string
	                 which spans many lines."""
	"C# style": @"Also nice for \ and stuff"

* Top-level can be key-value pairs, or a value.
* Keys need not be quoted if identifiers.
* = or : can be used to separate keys and values.
* Key-value pairs need not be separated with ;
* Array values need not be separated with ,
* Trailing , allowed in arrays.

""" starts a verbatim multiline string

@" starts a C# style verbatim string which ends on next quote (except "" which is a single-quote).

Numbers can be represented in any common form:
42, 1e-32, 0xCAFE, 0b1010

+inf, -inf, +NaN are valid numbers

key = { }   can be written as   key { }

Indentation is enforced, and must be done with tabs.
Tabs anywhere else is not allowed.
