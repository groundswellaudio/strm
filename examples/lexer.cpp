
#include <strm/strm.hpp>

enum class token_kind : unsigned char {
	#define KW(N) N##_,
	#define SYM(S, NAME) NAME,
	
	#include "lex.def"
	
	#undef KW
	#undef SYM
	
	id,
	invalid
};

int main(){
	
	const char* src = "struct { int x; float y; }; template <int Z> void foo(){} ";
	
	#define STR2(X) #X
	#define STR(X) STR2(X)
	
	#define KW(N)         ,strm::lit<STR(N)>  >> token_kind::N##_
	#define SYM(S, NAME)  ,strm::lit<STR(S)>  >> token_kind::NAME
	
	auto x = strm::match(
		src,
		token_kind::invalid,
		strm::identifier >> token_kind::id
		#include "lex.def"
	);
}