# strm
A simple utility for string pattern matching in C++20. 

## Usage 

```cpp

const char* src = "patterns everywhere 123";

while(true)
{
	auto r = strm::match(src, 
		"no match",
		strm::lit<"patterns">    >> "one",
		strm::lit<"everywhere">  >> "two",
		strm::int_num            >> "number",
		strm::lit<" ">           >> "space"
	);
	std::cout << r << " ";

	if (not *src)
		break;
}
```

Output : `one space two space number`
