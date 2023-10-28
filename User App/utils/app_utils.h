/*
 * utils.h
 *
 *  Created on: Sep 20, 2023
 *      Author: Ishaan
 *
 *  Quick utility functions to make code cleaner and a bit more readable
 */

#ifndef UTILS_APP_UTILS_H_
#define UTILS_APP_UTILS_H_

#include <span>
#include <array>
#include <string>
#include <cmath>

//===================== NUMERICAL CONSTANTS =====================

//no real convenient way to set up pi, so just defining a literal
constexpr float PI = 3.14159265358979323846;
constexpr float TWO_PI = 2*PI;

//=========================== CALLBACK FUNCTION HELPERS ========================

/*
 * Callback function "typedefs" essentially
 * I'll start with this: IT'S REEEEEEEAAAAALLLYYYYY DIFFICULT 'CLEANLY' CALL `void()` MEMBER FUNCTIONS OF CLASSES
 * WHILE ALSO MAINTAINING LOW OVERHEAD (i.e. AVOIDING STD::FUNCTION)
 * 	\--> Re: this latter point, people claim that std::function requires heap usage and a lotta extra bloat
 *
 * As such, I've defined three basically container classes that can hold + call callback functions
 *
 * 	>>> `Callback Function` is the most generic of these types
 * 		- Lets you attach a global-scope c-style function or any kinda static function
 * 		- Default constructor is "safe" i.e. calling an uninitialized `Callback Function` will do nothing rather than seg fault
 * 		- call the callback using the standard `()` operator syntax
 *
 * 	>>> `Instance Callback Function` is slightly more specialized:
 * 		- Lets you call a instance method of a particular class on a provided instance
 * 		- Default constructor should be "safe" i.e. calling an uninitialized `Instance Callback Function` will do nothing
 * 			\--> this may incur a slight performance penalty, but safety may be preferred here
 * 			\--> I'm hoping some kinda compiler optimization happens here, but ehhh we'll see
 * 		- call the `instance.callback()` using the standard `()` operator syntax
 *
 * 	>>> `Context Callback Function` is the most generic
 * 		- Lets you attach a global-scope c-style function or any kinda static function
 * 		- but also pass a generic pointer to some `context` to the function
 * 			\--> Anticipated to use the `context` field to pass an instance of a class (or some kinda struct)
 * 		- anticipated use is with a forwarding function that takes a `void*` or `<Type>*` argument
 * 			\--> it will `static_cast()` the `context` back to the intended type, then call one of its instance methods
 * 		- default constructor is "safe" i.e. calling an uninitialized `Context Callback Function` will do nothing rather than seg fault
 * 		- call the callback using the standard `()` operator syntax
 *
 * All the `()` operators are aggressively optimized to minimize as much overhead as possible
 *
 * Insipred by this response in a PJRC forum:
 * https://forum.pjrc.com/threads/70986-Lightweight-C-callbacks?p=311948&viewfull=1#post311948
 * and this talk (specifically at this timestamp):
 * https://youtu.be/hbx0WCc5_-w?t=412
 */

class Callback_Function {
public:
	static inline void empty_cb() {} //upon default initialization, just point to this empty function
	Callback_Function(void(*_func)(void)): func(_func) {}
	Callback_Function(): func(empty_cb) {}
	void __attribute__((optimize("O3"))) operator()() {func();}
	void __attribute__((optimize("O3"))) operator()() const {func();}
private:
	void(*func)(void);
};


template<typename T> //need to specialize a particular target class
class Instance_Callback_Function {
public:
	Instance_Callback_Function(): instance(nullptr), func(nullptr) {}
	Instance_Callback_Function(T* _instance, void(T::*_func)()): instance(_instance), func(_func) {}
	void __attribute__((optimize("O3"))) operator()() {if(instance != nullptr)  ((*instance).*func)();}
	void __attribute__((optimize("O3"))) operator()() const {if(instance != nullptr) ((*instance).*func)();}
private:
	T* instance;
	void(T::*func)();
};


template<typename T = void> //defaults to generic type
class Context_Callback_Function {
public:
	static inline void empty_cb(T* context) {} //upon default initialization, just point to this empty function
	Context_Callback_Function(T* _context, void(*_func)(T*)): context(_context), func(_func) {}
	Context_Callback_Function(): context((T*)nullptr), func(empty_cb) {}
	void __attribute__((optimize("O3"))) operator()() {(*func)(context);}
	void __attribute__((optimize("O3"))) operator()() const {(*func)(context);}
private:
	T* context;
	void(*func)(T*);
};

//============================================== ARRAY TO SPAN SLICING AND CONVERSION UTILTIES ===============================================

/*
 * function that takes a std::array along with slice indices [begin, end)
 * And returns a span with dynamic extent that refers to the section of the array
 * ASSUMES INDICES ARE VALID - DOING SO FOR PERFORMANCE REASONS
 *
 * Have to define this function in the header file since it's templated (necessary evil)
 */

template <typename T, size_t len>
inline std::span<T, std::dynamic_extent> spn(std::array<T, len>& arr, const size_t begin, const size_t end) {
	return std::span<T, std::dynamic_extent>(arr.begin() + begin, arr.begin() + end);
}

/*
 * overload of previous function that takes a std::array along with a size parameter (exclusive)
 * And returns a span with dynamic extent that refers from the beginning to the extent of that particular array
 * ASSUMES LENGTH PARAMETER IS VALID - DOING SO FOR PERFORMANCE REASONS
 *
 * Have to define this function in the header file since it's templated (necessary evil)
 */
template <typename T, size_t len>
inline std::span<T, std::dynamic_extent> spn(std::array<T, len>& arr, const size_t subarray_len) {
	return spn(arr, 0, subarray_len);
}

//########## CONST OVERLOADS OF PREVIOUS CASTING FUNCTIONS ##########

template <typename T, size_t len>
inline std::span<const T, std::dynamic_extent> spn(const std::array<T, len>& arr, const size_t begin, const size_t end) {
	return std::span<const T, std::dynamic_extent>(arr.begin() + begin, arr.begin() + end);
}

template <typename T, size_t len>
inline std::span<const T, std::dynamic_extent> spn(const std::array<T, len>& arr, const size_t subarray_len) {
	return spn(arr, 0, subarray_len);
}
//============================================================================================================================================

/*
 * #### BYTE PACKING AND UNPACKING UTILITIES ####
 *
 * I've been hemming and hawing about what's the most robust, c++ style way to convert between primitive data types and their underlying bit representations
 * And haven't been getting too many satisfying answers. A lot of the techniques for doing so may be affected by the endianness of the underlying machine.
 * So far, I've come across `reinterpret_cast`, using unions, and memcpy.
 *
 * I'm gonna try the `reinterpret_cast` technique due to its performance and canonically c++ syntax, but will revisit this implementation if promises arise
 * NOTE: apparently the modern C++20 way to do this is to use `bit_cast<>`--this should guarantee defined behavior across compilers with reasonable performance
 * However, STM32CubeIDE doesn't support C++20 just yet--will table this change for the future [TODO]
 *
 * Additionally, I'm gonna avoid the use of std::strings in the code generally and encode everything as a character array
 * This should jive a little better with fixed-sized data structures and general memory safety of encoding/packing operations done throughout code
 *
 * NOTE: THESE METHODS ASSUME THAT `buf` HAS VALID ARRAY INDICES [0], [1], [2], [3] (or [`n`] for the string packing function)
 * NO BOUNDS CHECKING IS EXPLICITLY DONE FOR PERFORMANCE REASONS
 */

void pack(const uint32_t val, std::span<uint8_t, std::dynamic_extent> buf); //pack a uint32_t value in big endian order
void pack(const int32_t val, std::span<uint8_t, std::dynamic_extent> buf); //pack an int32_t value in big endian order
void pack(const float val, std::span<uint8_t, std::dynamic_extent> buf); //pack an IEEE 754 single-precision float in big endian order
void pack(const std::string& text, std::span<uint8_t, std::dynamic_extent> buf); //pack an ASCII string into bytes excluding any NULL termination

uint32_t unpack_uint32(std::span<const uint8_t, std::dynamic_extent> buf); //unpack a uint32_t value in big endian order
int32_t unpack_int32(std::span<const uint8_t, std::dynamic_extent> buf); //unpack an int32_t value in big endian order
float unpack_float(std::span<const uint8_t, std::dynamic_extent> buf); //unpack an IEEE 754 single-precision float in big endian order

//============================================================================================================================================
 /*
  * String literal container initializer
  * Useful for initializing std::arrays to hold string values to have a consistent way of storing aggregate, fixed-size data in the program
  *
  * Code lifted from:
  * https://stackoverflow.com/questions/33484233/how-to-initialize-a-stdarraychar-n-with-a-string-literal-omitting-the-trail
  */

template <size_t N, size_t ... Is>
constexpr std::array<uint8_t, N - 1> s2a(const char (&a)[N], std::index_sequence<Is...>)
{
    return {{a[Is]...}};
}

template <size_t N>
constexpr std::array<uint8_t, N - 1> s2a(const char (&a)[N])
{
    return s2a(a, std::make_index_sequence<N - 1>());
}

//==========================================================================================================================================
/*
 * Float string formatting utility
 *
 * Simple utility to print floats with a fixed precision
 * since float formatting can be a bit tricky/memory intensive through library techniques
 * and sometimes requires compiler/build settings to be changed affecting code portability
 *
 * I'll templatize this function for a slight degree of anticipated compiler optimization
 * however, this isn't meant to be a fast function so use in non-performance-critical scenarios
 * ASSUMING THAT THE FLOATING POINT VALUE CAN FIT INTO A SIGNED LONG TYPE (+/- 2.147 billion)
 *
 * Inspired by:
 *  - https://stackoverflow.com/questions/47837838/get-decimal-part-of-a-number
 *  - https://stackoverflow.com/questions/28334435/stm32-printf-float-variable
 *  - https://stackoverflow.com/questions/6143824/add-leading-zeroes-to-string-without-sprintf
 */

template<size_t precision> //how many decimal points to print
inline std::string f2s(float val) {
	constexpr float scaling = std::pow(10.0, (float)precision);

	float integer_part; //modf requires a float argument for the integer part
	float decimal = std::modf(val, &integer_part); //separate the float into its decimal and integer part

	//format the decimal part of the string first by calculating precision, then zero padding appropriately
	std::string decimal_string = std::to_string((long)std::round(decimal * scaling));
	std::string padded_decimal_string = std::string(precision - std::min(precision, decimal_string.length()), '0') + decimal_string;

	//concatenate integer and decimal parts of the string and return
	return std::to_string((long)integer_part) + "." + padded_decimal_string;
}

#endif /* UTILS_APP_UTILS_H_ */
