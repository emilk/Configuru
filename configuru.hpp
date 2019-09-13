/*
www.github.com/emilk/configuru

# Configuru
	Configuru, an experimental config library for C++, by Emil Ernerfeldt.

# License
	This software is in the public domain. Where that dedication is not
	recognized, you are granted a perpetual, irrevocable license to copy
	and modify this file as you see fit.

	That being said, I would appreciate credit!
	If you find this library useful, send a tweet to [@ernerfeldt](https://twitter.com/ernerfeldt) or mail me at emil.ernerfeldt@gmail.com.

# Version history
	0.0.0: 2014-07-21 - Initial steps
	0.1.0: 2015-11-08 - First commit as stand-alone library
	0.2.0: 2016-03-25 - check_dangling changes
	0.2.1: 2016-04-11 - mark_accessed in dump_string by default
	0.2.2: 2016-07-27 - optimizations
	0.2.3: 2016-08-09 - optimizations + add Config::emplace(key, value)
	0.2.4: 2016-08-18 - fix compilation error for when CONFIGURU_VALUE_SEMANTICS=0
	0.3.0: 2016-09-15 - Add option to not align values (object_align_values)
	0.3.1: 2016-09-19 - Fix crashes on some compilers/stdlibs
	0.3.2: 2016-09-22 - Add support for Config::array(some_container)
	0.3.3: 2017-01-10 - Add some missing iterator members
	0.3.4: 2017-01-17 - Add cast conversion to std::array
	0.4.0: 2017-04-17 - Automatic (de)serialization with serialize/deserialize with https://github.com/cbeck88/visit_struct
	0.4.1: 2017-05-21 - Make it compile on VC++
	0.4.2: 2018-11-02 - Automatic serialize/deserialize of maps and enums

# Getting started
	For using:
		Just `#include <configuru.hpp>` where you want to use Configuru

	Then, in only one .cpp file:
        #define CONFIGURU_IMPLEMENTATION 1
        #include <configuru.cpp>

	For more info, please see README.md (at www.github.com/emilk/configuru).
*/

//  dP""b8  dP"Yb  88b 88 888888 88  dP""b8 88   88 88""Yb 88   88
// dP   `" dP   Yb 88Yb88 88__   88 dP   `" 88   88 88__dP 88   88
// Yb      Yb   dP 88 Y88 88""   88 Yb  "88 Y8   8P 88"Yb  Y8   8P
//  YboodP  YbodP  88  Y8 88     88  YboodP `YbodP' 88  Yb `YbodP'

// Disable all warnings from gcc/clang/msvc:
#if defined(__clang__)
	#pragma clang system_header
#elif defined(__GNUC__)
	#pragma GCC system_header
#elif defined(_MSC_VER)
	#pragma warning( push, 0)
	#pragma warning( disable : 4715 )  
#endif

#ifndef CONFIGURU_HEADER_HPP
#define CONFIGURU_HEADER_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef VISITABLE_STRUCT
	#include <unordered_map>
#endif

#ifndef CONFIGURU_ONERROR
	#define CONFIGURU_ONERROR(message_str) \
		throw std::runtime_error(message_str)
#endif // CONFIGURU_ONERROR

#ifndef CONFIGURU_ASSERT
	#include <cassert>
	#define CONFIGURU_ASSERT(test) assert(test)
#endif // CONFIGURU_ASSERT

#ifndef CONFIGURU_ON_DANGLING
	/// CONFIGURU_ON_DANGLING(message_str) is called by check_dangling() if there are any unaccessed keys.
	#define CONFIGURU_ON_DANGLING(message_str) \
		CONFIGURU_ONERROR(message_str)
#endif // CONFIGURU_ON_DANGLING

#ifdef __GNUC__
#define CONFIGURU_NORETURN __attribute__((noreturn))
#elif __MINGW32__
#define CONFIGURU_NORETURN __attribute__((noreturn))
#elif __clang__
#define CONFIGURU_NORETURN __attribute__((noreturn))
#elif _MSC_VER
#define CONFIGURU_NORETURN
#endif

#ifndef CONFIGURU_IMPLICIT_CONVERSIONS
	/// Set to 1 to allow  `int x = some_cfg,`
	#define CONFIGURU_IMPLICIT_CONVERSIONS 0
#endif

#ifndef CONFIGURU_VALUE_SEMANTICS
	/// If set, all copies are deep clones.
	/// If 0, all copies of objects and array are shallow (ref-counted).
	#define CONFIGURU_VALUE_SEMANTICS 0
#endif

#undef Bool // Needed on Ubuntu 14.04 with GCC 4.8.5
#undef check // Needed on OSX

/// The Configuru namespace.
namespace configuru
{
	struct DocInfo;
	using DocInfo_SP = std::shared_ptr<DocInfo>;

	using Index = unsigned;
	const Index BAD_INDEX = static_cast<Index>(-1);

	struct Include
	{
		DocInfo_SP doc;
		Index      line = BAD_INDEX;

		Include() {}
		Include(DocInfo_SP d, Index l) : doc(d), line(l) {}
	};

	/// Helper for describing a document.
	struct DocInfo
	{
		std::vector<Include> includers;

		std::string filename;

		DocInfo(const std::string& fn) : filename(fn) { }

		void append_include_info(std::string& ret, const std::string& indent="    ") const;
	};

	struct BadLookupInfo;

	/// Helper: value in an object.
	template<typename Config_T>
	struct Config_Entry
	{
		Config_T     _value;
		Index        _nr       = BAD_INDEX; ///< Size of the object prior to adding this entry
		mutable bool _accessed = false;     ///< Set to true if accessed.

		Config_Entry() {}
		Config_Entry(Config_T value, Index nr) : _value(std::move(value)), _nr(nr) {}
	};

	using Comment = std::string;
	using Comments = std::vector<Comment>;

	/// Captures the comments related to a Config value.
	struct ConfigComments
	{
		/// Comments on preceeding lines.
		/// Like this.
		Comments prefix;
		Comments postfix; ///< After the value, on the same line. Like this.
		Comments pre_end_brace; /// Before the closing } or ]

		ConfigComments() {}

		bool empty() const;
		void append(ConfigComments&& other);
	};

	/// A dynamic config variable.
	class Config;

	/** Overload this (in configuru namespace) for you own types, e.g:

		```
		namespace configuru {
			template<>
			inline Vector2f as(const Config& config)
			{
				auto&& array = config.as_array();
				config.check(array.size() == 2, "Expected Vector2f");
				return {(float)array[0], (float)array[1]};
			}
		}
		```
	*/
	template<typename T>
	inline T as(const configuru::Config& config);

	/// A dynamic config variable.
	/// Acts like something out of Python or Lua.
	/// If CONFIGURU_VALUE_SEMANTICS all copies of this will be deep copies.
	/// If not, it will use reference-counting for objects and arrays,
	/// meaning all copies will be shallow copies.
	class Config
	{
	public:
		enum Type
		{
			Uninitialized, ///< Accessing a Config of this type is always an error.
			BadLookupType, ///< We are the result of a key-lookup in a Object with no hit. We are in effect write-only.
			Null, Bool, Int, Float, String, Array, Object
		};

		using ObjectEntry = Config_Entry<Config>;

		using ConfigArrayImpl = std::vector<Config>;
		using ConfigObjectImpl = std::map<std::string, ObjectEntry>;
		struct ConfigArray
		{
			#if !CONFIGURU_VALUE_SEMANTICS
				std::atomic<unsigned> _ref_count { 1 };
			#endif
			ConfigArrayImpl _impl;
		};
		struct ConfigObject;

		// ----------------------------------------
		// Constructors:

		/// Creates an uninitialized Config.
		Config()                     : _type(Uninitialized) { }
		Config(std::nullptr_t)       : _type(Null)    { }
		Config(float f)              : _type(Float)   { _u.f = f; }
		Config(double f)             : _type(Float)   { _u.f = f; }
		Config(bool b)               : _type(Bool)    { _u.b = b; }
		Config(int i)                : _type(Int)     { _u.i = i; }
		Config(unsigned int i)       : _type(Int)     { _u.i = i; }
		Config(long i)               : _type(Int)     { _u.i = i; }
		Config(unsigned long i)      : Config(static_cast<unsigned long long>(i)) {}
		Config(long long i)          : _type(Int)     { _u.i = i; }
		Config(unsigned long long i) : _type(Int)
		{
			if ((i & 0x8000000000000000ull) != 0) {
				CONFIGURU_ONERROR("Integer too large to fit into 63 bits");
			}
			_u.i = static_cast<int64_t>(i);
		}
		Config(const char* str);
		Config(std::string str);

		/** This constructor is a short-form for Config::object(...).
		    We have no short-form for Config::array(...),
		    as that is less common and can lead to ambiguities.
		    Usage:

		    ```
				Config cfg {
					{ "key",          "value" },
					{ "empty_array",  Config::array() },
					{ "array",        Config::array({1, 2, 3}) },
					{ "empty_object", Config::object() },
					{ "object",       Config::object({
						{ "nested_key", "nested_value" },
					})},
					{ "another_object", {
						{ "nested_key", "nested_value" },
					}},
				};
		    ```
		*/
		Config(std::initializer_list<std::pair<std::string, Config>> values);

		/// Array constructor
		template<typename T>
		Config(const std::vector<T>& values) : _type(Uninitialized)
		{
			make_array();
			_u.array->_impl.reserve(values.size());
			for (const auto& v : values) {
				push_back(v);
			}
		}

		/// Array constructor
		Config(const std::vector<bool>& values) : _type(Uninitialized)
		{
			make_array();
			_u.array->_impl.reserve(values.size());
			for (const auto v : values) {
				push_back(!!v);
			}
		}

		/// Object constructor
		template<typename T>
		Config(const std::map<std::string, T>& values) : _type(Uninitialized)
		{
			make_object();
			for (const auto& p : values) {
				(*this)[p.first] = p.second;
			}
		}

		/// Used by the parser - no need to use directly.
		void make_object();

		/// Used by the parser - no need to use directly.
		void make_array();

		/// Used by the parser - no need to use directly.
		void tag(const DocInfo_SP& doc, Index line, Index column);

		/// Preferred way to create an empty object.
		static Config object();

		/// Preferred way to create an object.
		static Config object(std::initializer_list<std::pair<std::string, Config>> values);

		/// Preferred way to create an empty array.
		static Config array();

		/// Preferred way to create an array.
		static Config array(std::initializer_list<Config> values);

		/// Preferred way to create an array from an STL container.
		template<typename Container>
		static Config array(const Container& container)
		{
			Config ret;
			ret.make_array();
			auto& impl = ret._u.array->_impl;
			impl.reserve(container.size());
			for (auto&& v : container) {
				impl.emplace_back(v);
			}
			return ret;
		}

		// ----------------------------------------

		~Config();

		Config(const Config& o);
		Config(Config&& o) noexcept;
		Config& operator=(const Config& o);

		/// Will still remember file/line when assigned an object which has no file/line
		Config& operator=(Config&& o) noexcept;

		/// Swaps file/line too.
		void swap(Config& o) noexcept;

		#ifdef CONFIG_EXTENSION
			CONFIG_EXTENSION
		#endif

		// ----------------------------------------
		// Inspectors:

		Type type() const { return _type; }

		bool is_uninitialized() const { return _type == Uninitialized;    }
		bool is_null()          const { return _type == Null;             }
		bool is_bool()          const { return _type == Bool;             }
		bool is_int()           const { return _type == Int;              }
		bool is_float()         const { return _type == Float;            }
		bool is_string()        const { return _type == String;           }
		bool is_object()        const { return _type == Object;           }
		bool is_array()         const { return _type == Array;            }
		bool is_number()        const { return is_int() || is_float();    }

		/// Returns file:line iff available.
		std::string where() const;

		/// BAD_INDEX if not set.
		Index line() const { return _line; }

		/// Handle to document.
		const DocInfo_SP& doc() const { return _doc; }
		void set_doc(const DocInfo_SP& doc) { _doc = doc; }

		// ----------------------------------------
		// Convertors:

#if CONFIGURU_IMPLICIT_CONVERSIONS
		/// Explicit casting, for overloads of as<T>
		template<typename T>
		explicit operator T() const { return as<T>(*this); }

		inline operator bool()                    const { return as_bool();   }
		inline operator signed char()             const { return as_integer<signed char>();         }
		inline operator unsigned char()           const { return as_integer<unsigned char>();       }
		inline operator signed short()            const { return as_integer<signed short>();         }
		inline operator unsigned short()          const { return as_integer<unsigned short>();       }
		inline operator signed int()              const { return as_integer<signed int>();         }
		inline operator unsigned int()            const { return as_integer<unsigned int>();       }
		inline operator signed long()             const { return as_integer<signed long>();        }
		inline operator unsigned long()           const { return as_integer<unsigned long>();      }
		inline operator signed long long()        const { return as_integer<signed long long>();   }
		inline operator unsigned long long()      const { return as_integer<unsigned long long>(); }
		inline operator float()                   const { return as_float();  }
		inline operator double()                  const { return as_double(); }
		inline operator std::string()             const { return as_string(); }
		inline operator Config::ConfigArrayImpl() const { return as_array();  }

		/// Convenience conversion to std::vector
		template<typename T>
		operator std::vector<T>() const
		{
			const auto& array = as_array();
			std::vector<T> ret;
			ret.reserve(array.size());
			for (auto&& config : array) {
				ret.push_back((T)config);
			}
			return ret;
		}

		/// Convenience conversion to std::array
		template<typename T, size_t N>
		operator std::array<T, N>() const
		{
			const auto& array = as_array();
			check(array.size() == N, "Array size mismatch.");
			std::array<T, N> ret;
			std::copy(array.begin(), array.end(), ret.begin());
			return ret;
		}

		/// Convenience conversion of an array of length 2 to an std::pair.
		/// TODO: generalize for tuples.
		template<typename Left, typename Right>
		operator std::pair<Left, Right>() const
		{
			const auto& array = as_array();
			check(array.size() == 2u, "Mismatched array length.");
			return {(Left)array[0], (Right)array[1]};
		}
#else
		/// Explicit casting, since C++ handles implicit casts real badly.
		template<typename T>
		explicit operator T() const { return as<T>(*this); }

		/// Convenience conversion to std::vector
		template<typename T>
		explicit operator std::vector<T>() const
		{
			const auto& array = as_array();
			std::vector<T> ret;
			ret.reserve(array.size());
			for (auto&& config : array) {
				ret.push_back(static_cast<T>(config));
			}
			return ret;
		}

		/// Convenience conversion to std::array
		template<typename T, size_t N>
		explicit operator std::array<T, N>() const
		{
			const auto& array = as_array();
			check(array.size() == N, "Array size mismatch.");
			std::array<T, N> ret;
			for (size_t i =0; i < N; ++i) {
				ret[i] = static_cast<T>(array[i]);
			}
			return ret;
		}

		/// Convenience conversion of an array of length 2 to an std::pair.
		/// TODO: generalize for tuples.
		template<typename Left, typename Right>
		explicit operator std::pair<Left, Right>() const
		{
			const auto& array = as_array();
			check(array.size() == 2u, "Mismatched array length.");
			return {static_cast<Left>(array[0]), static_cast<Right>(array[1])};
		}
#endif

		const std::string& as_string() const { assert_type(String); return *_u.str; }
		const char* c_str() const { assert_type(String); return _u.str->c_str(); }

		/// The Config must be a boolean.
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
			check(static_cast<int64_t>(static_cast<IntT>(_u.i)) == _u.i, "Integer out of range");
			return static_cast<IntT>(_u.i);
		}

		float as_float() const
		{
			if (_type == Int) {
				return static_cast<float>(_u.i);
			} else {
				assert_type(Float);
				return static_cast<float>(_u.f);
			}
		}

		double as_double() const
		{
			if (_type == Int) {
				return static_cast<double>(_u.i);
			} else {
				assert_type(Float);
				return static_cast<double>(_u.f);
			}
		}

		/// Extract the value of this Config.
		template<typename T>
		T get() const;

		/// Returns the value or `default_value` if this is the result of a bad lookup.
		template<typename T>
		T get_or(const T& default_value) const
		{
			if (_type == BadLookupType) {
				return default_value;
			} else {
				return static_cast<T>(*this);
			}
		}

		// ----------------------------------------
		// Array:

		/// Length of an array
		size_t array_size() const
		{
			return as_array().size();
		}

		/// Only use this for iterating over an array: `for (Config& e : cfg.as_array()) { ... }`
		ConfigArrayImpl& as_array()
		{
			assert_type(Array);
			return _u.array->_impl;
		}

		/// Only use this for iterating over an array: `for (Config& e : cfg.as_array()) { ... }`
		const ConfigArrayImpl& as_array() const
		{
			assert_type(Array);
			return _u.array->_impl;
		}

		/// Array indexing
		Config& operator[](size_t ix)
		{
			auto&& array = as_array();
			check(ix < array.size(), "Array index out of range");
			return array[ix];
		}

		/// Array indexing
		const Config& operator[](size_t ix) const
		{
			auto&& array = as_array();
			check(ix < array.size(), "Array index out of range");
			return array[ix];
		}

		/// Append a value to this array.
		void push_back(Config value)
		{
			as_array().push_back(std::move(value));
		}

		// ----------------------------------------
		// Object:

		/// Number of elementsi n this object
		size_t object_size() const;

		/// Only use this for iterating over an object:
		/// `for (auto& p : cfg.as_object()) { p.value() = p.key(); }`
		ConfigObject& as_object()
		{
			assert_type(Object);
			return *_u.object;
		}

		/// Only use this for iterating over an object:
		/// `for (const auto& p : cfg.as_object()) { cout << p.key() << ": " << p.value(); }`
		const ConfigObject& as_object() const
		{
			assert_type(Object);
			return *_u.object;
		}

		/// Look up a value in an Object. Returns a BadLookupType Config if the key does not exist.
		const Config& operator[](const std::string& key) const;

		/// Prefer `obj.insert_or_assign(key, value);` to `obj[key] = value;` when inserting and performance is important!
		Config& operator[](const std::string& key);

		/// For indexing with string literals:
		template<std::size_t N>
		Config& operator[](const char (&key)[N]) { return operator[](std::string(key)); }
		template<std::size_t N>
		const Config& operator[](const char (&key)[N]) const { return operator[](std::string(key)); }

		/// Check if an object has a specific key.
		bool has_key(const std::string& key) const;

		/// Like has_key, but STL compatible.
		size_t count(const std::string& key) const { return has_key(key) ? 1 : 0; }

		/// Returns true iff the value was inserted, false if they key was already there.
		bool emplace(std::string key, Config value);

		/// Like `foo[key] = value`, but faster.
		void insert_or_assign(const std::string& key, Config&& value);

		/// Erase a key from an object.
		bool erase(const std::string& key);

		/// Get the given value in this object.
		template<typename T>
		T get(const std::string& key) const
		{
			return as<T>((*this)[key]);
		}

		/// Look for the given key in this object, and return default_value on failure.
		template<typename T>
		T get_or(const std::string& key, const T& default_value) const;

		/// Look for the given key in this object, and return default_value on failure.
		std::string get_or(const std::string& key, const char* default_value) const
		{
			return get_or<std::string>(key, default_value);
		}

		/// obj.get_or({"a", "b". "c"}, 42) - like obj["a"]["b"]["c"], but returns 42 if any of the keys are *missing*.
		template<typename T>
		T get_or(std::initializer_list<std::string> keys, const T& default_value) const;

		/// obj.get_or({"a", "b". "c"}, 42) - like obj["a"]["b"]["c"], but returns 42 if any of the keys are *missing*.
		std::string get_or(std::initializer_list<std::string> keys, const char* default_value) const
		{
			return get_or<std::string>(keys, default_value);
		}

		// --------------------------------------------------------------------------------

		/// Compare Config values recursively.
		static bool deep_eq(const Config& a, const Config& b);

#if !CONFIGURU_VALUE_SEMANTICS // No need for a deep_clone method when all copies are deep clones.
		/// Copy this Config value recursively.
		Config deep_clone() const;
#endif

		// ----------------------------------------

		/// Visit dangling (unaccessed) object keys recursively.
		void visit_dangling(const std::function<void(const std::string& key, const Config& value)>& visitor) const;

		/// Will check for dangling (unaccessed) object keys recursively and call CONFIGURU_ON_DANGLING on all found.
		void check_dangling() const;

		/// Set the 'access' flag recursively,
		void mark_accessed(bool v) const;

		// ----------------------------------------

		/// Was there any comments about this value in the input?
		bool has_comments() const
		{
			return _comments && !_comments->empty();
		}

		/// Read/write of comments.
		ConfigComments& comments()
		{
			if (!_comments) {
				_comments.reset(new ConfigComments());
			}
			return *_comments;
		}

		/// Read comments.
		const ConfigComments& comments() const
		{
			static const ConfigComments s_empty {};
			if (_comments) {
				return *_comments;
			} else {
				return s_empty;
			}
		}

		/// Returns either "true", "false", the constained string, or the type name.
		const char* debug_descr() const;

		/// Human-readable version of the type ("integer", "bool", etc).
		static const char* type_str(Type t);

		// ----------------------------------------
		// Helper functions for checking the type is what we expect:

		inline void check(bool b, const char* msg) const
		{
			if (!b) {
				on_error(msg);
			}
		}

		void assert_type(Type t) const;

		void on_error(const std::string& msg) const CONFIGURU_NORETURN;

	private:
		void free();

		using ConfigComments_UP = std::unique_ptr<ConfigComments>;

		union {
			bool               b;
			int64_t            i;
			double             f;
			const std::string* str;
			ConfigObject*      object;
			ConfigArray*       array;
			BadLookupInfo*     bad_lookup;
		} _u;

		DocInfo_SP        _doc; // So we can name the file
		ConfigComments_UP _comments;
		Index             _line = BAD_INDEX; // Where in the source, or BAD_INDEX. Lines are 1-indexed.
		Type              _type = Uninitialized;
	};

	// ------------------------------------------------------------------------

	struct Config::ConfigObject
	{
		#if !CONFIGURU_VALUE_SEMANTICS
			std::atomic<unsigned> _ref_count { 1 };
		#endif
		ConfigObjectImpl      _impl;

		class iterator
		{
		public:
			iterator() = default;
			explicit iterator(ConfigObjectImpl::iterator it) : _it(std::move(it)) {}

			const iterator& operator*() const {
				_it->second._accessed = true;
				return *this;
			}

			iterator& operator++() {
				++_it;
				return *this;
			}

			friend bool operator==(const iterator& a, const iterator& b) {
				return a._it == b._it;
			}

			friend bool operator!=(const iterator& a, const iterator& b) {
				return a._it != b._it;
			}

			const std::string& key()   const { return _it->first;         }
			Config&            value() const { return _it->second._value; }

		private:
			ConfigObjectImpl::iterator _it;
		};

		class const_iterator
		{
		public:
			const_iterator() = default;
			explicit const_iterator(ConfigObjectImpl::const_iterator it) : _it(std::move(it)) {}

			const const_iterator& operator*() const {
				_it->second._accessed = true;
				return *this;
			}

			const_iterator& operator++() {
				++_it;
				return *this;
			}

			friend bool operator==(const const_iterator& a, const const_iterator& b) {
				return a._it == b._it;
			}

			friend bool operator!=(const const_iterator& a, const const_iterator& b) {
				return a._it != b._it;
			}

			const std::string& key()   const { return _it->first;         }
			const Config&      value() const { return _it->second._value; }

		private:
			ConfigObjectImpl::const_iterator _it;
		};

		iterator       begin()        { return iterator{_impl.begin()};        }
		iterator       end()          { return iterator{_impl.end()};          }
		const_iterator begin()  const { return const_iterator{_impl.cbegin()}; }
		const_iterator end()    const { return const_iterator{_impl.cend()};   }
		const_iterator cbegin() const { return const_iterator{_impl.cbegin()}; }
		const_iterator cend()   const { return const_iterator{_impl.cend()};   }
	};

	// ------------------------------------------------------------------------

	inline bool operator==(const Config& a, const Config& b)
	{
		return Config::deep_eq(a, b);
	}

	inline bool operator!=(const Config& a, const Config& b)
	{
		return !Config::deep_eq(a, b);
	}

	// ------------------------------------------------------------------------

	template<> inline bool                           Config::get() const { return as_bool();   }
	template<> inline signed char                    Config::get() const { return as_integer<signed char>();         }
	template<> inline unsigned char                  Config::get() const { return as_integer<unsigned char>();       }
	template<> inline signed short                   Config::get() const { return as_integer<signed short>();         }
	template<> inline unsigned short                 Config::get() const { return as_integer<unsigned short>();       }
	template<> inline signed int                     Config::get() const { return as_integer<signed int>();         }
	template<> inline unsigned int                   Config::get() const { return as_integer<unsigned int>();       }
	template<> inline signed long                    Config::get() const { return as_integer<signed long>();        }
	template<> inline unsigned long                  Config::get() const { return as_integer<unsigned long>();      }
	template<> inline signed long long               Config::get() const { return as_integer<signed long long>();   }
	template<> inline unsigned long long             Config::get() const { return as_integer<unsigned long long>(); }
	template<> inline float                          Config::get() const { return as_float();  }
	template<> inline double                         Config::get() const { return as_double(); }
	template<> inline const std::string&             Config::get() const { return as_string(); }
	template<> inline std::string                    Config::get() const { return as_string(); }
	template<> inline const Config::ConfigArrayImpl& Config::get() const { return as_array();  }
	// template<> inline std::vector<std::string>     Config::get() const { return as_vector<T>();   }

	// ------------------------------------------------------------------------

	template<typename T>
	inline T as(const configuru::Config& config)
	{
		return config.get<T>();
	}

	template<typename T>
	T Config::get_or(const std::string& key, const T& default_value) const
	{
		auto&& object = as_object()._impl;
		auto it = object.find(key);
		if (it == object.end()) {
			return default_value;
		} else {
			const auto& entry = it->second;
			entry._accessed = true;
			return as<T>(entry._value);
		}
	}

	template<typename T>
	T Config::get_or(std::initializer_list<std::string> keys, const T& default_value) const
	{
		const Config* obj = this;
		for (const auto& key : keys)
		{
			if (obj->has_key(key)) {
				obj = &(*obj)[key];
			} else {
				return default_value;
			}
		}
		return as<T>(*obj);
	}

	// ------------------------------------------------------------------------

	/// Prints in JSON but in a fail-safe manner, allowing uninitialized keys and inf/nan.
	std::ostream& operator<<(std::ostream& os, const Config& cfg);

	// ------------------------------------------------------------------------

	/// Recursively visit all values in a config.
	template<class Config, class Visitor>
	void visit_configs(Config&& config, Visitor&& visitor)
	{
		visitor(config);
		if (config.is_object()) {
			for (auto&& p : config.as_object()) {
				visit_configs(p.value(), visitor);
			}
		} else if (config.is_array()) {
			for (auto&& e : config.as_array()) {
				visit_configs(e, visitor);
			}
		}
	}

	inline void clear_doc(Config& root) // TODO: shouldn't be needed. Replace with some info of whether a Config is the root of the document it is in.
	{
		visit_configs(root, [&](Config& cfg){ cfg.set_doc(nullptr); });
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
		if (dst.is_object() && src.is_object()) {
			for (auto&& p : src.as_object()) {
				merge_replace(dst[p.key()], p.value());
			}
		} else {
			dst = src;
		}
	}
	 */

	// ----------------------------------------------------------

	/// Thrown on a syntax error.
	class ParseError : public std::exception
	{
	public:
		ParseError(const DocInfo_SP& doc, Index line, Index column, const std::string& msg)
			: _line(line), _column(column)
		{
			_what = doc->filename + ":" + std::to_string(line) + ":" + std::to_string(column);
			doc->append_include_info(_what);
			_what += ": " + msg;
		}

		/// Will name the file name, line number, column and description.
		const char* what() const noexcept override
		{
			return _what.c_str();
		}

		Index line()   const noexcept { return _line; }
		Index column() const noexcept { return _column; }

	private:
		Index _line, _column;
		std::string _what;
	};

	// ----------------------------------------------------------

	/// This struct basically contain all the way we can tweak the file format.
	struct FormatOptions
	{
		/// Indentation should be a single tab,
		/// multiple spaces or an empty string.
		/// An empty string means the output will be compact.
		std::string indentation              = "\t";
		bool        enforce_indentation      = true;  ///< Must have correct indentation?
		bool        end_with_newline         = true;  ///< End each file with a newline (unless compact).

		// Top file:
		bool        empty_file               = false; ///< If true, an empty file is an empty object.
		bool        implicit_top_object      = true;  ///< Ok with key-value pairs top-level?
		bool        implicit_top_array       = true;  ///< Ok with several values top-level?

		// Comments:
		bool        single_line_comments     = true;  ///< Allow this?
		bool        block_comments           = true;  /* Allow this? */
		bool        nesting_block_comments   = true;  ///< /* Allow /*    this? */ */

		// Numbers:
		bool        inf                      = true;  ///< Allow +inf, -inf
		bool        nan                      = true;  ///< Allow +NaN
		bool        hexadecimal_integers     = true;  ///< Allow 0xff
		bool        binary_integers          = true;  ///< Allow 0b1010
		bool        unary_plus               = true;  ///< Allow +42
		bool        distinct_floats          = true;  ///< Print 9.0 as "9.0", not just "9". A must for round-tripping.

		// Arrays
		bool        array_omit_comma         = true;  ///< Allow [1 2 3]
		bool        array_trailing_comma     = true;  ///< Allow [1, 2, 3,]

		// Objects:
		bool        identifiers_keys         = true;  ///< { is_this_ok: true }
		bool        object_separator_equal   = false; ///< { "is_this_ok" = true }
		bool        allow_space_before_colon = false; ///< { "is_this_ok" : true }
		bool        omit_colon_before_object = false; ///< { "nested_object" { } }
		bool        object_omit_comma        = true;  ///< Allow {a:1 b:2}
		bool        object_trailing_comma    = true;  ///< Allow {a:1, b:2,}
		bool        object_duplicate_keys    = false; ///< Allow {"a":1, "a":2}
		bool        object_align_values      = true;  ///< Add spaces after keys to align subsequent values.

		// Strings
		bool        str_csharp_verbatim      = true;  ///< Allow @"Verbatim\strings"
		bool        str_python_multiline     = true;  ///< Allow """ Python\nverbatim strings """
		bool        str_32bit_unicode        = true;  ///< Allow "\U0030dbfd"
		bool        str_allow_tab            = true;  ///< Allow unescaped tab in string.

		// Special
		bool        allow_macro              = true;  ///< Allow `#include "some_other_file.cfg"`

		// When writing:
		bool        write_comments           = true;

		/// Sort keys lexicographically. If false, sort by order they where added.
		bool        sort_keys                = false;

		/// When printing, write uninitialized values as UNINITIALIZED. Useful for debugging.
		bool        write_uninitialized      = false;

		/// Dumping should mark the json as accessed?
		bool        mark_accessed            = true;

		bool compact() const { return indentation.empty(); }
	};

	/// Returns FormatOptions that are describe a JSON file format.
	inline FormatOptions make_json_options()
	{
		FormatOptions options;

		options.indentation              = "\t";
		options.enforce_indentation      = false;

		// Top file:
		options.empty_file               = false;
		options.implicit_top_object      = false;
		options.implicit_top_array       = false;

		// Comments:
		options.single_line_comments     = false;
		options.block_comments           = false;
		options.nesting_block_comments   = false;

		// Numbers:
		options.inf                      = false;
		options.nan                      = false;
		options.hexadecimal_integers     = false;
		options.binary_integers          = false;
		options.unary_plus               = false;
		options.distinct_floats          = true;

		// Arrays
		options.array_omit_comma         = false;
		options.array_trailing_comma     = false;

		// Objects:
		options.identifiers_keys         = false;
		options.object_separator_equal   = false;
		options.allow_space_before_colon = true;
		options.omit_colon_before_object = false;
		options.object_omit_comma        = false;
		options.object_trailing_comma    = false;
		options.object_duplicate_keys    = false; // To be 100% JSON compatile, this should be true, but it is error prone.
		options.object_align_values      = true;  // Looks better.

		// Strings
		options.str_csharp_verbatim      = false;
		options.str_python_multiline     = false;
		options.str_32bit_unicode        = false;
		options.str_allow_tab            = false;

		// Special
		options.allow_macro              = false;

		// When writing:
		options.write_comments           = false;
		options.sort_keys                = false;

		return options;
	}

	/// Returns format options that allow us parsing most files.
	inline FormatOptions make_forgiving_options()
	{
		FormatOptions options;

		options.indentation              = "\t";
		options.enforce_indentation      = false;

		// Top file:
		options.empty_file               = true;
		options.implicit_top_object      = true;
		options.implicit_top_array       = true;

		// Comments:
		options.single_line_comments     = true;
		options.block_comments           = true;
		options.nesting_block_comments   = true;

		// Numbers:
		options.inf                      = true;
		options.nan                      = true;
		options.hexadecimal_integers     = true;
		options.binary_integers          = true;
		options.unary_plus               = true;
		options.distinct_floats          = true;

		// Arrays
		options.array_omit_comma         = true;
		options.array_trailing_comma     = true;

		// Objects:
		options.identifiers_keys         = true;
		options.object_separator_equal   = true;
		options.allow_space_before_colon = true;
		options.omit_colon_before_object = true;
		options.object_omit_comma        = true;
		options.object_trailing_comma    = true;
		options.object_duplicate_keys    = true;

		// Strings
		options.str_csharp_verbatim      = true;
		options.str_python_multiline     = true;
		options.str_32bit_unicode        = true;
		options.str_allow_tab            = true;

		// Special
		options.allow_macro              = true;

		// When writing:
		options.write_comments           = false;
		options.sort_keys                = false;

		return options;
	}

	/// The CFG file format.
	static const FormatOptions CFG       = FormatOptions();

	/// The JSON file format.
	static const FormatOptions JSON      = make_json_options();

	/// A very forgiving file format, when parsing stuff that is not strict.
	static const FormatOptions FORGIVING = make_forgiving_options();

	struct ParseInfo
	{
		std::map<std::string, Config> parsed_files; // Two #include gives same Config tree.
	};

	/// The parser may throw ParseError.
	/// `str` should be a zero-ended Utf-8 encoded string of characters.
	/// The `name` should be something akin to a filename. It is only for error reporting.
	Config parse_string(const char* str, const FormatOptions& options, const char* name);
	Config parse_file(const std::string& path, const FormatOptions& options);

	/// Advanced usage:
	Config parse_string(const char* str, const FormatOptions& options, DocInfo _doc, ParseInfo& info);
	Config parse_file(const std::string& path, const FormatOptions& options, DocInfo_SP doc, ParseInfo& info);

	// ----------------------------------------------------------
	/// Writes the config as a string in the given format.
	/// May call CONFIGURU_ONERROR if the given config is invalid. This can happen if
	/// a Config is unitialized (and options write_uninitialized is not set) or
	/// a Config contains inf/nan (and options.inf/options.nan aren't set).
	std::string dump_string(const Config& config, const FormatOptions& options);

	/// Writes the config to a file. Like dump_string, but can may also call CONFIGURU_ONERROR
	/// if it fails to write to the given path.
	void dump_file(const std::string& path, const Config& config, const FormatOptions& options);

	// ----------------------------------------------------------
	// Automatic (de)serialize of most things.
	// Include <visit_struct/visit_struct.hpp> (from https://github.com/cbeck88/visit_struct)
	// before including <configuru.hpp> to get this feature.

	#ifdef VISITABLE_STRUCT
		template <typename Type> struct is_container : std::false_type { };
		// template <typename... Ts> struct is_container<std::list<Ts...> > : std::true_type { };
		template <typename... Ts> struct is_container<std::vector<Ts...> > : std::true_type { };

		template <typename Type> struct is_map : std::false_type { };
		template <typename... Ts> struct is_map<std::map<Ts...> > : std::true_type { };
		template <typename... Ts> struct is_map<std::unordered_map<Ts...> > : std::true_type { };

		// ----------------------------------------------------------------------------

		Config serialize(const std::string& some_string);

		template<typename T>
		typename std::enable_if<std::is_arithmetic<T>::value, Config>::type
		serialize(const T& some_value);

		template<typename T>
		typename std::enable_if<std::is_enum<T>::value, Config>::type
		serialize(const T& some_enum);

		template<typename T, size_t N>
		Config serialize(T (&some_array)[N]);

		template<typename T>
		typename std::enable_if<is_container<T>::value, Config>::type
		serialize(const T& some_container);

		template<typename T>
		typename std::enable_if<is_map<T>::value, Config>::type
		serialize(const T& some_map);

		template<typename T>
		typename std::enable_if<visit_struct::traits::is_visitable<T>::value, Config>::type
		serialize(const T& some_struct);

		// ----------------------------------------------------------------------------

		inline Config serialize(const std::string& some_string)
		{
			return Config(some_string);
		}

		template<typename T>
		typename std::enable_if<std::is_arithmetic<T>::value, Config>::type
		serialize(const T& some_value)
		{
			return Config(some_value);
		}

		template<typename T>
		typename std::enable_if<std::is_enum<T>::value, Config>::type
		serialize(const T& some_enum)
		{
			return Config(static_cast<int>(some_enum));
		}

		template<typename T, size_t N>
		Config serialize(T (&some_array)[N])
		{
			auto config = Config::array();
			for (size_t i = 0; i < N; ++i) {
				config.push_back(serialize(some_array[i]));
			}
			return config;
		}

		template<typename T>
		typename std::enable_if<is_container<T>::value, Config>::type
		serialize(const T& some_container)
		{
			auto config = Config::array();
			for (const auto& value : some_container) {
				config.push_back(serialize(value));
			}
			return config;
		}

		template<typename T>
		typename std::enable_if<is_map<T>::value, Config>::type
		serialize(const T& some_map)
		{
			auto config = Config::array();
			for (const auto& pair : some_map) {
				config.push_back(Config::array({serialize(pair.first), serialize(pair.second)}));
			}
			return config;
		}

		template<typename T>
		typename std::enable_if<visit_struct::traits::is_visitable<T>::value, Config>::type
		serialize(const T& some_struct)
		{
			auto config = Config::object();
			visit_struct::apply_visitor([&config](const std::string& name, const auto& value) {
				config[name] = serialize(value);
			}, some_struct);
			return config;
		}

		// ----------------------------------------------------------------------------

		/// Called when there is a problem in deserialize.
		using ConversionError = std::function<void(std::string)>;

		void deserialize(std::string* some_string, const Config& config, const ConversionError& on_error);

		template<typename T>
		typename std::enable_if<std::is_arithmetic<T>::value>::type
		deserialize(T* some_value, const Config& config, const ConversionError& on_error);

		template<typename T>
		typename std::enable_if<std::is_enum<T>::value>::type
		deserialize(T* some_enum, const Config& config, const ConversionError& on_error);

		template<typename T, size_t N>
		typename std::enable_if<std::is_arithmetic<T>::value>::type
		deserialize(T (*some_array)[N], const Config& config, const ConversionError& on_error);

		template<typename T>
		typename std::enable_if<is_container<T>::value>::type
		deserialize(T* some_container, const Config& config, const ConversionError& on_error);

		template<typename T>
		typename std::enable_if<is_map<T>::value>::type
		deserialize(T* some_map, const Config& config, const ConversionError& on_error);

		template<typename T>
		typename std::enable_if<visit_struct::traits::is_visitable<T>::value>::type
		deserialize(T* some_struct, const Config& config, const ConversionError& on_error);

		// ----------------------------------------------------------------------------

		inline void deserialize(std::string* some_string, const Config& config, const ConversionError& on_error)
		{
			*some_string = config.as_string();
		}

		template<typename T>
		typename std::enable_if<std::is_arithmetic<T>::value>::type
		deserialize(T* some_value, const Config& config, const ConversionError& on_error)
		{
			*some_value = as<T>(config);
		}

		template<typename T>
		typename std::enable_if<std::is_enum<T>::value>::type
		deserialize(T* some_enum, const Config& config, const ConversionError& on_error)
		{
			*some_enum = static_cast<T>(as<int>(config));
		}

		template<typename T, size_t N>
		typename std::enable_if<std::is_arithmetic<T>::value>::type
		deserialize(T (*some_array)[N], const Config& config, const ConversionError& on_error)
		{
			if (!config.is_array()) {
				if (on_error) {
					on_error(config.where() + "Expected array");
				}
			} else if (config.array_size() != N) {
				if (on_error) {
					on_error(config.where() + "Expected array to be " + std::to_string(N) + " long.");
				}
			} else {
				for (size_t i = 0; i < N; ++i) {
					deserialize(&(*some_array)[i], config[i], on_error);
				}
			}
		}

		template<typename T>
		typename std::enable_if<is_container<T>::value>::type
		deserialize(T* some_container, const Config& config, const ConversionError& on_error)
		{
			if (!config.is_array()) {
				if (on_error) {
					on_error(config.where() + "Failed to deserialize container: config is not an array.");
				}
			} else {
				some_container->clear();
				some_container->reserve(config.array_size());
				for (const auto& value : config.as_array()) {
					some_container->push_back({});
					deserialize(&some_container->back(), value, on_error);
				}
			}
		}

		template<typename T>
		typename std::enable_if<is_map<T>::value>::type
		deserialize(T* some_map, const Config& config, const ConversionError& on_error)
		{
			if (!config.is_array()) {
				if (on_error) {
					on_error(config.where() + "Failed to deserialize map: config is not an array.");
				}
			} else {
				some_map->clear();
				for (const auto& pair : config.as_array()) {
					if (pair.is_array() && pair.array_size() == 2) {
						typename T::key_type key;
						deserialize(&key, pair[0], on_error);
						typename T::mapped_type value;
						deserialize(&value, pair[1], on_error);
						some_map->emplace(std::make_pair(std::move(key), std::move(value)));
					} else {
						if (on_error) {
							on_error(config.where() + "Failed to deserialize map: expected array of [key, value] array-pairs.");
						}
					}
				}
			}
		}

		template<typename T>
		typename std::enable_if<visit_struct::traits::is_visitable<T>::value>::type
		deserialize(T* some_struct, const Config& config, const ConversionError& on_error)
		{
			if (!config.is_object()) {
				if (on_error) {
					on_error(config.where() + "Failed to deserialize object: config is not an object.");
				}
			} else {
				visit_struct::apply_visitor([&config, &on_error](const std::string& name, auto& value) {
					if (config.has_key(name)) {
						deserialize(&value, config[name], on_error);
					}
				}, *some_struct);
			}
		}
	#endif // VISITABLE_STRUCT

} // namespace configuru

#endif // CONFIGURU_HEADER_HPP

// Make sure that msvc will reset to the default warning level
#if defined(_MSC_VER)
	#pragma warning( pop )
#endif
