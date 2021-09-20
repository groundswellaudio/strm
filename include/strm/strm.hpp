#ifndef STRM_CPP_LIBRARY_HEADER
#define STRM_CPP_LIBRARY_HEADER

#include <type_traits>
#include <utility>
#include <iostream>

#define STRM_FWD(X) static_cast<decltype(X)&&>(X)

namespace strm {

// ====================================================================

enum matcher_status : char {
	running = 0,
	matched = 1,
	failed  = 2
};

template <class T>
concept matcher = requires (const T m, char C) 
{	
	T{m.get_next(C)};
	char(m.min());
	char(m.max());
	matcher_status{m.status()};
	bool( m.can_fail() );
};

template <class T, std::size_t N>
consteval auto min(T(&&arr)[N]) {
	T res = arr[0];
	for (auto it = &arr[0]; it != &arr[N]; ++it){
		res = *it < res ? *it : res;
	}
	return res;
}

template <class T, std::size_t N>
consteval auto max(T(&&arr)[N]) {
	T res = arr[0];
	for (auto it = &arr[0]; it != &arr[N]; ++it){
		res = *it > res ? *it : res;
	}
	return res;
}

constexpr bool is_between(char c, char min, char max){
	return (c >= min and c <= max);
}

constexpr bool is_letter(char c){
	return is_between(c, 'a', 'z') or is_between(c, 'A', 'Z');
}

constexpr bool is_digit(char c) { 
	return is_between(c, '0', '9'); 
}

// ==============================================================
// matchers

template <int N>
struct string_lit {
	
	static constexpr auto size = N;
	
	constexpr string_lit(const char(&arr)[N]){
		for (int k = 0; k < N; ++k) data[k] = arr[k];
	}

	char data[N];
};

template <auto str>
struct string_matcher {

	static constexpr auto size = str.size - 1;
	
	constexpr auto get_next(char C) const 
	{
		if ( index >= size || index == -2 )
			return string_matcher{-2};
		
		if ( index == -1 )
			return *this;
		
		if (str.data[index] == C) 
			return string_matcher{index + 1};
		else
			return string_matcher{-1};
	}
	
	constexpr auto status() const {
		switch(index) {
			case -2 : return matched;
			case -1 : return failed;
			default : return running;
		}
	}
	
	constexpr char min() const { return (index < 0 || index >= size) ? str.data[0] : str.data[index]; }
	constexpr char max() const { return min(); }
	
	constexpr bool can_fail() const { return true;           }
	
	int index = 0;
};

// user-side helper
template <string_lit Str>
inline constexpr auto lit = string_matcher<Str>{};

static_assert( matcher<decltype(lit<"test">)> );


#define STRM_MATCHER_BASE() \
	static constexpr char match_ = -2; \
	static constexpr char fail_  = -1; \
	constexpr auto status() const { \
		switch(state_) { case match_ : return matched; case fail_ : return failed; default : return running; } \
	}
	
///
/// Basic floating point number matcher (DDDD.DDDDD)
///
struct float_matcher {
	
	STRM_MATCHER_BASE()
	
	constexpr auto get_next(char C) const { return float_matcher{next_state(C)}; }
	
	static constexpr auto min_ = strm::min({'0', '.'});
	static constexpr auto max_ = strm::max({'9', '.'});
	
	constexpr char min() const  { return (state_ == 2) ? '0' : min_; }
	constexpr char max() const  { return (state_ == 2) ? '9' : max_; }
	
	constexpr bool can_fail() const { return state_ == 0; }
	
	constexpr char next_state(char C) const 
	{	
		switch(state_)
		{
			case 0 : // the first state
				return is_digit(C) ? 1 : (C == '.') ? 2 : fail_;
			case 1 : // at least one digit
				return is_digit(C) ? 1 : (C == '.') ? 2 : match_;
			case 2 : // after '.'
				return is_digit(C) ? 2 : match_;
			default : 
				return state_;
		}
	}
	
	char state_ = 0;
};

static_assert( matcher<float_matcher> );

inline constexpr float_matcher float_num;

///
/// A C-like identifier matcher
/// Match a '_' or letter optionally followed by a sequence of (letter | digit | '_')
///
struct identifier_matcher {
	
	STRM_MATCHER_BASE()
	
	using Self = identifier_matcher;
	
	static constexpr auto min1 = strm::min({'_', 'a', 'A'});
	static constexpr auto max1 = strm::max({'_', 'z', 'Z'});
	
	static constexpr auto min2 = strm::min({'_', 'a', 'A', '0'});
	static constexpr auto max2 = strm::max({'_', 'z', 'Z', '9'});
	
	constexpr bool can_fail() const { return state_ == 0;      }
	constexpr char min() const { return (state_ == 0) ? min1 : min2; }
	constexpr char max() const { return (state_ == 0) ? max1 : max2; }
	
	constexpr auto get_next(char C) const 
	{
		return ( is_letter(C) || C == '_' ) ? Self{1} 
			: (state_ == 0) ? Self{fail_}
			: is_digit(C)   ? Self{1}
			: Self{match_};
	}
	
	char state_ = 0;
};

static_assert( matcher<identifier_matcher> );

inline constexpr identifier_matcher identifier;

/// 
/// Basic integer matcher
/// Match a sequence of digits
///
struct integer_matcher {
	
	STRM_MATCHER_BASE()
	
	using Self = integer_matcher;
	
	constexpr auto get_next(char C) const 
	{
		return is_digit(C) ? ((state_ == 0) ? Self{1} : Self{state_}) 
				           : ((state_ == 0) ? Self{fail_} : Self{match_});
	}
	
	constexpr bool can_fail() const { return state_ == 0;      }
	
	constexpr char min() const { return '0'; }
	constexpr char max() const { return '9'; }
	
	char state_ = 0;
};

static_assert( matcher<integer_matcher> );

inline constexpr integer_matcher int_num;

#undef STRM_MATCHER_BASE

///
/// Utility wrapper to bind a matcher to a result
///
template <matcher Matcher, class Result>
struct case_
{
	case_(Matcher, Result r) : result{static_cast<Result&&>(r)} {}
	
	using matcher = Matcher;
	Result result;
};

namespace impl {
	template <class T>
	struct fn_ {
		T operator()() { return value; }
		T value;
	};
	
	template <class T>
	decltype(auto) fn_wrap ( T v ) {
		if constexpr ( requires {v();} )
			return v;
		else
			return fn_<T>{static_cast<T&&>(v)};
	}
}

template <matcher Matcher, class Result>
constexpr auto operator >> (Matcher m, Result r)
{
	using R = decltype(impl::fn_wrap(r));
	return case_<Matcher, R>{ m, impl::fn_wrap(r) };
}

namespace impl {
	
	template <class T>
	void type_()
	{
		std::cout << __PRETTY_FUNCTION__ << std::endl;
	}

	template <unsigned char = 1>
	struct find_type_i;

	template <>
	struct find_type_i<1> {
		template <int Idx, class T, class... Ts>
		using f = typename find_type_i<(Idx != 1)>::template f<Idx - 1, Ts...>;
	};

	template <>
	struct find_type_i<0> {
		template <int, class T, class... Ts>
		using f = T;
	};

	template <int K, class... Ts>
	using type_pack_element = typename find_type_i<(K != 0)>::template f<K, Ts...>;

	template <class T>
	struct tag{};
	
	// ====================================================================

	template <bool P>
	struct cond {
		template <class A, class B>
		using f = A;
	};

	template <>
	struct cond<false> {
		template <class A, class B>
		using f = B;
	};

	template <bool P, class A, class B>
	using conditional = typename cond<P>::template f<A, B>;
	
	// ====================================================================
	// a simple tuple to store actions
	
	template <std::size_t Idx, class T>
	struct tuple_leaf { T value; };
	
	template <std::size_t Idx, class T>
	constexpr auto& get(tuple_leaf<Idx, T>& l) { return l.value; }

	template <class Seq, class...>
	struct tuple_data;

	template <std::size_t... Idx, class... Ts>
	struct tuple_data<std::index_sequence<Idx...>, Ts...> : tuple_leaf<Idx, Ts>... {};
	
	template <class... Ts>
	struct tuple : tuple_data<std::make_index_sequence<sizeof...(Ts)>, Ts...> {};

	template <class... Ts>
	tuple(Ts...) -> tuple<Ts...>;
	
	// ====================================================================
	
	template <class Result, class Iterator>
	struct fallback
	{
		constexpr decltype(auto) operator()(Iterator& dest) 
		{
			dest = saved_iterator;
			if constexpr ( requires {result();} )
				return result();
			else
				return result;
		}
		
		Result result;
		Iterator saved_iterator;
	};
	
	template <class R, class I>
	fallback(R, I) -> fallback<R, I>;
	
	template <class Action>
	struct default_case 
	{ 
		constexpr decltype(auto) operator()(auto) 
		{
			if constexpr ( requires { action(); } )
				return action();
			else
				return action;
		}
		
		Action action;
	};
	
	template <class A>
	default_case(A) -> default_case<A>;
	
	// ====================================================================
	
	template <int A, class... Ts>
	struct matcher_list;
	
	// traverse the status array and pick the matchers
	template <char Pick, bool LookForMatch>
	struct make_next_list 
	{
		template <int Clk, auto Data, class Head, class... Tail> 
		using f = typename make_next_list< Head::matcher.status(), LookForMatch >
			::template f<(Clk - 1), Data, Head, Tail...>;
	};
	
	// -2 : pick
	// -1 : stop
	// 0 : matcher is running, keep it
	// 1 : matcher successful, update state and toss it
	// 2 : matcher failed, toss it
	
	template <bool B>
	struct make_next_list<-1, B> 
	{
		template <int, auto Data, class... Ts>
		using f = matcher_list< Data, Ts... >;
	};
	
	template <bool B>
	struct make_next_list<0, B> 
	{
		template <int Clk, auto Data, class Head, class... Tail>
		using f = typename make_next_list< (Clk == 0 ? -1 : -2), B >
			::template f<Clk, Data, Tail..., Head>;
	};
	
	// Found a "matched" status, filter it out and update the CurrentActionIndex
	template <>
	struct make_next_list<1, true>
	{
		template <int Clk, auto Data, class Head, class... Tail>
		using f = typename make_next_list< (Clk == 0 ? -1 : -2), false>
			::template f<Clk, Head::index, Tail...>;
	};
	
	// Found a "matched" but we don't care cause we already found a "matched" in this row
	template <>
	struct make_next_list<1, false>
	{
		template <int Clk, auto Data, class Head, class... Tail>
		using f = typename make_next_list< (Clk == 0 ? -1 : -2), false>
			::template f<Clk, Data, Tail...>;
	};
	
	// Filter out the matcher
	template <bool B>
	struct make_next_list<2, B> 
	{	
		template <int Clk, auto Data, class Head, class... Tail>
		using f = typename make_next_list< (Clk == 0 ? -1 : -2), B >
			::template f<Clk, Data, Tail...>;
	};
	
	template <auto Data, class... Cases>
	using make_next_list_t =
	typename make_next_list<-2, (sizeof...(Cases) < 100000000000000)> // trick the compiler into deferring the alias
		::template f
		<sizeof...(Cases), 
		 Data, 
		 Cases...
		>;
		
	// ====================================================================
	
	template <int ActionIdx, class... Cases>
	struct matcher_list
	{	
		static constexpr auto action_index = ActionIdx;
		
		static constexpr bool failed = false;
		static constexpr auto min = strm::min({static_cast<char>(Cases::matcher.min())...});
		static constexpr auto max = strm::max({static_cast<char>(Cases::matcher.max())...});
		static constexpr bool can_fail = (Cases::matcher.can_fail() && ...);
		
		template <char C> 
		using get_next = make_next_list_t<ActionIdx, typename Cases::template get_next<C>...>;
	};
	
	template <int ActionIdx>
	struct matcher_list<ActionIdx>
	{
		static constexpr auto action_index = ActionIdx;
		static constexpr bool failed = true;
	};
	
	template <class... Ts>
	matcher_list(Ts...) -> matcher_list<-1, Ts...>;
	
	template <auto M, std::size_t Idx>
	struct w_index {
		static constexpr auto index = Idx;
		static constexpr auto matcher = M;
		
		template <char C>
		using get_next = w_index<M.get_next(C), Idx>;
	};
	
	template <class Seq>
	struct make_list_impl;
	
	template <std::size_t... Idx>
	struct make_list_impl<std::index_sequence<Idx...>> {
	
		template <class... Ts>
		using f = matcher_list<-1, w_index<Ts{}, Idx>...>;
	};
	
	template <std::size_t Sz>
	using make_seq = std::make_index_sequence<Sz>;
	
	template <class... Ts>
	using make_matchers_list = typename make_list_impl< make_seq<sizeof...(Ts)> >::
		template f <Ts...>;

	#define REP5(N) M(N) M(N + 1) M(N + 2) M(N + 3) M(N + 4)
	#define REP10(N) REP5(N) REP5(N + 5)
	#define REP15(N) REP10(N) REP5(N + 10)
	#define REP30(N) REP10(N) REP10(N + 10) REP10(N + 20)
	#define REP50(N) REP10(N) REP10(N + 10) REP10(N + 20) REP10(N + 30) REP10(N + 40)
	#define REP100(N) REP50(N) REP50(N + 50)
	#define REP255(N) REP100(N) REP100(N + 100) REP50(N + 200) REP5(N + 250)
	
	// ==================================================================
	// match impl
	
	template <class Next, class Prev>
	auto match_impl_tail(auto& src, auto&& current, auto&& actions)
	{
		if constexpr ( Next::action_index != Prev::action_index && Next::can_fail )
		{
			auto&& next_current = impl::fallback{impl::get<Next::action_index>(actions), src};
			return match_impl<Next>( src, STRM_FWD(next_current), STRM_FWD(actions) );
		}
		return match_impl<Next>( src, STRM_FWD(current), STRM_FWD(actions) );
	}

	template <class List, class Src>
	auto match_impl(Src& src, auto&& current_action, auto&& actions) -> decltype( current_action(src) )
	{
		constexpr auto min = List::min;
		constexpr auto max = List::max;
	
		#define IMPL(N) \
			using NextList = typename List::template get_next<N>; \
			type_<NextList>(); \
			if constexpr ( not NextList::failed ) \
			{ \
				++src; \
				return impl::match_impl_tail<NextList, List>(src, STRM_FWD(current_action), STRM_FWD(actions)); \
			} 
		
		#define M(N) case N : { \
			 \
			if constexpr (N >= min && N <= max) \
			{ \
				IMPL(N) \
			} \
			break; \
		} \
	
		constexpr auto Range = max - min;
	
		if constexpr (Range == 0)
		{
			// this case should be common, e.g. when there is only a string matcher left
			if (*src == min)
			{
				IMPL( min )
			}
		}
		else if constexpr (Range + 1 <= 5)
		{
			switch(static_cast<unsigned char>(*src))
			{
				REP5( min )
				default : 
					break;
			}
		}
		else if constexpr (Range + 1 <= 15)
		{
			switch(static_cast<unsigned char>(*src))
			{
				REP15( min )
				default : 
					break;
			}
		}
		else if constexpr (Range + 1 <= 30)
		{
			switch(static_cast<unsigned char>(*src))
			{
				REP30( min )
				default :
					break;
			}
		}
		else if constexpr (Range + 1 <= 50)
		{
			switch(static_cast<unsigned char>(*src))
			{
				REP50( min )
				default :
					break;
			}
		}
		else
		{
			switch(static_cast<unsigned char>(*src))
			{
				REP255(0)
				default :
					break;
			}
		}
	
		// Instead of having the final branch at every case, and generating more code than we need
		// we can put it here at the end of the function. However we're assuming that
		// no matcher will ever not generate a "fail" status when receiving a 0, which is
		// probably a fair assumption?
		
		using NextList = typename List::template get_next<0>;
		
		constexpr auto idx = NextList::action_index;
		
		if constexpr ( idx != -1 )
		{
			auto& res = impl::get<idx>( actions );
			if constexpr ( requires {res();} )
				return res();
			else
				return res;
		}
		else
			return current_action(src);
	
		#undef M
		#undef IMPL
	}

	#undef REP5
	#undef REP10
	#undef REP15
	#undef REP30
	#undef REP50
	#undef REP100
	#undef REP255
	
	// ==================================================================

} // IMPL 

template <class It>
concept stream = requires (It& it) { 
	++it;
	char(*it); 
};

template <stream Iter, class Default, class... Cases>
constexpr decltype(auto) match(Iter& src, Default&& default_, Cases... cases)
{
	using def_case = impl::default_case<std::decay_t<Default&&>>;
		
	return match_impl
	< 
	 impl::make_matchers_list< typename Cases::matcher... >
	>
	(src,
	 impl::default_case{ impl::fn_wrap(default_) },
	 impl::tuple{cases.result...}
	);
}

} // STRM

#undef STRM_FWD

#endif
