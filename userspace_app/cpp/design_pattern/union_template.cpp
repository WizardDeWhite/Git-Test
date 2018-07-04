#include <iostream>

// g++ -std=c++11 union_template.cpp

using namespace std;

template<typename X, typename... Ts>
struct Position;

template<typename X>
struct Position<X> {
    static const int pos = -1;
};

template<typename X, typename... Ts>
struct Position<X, X, Ts...> {
    static const int pos = 0;
};

template<typename X, typename T, typename... Ts>
struct Position<X, T, Ts...> {
    static const int pos = Position<X, Ts...>::pos != -1 ? Position<X, Ts...>::pos + 1 : -1;
};

template<typename... Ts>
struct Type_Info;

template<typename T, typename... Ts>
struct Type_Info<T&, Ts...> {
    static const bool no_reference_types = false;
    static const bool no_duplicates = Position<T, Ts...>::pos == -1 && Type_Info<Ts...>::no_duplicates;
    static const size_t size = Type_Info<Ts...>::size > sizeof(T&) ? Type_Info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + Type_Info<Ts...>::count;
};

template<typename T, typename... Ts>
struct Type_Info<T, Ts...> {
    static const bool no_reference_types = Type_Info<Ts...>::no_reference_types;
    static const bool no_duplicates = Position<T, Ts...>::pos == -1 && Type_Info<Ts...>::no_duplicates;
    static const size_t size = Type_Info<Ts...>::size > sizeof(T) ? Type_Info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + Type_Info<Ts...>::count;
};

template<>
struct Type_Info<> {
    static const bool no_reference_types = true;
    static const bool no_duplicates = true;
    static const size_t count = 0;
    static const size_t size = 0;
};

template<typename... Types>
class static_variant {
    static_assert(Type_Info<Types...>::no_reference_types, "Reference types are not permitted in static_variant.");
    static_assert(Type_Info<Types...>::no_duplicates, "static_variant type arguments contain duplicate types.");

    int64_t _tag;
    char storage[Type_Info<Types...>::size];

    template<typename X>
    void init(const X& x) {
        _tag = Position<X, Types...>::pos;
        new(storage) X(x);
    }

    template<typename X>
    void init(X&& x) {
        _tag = Position<X, Types...>::pos;
        new(storage) X( std::move(x) );
    }

public:
    template<typename X>
    struct tag
    {
       static_assert(
         Position<X, Types...>::pos != -1,
         "Type not in static_variant."
       );
       static const int value = Position<X, Types...>::pos;
    };

    int64_t which() const {return _tag;}
    friend bool operator == ( const static_variant& a, const static_variant& b )
    {
       return a.which() == b.which();
    }
    friend bool operator < ( const static_variant& a, const static_variant& b )
    {
       return a.which() < b.which();
    }

    static_variant()
    {
       _tag = 0;
       //storage_ops<0, Types...>::con(0, storage);
    }
    ~static_variant() {
       //storage_ops<0, Types...>::del(_tag, storage);
    }

    template<typename X>
    static_variant(const X& v) {
        static_assert( Position<X, Types...>::pos != -1, "Type not in static_variant.");
        init(v);
    }

    template<typename X>
    X& get() {
        static_assert(
            Position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        if(_tag == Position<X, Types...>::pos) {
            void* tmp(storage);
            return *reinterpret_cast<X*>(tmp);
        } else {
            //FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename X>
    const X& get() const {
        static_assert(
            Position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        if(_tag == Position<X, Types...>::pos) {
            const void* tmp(storage);
            return *reinterpret_cast<const X*>(tmp);
        } else {
            //FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
        }
    }
};

int main()
{
	// sizeof(Type_Info<>) is the largest size of all the type
	cout << "sizeof(Type_Info<int, char>): " << Type_Info<int, char>::size << endl;
	cout << "sizeof(Type_Info<int, char, double>): " << Type_Info<int, char, double>::size << endl;
	cout << endl;

	// pos stands for the position of the first type in following list of
	// types
	cout << "Position of int in <int, int>: " << Position<int, int>::pos << endl;
	cout << "Position of double in <int, char, double>: " << Position<double, int, char, double>::pos << endl;
	cout << endl;

	typedef static_variant<int, char, double> S_Var;
	cout << "char position in <int, char, double>: " << S_Var::tag<char>::value << endl;
	cout << endl;

	// define a static_variant variable
	S_Var a('a'), b(5), c(8);
	if (a == b) {
		cout << "a('a') and b(5) are the same" << endl;
	} else {
		cout << "a('a') and b(5) are different" << endl;
	}
	if (b == c) {
		cout << "b(5) and c(8) are the same" << endl;
	} else {
		cout << "b(5) and c(8) are different" << endl;
	}
	cout << endl;

	char val_a = a.get<char>();
	int val_b = b.get<int>();
	cout << "content in val_a: " << val_a << endl;
	cout << "content in val_b: " << val_b << endl;
	cout << endl;

	return 0;
}
