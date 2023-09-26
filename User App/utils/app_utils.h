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


//#### ARRAY TO SPAN SLICING AND CONVERSION UTILTIES ####

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

//============================================== CONST OVERLOADS OF PREVIOUS CASTING FUNCTIONS ===============================================
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


#endif /* UTILS_APP_UTILS_H_ */
