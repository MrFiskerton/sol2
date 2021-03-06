#define SOL_CHECK_ARGUMENTS 1

#include <sol.hpp>

#include <catch.hpp>

#include <algorithm>
#include <numeric>
#include <iostream>

TEST_CASE("operators/default", "test that generic equality operators and all sorts of equality tests can be used") {
	struct T {};
	struct U {
		int a;
		U(int x = 20) : a(x) {}
		bool operator==(const U& r) {
			return a == r.a;
		}
	};
	struct V {
		int a;
		V(int x = 20) : a(x) {}
		bool operator==(const V& r) const {
			return a == r.a;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	T t1;
	T& t2 = t1;
	T t3;
	U u1;
	U u2{ 30 };
	U u3;
	U v1;
	U v2{ 30 };
	U v3;
	lua["t1"] = &t1;
	lua["t2"] = &t2;
	lua["t3"] = &t3;
	lua["u1"] = &u1;
	lua["u2"] = &u2;
	lua["u3"] = &u3;
	lua["v1"] = &v1;
	lua["v2"] = &v2;
	lua["v3"] = &v3;

	SECTION("regular") {
		lua.new_usertype<T>("T");
		lua.new_usertype<U>("U");
		lua.new_usertype<V>("V");

		// Can only compare identity here
		REQUIRE_NOTHROW([&] {
			lua.script("assert(t1 == t1)");
			lua.script("assert(t2 == t2)");
			lua.script("assert(t3 == t3)");
		}());
		REQUIRE_NOTHROW([&] {
			lua.script("assert(t1 == t2)");
			lua.script("assert(not (t1 == t3))");
			lua.script("assert(not (t2 == t3))");
		}());
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		REQUIRE_NOTHROW([&] {
			lua.script("assert(u1 == u1)");
			lua.script("assert(u2 == u2)");
			lua.script("assert(u3 == u3)");
		}());
		REQUIRE_NOTHROW([&] {
			lua.script("assert(not (u1 == u2))");
			lua.script("assert(u1 == u3)");
			lua.script("assert(not (u2 == u3))");
		}());
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		REQUIRE_NOTHROW([&] {
			lua.script("assert(v1 == v1)");
			lua.script("assert(v2 == v2)");
			lua.script("assert(v3 == v3)");
		}());
		REQUIRE_NOTHROW([&] {
			lua.script("assert(not (v1 == v2))");
			lua.script("assert(v1 == v3)");
			lua.script("assert(not (v2 == v3))");
		}());
	}
	SECTION("simple") {
		lua.new_simple_usertype<T>("T");
		lua.new_simple_usertype<U>("U");
		lua.new_simple_usertype<V>("V");

		// Can only compare identity here
		REQUIRE_NOTHROW([&] {
			lua.script("assert(t1 == t1)");
			lua.script("assert(t2 == t2)");
			lua.script("assert(t3 == t3)");
		}());
		REQUIRE_NOTHROW([&] {
			lua.script("assert(t1 == t2)");
			lua.script("assert(not (t1 == t3))");
			lua.script("assert(not (t2 == t3))");
		}());
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		REQUIRE_NOTHROW([&] {
			lua.script("assert(u1 == u1)");
			lua.script("assert(u2 == u2)");
			lua.script("assert(u3 == u3)");
		}());
		REQUIRE_NOTHROW([&] {
			lua.script("assert(not (u1 == u2))");
			lua.script("assert(u1 == u3)");
			lua.script("assert(not (u2 == u3))");
		}());
		// Object should compare equal to themselves
		// (and not invoke operator==; pointer test should be sufficient)
		REQUIRE_NOTHROW([&] {
			lua.script("assert(v1 == v1)");
			lua.script("assert(v2 == v2)");
			lua.script("assert(v3 == v3)");
		}());
		REQUIRE_NOTHROW([&] {
			lua.script("assert(not (v1 == v2))");
			lua.script("assert(v1 == v3)");
			lua.script("assert(not (v2 == v3))");
		}());
	}
}

TEST_CASE("operators/call", "test call operator generation") {
	struct callable {
		int operator ()(int a, std::string b) {
			return a + b.length();
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("regular") {
		lua.new_usertype<callable>("callable");
		{
			lua.safe_script("obj = callable.new()");
			lua.safe_script("v = obj(2, 'bark woof')");
			int v = lua["v"];
			REQUIRE(v == 11);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<callable>("callable");
		{
			lua.safe_script("obj = callable.new()");
			lua.safe_script("v = obj(2, 'bark woof')");
			int v = lua["v"];
			REQUIRE(v == 11);
		}
	}
}

struct stringable {
	static const void* last_print_ptr;
};
const void* stringable::last_print_ptr = nullptr;

std::ostream& operator<< (std::ostream& ostr, const stringable& o) {
	stringable::last_print_ptr = static_cast<const void*>(&o);
	return ostr << " { stringable! }";
}

TEST_CASE("operators/stringable", "test std::ostream stringability") {
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("regular") {
		lua.new_usertype<stringable>("stringable");
		{
			lua.safe_script("obj = stringable.new()");
			lua.safe_script("print(obj)");
			stringable& obj = lua["obj"];
			REQUIRE(stringable::last_print_ptr == &obj);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<stringable>("stringable");
		{
			lua.safe_script("obj = stringable.new()");
			lua.safe_script("print(obj)");
			stringable& obj = lua["obj"];
			REQUIRE(stringable::last_print_ptr == &obj);
		}
	}
}

TEST_CASE("operators/container-like", "test that generic begin/end and iterator are automatically bound") {
#if SOL_LUA_VERSION > 501
	struct container {
		typedef int* iterator;
		typedef int value_type;

		value_type values[10];

		container() {
			std::iota(begin(), end(), 1);
		}

		iterator begin() {
			return &values[0];
		}

		iterator end() {
			return &values[0] + 10;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("regular") {
		lua.new_usertype<container>("container");
		{
			lua.safe_script("obj = container.new()");
			lua.safe_script("i = 0 for k, v in pairs(obj) do i = i + 1 assert(k == v) end");
			std::size_t i = lua["i"];
			REQUIRE(i == 10);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<container>("container");
		{
			lua.safe_script("obj = container.new()");
			lua.safe_script("i = 0 for k, v in pairs(obj) do i = i + 1 assert(k == v) end");
			std::size_t i = lua["i"];
			REQUIRE(i == 10);
		}
	}
#else
	REQUIRE(true);
#endif
}

TEST_CASE("operators/length", "test that size is automatically bound to the length operator") {
	struct sizable {
		std::size_t size() const {
			return 6;
		}
	};

	sol::state lua;
	lua.open_libraries(sol::lib::base);

	SECTION("regular") {
		lua.new_usertype<sizable>("sizable");
		{ 
			lua.safe_script("obj = sizable.new()"); 
			lua.safe_script("s = #obj");
			std::size_t s = lua["s"];
			REQUIRE(s == 6);
		}
	}
	SECTION("simple") {
		lua.new_simple_usertype<sizable>("sizable");
		{
			lua.safe_script("obj = sizable.new()");
			lua.safe_script("s = #obj");
			std::size_t s = lua["s"];
			REQUIRE(s == 6);
		}
	}
}
