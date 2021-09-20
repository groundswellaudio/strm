#include <strm/strm.hpp>
#include <cassert>
#include <iostream>

constexpr unsigned long hash(std::string_view v)
{	
	auto it = v.begin();
	const auto end = v.end();
	
	unsigned long hash = 5381;
	int c = *it;
	
	while( it != end)
		hash = ((hash << 5) + hash) + c;

	return hash;
}

constexpr bool failed(auto m) {
	return m.status() == strm::failed;
}

constexpr bool matched(auto m) {
	return m.status() == strm::matched;
}

constexpr bool running(auto m) {
	return m.status() == strm::running;
}

void test_literal()
{
	constexpr auto l  = strm::lit<"hey">;
	
	static_assert( l.size == 3 );
	
	constexpr auto l2 = l.get_next('h');
	static_assert( running(l2) );
	
	constexpr auto l3 = l2.get_next('e');
	static_assert( running(l3) );
	
	constexpr auto l4 = l3.get_next('y');
	static_assert( running(l3) );
	
	static_assert( l4.index == 3 );
	
	static_assert( matched(l4.get_next('8')) );
	
	static_assert( failed(l3.get_next('3')) );
	static_assert( failed(l.get_next('a'))  );
}

void test_lit2()
{
	constexpr auto l = strm::lit<"==">;
	
	constexpr auto z = l.get_next('=');
	
	static_assert( running(z) );
	
	constexpr auto zz = l.get_next(' ');
	
	static_assert( failed(zz) );
	static_assert( failed(zz.get_next('0')) );
}

void test_intnum()
{
	constexpr auto m = strm::int_num;
	
	constexpr auto m2 = m.get_next('2');
	
	static_assert( running(m2) );
	
	constexpr auto m3 = m2.get_next('3');
	
	static_assert( running(m3) );
	
	static_assert( matched(m3.get_next('Z')) );
	
	static_assert( m.min() == '0' );
	static_assert( m.max() == '9' );
}

void test_identifier()
{
	constexpr auto m = strm::identifier;
	constexpr auto m2 = m.get_next('h');
	
	static_assert( running(m2) );
	
	constexpr auto m3 = m2.get_next('e');
	constexpr auto m4 = m3.get_next('z');
	constexpr auto m5 = m4.get_next('o');
	
	static_assert( running(m5) );
	
	static_assert( running(m2.get_next('_')) );
	static_assert( running(m2.get_next('4')) );
	
	static_assert( matched(m2.get_next('.')) );
	
	static_assert( failed(m.get_next('4')) );
}

template <auto A, auto B>
void assert_eq(){
	static_assert( A == B );
}

int main(){
	
	auto&& src = "hello123";
	auto it = src;
	
	{
		auto r = strm::match(it,
			-1,
			strm::lit<"hello">    >> 0,
			strm::lit<"world">    >> 1,
			strm::lit<"bonjour">  >> 2 
		);
		
		std::cout << r << std::endl;
		std::cout << it << std::endl;
		
		assert(r == 0);
		
		assert(*it == '1');
	}
	
	{
		auto&& str = "= ";
		auto it = str;
		
		auto r = strm::match(it, 
			-1,
			strm::lit<"=="> >> 0,
			strm::lit<"=">  >> 1
		);
		
		std::cout << r << std::endl;
		assert( r == 1 );
		assert( *it == ' ' );
	}
	
	auto&& num = "123";
	
	{
		auto i = num;
		
		auto r = strm::match(i, 
			-1,
			strm::lit<"nope"> >> 0,
			strm::int_num     >> 1,
			strm::lit<"12">   >> 2
		);
		
		std::cout << r << std::endl;
		assert( r == 1 );
		assert( *i == '\0' );
	}
	
	{
		auto&& numxx = "123xx";
		
		auto i = numxx;
		
		// 1 and 2 matches at the same time, but 1 appears first so is given priority
		auto r = strm::match(i, 
			-1,
			strm::lit<"nope"> >> 0,
			strm::lit<"123">  >> 1,
			strm::int_num     >> 2
		);
		
		std::cout << r << std::endl;
		
		assert( r == 1 );
	}
	
	// same as above
	{
		auto i = it;
		
		auto r = strm::match(i, 
			-1,
			strm::lit<"nope"> >> 0,
			strm::int_num     >> 1,
			strm::lit<"123">  >> 2
		);
		
		assert( r == 1 );
		assert( *i == '\0' );
	}
	
	
	{
		auto&& str = "{} struct 500";
		auto i = str;
		
		auto r = strm::match(i, 
			-1,
			strm::lit<"{">     >> 0,
			strm::lit<"}">     >> 1
		);
		
		assert( r == 0 );
		assert( *i == '}' );
	}
	
	// testing identifiers
	{
		auto&& str = "wussup54++{}";
		
		auto i = str;
		
		auto r = strm::match(i, 
			-1,
			strm::lit<"wu">  >> 0,
			strm::identifier >> 1,
			strm::int_num    >> 2
		);
		
		assert( r == 1 );
		std::cout << i << std::endl;
		assert( *i == '+' ); 
	}
}