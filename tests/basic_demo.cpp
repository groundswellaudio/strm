#include <strm/strm.hpp>
#include <cassert>
#include <string_view>

auto do_match(const char*& it, int& number_of_spaces)
{	
	return strm::match(it, 
			"no match",
			strm::lit<"patterns">    >> "one",
			strm::lit<"everywhere">  >> "two",
			strm::int_num            >> "number",
			strm::lit<" ">           >> [&number_of_spaces] () {
				++number_of_spaces;
				return "space";
			}
	);
}

using std::string_view;

int main()
{	
	const char* src = "patterns everywhere 123";
	
	int number_of_spaces = 0;
	
	assert( string_view{"one"}    == do_match(src, number_of_spaces) );
	assert( string_view{"space"}  == do_match(src, number_of_spaces) );
	assert( string_view{"two"}    == do_match(src, number_of_spaces) );
	assert( string_view{"space"}  == do_match(src, number_of_spaces) );
	assert( string_view{"number"} == do_match(src, number_of_spaces) );
	assert( number_of_spaces == 2 );
}