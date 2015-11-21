/*
www.github.com/emilk/configuru

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

*) Indentation/style must be correct in input.
*) Round-trip parse/write of comments.
*) Novel(?) method for finding typos in config files:
	When reading a config, "forgotten" keys are warned about. For instance:

		auto cfg = configuru::parse_string("colour = [1,1,1]");
		auto color = cfg.get_or("color", Color(0, 0, 0));
		cfg.check_dangling(); // Will warn about how we never read "colour",

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

	const auto& list = cfg["list"];
	if (cfg["list"].is_list()) {
		std::cout << "First element: " << cfg["list"][0u];
		for (const Config& element : cfg["list"].as_list()) {
		}
	}

	for (auto& p : cfg["object"].as_map()) {
		std::cout << "Key: "   << p.first << std::endl;
		std::cout << "Value: " << (float)p.second;
	}

	cfg.check_dangling(); // Make sure we haven't forgot reading a key!

	// You can modify the read config:
	cfg["message"] = "goodbye";

	write_config_file("output.cfg", cfg); // Will include comments in the original.

Usage (writing):
----------------

	Config cfg = Config::new_map();
	cfg["pi"] = 3.14;
	cfg["list"] = Config::new_list();
	cfg["list"].push_back(Config(42));
	write_config_file("output.cfg", cfg);

Goals:
========
* Debugable:
	* Find typos most config libs miss (typos in keys).
	* Cleverly help to point out mismatched braces in the right place.
	* Easily find source of typos with line numbers and helpful error messages.
* Allow comments in configs.
* Pretty printing.
* Extensible.

# Non-goals:
* Low overhead

# TODO:
* Strict json read/write.
* operator << for printing Config:s.
* Code cleanup.
* Nicer syntax for creating configs.


Config format
-------------

Like JSON, but with simplifications. Example file:

```
values = [1 2 3 4 5 6]
map {
	nested_key = +inf
}
python_style  = """This is a string
which spans many lines."""
"C# style" = @"Also nice for \ and stuff"
}
```

* Top-level can be key-value pairs, or a value.
* Keys need not be quoted if identifiers.
* = or : can be used to separate keys and values.
* Key-value pairs need not be separated with ;
* List values need not be separated with ,
* Trailing , allowed in lists.

""" starts a verbatim multiline string

@" starts a C# style verbatim string which ends on next quote (except "" which is a single-quote).

Numbers can be represented in any common form:
42, 1e-32, 0xCAFE, 0b1010

+inf, -inf, +NaN are valid numbers

key = { }   can be written as   key { }

Indentation is enforced, and must be done with tabs.
Tabs anywhere else is not allowed.
*/

//  dP""b8  dP"Yb  88b 88 888888 88  dP""b8 88   88 88""Yb 88   88
// dP   `" dP   Yb 88Yb88 88__   88 dP   `" 88   88 88__dP 88   88
// Yb      Yb   dP 88 Y88 88""   88 Yb  "88 Y8   8P 88"Yb  Y8   8P
//  YboodP  YbodP  88  Y8 88     88  YboodP `YbodP' 88  Yb `YbodP'

#ifndef CONFIGURU_HEADER_HPP
#define CONFIGURU_HEADER_HPP

#include <string>
#include <unordered_map>
#include <vector>

#ifndef CONFIGURU_ONERROR
	#define CONFIGURU_ONERROR(message_str) \
		throw std::runtime_error(message_str)
#endif // CONFIGURU_ONERROR

#ifndef CONFIGURU_ASSERT
	#include <cassert>
	#define CONFIGURU_ASSERT(test) assert(test)
#endif // CONFIGURU_ASSERT

#ifndef CONFIGURU_ON_DANGLING
	#include <iostream>
	#define CONFIGURU_ON_DANGLING(where, key) \
		std::cerr << where << "Key '" << key << "' never accessed" << std::endl
#endif // CONFIGURU_ON_DANGLING

namespace configuru
{
	struct DocInfo;
	using DocInfo_SP = std::shared_ptr<DocInfo>;

	struct Include {
		DocInfo_SP doc;
		unsigned line = (unsigned)-1;

		Include() {}
		Include(DocInfo_SP d, unsigned l) : doc(d), line(l) {}
	};

	struct DocInfo {
		std::vector<Include> includers;

		std::string filename;

		DocInfo(const std::string& fn) : filename(fn) { }

		void append_include_info(std::string& ret, const std::string& indent="    ") const;
	};

	struct BadLookupInfo;

	const unsigned BAD_ENTRY = -1;

	template<typename Config_T>
	struct Config_Entry
	{
		Config_T              value;
		unsigned              nr       = BAD_ENTRY; // Size of the map prior to adding this entry
		mutable bool          accessed = false;     // Set to true if accessed.
	};

	using Comment = std::string;
	using Comments = std::vector<Comment>;

	class Config;

	template<typename T>
	inline T as(const configuru::Config& config);

	/*
	 A dynamic config variable.
	 Acts like something out of Python or Lua.
	 Uses reference-counting for maps and lists.
	 This means all copies are shallow copies.
	*/
	class Config
	{
		enum Type {
			Invalid,
			BadLookupType, // We are the result of a key-lookup in a Map with no hit. We are in effect write-only.
			Null, Bool, Int, Float, String, List, Map
		};

	public:
		using MapEntry = Config_Entry<Config>;

		using ConfigListImpl = std::vector<Config>;
		using ConfigMapImpl  = std::unordered_map<std::string, MapEntry>;
		struct ConfigList {
			std::atomic<unsigned> ref_count { 1 };
			ConfigListImpl        impl;
		};
		struct ConfigMap {
			std::atomic<unsigned> ref_count { 1 };
			ConfigMapImpl        impl;
		};

		// ----------------------------------------
		// Constructors:

		Config() : _type(Invalid) { }
		explicit Config(std::nullptr_t) : _type(Null) { }
		explicit Config(bool b) : _type(Bool) {
			_u.b = b;
		}
		explicit Config(int i) : _type(Int) {
			_u.i = i;
		}
		explicit Config(int64_t i) : _type(Int) {
			_u.i = i;
		}
		explicit Config(size_t i) : _type(Int) {
			if ((i & 0x8000000000000000ull) != 0) {
				CONFIGURU_ONERROR("Integer too large to fit into 63 bits");
			}
			_u.i = static_cast<int64_t>(i);
		}
		Config(double f) : _type(Float) {
			_u.f = f;
		}
		Config(const char* str) : _type(String) {
			CONFIGURU_ASSERT(str != nullptr);
			_u.str = new std::string(str);
		}
		Config(std::string str) : _type(String) {
			_u.str = new std::string(move(str));
		}
		template<typename T>
		explicit Config(std::initializer_list<T> values) : _type(Invalid) {
			make_list();
			for (auto&& v : values) {
				push_back(Config(v));
			}
		}

		void make_map() {
			assert_type(Invalid);
			_type = Map;
			_u.map = new ConfigMap();
		}

		void make_list() {
			assert_type(Invalid);
			_type = List;
			_u.list = new ConfigList();
		}

		static Config new_map() {
			Config ret;
			ret.make_map();
			return ret;
		}

		static Config new_list() {
			Config ret;
			ret.make_list();
			return ret;
		}

		void tag(const DocInfo_SP& doc, unsigned line, unsigned column) {
			_doc = doc;
			_line = line;
			(void)column; // TODO: include this info too.
		}

		// ----------------------------------------

		~Config() {
			free();
		}

		Config(const Config& o);

		Config(Config&& o) noexcept { this->swap(o); }

		Config& operator=(const Config& o);
		Config& operator=(Config&& o) noexcept
		{
			this->swap(o);
			return *this;
		}

		void swap(Config& o) noexcept;

		Config& operator=(std::nullptr_t)
		{
			free();
			_type = Null;
			return *this;
		}

		Config& operator=(bool b)
		{
			free();
			_type = Bool;
			_u.b = b;
			return *this;
		}

		Config& operator=(int i)
		{
			free();
			_type = Int;
			_u.i = i;
			return *this;
		}

		Config& operator=(int64_t i)
		{
			free();
			_type = Int;
			_u.i = i;
			return *this;
		}

		Config& operator=(size_t i)
		{
			free();
			_type = Int;
			_u.i = i;
			return *this;
		}

		Config& operator=(double f)
		{
			free();
			_type = Float;
			_u.f = f;
			return *this;
		}

		Config& operator=(const char* str)
		{
			CONFIGURU_ASSERT(str != nullptr);
			free();
			_type = String;
			_u.str = new std::string(str);
			return *this;
		}

		Config& operator=(std::string str)
		{
			free();
			_type = String;
			_u.str = new std::string(move(str));
			return *this;
		}

		#ifdef CONFIG_EXTENSION
			CONFIG_EXTENSION
		#endif

		// ----------------------------------------
		// Inspectors:

		bool is_null()   const { return _type == Null;   }
		bool is_bool()   const { return _type == Bool;   }
		bool is_int()    const { return _type == Int;    }
		bool is_float()  const { return _type == Float;  }
		bool is_string() const { return _type == String; }
		bool is_map()    const { return _type == Map;    }
		bool is_list()   const { return _type == List;   }
		bool is_number() const { return is_int() || is_float(); }

		// Where in source where we defined?
		std::string where() const;
		unsigned line() const { return _line; }
		auto&&   doc()  const { return _doc;  }
		void set_doc(const DocInfo_SP& doc) { _doc = doc; }

		// ----------------------------------------
		// Convertors:

		template<typename T>
		explicit operator T() const { return configuru::as<T>(*this); }

		template<typename T>
		explicit operator std::vector<T>() const
		{
			std::vector<T> ret;
			for (auto&& config : as_list()) {
				ret.push_back((T)config);
			}
			return ret;
		}

		const std::string& as_string() const { assert_type(String); return *_u.str; }
		const char* c_str() const { assert_type(String); return _u.str->c_str(); }

		bool as_bool() const
		{
			assert_type(Bool);
			return _u.b;
		}
		template<typename IntT>
		IntT as_integer() const
		{
    		static_assert(std::is_integral<IntT>::value, "Not an integer.");
			assert_type(Int);
			check((int64_t)(IntT)_u.i == _u.i, "Integer out of range");
			return static_cast<IntT>(_u.i);
		}
		float as_float() const
		{
			if (_type == Int) {
				return _u.i;
			} else {
				assert_type(Float);
				return (float)_u.f;
			}
		}
		double as_double() const
		{
			if (_type == Int) {
				return _u.i;
			} else {
				assert_type(Float);
				return _u.f;
			}
		}

		template<typename T>
		T as() const;

		// ----------------------------------------
		// List:

		ConfigListImpl& as_list() {
			assert_type(List);
			return _u.list->impl;
		}

		const ConfigListImpl& as_list() const
		{
			assert_type(List);
			return _u.list->impl;
		}

		Config& operator[](unsigned ix)
		{
			auto&& list = as_list();
			check(ix < list.size(), "Out of range");
			return list[ix];
		}

		const Config& operator[](unsigned ix) const
		{
			auto&& list = as_list();
			check(ix < list.size(), "Out of range");
			return list[ix];
		}

		size_t list_size() const
		{
			return as_list().size();
		}

		void push_back(Config value) {
			as_list().push_back(std::move(value));
		}

		// ----------------------------------------
		// Map:

		size_t map_size() const
		{
			return as_map().size();
		}

		ConfigMapImpl& as_map() {
			assert_type(Map);
			return _u.map->impl;
		}

		const ConfigMapImpl& as_map() const
		{
			assert_type(Map);
			return _u.map->impl;
		}

		// TODO: remove these
		const Config& operator[](const char* key) const
		{
			return (*this)[std::string(key)];
		}

		Config& operator[](const char* key)
		{
			return (*this)[std::string(key)];
		}

		const Config& operator[](const std::string& key) const;
		Config& operator[](const std::string& key);

		bool has_key(const std::string& key) const
		{
			return as_map().count(key) != 0;
		}

		bool erase_key(const std::string& key);

		template<typename T>
		T get_or(const std::string& key, const T& default_value) const;

		template<typename T>
		T as_or(const T& default_value) const
		{
			if (_type == Invalid || _type == BadLookupType) {
				return default_value;
			} else {
				return (T)*this;
			}
		}

		std::string get_or(const std::string& key, const char* default_value) const
		{
			return get_or<std::string>(key, default_value);
		}

		// --------------------------------------------------------------------------------

		static bool deep_eq(const Config& a, const Config& b);

		// ----------------------------------------

		Config deep_clone() const;

		// ----------------------------------------

		// Will check for dangling (unaccessed) map keys recursively
		void check_dangling() const;

		void mark_accessed(bool v) const;

		// Comments on preceeding lines.
		// Like this.
		Comments prefix_comments;
		Comments postfix_comments; // After the value, on the same line. Like this.
		Comments pre_end_brace_comments; // Before the closing } or ]

		inline void check(bool b, const char* msg) const
		{
			if (!b) {
				on_error(msg);
			}
		}

		void assert_type(Type t) const;

		const char* debug_descr() const;

	private:
		void on_error(const std::string& msg) const;

		static const char* type_str(Type t);

	private:
		void free();

	private:
		Type  _type = Invalid;

		union {
			bool           b;
			int64_t        i;
			double         f;
			std::string*   str;
			ConfigMap*     map;
			ConfigList*    list;
			BadLookupInfo* bad_lookup; // In case of BadLookupType
		} _u;

		DocInfo_SP _doc; // So we can name the file
		unsigned   _line = (unsigned)-1; // Where in the source, or -1. Lines are 1-indexed.

		static Config s_invalid;
	};

	// ------------------------------------------------------------------------

	template<> inline bool                          Config::as() const { return as_bool();   }
	template<> inline signed int                    Config::as() const { return as_integer<signed int>();         }
	template<> inline unsigned int                  Config::as() const { return as_integer<unsigned int>();       }
	template<> inline signed long                   Config::as() const { return as_integer<signed long>();        }
	template<> inline unsigned long                 Config::as() const { return as_integer<unsigned long>();      }
	template<> inline signed long long              Config::as() const { return as_integer<signed long long>();   }
	template<> inline unsigned long long            Config::as() const { return as_integer<unsigned long long>(); }
	template<> inline float                         Config::as() const { return as_float();  }
	template<> inline double                        Config::as() const { return as_double(); }
	template<> inline const std::string&            Config::as() const { return as_string(); }
	template<> inline std::string                   Config::as() const { return as_string(); }
	template<> inline const Config::ConfigListImpl& Config::as() const { return as_list();   }
	// template<> inline std::vector<std::string>     Config::as() const { return as_vector<T>();   }

	// ------------------------------------------------------------------------

	template<typename T>
	inline T as(const configuru::Config& config)
	{
		//return (T)config;
		return config.as<T>();
	}

	template<typename T>
	T Config::get_or(const std::string& key, const T& default_value) const
	{
		auto&& map = as_map();
		auto it = map.find(key);
		if (it == map.end()) {
			return default_value;
		} else {
			const auto& entry = it->second;
			entry.accessed = true;
			return configuru::as<T>(entry.value);
		}
	}

	// ------------------------------------------------------------------------

	template<class Config, class Visitor>
	void visit_configs(Config&& config, Visitor&& visitor)
	{
		visitor(config);
		if (config.is_map()) {
			for (auto&& p : config.as_map()) {
				visit_configs(p.second.value, visitor);
			}
		} else if (config.is_list()) {
			for (auto&& e : config.as_list()) {
				visit_configs(e, visitor);
			}
		}
	}

	inline void clear_doc(Config& config) // TODO: shouldn't be needed. Replace with some info of wether a Config is the root of the document it is in.
	{
		visit_configs(config, [&](Config& config){ config.set_doc(nullptr); });
	}

	/*
	inline void replace_doc(Config& root, DocInfo_SP find, DocInfo_SP replacement)
	{
		visit_configs(root, [&](Config& config){
			if (config.doc() == find) {
				config.set_doc(replacement);
			}
		});
	}

	// Will try to merge from 'src' do 'dst', replacing with 'src' on any conflict.
	inline void merge_replace(Config& dst, const Config& src)
	{
		if (dst.is_map() && src.is_map()) {
			for (auto&& p : src.as_map()) {
				merge_replace(dst[p.first], src.second.entry);
			}
		} else {
			dst = src;
		}
	}
	 */

	// ----------------------------------------------------------

	class parse_error : public std::exception
	{
	public:
		parse_error(DocInfo_SP doc, unsigned line, unsigned column, std::string msg)
		: _line(line), _column(column)
		{
			_what = doc->filename + ":" + std::to_string(line) + ":" + std::to_string(column);
			doc->append_include_info(_what);
			_what += ": " + msg;
		}

		const char* what() const noexcept override {
			return _what.c_str();
		}

		unsigned line()   const noexcept { return _line; }
		unsigned column() const noexcept { return _column; }

	private:
		unsigned _line, _column;
		std::string _what;
	};

	// ----------------------------------------------------------

	struct ParseInfo {
		std::unordered_map<std::string, Config> parsed_files; // Two #include gives same Config tree.
	};

	/*
	 Zero-ended Utf-8 encoded string of characters.
	 */
	Config parse_config(const char* str, DocInfo _doc, ParseInfo& info) throw(parse_error);
	Config parse_config(const char* str, const char* name) throw(parse_error);

	Config parse_config_file(const std::string& path, DocInfo_SP doc, ParseInfo& info) throw(parse_error);
	Config parse_config_file(const std::string& path) throw(parse_error);

	// ----------------------------------------------------------
	/*
	 Writes in a pretty format with perfect reversibility of everything (including numbers).
	*/

	struct FormatOptions
	{
		bool json = false;
	};

	std::string write_config(const Config& config, FormatOptions options = FormatOptions());

	void write_config_file(const std::string& path, const Config& config, FormatOptions options = FormatOptions());
}

#endif // CONFIGURU_HEADER_HPP

// ----------------------------------------------------------------------------
// 88 8b    d8 88""Yb 88     888888 8b    d8 888888 88b 88 888888    db    888888 88  dP"Yb  88b 88
// 88 88b  d88 88__dP 88     88__   88b  d88 88__   88Yb88   88     dPYb     88   88 dP   Yb 88Yb88
// 88 88YbdP88 88"""  88  .o 88""   88YbdP88 88""   88 Y88   88    dP__Yb    88   88 Yb   dP 88 Y88
// 88 88 YY 88 88     88ood8 888888 88 YY 88 888888 88  Y8   88   dP""""Yb   88   88  YbodP  88  Y8


/* In one of your .cpp files you need to do the following:
#define CONFIGURU_IMPLEMENTATION
#include <configuru.hpp>

This will define all the Configuru functions so that the linker may find them.
*/

#if defined(CONFIGURU_IMPLEMENTATION) && !defined(CONFIGURU_HAS_BEEN_IMPLEMENTED)
#define CONFIGURU_HAS_BEEN_IMPLEMENTED

// ----------------------------------------------------------------------------
namespace configuru
{
	void DocInfo::append_include_info(std::string& ret, const std::string& indent) const {
		if (!includers.empty()) {
			ret += ", included at:\n";
			for (auto&& includer : includers) {
				ret += indent + includer.doc->filename + ":" + std::to_string(includer.line);
				includer.doc->append_include_info(ret, indent + "    ");
				ret += "\n";
			}
			ret.pop_back();
		}
	}

	struct BadLookupInfo {
		DocInfo_SP            doc;      // Of parent map
		unsigned              line;     // Of parent map
		std::string           key;
		std::atomic<unsigned> ref_count { 1 };
	};

	Config Config::s_invalid;

	Config::Config(const Config& o) : _type(Invalid)
	{
		*this = o;
	}

	void Config::swap(Config& o) noexcept
	{
		prefix_comments.swap(o.prefix_comments);
		postfix_comments.swap(o.postfix_comments);
		pre_end_brace_comments.swap(o.pre_end_brace_comments);
		std::swap(_type, o._type);
		std::swap(_u,    o._u);
		std::swap(_doc,  o._doc);
		std::swap(_line, o._line);
	}

	Config& Config::operator=(const Config& o)
	{
		if (&o == this) { return *this; }
		free();
		_type = o._type;
		if (_type == String) {
			_u.str = new std::string(*o._u.str);
		} else {
			memcpy(&_u, &o._u, sizeof(_u));
			if (_type == BadLookupType) { _u.list->ref_count += 1; }
			if (_type == List)          { _u.list->ref_count += 1; }
			if (_type == Map)           { _u.map->ref_count  += 1; }
		}
		_doc  = o._doc;
		_line = o._line;
		prefix_comments        = o.prefix_comments;
		postfix_comments       = o.postfix_comments;
		pre_end_brace_comments = o.pre_end_brace_comments;
		return *this;
	}

	const Config& Config::operator[](const std::string& key) const
	{
		auto&& map = as_map();
		auto it = map.find(key);
		if (it == map.end()) {
			on_error("Key '" + key + "' not in map");
			return s_invalid;
		} else {
			const auto& entry = it->second;
			entry.accessed = true;
			return entry.value;
		}
	}

	Config& Config::operator[](const std::string& key)
	{
		auto&& map = as_map();
		auto&& entry = map[key];
		if (entry.nr == BAD_ENTRY) {
			// New entry
			entry.nr = (unsigned)map.size() - 1;
			entry.value._type = BadLookupType;
			entry.value._u.bad_lookup = new BadLookupInfo{_doc, _line, key};
		} else {
			entry.accessed = true;
		}
		return entry.value;
	}

	bool Config::erase_key(const std::string& key)
	{
		auto& map = as_map();
		auto it = map.find(key);
		if (it == map.end()) {
			return false;
		} else {
			map.erase(it);
			return true;
		}
	}

	bool Config::deep_eq(const Config& a, const Config& b)
	{
		if (a._type != b._type) { return false; }
		if (a._type == Null)   { return true; }
		if (a._type == Bool)   { return a._u.b    == b._u.b;    }
		if (a._type == Int)    { return a._u.i    == b._u.i;    }
		if (a._type == Float)  { return a._u.f    == b._u.f;    }
		if (a._type == String) { return *a._u.str == *b._u.str; }
		if (a._type == Map)    {
			if (a._u.map == b._u.map) { return true; }
			auto&& a_map = a.as_map();
			auto&& b_map = b.as_map();
			if (a_map.size() != b_map.size()) { return false; }
			for (auto&& p: a_map) {
				auto it = b_map.find(p.first);
				if (it == b_map.end()) { return false; }
				if (!deep_eq(p.second.value, it->second.value)) { return false; }
			}
			return true;
		}
		if (a._type == List)    {
			if (a._u.list == b._u.list) { return true; }
			auto&& a_list = a.as_list();
			auto&& b_list = b.as_list();
			if (a_list.size() != b_list.size()) { return false; }
			for (size_t i=0; i<a_list.size(); ++i) {
				if (!deep_eq(a_list[i], a_list[i])) {
					return false;
				}
			}
			return true;
		}

		return false;
	}

	Config Config::deep_clone() const
	{
		Config ret = *this;
		if (ret._type == Map) {
			ret = Config::new_map();
			for (auto&& p : this->as_map()) {
				auto& dst = ret._u.map->impl[p.first];
				dst.nr    = p.second.nr;
				dst.value = p.second.value.deep_clone();
			}
		}
		if (ret._type == List) {
			ret = Config::new_list();
			for (auto&& value : this->as_list()) {
				ret.push_back( value.deep_clone() );
			}
		}
		return ret;
	}

	void Config::check_dangling() const
	{
		// TODO: iterating over a map should count as accessing the values
		if (is_map()) {
			for (auto&& p : as_map()) {
				auto&& entry = p.second;
				auto&& value = entry.value;
				if (entry.accessed) {
					value.check_dangling();
				} else {
					auto where_str = value.where();
					CONFIGURU_ON_DANGLING(where_str.c_str(), p.first.c_str());
				}
			}
		} else if (is_list()) {
			for (auto&& e : as_list()) {
				e.check_dangling();
			}
		}
	}

	void Config::mark_accessed(bool v) const
	{
		if (is_map()) {
			for (auto&& p : as_map()) {
				auto&& entry = p.second;
				entry.accessed = v;
				entry.value.mark_accessed(v);
			}
		} else if (is_list()) {
			for (auto&& e : as_list()) {
				e.mark_accessed(v);
			}
		}
	}

	const char* Config::debug_descr() const {
		switch (_type) {
			case Invalid:       return "invalid";
			case BadLookupType: return "undefined";
			case Null:          return "null";
			case Bool:          return _u.b ? "true" : "false";
			case Int:           return "integer";
			case Float:         return "float";
			case String:        return _u.str->c_str();
			case List:          return "list";
			case Map:           return "map";
			default:            return "BROKEN Config";
		}
	}

	const char* Config::type_str(Type t)
	{
		switch (t) {
			case Invalid:       return "invalid";
			case BadLookupType: return "undefined";
			case Null:          return "null";
			case Bool:          return "bool";
			case Int:           return "integer";
			case Float:         return "float";
			case String:        return "string";
			case List:          return "list";
			case Map:           return "map";
			default:            return "BROKEN Config";
		}
	}

	void Config::free()
	{
		if (_type == BadLookupType) {
			if (--_u.bad_lookup->ref_count == 0) {
				delete _u.bad_lookup;
			}
		} else if (_type == Map) {
			if (--_u.map->ref_count == 0) {
				delete _u.map;
			}
		} else if (_type == List) {
			if (--_u.list->ref_count == 0) {
				delete _u.list;
			}
		} else if (_type == String) {
			delete _u.str;
		}
		_type = Invalid;
	}

	std::string where_is(const DocInfo_SP& doc, unsigned line)
	{
		if (doc) {
			std::string ret = doc->filename;
			if (line != BAD_ENTRY) {
				ret += ":" + std::to_string(line);
			}
			doc->append_include_info(ret);
			ret += ": ";
			return ret;
		} else if (line != BAD_ENTRY) {
			return "line " + std::to_string(line) + ": ";
		} else {
			return "";
		}
	}

	std::string Config::where() const
	{
		return where_is(_doc, _line);
	}

	void Config::on_error(const std::string& msg) const
	{
		CONFIGURU_ONERROR(where() + msg);
	}

	void Config::assert_type(Type t) const
	{
		if (_type == BadLookupType) {
			auto where = where_is(_u.bad_lookup->doc, _u.bad_lookup->line);
			CONFIGURU_ONERROR(where + "Failed to find key '" + _u.bad_lookup->key + "'");
		} else if (_type != t) {
			CONFIGURU_ONERROR(where() + "Expected " + type_str(t) + ", got " + type_str(_type));
		}
	}
}


// ----------------------------------------------------------------------------
// 88""Yb    db    88""Yb .dP"Y8 888888 88""Yb
// 88__dP   dPYb   88__dP `Ybo." 88__   88__dP
// 88"""   dP__Yb  88"Yb  o.`Y8b 88""   88"Yb
// 88     dP""""Yb 88  Yb 8bodP' 888888 88  Yb

#include <cerrno>
#include <cstdlib>

using namespace configuru;

void append(Comments& a, Comments&& b)
{
	for (auto&& entry : b) {
		a.emplace_back(std::move(entry));
	}
}

namespace configuru
{
	// Returns the number of bytes written, or 0 on error
	size_t encode_utf8(std::string& dst, uint64_t c)
	{
		if (c <= 0x7F)  // 0XXX XXXX - one byte
		{
			dst += (char) c;
			return 1;
		}
		else if (c <= 0x7FF)  // 110X XXXX - two bytes
		{
			dst += (char) ( 0xC0 | (c >> 6) );
			dst += (char) ( 0x80 | (c & 0x3F) );
			return 2;
		}
		else if (c <= 0xFFFF)  // 1110 XXXX - three bytes
		{
			dst += (char) (0xE0 | (c >> 12));
			dst += (char) (0x80 | ((c >> 6) & 0x3F));
			dst += (char) (0x80 | (c & 0x3F));
			return 3;
		}
		else if (c <= 0x1FFFFF)  // 1111 0XXX - four bytes
		{
			dst += (char) (0xF0 | (c >> 18));
			dst += (char) (0x80 | ((c >> 12) & 0x3F));
			dst += (char) (0x80 | ((c >> 6) & 0x3F));
			dst += (char) (0x80 | (c & 0x3F));
			return 4;
		}
		else if (c <= 0x3FFFFFF)  // 1111 10XX - five bytes
		{
			dst += (char) (0xF8 | (c >> 24));
			dst += (char) (0x80 | (c >> 18));
			dst += (char) (0x80 | ((c >> 12) & 0x3F));
			dst += (char) (0x80 | ((c >> 6) & 0x3F));
			dst += (char) (0x80 | (c & 0x3F));
			return 5;
		}
		else if (c <= 0x7FFFFFFF)  // 1111 110X - six bytes
		{
			dst += (char) (0xFC | (c >> 30));
			dst += (char) (0x80 | ((c >> 24) & 0x3F));
			dst += (char) (0x80 | ((c >> 18) & 0x3F));
			dst += (char) (0x80 | ((c >> 12) & 0x3F));
			dst += (char) (0x80 | ((c >> 6) & 0x3F));
			dst += (char) (0x80 | (c & 0x3F));
			return 6;
		}
		else {
			return 0; // Error
		}
	}

	std::string quote(char c) {
		if (c == 0)    { return "<eof>";   }
		if (c == ' ')  { return "<space>"; }
		if (c == '\n') { return "'\\n'";   }
		if (c == '\t') { return "'\\t'";   }
		if (c == '\r') { return "'\\r'";   }
		if (c == '\b') { return "'\\b'";   }
		return (std::string)"'" + c + "'";
	}

	struct state {
		const char* ptr;
		unsigned    line_nr;
		const char* line_start;
	};

	struct Parser {
		Parser(const char* str, DocInfo_SP doc, ParseInfo& info);

		bool skip_white(Comments* out_comments, int& out_indentation, bool break_on_newline);

		bool skip_white_ignore_comments() {
			int indentation;
			return skip_white(nullptr, indentation, false);
		}

		bool skip_pre_white(Comments* out_comments, int& out_indentation) {
			return skip_white(out_comments, out_indentation, false);
		}
		bool skip_post_white(Comments* out_comments) {
			int indentation;
			return skip_white(out_comments, indentation, true);
		}
		bool skip_pre_white(Config* config, int& out_indentation) {
			return skip_pre_white(&config->prefix_comments, out_indentation);
		}
		bool skip_post_white(Config* config) {
			return skip_post_white(&config->postfix_comments);
		}

		Config top_level();
		void parse_value(Config& out, bool* out_did_skip_postwhites);
		void parse_list(Config& dst);
		void parse_list_contents(Config& dst);
		void parse_map(Config& dst);
		void parse_map_contents(Config& dst);
		void parse_finite_number(Config& dst);
		std::string parse_string();
		uint64_t parse_hex(int count);
		void parse_macro(Config& dst);

		void tag(Config& var)
		{
			var.tag(_doc, _line_nr, column());
		}

		state get_state() const
		{
			return { _ptr, _line_nr, _line_start };
		}

		void set_state(state s) {
			_ptr        = s.ptr;
			_line_nr    = s.line_nr;
			_line_start = s.line_start;
		}

		unsigned column() const
		{
			return (unsigned)(_ptr - _line_start + 1);
		}

		void throw_error(const std::string& desc) {
			throw parse_error(_doc, _line_nr, column(), desc + " (at " + quote(_ptr[0]) + ")");
		}

		void throw_indentation_error(int found_tabs, int expected_tabs) {
			char buff[128];
			snprintf(buff, sizeof(buff), "Bad indentation: expected %d tabs, found %d", found_tabs, expected_tabs);
			throw_error(buff);
		}

		void parse_assert(bool b, const char* error_msg) {
			if (!b) {
				throw_error(error_msg);
			}
		}

		void swallow(char c) {
			if (_ptr[0] == c) {
				_ptr += 1;
			} else {
				throw_error("Expected " + quote(c));
			}
		}

		bool try_swallow(const char* str) {
			auto n = strlen(str);
			if (strncmp(str, _ptr, n) == 0) {
				_ptr += n;
				return true;
			} else {
				return false;
			}
		}

		void swallow(const char* str, const char* error_msg) {
			parse_assert(try_swallow(str), error_msg);
		}

	private:
		bool IDENT_STARTERS[256] = { 0 };
		bool IDENT_CHARS[256]    = { 0 };

	private:
		DocInfo_SP  _doc;
		ParseInfo&  _info;

		const char* _ptr;
		unsigned    _line_nr;
		const char* _line_start;
		int         _indentation = 0; // Expected number of tabs between a \n and the next key/value
	};

	// --------------------------------------------

	// Sets an inclusive range
	void set_range(bool lookup[256], char a, char b)
	{
		for (char c=a; c<=b; ++c) {
			lookup[c] = true;
		}
	}

	Parser::Parser(const char* str, DocInfo_SP doc, ParseInfo& info) : _doc(doc), _info(info)
	{
		_line_nr    = 1;
		_ptr        = str;
		_line_start = str;

		IDENT_STARTERS['_'] = true;
		set_range(IDENT_STARTERS, 'a', 'z');
		set_range(IDENT_STARTERS, 'A', 'Z');

		IDENT_CHARS['_'] = true;
		set_range(IDENT_CHARS, 'a', 'z');
		set_range(IDENT_CHARS, 'A', 'Z');
		set_range(IDENT_CHARS, '0', '9');
	}

	// Returns true if we did skip white-space.
	// out_indentation is the number of *tabs* on the last line we did skip on.
	// iff out_indentation is -1 there is a non-tab
	bool Parser::skip_white(Comments* out_comments, int& out_indentation, bool break_on_newline)
	{
		auto start_ptr = _ptr;
		out_indentation = 0;

		for (;;) {
			if (_ptr[0] == '\n') {
				// Unix style newline
				_ptr += 1;
				_line_nr += 1;
				_line_start = _ptr;
				out_indentation = 0;
				if (break_on_newline) { return true; }
			}
			else if (_ptr[0] == '\r') {
				// CR-LF - windows style newline
				parse_assert(_ptr[1] == '\n', "CR with no LF");
				_ptr += 2;
				_line_nr += 1;
				_line_start = _ptr;
				out_indentation = 0;
				if (break_on_newline) { return true; }
			}
			else if (_ptr[0] == '\t') {
				++_ptr;
				parse_assert(out_indentation != -1, "Tabs should only occur on the start of a line!");
				++out_indentation;
			}
			else if (_ptr[0] == ' ') {
				++_ptr;
				out_indentation = -1;
			}
			else if (_ptr[0] == '/' && _ptr[1] == '/') {
				// Single line comment
				auto start = _ptr;
				_ptr += 2;
				while (_ptr[0] && _ptr[0] != '\n') {
					_ptr += 1;
				}
				if (out_comments) { out_comments->emplace_back(start, _ptr - start); }
				out_indentation = 0;
				if (break_on_newline) { return true; }
			}
			else if (_ptr[0] == '/' && _ptr[1] == '*') {
				// Multi-line comment
				auto state = get_state(); // So we can point out the start if there's an error
				_ptr += 2;
				unsigned nesting = 1; // We allow nested /**/ comments
				do
				{
					if (_ptr[0]==0) {
						set_state(state);
						throw_error("Non-ending /* comment");
					}
					else if (_ptr[0]=='/' && _ptr[1]=='*') {
						_ptr += 2;
						nesting += 1;
					}
					else if (_ptr[0]=='*' && _ptr[1]=='/') {
						_ptr += 2;
						nesting -= 1;
					}
					else if (_ptr[0] == '\n') {
						_ptr += 1;
						_line_nr += 1;
						_line_start = _ptr;
					} else {
						_ptr += 1;
					}
				} while (nesting > 0);
				if (out_comments) { out_comments->emplace_back(state.ptr, _ptr - state.ptr); }
				out_indentation = -1;
				if (break_on_newline) { return true; }
			}
			else {
				break;
			}
		}

		if (start_ptr == _ptr) {
			out_indentation = -1;
			return false;
		} else {
			return true;
		}
	}

	/*
	The top-level can be any value, OR the innerds of an map:
	foo = 1
	"bar": 2
	*/
	Config Parser::top_level()
	{
		auto state = get_state();

		bool is_map = false;
		skip_white_ignore_comments();

		if (IDENT_STARTERS[_ptr[0]]) {
			is_map = true;
		} else if (_ptr[0] == '"' || _ptr[0] == '@') {
			parse_string();
			skip_white_ignore_comments();
			is_map = (_ptr[0] == ':' || _ptr[0] == '=');
		}

		set_state(state); // restore

		Config ret;
		tag(ret);

		if (is_map) {
			parse_map_contents(ret);
		} else {
			parse_list_contents(ret);
		}

		skip_post_white(&ret);

		parse_assert(_ptr[0] == 0, "Expected EoF");

		if (!is_map && ret.list_size()==0) {
			// Empty file:
			auto empty_map = Config::new_map();
			append(empty_map.prefix_comments,        std::move(ret.prefix_comments));
			append(empty_map.postfix_comments,       std::move(ret.postfix_comments));
			append(empty_map.pre_end_brace_comments, std::move(ret.pre_end_brace_comments));
			return empty_map;
		}

		if (!is_map && ret.list_size()==1) {
			// A single value - not a list after all:
			Config first( std::move(ret[0u]) );
			append(first.prefix_comments,        std::move(ret.prefix_comments));
			append(first.postfix_comments,       std::move(ret.postfix_comments));
			append(first.pre_end_brace_comments, std::move(ret.pre_end_brace_comments));
			return first;
		}

		return ret;
	}

	void Parser::parse_value(Config& dst, bool* out_did_skip_postwhites)
	{
		int line_indentation;
		skip_pre_white(&dst, line_indentation);
		tag(dst);

		if (line_indentation >= 0 && _indentation - 1 != line_indentation) {
			throw_indentation_error(_indentation - 1, line_indentation);
		}

		if (_ptr[0] == '"' || _ptr[0] == '@') {
			dst = parse_string();
		}
		else if (_ptr[0] == 'n') {
			parse_assert(_ptr[1]=='u' && _ptr[2]=='l' && _ptr[3]=='l', "Expected 'null'");
			_ptr += 4;
			parse_assert(!IDENT_CHARS[_ptr[0]], "Expected 'null'");
			dst = nullptr;
		}
		else if (_ptr[0] == 't') {
			parse_assert(_ptr[1]=='r' && _ptr[2]=='u' && _ptr[3]=='e', "Expected 'true'");
			_ptr += 4;
			parse_assert(!IDENT_CHARS[_ptr[0]], "Expected 'true'");
			dst = true;
		}
		else if (_ptr[0] == 'f') {
			parse_assert(_ptr[1]=='a' && _ptr[2]=='l' && _ptr[3]=='s' && _ptr[4]=='e', "Expected 'false'");
			_ptr += 5;
			parse_assert(!IDENT_CHARS[_ptr[0]], "Expected 'false'");
			dst = false;
		}
		else if (_ptr[0] == '{') {
			parse_map(dst);
		}
		else if (_ptr[0] == '[') {
			parse_list(dst);
		}
		else if (_ptr[0] == '#') {
			parse_macro(dst);
		}
		else {
			// Some kind of number:

			if (_ptr[0] == '-' && _ptr[1] == 'i' && _ptr[2]=='n' && _ptr[3]=='f') {
				_ptr += 4;
				parse_assert(!IDENT_CHARS[_ptr[0]], "Expected -inf");
				dst = -std::numeric_limits<double>::infinity();
			}
			else if (_ptr[0] == '+' && _ptr[1] == 'i' && _ptr[2]=='n' && _ptr[3]=='f') {
				_ptr += 4;
				parse_assert(!IDENT_CHARS[_ptr[0]], "Expected +inf");
				dst = std::numeric_limits<double>::infinity();
			}
			else if (_ptr[0] == '+' && _ptr[1] == 'N' && _ptr[2]=='a' && _ptr[3]=='N') {
				_ptr += 4;
				parse_assert(!IDENT_CHARS[_ptr[0]], "Expected +NaN");
				dst = std::numeric_limits<double>::quiet_NaN();
			}
			else {
				parse_finite_number(dst);
			}
		}

		*out_did_skip_postwhites = skip_post_white(&dst);
	}

	void Parser::parse_list(Config& list)
	{
		auto state = get_state();

		swallow('[');

		_indentation += 1;
		parse_list_contents(list);
		_indentation -= 1;

		if (_ptr[0] == ']') {
			_ptr += 1;
		} else {
			set_state(state);
			throw_error("Non-terminated list");
		}
	}

	void Parser::parse_list_contents(Config& list)
	{
		list.make_list();

		for (;;)
		{
			Config value;
			int line_indentation;
			skip_pre_white(&value, line_indentation);

			if (_ptr[0] == ']') {
				if (line_indentation >= 0 && _indentation - 1 != line_indentation) {
					throw_indentation_error(_indentation - 1, line_indentation);
				}
				list.pre_end_brace_comments = value.prefix_comments;
				break;
			}

			if (!_ptr[0]) {
				list.pre_end_brace_comments = value.prefix_comments;
				break;
			}

			if (line_indentation >= 0 && _indentation != line_indentation) {
				throw_indentation_error(_indentation, line_indentation);
			}

			if (IDENT_STARTERS[_ptr[0]]) {
				throw_error("Found identifier; expected value. Did you mean to use an {object} rather than a [list]?");
			}

			bool has_separator;
			parse_value(value, &has_separator);

			if (_ptr[0]==',') {
				_ptr += 1;
				skip_post_white(&value);
				has_separator = true;
			}

			parse_assert(has_separator || !_ptr[0] || _ptr[0] == ']', "Expected a space, newline, comma or closing bracket");
			list.push_back(std::move(value));
		}
	}

	void Parser::parse_map(Config& map)
	{
		auto state = get_state();

		swallow('{');

		_indentation += 1;
		parse_map_contents(map);
		_indentation -= 1;

		if (_ptr[0] == '}') {
			_ptr += 1;
		} else {
			set_state(state);
			throw_error("Non-terminated map");
		}
	}

	void Parser::parse_map_contents(Config& map)
	{
		map.make_map();

		for (;;)
		{
			Config value;
			int line_indentation;
			skip_pre_white(&value, line_indentation);

			if (_ptr[0] == '}') {
				if (line_indentation >= 0 && _indentation - 1 != line_indentation) {
					throw_indentation_error(_indentation - 1, line_indentation);
				}
				map.pre_end_brace_comments = value.prefix_comments;
				break;
			}

			if (!_ptr[0]) {
				map.pre_end_brace_comments = value.prefix_comments;
				break;
			}

			if (line_indentation >= 0 && _indentation != line_indentation) {
				throw_indentation_error(_indentation, line_indentation);
			}

			std::string key;

			if (IDENT_STARTERS[_ptr[0]]) {
				while (IDENT_CHARS[_ptr[0]]) {
					key += _ptr[0];
					_ptr += 1;
				}
			}
			else if (_ptr[0] == '"' || _ptr[0] == '@') {
				key = parse_string();
			} else {
				throw_error("Map key expected (either an identifier or a quoted string), got " + quote(_ptr[0]));
			}

			skip_white_ignore_comments();
			if (_ptr[0] == ':' || _ptr[0] == '=') {
				_ptr += 1;
				skip_white_ignore_comments();
			} else if (_ptr[0] == '{' || _ptr[0] == '#') {
				// Ok to ommit =: in this case
			} else {
				throw_error("Expected one of '=', ':', '{' or '#' after map key");
			}

			if (map.has_key(key)) {
				throw_error("Duplicate key: \"" + key + "\"");
			}

			bool has_separator;
			parse_value(value, &has_separator);

			if (_ptr[0] == ',') {
				_ptr += 1;
				skip_post_white(&value);
				has_separator = true;
			} else if (_ptr[0] == ';') {
				_ptr += 1;
				skip_post_white(&value);
				has_separator = true;
			}

			parse_assert(has_separator || !_ptr[0] || _ptr[0] == '}', "Expected a space, comma, semi-color or closing curly brace");

			map[key] = std::move(value);
		}
	}

	void Parser::parse_finite_number(Config& dst)
	{
		int sign = +1;

		if (_ptr[0] == '+') {
			_ptr += 1;
			skip_white_ignore_comments();
		}
		if (_ptr[0] == '-') {
			_ptr += 1;
			skip_white_ignore_comments();
			sign = -1;
		}

		parse_assert(_ptr[0] != '+' && _ptr[0] != '-', "Duplicate sign");

		// Check if it's an integer:
		if (_ptr[0] == '0' && _ptr[1] == 'x') {
			// hex
			_ptr += 2;
			dst = sign * (int64_t)strtoull(_ptr, (char**)&_ptr, 16);
		}
		if (_ptr[0] == '0' && _ptr[1] == 'b') {
			// binary
			_ptr += 2;
			dst = sign * (int64_t)strtoull(_ptr, (char**)&_ptr, 2);
		}

		const char* p = _ptr;

		while ('0' <= *p && *p <= '9') {
			p += 1;
		}
		if (*p == '.' || *p == 'e' || *p == 'E') {
			// Floating point
			dst = sign * strtod(_ptr, (char**)&_ptr);
		} else {
			// Integer:
			dst = sign * strtoll(_ptr, (char**)&_ptr, 10);
		}
	}

	std::string Parser::parse_string()
	{
		auto state = get_state();

		if (_ptr[0] == '@') {
			// C# style verbatim string - everything until the next " except "" which is ":
			_ptr += 1;
			swallow('"');

			std::string str;

			for (;;) {
				if (_ptr[0] == 0) {
					set_state(state);
					throw_error("Unterminated verbatim string");
				}
				else if (_ptr[0] == '\n') {
					throw_error("Newline in verbatim string");
				}
				else if (_ptr[0] == '"' && _ptr[1] == '"') {
					// Escaped quote
					_ptr += 2;
					str += '"';
				}
				else if (_ptr[0] == '"') {
					_ptr += 1;
					return str;
				}
				else {
					str += _ptr[0];
					_ptr += 1;
				}
			}
		}

		parse_assert(_ptr[0] == '"', "Quote (\") expected");

		if (_ptr[1] == '"' && _ptr[2] == '"') {
			// Multiline string - everything until the next """:
			_ptr += 3;
			const char* start = _ptr;
			for (;;) {
				if (_ptr[0]==0 || _ptr[1]==0 || _ptr[2]==0) {
					set_state(state);
					throw_error("Unterminated multiline string");
				}

				if (_ptr[0] == '"' && _ptr[1] == '"' && _ptr[2] == '"' && _ptr[3] != '"') {
					std::string str(start, _ptr);
					_ptr += 3;
					return str;
				}

				if (_ptr[0] == '\n') {
					_ptr += 1;
					_line_nr += 1;
					_line_start = _ptr;
				} else {
					_ptr += 1;
				}
			}
		} else {
			// Normal string
			_ptr += 1; // Swallow quote

			std::string str;
			for (;;) {
				if (_ptr[0] == 0) {
					set_state(state);
					throw_error("Unterminated string");
				}
				if (_ptr[0] == '"') {
					_ptr += 1;
					return str;
				}
				if (_ptr[0] == '\n') {
					throw_error("Newline in string");
				}

				if (_ptr[0] == '\\') {
					// Escape sequence
					_ptr += 1;

					if (_ptr[0] == '"') {
						str += '"';
						_ptr += 1;
					} else if (_ptr[0] == '\'') {
						str += '\'';
						_ptr += 1;
					} else if (_ptr[0] == '\\') {
						str += '\\';
						_ptr += 1;
					} else if (_ptr[0] == '/') {
						str += '/';
						_ptr += 1;
					} else if (_ptr[0] == '0') {
						str += '\0';
						_ptr += 1;
					} else if (_ptr[0] == 'b') {
						str += '\b';
						_ptr += 1;
					} else if (_ptr[0] == 'f') {
						str += '\f';
						_ptr += 1;
					} else if (_ptr[0] == 'n') {
						str += '\n';
						_ptr += 1;
					} else if (_ptr[0] == 'r') {
						str += '\r';
						_ptr += 1;
					} else if (_ptr[0] == 't') {
						str += '\t';
						_ptr += 1;
					} else if (_ptr[0] == 'u') {
						// Four hexadecimal characters
						_ptr += 1;
						uint64_t unicode = parse_hex(4);
						auto num_bytes_written = encode_utf8(str, unicode);
						parse_assert(num_bytes_written > 0, "Bad unicode codepoint");
					} else if (_ptr[0] == 'U') {
						// eight hexadecimal characters
						_ptr += 1;
						uint64_t unicode = parse_hex(8);
						auto num_bytes_written = encode_utf8(str, unicode);
						parse_assert(num_bytes_written > 0, "Bad unicode codepoint");
					} else {
						throw_error("Unknown escape character");
					}
				} else {
					str += _ptr[0];
					_ptr += 1;
				}
			}
		}
	}

	uint64_t Parser::parse_hex(int count)
	{
		uint64_t ret = 0;
		for (int i=0; i<count; ++i) {
			ret *= 16;
			char c = _ptr[i];
			if ('0' <= c && c <= '9') {
				ret += c - '0';
			} else if ('a' <= c && c <= 'f') {
				ret += c - 'a';
			} else if ('A' <= c && c <= 'F') {
				ret += c - 'A';
			} else {
				throw_error("Expected hexadecimal digit, got " + quote(_ptr[0]));
			}
		}
		_ptr += count;
		return ret;
	}

	void Parser::parse_macro(Config& dst)
	{
		swallow("#include", "Expected '#include'");
		skip_white_ignore_comments();

		bool absolute;
		char terminator;

		if (_ptr[0] == '"') {
			absolute = false;
			terminator = '"';
		} else if (_ptr[0] == '<') {
			absolute = true;
			terminator = '>';
		} else {
			throw_error("Expected \" or <");
		}

		auto state = get_state();
		_ptr += 1;
		auto start = _ptr;
		std::string path;
		for (;;) {
			if (_ptr[0] == 0) {
				set_state(state);
				throw_error("Unterminated include path");
			} else if (_ptr[0] == terminator) {
				path = std::string(start, _ptr-start);
				_ptr += 1;
				break;
			} else if (_ptr[0] == '\n') {
				throw_error("Newline in string");
			} else {
				_ptr += 1;
			}
		}

		if (!absolute) {
			auto my_path = _doc->filename;
			auto pos = my_path.find_last_of('/');
			if (pos != std::string::npos) {
				auto my_dir = my_path.substr(0, pos+1);
				path = my_dir + path;
			}
		}

		auto it = _info.parsed_files.find(path);
		if (it == _info.parsed_files.end()) {
			auto child_doc = std::make_shared<DocInfo>(path);
			child_doc->includers.emplace_back(_doc, _line_nr);
			dst = parse_config_file(path.c_str(), child_doc, _info);
			_info.parsed_files[path] = dst;
		} else {
			auto child_doc = it->second.doc();
			child_doc->includers.emplace_back(_doc, _line_nr);
			dst = it->second;
		}
	}

	// ----------------------------------------------------------------------------------------

	Config parse_config(const char* str, DocInfo_SP doc, ParseInfo& info) throw(parse_error)
	{
		Parser p(str, doc, info);
		return p.top_level();
	}

	Config parse_config(const char* str, const char* name) throw(parse_error)
	{
		ParseInfo info;
		return parse_config(str, std::make_shared<DocInfo>(name), info);
	}

	std::string read_text_file(const char* path)
	{
		FILE* fp = fopen(path, "rb");
		if (fp == nullptr) {
			CONFIGURU_ONERROR((std::string)"Failed to open '" + path + "' for reading: " + strerror(errno));
		}
		std::string contents;
		fseek(fp, 0, SEEK_END);
		contents.resize(ftell(fp));
		rewind(fp);
		auto num_read = fread(&contents[0], 1, contents.size(), fp);
		fclose(fp);
		if (num_read != contents.size()) {
			CONFIGURU_ONERROR((std::string)"Failed to read from '" + path + "': " + strerror(errno));
		}
		return contents;
	}

	Config parse_config_file(const std::string& path, DocInfo_SP doc, ParseInfo& info) throw(parse_error)
	{
		// auto file = util::FILEWrapper::read_text_file(path);
		auto file = read_text_file(path.c_str());
		return parse_config(file.c_str(), doc, info);
	}

	Config parse_config_file(const std::string& path) throw(parse_error)
	{
		ParseInfo info;
		return parse_config_file(path, std::make_shared<DocInfo>(path), info);
	}
}


// ----------------------------------------------------------------------------
// Yb        dP 88""Yb 88 888888 888888 88""Yb
//  Yb  db  dP  88__dP 88   88   88__   88__dP
//   YbdPYbdP   88"Yb  88   88   88""   88"Yb
//    YP  YP    88  Yb 88   88   888888 88  Yb

#include <cstdlib>  // strtod
#include <iomanip>
#include <sstream>

namespace configuru
{
	bool is_identifier(const char* p)
	{
		if (*p == '_'
			 || ('a' <= *p && *p <= 'z')
			 || ('A' <= *p && *p <= 'Z'))
		{
			++p;
			while (*p) {
				if (*p == '_'
					 || ('a' <= *p && *p <= 'z')
					 || ('A' <= *p && *p <= 'Z')
					 || ('0' <= *p && *p <= '9'))
				{
					++p;
				} else {
					return false;
				}
			}
			return true;
		} else {
			return false;
		}
	}

	bool is_simple(const Config& var)
	{
		if (var.is_list() || var.is_map()) { return false; }
		if (!var.prefix_comments.empty())  { return false; }
		if (!var.postfix_comments.empty()) { return false; }
		return true;
	}

	bool is_all_numbers(const Config& list)
	{
		for (auto& v: list.as_list()) {
			if (!v.is_number()) {
				return false;
			}
		}
		return true;
	}

	bool is_simple_list(const Config& list)
	{
		if (list.list_size() <= 16 && is_all_numbers(list)) {
			return true; // E.g., a 4x4 matrix
		}

		if (list.list_size() > 4) { return false; }
		for (auto& v: list.as_list()) {
			if (!is_simple(v)) {
				return false;
			}
		}
		return true;
	}

	struct Writer
	{
		DocInfo_SP        doc;
		FormatOptions     options;
		std::stringstream ss;

		void write_indent(unsigned indent)
		{
			for (unsigned i=0; i<indent; ++i) {
				ss << "\t";
			}
		}

		void write_prefix_comments(unsigned indent, const Comments& comments)
		{
			if (options.json) { return; }
			if (!comments.empty()) {
				ss << "\n";
				for (auto&& c : comments) {
					write_indent(indent);
					ss << c << "\n";
				}
			}
		}

		void write_postfix_comments(unsigned indent, const Comments& comments)
		{
			if (options.json) { return; }
			(void)indent; // TODO: reindent comments
			for (auto&& c : comments) {
				ss << " " << c;
			}
		}

		void write_pre_brace_comments(unsigned indent, const Comments& comments)
		{
			write_prefix_comments(indent, comments);
		}

		void write_value(unsigned indent, const Config& config,
							  bool write_prefix, bool write_postfix)
		{
			if (!options.json && config.doc() && config.doc() != this->doc) {
				write_config_file(config.doc()->filename, config);
				ss << "#include <" << config.doc()->filename << ">";
				return;
			}

			if (write_prefix) {
				write_prefix_comments(indent, config.prefix_comments);
			}

			if (config.is_null()) {
				ss << "null";
			} else if (config.is_bool()) {
				ss << ((bool)config ? "true" : "false");
			} else if (config.is_int()) {
				ss << (int64_t)config;
			} else if (config.is_float()) {
				write_number( (double)config );
			} else if (config.is_string()) {
				write_string(config.as_string());
			} else if (config.is_list()) {
				if (config.list_size() == 0 && config.pre_end_brace_comments.empty()) {
					ss << "[ ]";
				} else if (is_simple_list(config)) {
					ss << "[ ";
					auto&& list = config.as_list();
					for (size_t i = 0; i < list.size(); ++i) {
						write_value(indent + 1, list[i], false, true);
						if (options.json && i + 1 < list.size()) {
							ss << ", ";
						} else {
							ss << " ";
						}
					}
					write_pre_brace_comments(indent + 1, config.pre_end_brace_comments);
					ss << "]";
				} else {
					ss << "[\n";
					auto&& list = config.as_list();
					for (size_t i = 0; i < list.size(); ++i) {
						write_prefix_comments(indent + 1, list[i].prefix_comments);
						write_indent(indent + 1);
						write_value(indent + 1, list[i], false, true);
						if (options.json && i + 1 < list.size()) {
							ss << ",\n";
						} else {
							ss << "\n";
						}
					}
					write_pre_brace_comments(indent + 1, config.pre_end_brace_comments);
					write_indent(indent);
					ss << "]";
				}
			} else if (config.is_map()) {
				if (config.map_size() == 0 && config.pre_end_brace_comments.empty()) {
					ss << "{ }";
				} else {
					ss << "{\n";
					write_map_contents(indent + 1, config);
					write_indent(indent);
					ss << "}";
				}
			} else {
				throw std::runtime_error("Cannot serialize Config");
			}

			if (write_postfix) {
				write_postfix_comments(indent, config.postfix_comments);
			}
		}

		void write_map_contents(unsigned indent, const Config& config)
		{
#if 0
			for (auto&& p : config.as_map()) {
				write_prefix_comments(ident, p.second.value);
				write_indent(indent);
				write_key(p.first);
				ss << " = ";
				write_value(indent, p.second.value, false, true);
				ss << "\n";
			}
#else
			// Write in same order as input:
			auto&& map = config.as_map();
			std::vector<Config::ConfigMapImpl::const_iterator> pairs;
			size_t longest_key = 0;
			for (auto it=map.begin(); it!=map.end(); ++it) {
				pairs.push_back(it);
				#if 1
					longest_key = std::max(longest_key, it->first.size());
				#else
					if (!it->second.value.is_map()) {
						longest_key = std::max(longest_key, it->first.size());
					}
				#endif
			}
			std::sort(begin(pairs), end(pairs), [](auto a, auto b) {
				return a->second.nr < b->second.nr;
			});
			size_t i = 0;
			for (auto&& it : pairs) {
				auto&& value = it->second.value;
				write_prefix_comments(indent, value.prefix_comments);
				write_indent(indent);
				write_key(it->first);
				if (options.json) {
					ss << ": ";
					for (size_t i=it->first.size(); i<longest_key; ++i) {
						ss << " ";
					}
				} else if (value.is_map() && value.map_size() != 0) {
					ss << " ";
				} else {
					for (size_t i=it->first.size(); i<longest_key; ++i) {
						ss << " ";
					}
					ss << " = ";
				}
				write_value(indent, value, false, true);
				if (options.json && i + 1 < pairs.size()) {
					ss << ",\n";
				} else {
					ss << "\n";
				}
				i += 1;
			}
#endif

			write_pre_brace_comments(indent, config.pre_end_brace_comments);
		}

		void write_key(const std::string& str)
		{
			if (options.json || !is_identifier(str.c_str())) {
				write_string(str);
			} else {
				ss << str;
			}
		}

		void write_number(double val)
		{
			auto as_int = (int64_t)val;
			if ((double)as_int == val) {
				ss << as_int;
			} else if (std::isfinite(val)) {
				// No unnecessary zeros.

				auto as_float = (float)val;
				if ((double)as_float == val) {
					// It's actually a float!
					// Try short and nice:
					//auto str = to_string(val);
					std::stringstream temp_ss;
					temp_ss << as_float;
					auto str = temp_ss.str();
					if (std::strtof(str.c_str(), nullptr) == as_float) {
						ss << str;
						return;
					} else {
						ss << std::setprecision(8) << as_float;
					}
				} else {
					// Try short and nice:
					//auto str = to_string(val);
					std::stringstream temp_ss;
					temp_ss << val;
					auto str = temp_ss.str();
					if (std::strtod(str.c_str(), nullptr) == val) {
						ss << str;
					} else {
						ss << std::setprecision(16) << val;
					}
				}
			} else if (val == +std::numeric_limits<double>::infinity()) {
				ss << "+inf";
			} else if (val == -std::numeric_limits<double>::infinity()) {
				ss << "-inf";
			} else {
				ss << "+NaN";
			}
		}

		void write_string(const std::string& str)
		{
			const size_t LONG_LINE = 240;

			if (!options.json                       ||
			    str.find('\n') == std::string::npos ||
				str.length() < LONG_LINE            ||
				str.find("\"\"\"") != std::string::npos)
			{
				write_quoted_string(str);
			} else {
				write_verbatim_string(str);
			}
		}

		void write_hex_digit(unsigned num)
		{
			CONFIGURU_ASSERT(num < 16u);
			if (num < 10u) { ss << num; }
			else { ss << ('a' + num - 10); }
		}

		void write_hex_16(uint16_t n)
		{
			write_hex_digit((n >> 12) & 0x0f);
			write_hex_digit((n >>  8) & 0x0f);
			write_hex_digit((n >>  4) & 0x0f);
			write_hex_digit((n >>  0) & 0x0f);
		}

		void write_unicode_16(uint16_t c)
		{
			ss << "\\u";
			write_hex_16(c);
		}

		void write_quoted_string(const std::string& str) {
			ss << '"';
			for (char c : str) {
				if      (c == '\\') { ss << "\\\\"; }
				else if (c == '\"') { ss << "\\\""; }
				//else if (c == '\'') { ss << "\\\'"; }
				else if (c == '\0') { ss << "\\0";  }
				else if (c == '\b') { ss << "\\b";  }
				else if (c == '\f') { ss << "\\f";  }
				else if (c == '\n') { ss << "\\n";  }
				else if (c == '\r') { ss << "\\r";  }
				else if (c == '\t') { ss << "\\t";  }
				else if (0 <= c && c < 0x20) { write_unicode_16(c); }
				else { ss << c; }
			}
			ss << '"';
		}

		void write_verbatim_string(const std::string& str)
		{
			ss << "\"\"\"";
			ss << str;
			ss << "\"\"\"";
		}
	}; // struct Writer

	std::string write_config(const Config& config, FormatOptions options)
	{
		Writer w;
		w.options = options;
		w.doc = config.doc();

		if (!options.json && config.is_map()) {
			w.write_map_contents(0, config);
		} else {
			w.write_value(0, config, true, true);
			w.ss << "\n"; // Good form
		}

		return w.ss.str();
	}

	static void write_text_file(const char* path, const std::string& data)
	{
		auto fp = fopen(path, "wb");
		if (fp == nullptr) {
			CONFIGURU_ONERROR((std::string)"Failed to open '" + path + "' for writing: " + strerror(errno));
		}
		auto num_bytes_written = fwrite(data.data(), 1, data.size(), fp);
		fclose(fp);
		if (num_bytes_written != data.size()) {
			CONFIGURU_ONERROR((std::string)"Failed to write to '" + path + "': " + strerror(errno));
		}
	}

	void write_config_file(const std::string& path, const configuru::Config& config, FormatOptions options)
	{
		auto str = write_config(config, options);
		write_text_file(path.c_str(), str);
	}
} // namespace configuru

// ----------------------------------------------------------------------------

#endif // CONFIGURU_IMPLEMENTATION
