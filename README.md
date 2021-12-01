# strm
A small utility for strings and automata pattern matching in C++20. 

## Usage 

```cpp

const char* src = "patterns everywhere 123";

while(true)
{
	int number_of_spaces = 0;
	
	auto r = strm::match(src, 
		"no match", // default case
		strm::lit<"patterns">    >> "one",
		strm::lit<"everywhere">  >> "two",
		strm::int_num            >> "number",
		strm::lit<" ">           >> [&number_of_spaces] () {
			++number_of_spaces;
			return "space";
		}
	);
	std::cout << r << " ";

	if (not *src)
		break;
}
```

Output : `one space two space number`

The right-hand side of a pattern can be either a value or an invocable object. 
The iterator passed as first argument will be modified to points to
where the match failed or succeeded. 

If two patterns match at the same time, the one that appears first in the 
list of arguments will takes precedence. 

For exemple : 

```cpp
auto src = "hello...";

auto r = strm::match(src, 
			0,
			strm::identifier   >> 1, // a C-like identifier
			strm::lit<"hello"> >> 2
		);
assert( r == 1 );
```

Here, both patterns will match after `hello` has been parsed. 
However, the `identifier` pattern appears first, so the function returns 1. 

A note on compile-time : despite the ugly mechanisms (big macro-expanded switch and templates)
on which this library rely, the compile times aren't too large. 
See the `tests/lexer.cpp` which contains about 50 strings and compiles in about 15 seconds. 
Since this is typically the sort of code that you rarely need to recompile, that's not too bad. 

## Motivation 

C++ doesn't support pattern matching, and writing 
string patterns matching by hand is really tedious. 
To my knowledge there was no existing tool
(barring fully-fledged parsing librairies) to do that in C++,
though if you know the length of your strings in advance, you can 
use a switch with a compile-time hash. 

This goes further and allow you to use any (user-defined) automata 
(e.g. integer, floating-point number, or C-like identifier)
as a pattern. 

## Compiler support

Known to work with : 
* Clang 13+
* GCC 11+
