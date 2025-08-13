#pragma once

#include <tuple>
#include <vector>
#include <functional>
#include <string>
#include <limits>
#include <iostream>

template <typename T>
struct ref_wrap : public std::reference_wrapper<T>
{
	operator T&() const noexcept { return this->get(); }
	ref_wrap(T& other_) : std::reference_wrapper<T>(other_){}
	void operator =(T && other_) {this->get()=other_;}
};

enum class DataLayout 
{ 
	SoA, 	//Structure of arrays
	AoS 	//Array of structures
};

template <typename  TContainer>
class Iterator;

template <template <typename...> class Container, DataLayout TDataLayout, typename TItem>
struct DataLayoutPolicy;

template <template <typename...> class Container, template<typename...> class TItem, typename... Types>
struct DataLayoutPolicy<Container, DataLayout::AoS, TItem<Types...>> 
{
	using type = Container<TItem<Types...>>;
	using value_type = TItem<Types...>&;

	constexpr static value_type get( type& c_, std::size_t position_ ){ return value_type(*static_cast<TItem<Types...>*>(&c_[ position_ ])); }

	constexpr static void resize( type& c_, std::size_t size_ )	{ c_.resize( size_ ); }

	template <typename TValue>
	constexpr static void push_back( type& c_, TValue&& val_ ){ c_.push_back( val_ ); }
	static constexpr std::size_t size(type& c_){ return  c_.size(); }
};


template <template <typename...> class Container, template<typename...> class TItem, typename... Types>
struct DataLayoutPolicy<Container, DataLayout::SoA, TItem<Types...>> 
{
	using type = std::tuple<Container<Types>...>;
	using value_type = TItem<ref_wrap<Types>...>;

	constexpr static value_type get( type& c_, std::size_t position_ )
	{
		return do_get( c_, position_, std::make_integer_sequence<unsigned, sizeof...( Types )>() ); // unrolling parameter pack
	}

	constexpr static void resize( type& c_, std::size_t size_ )	
	{
		do_resize( c_, size_, std::make_integer_sequence<unsigned, sizeof...( Types )>() ); // unrolling parameter pack
	}

	template <typename TValue>
	constexpr static void push_back( type& c_, TValue&& val_ )
	{
		do_push_back( c_, std::forward<TValue>(val_), std::make_integer_sequence<unsigned, sizeof...( Types )>() ); // unrolling parameter pack
	}

	static constexpr std::size_t size(type& c_) { return std::get<0>( c_ ).size(); }

private:

	template <unsigned... Ids>
	constexpr static auto do_get( type& c_, std::size_t position_, std::integer_sequence<unsigned, Ids...> )
	{
		return value_type{ ref_wrap( std::get<Ids>( c_ )[ position_ ] )... }; // guaranteed copy elision
	}

	template <unsigned... Ids>
	constexpr static void do_resize( type& c_, unsigned size_, std::integer_sequence<unsigned, Ids...> )
	{
		( std::get<Ids>( c_ ).resize( size_ ), ... ); //fold expressions
	}

	template <typename TValue, unsigned... Ids>
	constexpr static void do_push_back( type& c_, TValue&& val_, std::integer_sequence<unsigned, Ids...> )
	{
		( std::get<Ids>( c_ ).push_back( std::get<Ids>( std::forward<TValue>( val_ ) ) ), ... ); // fold expressions
	}
};

template <template <typename T> class TContainer, DataLayout TDataLayout, typename TItem>
using policy_t = DataLayoutPolicy<TContainer, TDataLayout, TItem>;

template <template <typename ValueType> class TContainer, DataLayout TDataLayout, typename TItem>
struct BaseContainer
{
	using policy_t = policy_t<TContainer, TDataLayout, TItem>;
	using iterator		  = Iterator<BaseContainer<TContainer, TDataLayout, TItem>>;
	using difference_type = std::ptrdiff_t;
	using value_type	  = typename policy_t::value_type;
	using reference		  = typename policy_t::value_type&;
	using size_type		  = std::size_t;
	auto static constexpr data_layout = TDataLayout;

	BaseContainer() = default;

	BaseContainer(size_t size_)
	{
		resize(size_);
	}

	template <typename Fwd>
	void push_back( Fwd&& val )
	{
		policy_t::push_back( mValues, std::forward<Fwd>(val) );
	}

	std::size_t size()
	{
		return policy_t::size( mValues );
	}

	value_type operator[]( std::size_t position_ )
	{
		return policy_t::get( mValues, position_ );
	}

	void resize( size_t size_ )
	{
		policy_t::resize( mValues, size_ );
	}

	iterator	   begin() { return iterator( this, 0 ); }
	iterator	   end() { return iterator( this, size() ); }

private:

	typename policy_t::type mValues;

};

template <typename  TContainer>
class Iterator
{
private:

	using container_t = TContainer;
public:

	using policy_t = typename container_t::policy_t;
	using difference_type   = std::ptrdiff_t;
	using value_type	= typename policy_t::value_type;
	using reference			= value_type&;
	//using pointer			= value_type*;
	using iterator_category = std::bidirectional_iterator_tag;

	template<typename TTContainer>
	Iterator( TTContainer* container_, std::size_t position_ = 0 ):
		mContainer( container_ ), mIterPosition( position_ ) { }

	Iterator& operator=( Iterator const& other_ )
	{
		mIterPosition = other_.mIterPosition;
		// mContainer = other_.mContainer;
	}

	friend bool operator!=( Iterator const& lhs, Iterator const& rhs ) { return lhs.mIterPosition != rhs.mIterPosition;}
	friend bool operator==( Iterator const& lhs, Iterator const& rhs ) { return !operator!=(lhs, rhs);}

	operator bool() const { mIterPosition != std::numeric_limits<std::size_t>::infinity(); }

	Iterator& operator=( std::nullptr_t const& ) { mIterPosition = std::numeric_limits<std::size_t>::infinity();}

	template <typename T>
	void operator+=( T size_ ){ mIterPosition += size_; }

	template <typename T>
	void operator-=( T size_ ){ mIterPosition -= size_; }

	void operator++(){ return operator +=(1); }
	void operator--(){ return operator -=(1); }

	value_type operator*() { return (*mContainer)[ mIterPosition ]; }

private:

	container_t*        mContainer = nullptr;
	std::size_t         mIterPosition = std::numeric_limits<std::size_t>::infinity();

};