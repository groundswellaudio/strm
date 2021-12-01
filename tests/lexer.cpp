
#include <strm/strm.hpp>
#include <cassert> 

enum class tok : unsigned char {
	#define KW(N) N##_,
	#define SYM(S, NAME) NAME,
	
	#include "lex.def"
	
	#undef KW
	#undef SYM
	
	id,
	invalid
};

struct TokenStream {

	void scan()
	{	
		while(*it <= 32)
		{
			if (*it == 0)
				return;
			++it;
		}
		
		#define STR2(X) #X
		#define STR(X) STR2(X)
	
		#define KW(N)         strm::lit<STR(N)>  >> tok::N##_,
		#define SYM(S, NAME)  strm::lit<S>  >> tok::NAME,
		
		// caution : the "identifier" matcher should appear last!
		// otherwise it will take precedence over all the keywords
		
		tok_ = strm::match(
			it,
			tok::invalid,
			#include "lex.def"
			strm::identifier >> tok::id
		);
		
		#undef KW
		#undef SYM
	}
	
	auto& operator++() { scan(); return *this; }
	
	const char* it;
	tok tok_ = tok::invalid;
};

template <unsigned N>
void test_token_stream(TokenStream& it, tok(&&tokens)[N])
{
	int z = 0;
	for (auto k : tokens)
	{
		assert( it.tok_ == k );
		++it;
		++z;
	}
}

int main(){
	
	const char* src = "struct { int x; float y; }; template <int Z> void foo(){} ";
	
	auto iter = TokenStream{src};
	iter.scan();
	
	test_token_stream(
		iter,
		{tok::struct_, tok::lbrace, 
		 tok::int_, tok::id, tok::semicolon,
		 tok::float_, tok::id, tok::semicolon,
		 tok::rbrace, tok::semicolon,
		 tok::template_, tok::lesser, tok::int_, tok::id, tok::greater, 
		 tok::void_, tok::id, tok::lparens, tok::rparens,
		 tok::lbrace, tok::rbrace
		}
	);
}