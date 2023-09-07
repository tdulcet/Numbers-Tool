// Teal Dulcet

// Requires support for 128-bit integers (the __int128/__int128_t type).

// Support for arbitrary-precision integers requires the GNU Multiple Precision (GMP) library
// sudo apt-get update
// sudo apt-get install libgmp-dev

// Compile without GMP: g++ -std=gnu++17 -Wall -g -O3 -flto -march=native numbers.cpp -o numbers

// Compile with GMP: g++ -std=gnu++17 -Wall -g -O3 -flto -march=native numbers.cpp -o numbers -DHAVE_GMP -lgmpxx -lgmp

// Run: ./numbers [OPTION(S)]... [NUMBER(S)]...
// If any of the NUMBERS are negative, the first must be preceded by a --.

// Optionally configure the program to use an external factor command instead of the builtin prime factorization functionality. This may be faster when factoring some very large numbers, but slower when factoring large ranges of numbers. Just set the FACTOR define below to the factor command path, for example: "/usr/bin/factor". Alternatively, add the -DFACTOR='"<path>"' option when comping the program, for example -DFACTOR='"/usr/bin/factor"'.

// On Linux distributions with GNU Coreutils older than 9.0, including Ubuntu (https://bugs.launchpad.net/ubuntu/+source/coreutils/+bug/696618) and Debian (https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=608832), the factor command (part of GNU Coreutils) is built without arbitrary-precision/bignum support. If this is the case on your system and you are compiling this program with GMP, you would also need to build the factor command with GMP.

// Requires Make, the GNU C compiler, Autoconf and Automake
// sudo apt-get install build-essential autoconf autopoint texinfo bison gperf

/* Build GNU Coreutils

	Save current directory
	DIRNAME=$PWD

	cd /tmp/
	git clone --depth 1 https://github.com/coreutils/coreutils.git
	git clone https://github.com/coreutils/gnulib.git
	GNULIB_SRCDIR="$PWD/gnulib"
	cd coreutils/
	./bootstrap --gnulib-srcdir="$GNULIB_SRCDIR"
	./configure # --disable-gcc-warnings
	make -j "$(nproc)" CFLAGS="-g -O3 -flto"
	make -j "$(nproc)" check CFLAGS="-g -O3 -flto" RUN_EXPENSIVE_TESTS=yes RUN_VERY_EXPENSIVE_TESTS=yes

	Copy factor command to starting directory
	cp ./src/factor "$DIRNAME/" */

/* Build uutils coreutils (Cross-platform Rust rewrite of the GNU Coreutils, but does not currently support arbitrary-precision/bignums (https://github.com/uutils/coreutils/issues/1559))

	Requires Rust
	curl https://sh.rustup.rs -sSf | sh

	Save current directory
	DIRNAME=$PWD

	cd /tmp/
	git clone --depth 1 https://github.com/uutils/coreutils.git
	cd coreutils/
	make PROFILE=release
	make -j "$(nproc)" test

	Copy factor command to starting directory
	cp ./target/release/factor "$DIRNAME/" */

#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <climits>
#include <cfloat>
#include <limits>
#include <clocale>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cinttypes>
#include <regex>
#include <bit>
#include <getopt.h>
#if HAVE_GMP
#include <gmpxx.h>
#endif

using namespace std;

// factor command
// #define FACTOR "factor"

enum
{
	// DEV_DEBUG_OPTION = CHAR_MAX + 1,
	TO_OPTION = CHAR_MAX + 1,
	FROM_BASE_OPTION,
	BINARY_OPTION,
	TERNARY_OPTION,
	QUATERNARY_OPTION,
	QUINARY_OPTION,
	DECIMAL_OPTION,
	DUO_OPTION,
	VIGES_OPTION,
	// BASE36_OPTION,
	BRAILLE_OPTION,
	SPECIAL_OPTION,
	ASCII_OPTION,
	UPPER_OPTION,
	GETOPT_HELP_CHAR = CHAR_MIN - 2,
	GETOPT_VERSION_CHAR = CHAR_MIN - 3
};

enum scale_type
{
	scale_none,
	scale_SI,
	scale_IEC,
	scale_IEC_I
};

const char *const scale_to_args[] = {"none", "si", "iec", "iec-i"};

enum scale_type const scale_to_types[] = {scale_none, scale_SI, scale_IEC, scale_IEC_I};

const char *const suffix_power_char[] = {"", "K", "M", "G", "T", "P", "E", "Z", "Y", "R", "Q"};

const char *const roman[][13] = {
	{"I", "IV", "V", "IX", "X", "XL", "L", "XC", "C", "CD", "D", "CM", "M"}, // ASCII
	{"Ⅰ", "ⅠⅤ", "Ⅴ", "ⅠⅩ", "Ⅹ", "ⅩⅬ", "Ⅼ", "ⅩⅭ", "Ⅽ", "ⅭⅮ", "Ⅾ", "ⅭⅯ", "Ⅿ"}	 // Unicode
};

const short romanvalues[] = {1, 4, 5, 9, 10, 40, 50, 90, 100, 400, 500, 900, 1000};

const char *const greek[][36] = {
	{"α", "β", "γ", "δ", "ε", "ϛ", "ζ", "η", "θ", "ι", "κ", "λ", "μ", "ν", "ξ", "ο", "π", "ϟ", "ρ", "σ", "τ", "υ", "φ", "χ", "ψ", "ω", "ϡ", "α", "β", "γ", "δ", "ε", "ϛ", "ζ", "η", "θ"}, // Unicode lowercase
	{"Α", "Β", "Γ", "Δ", "Ε", "Ϛ", "Ζ", "Η", "Θ", "Ι", "Κ", "Λ", "Μ", "Ν", "Ξ", "Ο", "Π", "Ϟ", "Ρ", "Σ", "Τ", "Υ", "Φ", "Χ", "Ψ", "Ω", "Ϡ", "Α", "Β", "Γ", "Δ", "Ε", "Ϛ", "Ζ", "Η", "Θ"}  // Unicode uppercase
};

const short greekvalues[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000};

const string ONES[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
const string TEENS[] = {"", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
const string TENS[] = {"", "ten", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
// https://en.wikipedia.org/wiki/Names_of_large_numbers
const string THOUSANDPOWERS[] = {"", "thousand", "m", "b", "tr", "quadr", "quint", "sext", "sept", "oct", "non"};
const string THOUSANDONES[] = {"", "un", "duo", "tre", "quattuor", "quin", "se", "septe", "octo", "nove"};
const string THOUSANDTENS[] = {"", "dec", "vigint", "trigint", "quadragint", "quinquagint", "sexagint", "septuagint", "octogint", "nonagint"};
const string THOUSANDHUNDREDS[] = {"", "cent", "ducent", "trecent", "quadringent", "quingent", "sescent", "septingent", "octingent", "nongent"};

const string HEXONES[] = {ONES[0], ONES[1], ONES[2], ONES[3], ONES[4], ONES[5], ONES[6], ONES[7], ONES[8], ONES[9], "ann", "bet", "chris", "dot", "ernest", "frost"};
const string HEXTEENS[] = {TEENS[0], TEENS[1], TEENS[2], TEENS[3], TEENS[4], TEENS[5], TEENS[6], TEENS[7], TEENS[8], TEENS[9], "annteen", "betteen", "christeen", "dotteen", "ernesteen", "frosteen"};
const string HEXTENS[] = {TENS[0], TENS[1], TENS[2], TENS[3], TENS[4], TENS[5], TENS[6], TENS[7], TENS[8], TENS[9], "annty", "betty", "christy", "dotty", "ernesty", "frosty"};

const char *const morsecode[][11] = {
	// 0-9, -
	{"- - - - -", ". - - - -", ". . - - -", ". . . - -", ". . . . -", ". . . . .", "- . . . .", "- - . . .", "- - - . .", "- - - - .", "- . . . . -"},																				   // ASCII
	{"− − − − −", "• − − − −", "• • − − −", "• • • − −", "• • • • −", "• • • • •", "− • • • •", "− − • • •", "− − − • •", "− − − − •", "− • • • • −"},																				   // Bullet and minus sign
	{"– – – – –", "· – – – –", "· · – – –", "· · · – –", "· · · · –", "· · · · ·", "– · · · ·", "– – · · ·", "– – – · ·", "– – – – ·", "– · · · · –"},																				   // Middle dot and en dash
	{"▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄ ▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄ ▄ ▄ ▄▄▄▄", "▄ ▄ ▄ ▄ ▄", "▄▄▄▄ ▄ ▄ ▄ ▄", "▄▄▄▄ ▄▄▄▄ ▄ ▄ ▄", "▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄ ▄", "▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄", "▄▄▄▄ ▄ ▄ ▄ ▄ ▄▄▄▄"} // Blocks
};

const char *const gap = "   ";

const char *const braille[] = {"⠀", "⠁", "⠂", "⠃", "⠄", "⠅", "⠆", "⠇", "⠈", "⠉", "⠊", "⠋", "⠌", "⠍", "⠎", "⠏", "⠐", "⠑", "⠒", "⠓", "⠔", "⠕", "⠖", "⠗", "⠘", "⠙", "⠚", "⠛", "⠜", "⠝", "⠞", "⠟", "⠠", "⠡", "⠢", "⠣", "⠤", "⠥", "⠦", "⠧", "⠨", "⠩", "⠪", "⠫", "⠬", "⠭", "⠮", "⠯", "⠰", "⠱", "⠲", "⠳", "⠴", "⠵", "⠶", "⠷", "⠸", "⠹", "⠺", "⠻", "⠼", "⠽", "⠾", "⠿"};

const short brailleindexes[] = {26, 1, 3, 9, 25, 17, 11, 27, 19, 10}; // 0-9

const char *const exponents[] = {"⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹", "⁻"}; // 0-9, -

const char *const fractions[] = {"¼", "½", "¾", "⅐", "⅑", "⅒", "⅓", "⅔", "⅕", "⅖", "⅗", "⅘", "⅙", "⅚", "⅛", "⅜", "⅝", "⅞"};
const long double fractionvalues[] = {1.0L / 4.0L, 1.0L / 2.0L, 3.0L / 4.0L, 1.0L / 7.0L, 1.0L / 9.0L, 1.0L / 10.0L, 1.0L / 3.0L, 2.0L / 3.0L, 1.0L / 5.0L, 2.0L / 5.0L, 3.0L / 5.0L, 4.0L / 5.0L, 1.0L / 6.0L, 5.0L / 6.0L, 1.0L / 8.0L, 3.0L / 8.0L, 5.0L / 8.0L, 7.0L / 8.0L};

const regex re("^.*: ");

const char *const constants[] = {"π", "e"};
const long double constantvalues[] = {M_PI, M_E};

constexpr unsigned __int128 UINT128_MAX = numeric_limits<unsigned __int128>::max();
constexpr __int128 INT128_MAX = numeric_limits<__int128>::max();
constexpr __int128 INT128_MIN = numeric_limits<__int128>::min();

const long double max_bit = scalbn(1.0L, LDBL_MANT_DIG - 1);
const long double MAX = max_bit + (max_bit - 1);

/* debugging for developers.  Enables devmsg().
   This flag is used only in the GMP code.  */
bool dev_debug = false;

/* Prove primality or run probabilistic tests.  */
bool flag_prove_primality = true;

/* Number of Miller-Rabin tests to run when not proving primality.  */
constexpr int MR_REPS = 25;

template <typename T>
using T2 = typename conditional<is_integral_v<T>, make_unsigned<T>, common_type<T>>::type::type;

#ifndef FACTOR
template <size_t N>
constexpr auto primes()
{
	constexpr size_t limit = !(N & 1) ? N - 1 : N;
	constexpr size_t size = (limit - 1) / 2;
	constexpr auto sieve = [&size]() constexpr -> auto
	{
		array<bool, size> sieve{};
		// sieve.fill(true);
		// bitset<size> sieve{};
		// sieve.set();
		for (auto &x : sieve)
			x = true;

		for (size_t i = 0; i * i <= size; ++i)
		{
			if (sieve[i])
			{
				const size_t p = 3 + 2 * i;

				for (size_t j = (p * p - 3) / 2; j < size; j += p)
					sieve[j] = false;
			}
		}

		return sieve;
	}();

	// constexpr size_t nprimes = sieve.count();
	constexpr size_t nprimes = [](const array<bool, size> &sieve) constexpr -> size_t
	// constexpr size_t nprimes = [](const bitset<size> &sieve) constexpr -> size_t
	{
		size_t nprimes = 0;

		for (const auto &x : sieve)
		{
			if (x)
				++nprimes;
		}

		return nprimes;
	}(sieve);
	array<unsigned char, nprimes> primes{};
	// array<unsigned __int128, nprimes> magic{};

	size_t p = 2;
	for (size_t i = 0, j = 0; i < size; ++i)
	{
		if (sieve[i])
		{
			const size_t ap = 3 + 2 * i;
			primes[j] = ap - p;
			// magic[j] = UINT128_MAX / ap + 1;
			++j;
			p = ap;
		}
	}

	p = limit;
	bool is_prime = false;
	do
	{
		p += 2;
		is_prime = true;
		for (size_t i = 0, ap = 2; is_prime and ap * ap <= p; ap += primes[i], ++i)
		{
			// if (magic[i] * p < magic[i])
			if (!(p % ap))
			{
				is_prime = false;
				break;
			}
		}
	} while (!is_prime);

	// return tuple{primes, magic, p};
	return tuple{primes, p};
}

// constexpr auto [primes_diff, FIRST_OMITTED_PRIME] = primes<1 << 16>(); // 2^16 = 65536
// constexpr auto temp = primes<5000>();
// constexpr auto temp = primes<50000>();
constexpr auto temp = primes<1 << 16>(); // 2^16 = 65536
// constexpr auto temp = primes<500000>();
constexpr auto &primes_diff = get<0>(temp);
constexpr auto &FIRST_OMITTED_PRIME = get<1>(temp);

constexpr size_t PRIMES_PTAB_ENTRIES = size(primes_diff);
// static_assert(PRIMES_PTAB_ENTRIES == 669 - 1);
// static_assert(FIRST_OMITTED_PRIME == 5003); // 4999;
// static_assert(PRIMES_PTAB_ENTRIES == 5133 - 1);
// static_assert(FIRST_OMITTED_PRIME == 50021); // 49999
static_assert(PRIMES_PTAB_ENTRIES == 6542 - 1);
static_assert(FIRST_OMITTED_PRIME == 65537); // 65521
// static_assert(PRIMES_PTAB_ENTRIES == 41538 - 1);
// static_assert(FIRST_OMITTED_PRIME == 500009); // 499979

#if HAVE_GMP
template <typename T>
mpz_class import(const T &value)
{
	mpz_class result;
	mpz_import(result.get_mpz_t(), 1, -1, sizeof(value), 0, 0, &value);

	return result;
}

template <typename T>
T aexport(const mpz_class &value)
{
	T result;
	mpz_export(&result, 0, -1, sizeof(result), 0, 0, value.get_mpz_t());

	return result;
}

// const mpz_class aINT128_MAX = (mpz_class(1) << 127) - 1;
const mpz_class aINT128_MAX = import(INT128_MAX);
#endif
#endif

/* Number of bits in an uintmax_t.  */
constexpr size_t W = sizeof(uintmax_t) * CHAR_BIT;

/* Verify that uintmax_t does not have holes in its representation.  */
static_assert(UINTMAX_MAX >> (W - 1) == 1);

/* Number of bits in an unsigned __int128.  */
constexpr size_t X = sizeof(unsigned __int128) * CHAR_BIT;

/* Verify that unsigned __int128 does not have holes in its representation.  */
static_assert(UINT128_MAX >> (X - 1) == 1);

__int128 strtoi128(const char *nptr, char **endptr, int base)
{
	const char *s = nptr;
	int c;
	bool neg = false;

	do
	{
		c = *s++;
	} while (isspace(c));
	if (c == '-')
	{
		neg = true;
		c = *s++;
	}
	else if (c == '+')
		c = *s++;
	if ((base == 0 or base == 16) and c == '0' and (*s == 'x' or *s == 'X'))
	{
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	unsigned __int128 cutoff = neg ? -(unsigned __int128)INT128_MIN : INT128_MAX;
	const int cutlim = cutoff % base;
	cutoff /= base;
	unsigned __int128 acc;
	int any;
	for (acc = 0, any = 0;; c = *s++)
	{
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 or acc > cutoff or (acc == cutoff and c > cutlim))
			any = -1;
		else
		{
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0)
	{
		acc = neg ? INT128_MIN : INT128_MAX;
		errno = ERANGE;
	}
	else if (neg)
		acc = -acc;
	if (endptr != NULL)
		*endptr = (char *)(any ? s - 1 : nptr);
	return acc;
}

unsigned __int128 strtou128(const char *nptr, char **endptr, int base)
{
	const char *s = nptr;
	int c;
	bool neg = false;

	do
	{
		c = *s++;
	} while (isspace(c));
	if (c == '-')
	{
		neg = true;
		c = *s++;
	}
	else if (c == '+')
		c = *s++;
	if ((base == 0 or base == 16) and c == '0' and (*s == 'x' or *s == 'X'))
	{
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	const unsigned __int128 cutoff = UINT128_MAX / base;
	const int cutlim = UINT128_MAX % base;
	unsigned __int128 acc;
	int any;
	for (acc = 0, any = 0;; c = *s++)
	{
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 or acc > cutoff or (acc == cutoff and c > cutlim))
			any = -1;
		else
		{
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0)
	{
		acc = UINT128_MAX;
		errno = ERANGE;
	}
	else if (neg)
		acc = -acc;
	if (endptr != NULL)
		*endptr = (char *)(any ? s - 1 : nptr);
	return acc;
}

// Check if the argument is in the argument list
// Adapted from: https://github.com/coreutils/gnulib/blob/master/lib/argmatch.c
template <typename T>
T xargmatch(const char *const context, const char *const arg, const char *const *arglist, const size_t argsize, const T vallist[])
{
	const size_t arglen = strlen(arg);

	for (size_t i = 0; i < argsize; ++i)
		if (!strncmp(arglist[i], arg, arglen) and strlen(arglist[i]) == arglen)
			return vallist[i];

	cerr << "Error: Invalid argument " << quoted(arg) << " for " << quoted(context) << "\n";
	exit(1);

	// return -1;
}

// Auto-scale number to unit
// Adapted from: https://github.com/coreutils/coreutils/blob/master/src/numfmt.c
string outputunit(long double number, const scale_type scale, const bool all = false)
{
	ostringstream strm;

	unsigned x = 0;
	long double val = number;
	if (val >= -LDBL_MAX and val <= LDBL_MAX)
	{
		while (abs(val) >= 10)
		{
			++x;
			val /= 10;
		}
	}

	if (scale == scale_none)
	{
		if (x > LDBL_DIG)
		{
			cerr << "Error: Number too large to be printed: '" << number << "' (consider using --to)\n";
			return {};
		}

		strm << setprecision(LDBL_DIG) << number;
		return strm.str();
	}

	if (x > 33 - 1)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number too large to be printed: '" << number << "' (cannot handle numbers > 999Q)\n";
		return {};
	}

	double scale_base;

	switch (scale)
	{
	case scale_IEC:
	case scale_IEC_I:
		scale_base = 1024;
		break;
	case scale_none:
	case scale_SI:
	default:
		scale_base = 1000;
		break;
	}

	unsigned power = 0;
	if (number >= -LDBL_MAX and number <= LDBL_MAX)
	{
		while (abs(number) >= scale_base)
		{
			++power;
			number /= scale_base;
		}
	}

	long double anumber = abs(number);
	anumber += anumber < 10 ? 0.0005 : anumber < 100 ? 0.005
								   : anumber < 1000	 ? 0.05
													 : 0.5;

	string str;

	if (number and anumber < 1000 and power > 0)
	{
		strm << setprecision(LDBL_DIG) << number;
		str = strm.str();

		const unsigned length = 5 + (number < 0 ? 1 : 0);
		if (str.length() > length)
		{
			const int prec = anumber < 10 ? 3 : anumber < 100 ? 2
															  : 1;
			strm.str("");
			strm << setprecision(prec) << fixed << number;
			str = strm.str();
		}
	}
	else
	{
		strm << setprecision(0) << fixed << number;
		str = strm.str();
	}

	str += power < size(suffix_power_char) ? suffix_power_char[power] : "(error)";

	if (scale == scale_IEC_I and power > 0)
		str += "i";

	return str;
}

// Output number in bases 2 - 36
template <typename T>
string outputbase(const T number, const short base = 10, const bool uppercase = false)
{
	if (base < 2 or base > 36)
	{
		cerr << "Error: <BASE> must be 2 - 36.\n";
		exit(1);
	}

	// T2<T> anumber = abs(number);
	T2<T> anumber = number;
	anumber = number < 0 ? -anumber : anumber;

	string str;

	do
	{
		char digit = anumber % base;

		digit += digit < 10 ? '0' : (uppercase ? 'A' : 'a') - 10;

		str = digit + str;

		anumber /= base;

	} while (anumber > 0);

	if (number < 0)
		str = '-' + str;

	return str;
}

// Output numbers 1 - 3999 as Roman numerals
template <typename T>
string outputroman(const T number, const bool unicode, const bool all = false)
{
	// T2<T> anumber = abs(number);
	T2<T> anumber = number;
	anumber = number < 0 ? -anumber : anumber;

	if (anumber < 1 or anumber > 3999)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number must be between 1 - 3999\n";
		return {};
	}

	string str;

	if (number < 0)
		str = '-';

	for (int i = size(romanvalues) - 1; anumber > 0; --i)
	{
		T2<T> div = anumber / romanvalues[i];
		if (div)
		{
			anumber %= romanvalues[i];
			while (div--)
				str += roman[unicode][i];
		}
	}

	return str;
}

// Output numbers 1 - 9999 as Greek numerals
template <typename T>
string outputgreek(const T number, const bool uppercase, const bool all = false)
{
	// T2<T> anumber = abs(number);
	T2<T> anumber = number;
	anumber = number < 0 ? -anumber : anumber;

	if (anumber < 1 or anumber > 9999)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number must be between 1 - 9999\n";
		return {};
	}

	string str;

	if (number < 0)
		str = '-';

	for (int i = size(greekvalues) - 1; anumber > 0; --i)
	{
		if (anumber / greekvalues[i])
		{
			anumber %= greekvalues[i];
			if (greekvalues[i] >= 1000)
				str += "͵"; // lower left keraia
			str += greek[uppercase][i];
			if (greekvalues[i] < 1000 and !anumber)
				str += "ʹ"; // keraia
		}
	}

	return str;
}

// Convert number to string
template <typename T>
string tostring(T arg)
{
	if constexpr (is_same_v<T, __int128> or is_same_v<T, unsigned __int128>)
		return outputbase(arg);
	else
	{
		ostringstream strm;
		strm << arg;
		return strm.str();
	}
}

// Output number as Morse code
template <typename T>
string outputmorsecode(const T &number, const unsigned style)
{
	// const T2<T> n = abs(number);
	T2<T> n = number;
	n = number < 0 ? -n : n;

	string str;
	const string text = tostring(n);

	if (number < 0)
	{
		str = morsecode[style][10];
		str += gap;
	}

	for (size_t i = 0; i < text.length(); ++i)
	{
		if (i)
			str += gap;
		str += morsecode[style][text[i] - '0'];
	}

	return str;
}

// Output number as Braille
template <typename T>
string outputbraille(const T &number)
{
	// const T2<T> n = abs(number);
	T2<T> n = number;
	n = number < 0 ? -n : n;

	string str;
	const string text = tostring(n);

	if (number < 0)
	{
		str = braille[16];
		str += braille[36];
	}

	str += braille[60]; // Number indicator

	for (char i : text)
		str += braille[brailleindexes[i - '0']];

	return str;
}

// Output number as exponent
string outputexponent(const intmax_t number)
{
	const short base = 10;
	// uintmax_t anumber = abs(number);
	uintmax_t anumber = number;
	anumber = number < 0 ? -anumber : anumber;
	string str;

	do
	{
		str = exponents[anumber % base] + str;

		anumber /= base;

	} while (anumber > 0);

	if (number < 0)
		str = exponents[10] + str;

	return str;
}

string thousandpower(size_t power)
{
	string str;

	if (power < size(THOUSANDPOWERS))
	{
		str += THOUSANDPOWERS[power];
		if (power > 1)
			str += "illion";
		return str;
	}

	--power;

	const unsigned scale = 1000;
	while (power > 0)
	{
		string astr;

		unsigned m = power % scale;
		power /= scale;
		if (m)
		{
			const unsigned h = m / 100;
			const unsigned t = (m % 100) / 10;
			const unsigned u = m % 10;

			if (u)
				astr += m >= 10 ? THOUSANDONES[u] : THOUSANDPOWERS[u + 1] + 'i';
			if (u and t)
			{
				if ((u == 3 or u == 6) and t >= 2 and t <= 5)
					astr += 's';
				else if (u == 7 or u == 9)
				{
					if (t == 1 or (t >= 3 and t <= 7))
						astr += 'n';
					else if (t == 2 or t == 8)
						astr += 'm';
				}
				else if (u == 6 and t == 8)
					astr += 'x';
			}
			if (t)
				astr += THOUSANDTENS[t] + (t >= 3 and h ? 'a' : 'i');
			else if (u and h)
			{
				if ((u == 3 or u == 6) and h >= 3 and h <= 5)
					astr += 's';
				else if (u == 7 or u == 9)
				{
					if (h >= 1 and h <= 7)
						astr += 'n';
					else if (h == 8)
						astr += 'm';
				}
				else if (u == 6 and (h == 1 or h == 8))
					astr += 'x';
			}
			if (h)
				astr += THOUSANDHUNDREDS[h] + 'i';
			astr += "lli";
		}
		else
			astr = "nilli";

		str = astr + str;
	}

	str += "on";
	return str;
}

// Output number as text
// Adapted from: https://rosettacode.org/wiki/Number_names
template <typename T>
string outputtext(const T &number, const bool special)
{
	// T2<T> n = abs(number);
	T2<T> n = number;
	n = number < 0 ? -n : n;

	string str;

	if (number < 0)
		// str = '-';
		str = "negative ";
	if (special and n <= 12 * 12 * 12)
	{
		if (n == 2)
		{
			str += "pair";
			return str;
		}
		if (n == 13)
		{
			str += "baker's dozen";
			return str;
		}
		if (n == 20)
		{
			str += "score";
			return str;
		}
		if (n % 12 == 0)
		{
			T temp = n / 12;
			if (temp >= 1 and temp < 12)
			{
				if (temp > 1)
					str += outputtext(temp, false) + ' ';

				str += "dozen";
				return str;
			}
			if (temp % 12 == 0)
			{
				temp /= 12;
				if (temp >= 1 and temp < 12)
				{
					if (temp > 1)
						str += outputtext(temp, false) + ' ';

					str += "gross";
					return str;
				}
				if (temp == 12)
				{
					str += "great gross";
					return str;
				}
			}
		}
	}
	if (n < 10)
	{
		if constexpr (!is_integral_v<T>)
			str += ONES[n.get_ui()];
		else
			str += ONES[n];
		return str;
	}

	string astr;

	const unsigned scale = 1000;
	for (size_t index = 0; n > 0; ++index)
	{
		unsigned h;
		if constexpr (!is_integral_v<T>)
			h = mpz_class(n % scale).get_ui();
		else
			h = n % scale;
		n /= scale;
		if (h)
		{
			string aastr;
			if (n)
				// aastr += ' ';
				aastr += astr.empty() and (h < 100 or !(h % 100)) ? " and " : ", ";
			if (h >= 100)
			{
				aastr += ONES[h / 100] + " hundred";
				h %= 100;
				if (h)
					// aastr += ' ';
					aastr += " and ";
			}
			if (h >= 20 or h == 10)
			{
				aastr += TENS[h / 10];
				h %= 10;
				if (h)
					aastr += '-';
			}
			if (h < 20 and h > 10)
				aastr += TEENS[h - 10];
			else if (h < 10 and h > 0)
				aastr += ONES[h];
			if (index)
				aastr += ' ' + thousandpower(index);
			astr = aastr + astr;
		}
	}

	str += astr;
	return str;
}

// Output hexadecimal number as text
template <typename T>
string outputhextext(const T &number)
{
	// T2<T> n = abs(number);
	T2<T> n = number;
	n = number < 0 ? -n : n;

	string str;

	const unsigned scale = 0x100;
	do
	{
		unsigned h;
		if constexpr (!is_integral_v<T>)
			h = mpz_class(n % scale).get_ui();
		else
			h = n % scale;
		n /= scale;
		string astr;
		if (n)
			astr += ' ';
		if (h >= 0x20 or h == 0x10)
		{
			astr += HEXTENS[h / 0x10];
			h %= 0x10;
			if (h)
				astr += '-';
		}
		if (h < 0x20 and h > 0x10)
			astr += HEXTEENS[h - 0x10];
		else if (h < 0x10 and (h > 0x0 or astr.length() <= 1))
		{
			if (n and astr.length() == 1)
				astr += "oh-";
			astr += HEXONES[h];
		}
		str = astr + str;
	} while (n > 0);

	if (number < 0)
		// str = '-' + str;
		str = "negative " + str;

	return str;
}

// Execute command
int exec(const char *const cmd, string &result)
{
	FILE *pipe = popen(cmd, "r");
	if (!pipe)
		throw runtime_error("popen() failed!");
	try
	{
		char buffer[128];
		while (fgets(buffer, sizeof(buffer), pipe))
		{
			result += buffer;
		}
		if (result.length() > 0)
			result.erase(result.length() - 1);
	}
	catch (...)
	{
		pclose(pipe);
		throw;
	}
	return pclose(pipe);
}

#ifndef FACTOR
template <typename T>
constexpr T diff(const T a, const T b)
{
	return a >= b ? a - b : b - a;
}

template <typename T>
constexpr T mulm(T a, T b, const T mod)
{
	static_assert(is_integral_v<T>, "");
	// assert(mod>0);
	// assert(a<mod);
	// assert(b<mod);
	T res = 0;

	for (; b != 0; b >>= 1)
	{
		if ((b & 1) != 0)
			res = (res + a) % mod;

		a = (a << 1) % mod;
	}

	return res;
}

template <typename T>
constexpr T powm(T base, T exp, const T mod)
{
	static_assert(is_integral_v<T>, "");
	// assert(mod>1);
	T res = 1;

	for (; exp != 0; exp >>= 1)
	{
		if ((exp & 1) != 0)
			// res = (res * base) % mod;
			res = mulm(res, base, mod);

		// base = (base * base) % mod;
		base = mulm(base, base, mod);
	}

	return res;
}

template <typename T1, typename T2>
void factor(T1 &t, map<T2, size_t> &factors);

template <typename T1, typename T2>
void factor_using_division(T1 &t, map<T2, size_t> &factors)
{
	if (dev_debug)
		cerr << "[trial division] ";

	size_t p = 0;
#if HAVE_GMP
	if constexpr (!is_integral_v<T1>)
	{
		p = mpz_scan1(t.get_mpz_t(), 0);
		mpz_fdiv_q_2exp(t.get_mpz_t(), t.get_mpz_t(), p);
	}
	else
#endif
	{
		// p = __builtin_ctz(t);
		p = __countr_zero(t);
		t >>= p;
	}
	if (p)
		factors[2] += p;

	p = 3;
	for (size_t i = 1; i <= PRIMES_PTAB_ENTRIES;)
	{
		if (t % p != 0)
		{
			if (i < PRIMES_PTAB_ENTRIES)
				p += primes_diff[i];
			++i;
			if (t < p * p)
				break;
		}
		else
		{
			// mpz_tdiv_q_ui(t.get_mpz_t(), t.get_mpz_t(), p);
			t /= p;
			++factors[p];
		}
	}
}

template <typename T>
bool millerrabin(const T &n, const T &nm1, const T &x, T &y, const T &q, const size_t k)
{
#if HAVE_GMP
	if constexpr (!is_integral_v<T>)
		mpz_powm(y.get_mpz_t(), x.get_mpz_t(), q.get_mpz_t(), n.get_mpz_t());
	else
#endif
		y = powm(x, q, n);

	if (y == 1 or y == nm1)
		return true;

	for (size_t i = 1; i < k; ++i)
	{
#if HAVE_GMP
		if constexpr (!is_integral_v<T>)
			mpz_powm_ui(y.get_mpz_t(), y.get_mpz_t(), 2, n.get_mpz_t());
		else
#endif
			// y = powm(y, T(2), n);
			y = mulm(y, y, n);

		if (y == nm1)
			return true;
		if (y == 1)
			return false;
	}
	return false;
}

template <typename T>
bool prime_p(const T &n)
{
	bool is_prime;
	T tmp = 0;
	map<T, size_t> factors;

	if constexpr (!is_integral_v<T>)
	{
		// n.fits_ulong_p()
		if (n.fits_slong_p())
		{
			const unsigned long an = n.get_ui();
			return prime_p(an);
		}
#if HAVE_GMP
		if constexpr (INTMAX_MAX > LONG_MAX and n <= INTMAX_MAX)
		{
			const uintmax_t an = aexport<uintmax_t>(n);
			return prime_p(an);
		}
		if (n <= aINT128_MAX)
		{
			const unsigned __int128 an = aexport<unsigned __int128>(n);
			return prime_p(an);
		}
#endif
	}
	else if constexpr (is_same_v<T, unsigned __int128>)
	{
		if (n <= INTMAX_MAX)
		{
			const uintmax_t an = n;
			return prime_p(an);
		}
	}

	if (n <= 1)
		return false;

	/* We have already casted out small primes.  */
	if (n < FIRST_OMITTED_PRIME * FIRST_OMITTED_PRIME)
		return true;

	/* Precomputation for Miller-Rabin.  */
	const T nm1 = n - 1;

	/* Find q and k, where q is odd and n = 1 + 2**k * q.  */
	size_t k = 0;
	T q = nm1;
#if HAVE_GMP
	if constexpr (!is_integral_v<T>)
	{
		k = mpz_scan1(nm1.get_mpz_t(), 0);
		mpz_tdiv_q_2exp(q.get_mpz_t(), nm1.get_mpz_t(), k);
	}
	else
#endif
	{
		// k = __builtin_ctz(q);
		k = __countr_zero(q);
		q >>= k;
	}

	T a = 2;

	/* Perform a Miller-Rabin test, finds most composites quickly.  */
	if (!millerrabin(n, nm1, a, tmp, q, k))
		return false;

	if (flag_prove_primality)
	{
		/* Factor n-1 for Lucas.  */
		tmp = nm1;
		if constexpr (!is_integral_v<T>)
		{
			// tmp.fits_ulong_p()
			if (tmp.fits_slong_p())
			{
				unsigned long atmp = tmp.get_ui();
				factor(atmp, factors);
				tmp = atmp;
			}
#if HAVE_GMP
			else if constexpr (INTMAX_MAX > LONG_MAX and tmp <= INTMAX_MAX)
			{
				uintmax_t atmp = aexport<uintmax_t>(tmp);
				factor(atmp, factors);
				tmp = import(atmp);
			}
			else if (tmp <= aINT128_MAX)
			{
				unsigned __int128 atmp = aexport<unsigned __int128>(tmp);
				factor(atmp, factors);
				tmp = import(atmp);
			}
#endif
			else
				factor(tmp, factors);
		}
		else if constexpr (is_same_v<T, unsigned __int128>)
		{
			if (tmp <= INTMAX_MAX)
			{
				uintmax_t atmp = tmp;
				factor(atmp, factors);
				tmp = atmp;
			}
			else
				factor(tmp, factors);
		}
		else
			factor(tmp, factors);
	}

	/* Loop until Lucas proves our number prime, or Miller-Rabin proves our
	   number composite.  */
	for (size_t r = 0; r < PRIMES_PTAB_ENTRIES; ++r)
	{
		if (flag_prove_primality)
		{
			is_prime = true;
			for (const auto &[p, e] : factors)
			{
#if HAVE_GMP
				if constexpr (!is_integral_v<T>)
					mpz_powm(tmp.get_mpz_t(), a.get_mpz_t(), T(nm1 / p).get_mpz_t(), n.get_mpz_t());
				else
#endif
					tmp = powm(a, nm1 / p, n);
				is_prime = tmp != 1;

				if (!is_prime)
					break;
			}
		}
		else
		{
			/* After enough Miller-Rabin runs, be content.  */
			is_prime = (r == MR_REPS - 1);
		}

		if (is_prime)
			return is_prime;

		a += primes_diff[r]; /* Establish new base.  */

		if (!millerrabin(n, nm1, a, tmp, q, k))
			return false;
	}

	cerr << "Lucas prime test failure.  This should not happen\n";
	abort();
}

template <typename T1, typename T2>
void factor_using_pollard_rho(T1 &n, size_t a, map<T2, size_t> &factors)
{
	T1 x = 2, z = 2, y = 2, P = 1;
	T1 t = 0;

	if (dev_debug)
		cerr << "[pollard-rho (" << a << ")] ";

	unsigned long long k = 1;
	unsigned long long l = 1;

	while (n != 1)
	{
		// assert(a < n);
		while (true)
		{
			bool factor_found = false;
			do
			{
				if constexpr (!is_integral_v<T1>)
					x = ((x * x) % n) + a;
				else
					x = mulm(x, x, n) + a;

				if constexpr (!is_integral_v<T1>)
					P = (P * (z - x)) % n;
				else
					P = mulm(P, diff(z, x), n);

				if (k % 32 == 1)
				{
					if (gcd(P, n) != 1)
					{
						factor_found = true;
						break;
					}
					y = x;
				}
			} while (--k != 0);

			if (factor_found)
				break;

			z = x;
			k = l;
			l *= 2;
			for (unsigned long long i = 0; i < k; ++i)
			{
				if constexpr (!is_integral_v<T1>)
					x = ((x * x) % n) + a;
				else
					x = mulm(x, x, n) + a;
			}

			y = x;
		}

		do
		{
			if constexpr (!is_integral_v<T1>)
				y = ((y * y) % n) + a;
			else
				y = mulm(y, y, n) + a;

			if constexpr (!is_integral_v<T1>)
				t = gcd(z - y, n);
			else
				t = gcd(diff(z, y), n);
		} while (t == 1);

		n /= t; /* divide by t, before t is overwritten */

		if (!prime_p(t))
		{
			if (dev_debug)
				cerr << "[composite factor--restarting pollard-rho] ";
			if constexpr (!is_integral_v<T1>)
			{
				// t.fits_ulong_p()
				if (t.fits_slong_p())
				{
					unsigned long at = t.get_ui();
					factor_using_pollard_rho(at, a + 1, factors);
					t = at;
				}
#if HAVE_GMP
				else if constexpr (INTMAX_MAX > LONG_MAX and t <= INTMAX_MAX)
				{
					uintmax_t at = aexport<uintmax_t>(t);
					factor_using_pollard_rho(at, a + 1, factors);
					t = import(at);
				}
				else if (t <= aINT128_MAX)
				{
					unsigned __int128 at = aexport<unsigned __int128>(t);
					factor_using_pollard_rho(at, a + 1, factors);
					t = import(at);
				}
#endif
				else
					factor_using_pollard_rho(t, a + 1, factors);
			}
			else if constexpr (is_same_v<T1, unsigned __int128>)
			{
				if (t <= INTMAX_MAX)
				{
					uintmax_t at = t;
					factor_using_pollard_rho(at, a + 1, factors);
					t = at;
				}
				else
					factor_using_pollard_rho(t, a + 1, factors);
			}
			else
				factor_using_pollard_rho(t, a + 1, factors);
		}
		else
		{
#if HAVE_GMP
			if constexpr (!is_integral_v<T2> and is_same_v<T1, unsigned __int128>)
				++factors[import(t)];
			else
#endif
				++factors[t];
		}

		if (prime_p(n))
		{
#if HAVE_GMP
			if constexpr (!is_integral_v<T2> and is_same_v<T1, unsigned __int128>)
				++factors[import(n)];
			else
#endif
				++factors[n];
			break;
		}

		x %= n;
		z %= n;
		y %= n;
	}
}

/* Use Pollard-rho to compute the prime factors of
   arbitrary-precision T, and put the results in FACTORS.  */
template <typename T1, typename T2>
void factor(T1 &t, map<T2, size_t> &factors)
{
	if (t != 0)
	{
		// assert(t >= 2);
		factor_using_division(t, factors);

		if (t != 1)
		{
			// assert(t >= 2);
			if (dev_debug)
				cerr << "[is number prime?] ";
			if (prime_p(t))
			{
#if HAVE_GMP
				if constexpr (!is_integral_v<T2> and is_same_v<T1, unsigned __int128>)
					++factors[import(t)];
				else
#endif
					++factors[t];
			}
			else
				factor_using_pollard_rho(t, 1, factors);
		}
	}
}
#else
// Execute factor command to get prime factors of number
template <typename T>
void factor(const T &number, map<T2<T>, size_t> &counts)
{
	const string cmd = string(FACTOR) + R"( ")" + tostring(number) + R"(" 2>&1)";

	string result;
	if (exec(cmd.c_str(), result))
	{
		cerr << "Error: " << result /*  << "\n" */;
	}

	result = regex_replace(result, re, "");

	istringstream strm(result);

	T2<T> temp;
	if constexpr (is_same_v<T2<T>, unsigned __int128>)
	{
		string token;
		while (strm >> token)
		{
			temp = strtou128(token.c_str(), NULL, 10);
			++counts[temp];
		}
	}
	else
	{
		while (strm >> temp)
			++counts[temp];
	}
}
#endif

// Output prime factors of number
template <typename T>
string outputfactors(const T &number, const bool print_exponents, const bool unicode, const bool all = false)
{
	if (number < 1)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number must be > 0\n";
		return {};
	}

	// const T2<T> n = abs(number);
	T2<T> n = number;
	// n = number < 0 ? -n : n;

	ostringstream strm;
	map<T2<T>, size_t> counts;
	factor(n, counts);

	for (const auto &[prime, exponent] : counts)
	{
		for (size_t j = 0; j < exponent; ++j)
		{
			if (strm.tellp())
				strm << ' ' << (unicode ? "×" : "*") << ' ';
			// strm << ' ';
			if constexpr (is_same_v<T2<T>, unsigned __int128>)
				strm << outputbase(prime);
			else
				strm << prime;
			if (print_exponents and exponent > 1)
			{
				if (unicode)
					strm << outputexponent(exponent);
				else
					strm << '^' << exponent;
				break;
			}
		}
	}

	return strm.str();
}

// Get divisors of number
template <typename T>
vector<T2<T>> divisor(T number)
{
	map<T2<T>, size_t> counts;
	factor(number, counts);
	vector<T2<T>> divisors{1};

	for (const auto &[prime, exponent] : counts)
	{
		const size_t count = divisors.size();
		T2<T> multiplier = 1;
		for (size_t j = 0; j < exponent; ++j)
		{
			multiplier *= prime;
			for (size_t i = 0; i < count; ++i)
				divisors.push_back(divisors[i] * multiplier);
		}
	}

	divisors.pop_back();
	sort(divisors.begin(), divisors.end());

	return divisors;
}

// Output divisors of number
template <typename T>
string outputdivisors(const T &number, const bool all = false)
{
	if (number < 1)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number must be > 0\n";
		return {};
	}

	ostringstream strm;
	// const T2<T> n = abs(number);
	const T2<T> &n = number;
	// n = number < 0 ? -n : n;

	vector<T2<T>> divisors = divisor(n);

	for (size_t i = 0; i < divisors.size(); ++i)
	{
		if (i)
			strm << ' ';
		if constexpr (is_same_v<T2<T>, unsigned __int128>)
			strm << outputbase(divisors[i]);
		else
			strm << divisors[i];
	}

	return strm.str();
}

// Output aliquot sum of number
template <typename T>
string outputaliquot(const T &number, const bool all = false)
{
	if (number < 2)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number must be > 1\n";
		return {};
	}

	ostringstream strm;
	// const T2<T> n = abs(number);
	const T2<T> &n = number;
	// n = number < 0 ? -n : n;

	vector<T2<T>> divisors = divisor(n);
	const T2<T> sum = accumulate(divisors.begin(), divisors.end(), T2<T>(0));

	if constexpr (is_same_v<T2<T>, unsigned __int128>)
		strm << outputbase(sum);
	else
		strm << sum;
	strm << " (";

	if (sum == n)
		strm << "Perfect!";
	else if (sum < n)
		strm << "Deficient";
	else if (sum > n)
		strm << "Abundant";

	strm << ")";

	return strm.str();
}

// Output if number is prime or composite
template <typename T>
string outputprime(const T &number, const bool all = false)
{
	if (number < 2)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number must be > 1\n";
		return {};
	}

	string str;
	// const T2<T> n = abs(number);
	const T2<T> &n = number;
	// n = number < 0 ? -n : n;

	vector<T2<T>> divisors = divisor(n);

	str = divisors.size() == 1 ? "Prime!" : "Composite (Not prime)";

	return str;
}

// Convert fractions and constants to Unicode characters
// Adapted from: https://github.com/tdulcet/Table-and-Graph-Libs/blob/master/graphs.hpp
string outputfraction(const long double number)
{
	bool output = false;

	ostringstream strm;
	strm << setprecision(LDBL_DIG);

	const long double n = abs(number);
	if (n <= MAX)
	{
		long double intpart = 0;
		long double fractionpart = abs(modf(number, &intpart));

		for (size_t i = 0; i < size(fractions) and !output; ++i)
		{
			if (abs(fractionpart - fractionvalues[i]) <= DBL_EPSILON * n)
			{
				if (intpart == 0 and number < 0)
					strm << '-';
				else if (intpart != 0)
					strm << intpart;

				strm << fractions[i];

				output = true;
			}
		}

		if (n > DBL_EPSILON)
		{
			for (size_t i = 0; i < size(constants) and !output; ++i)
			{
				if (abs(fmod(number, constantvalues[i])) <= DBL_EPSILON * n)
				{
					intpart = number / constantvalues[i];

					if (intpart == -1)
						strm << '-';
					else if (intpart != 1)
						strm << intpart;

					strm << constants[i];

					output = true;
				}
			}
		}
	}

	if (!output)
		strm << number;

	return strm.str();
}

// Output all for integer numbers
template <typename T>
void outputall(const T ll, const bool print_exponents, const bool unicode, const bool uppercase, const bool special)
{
	// cout << "\n\tLocale:\t\t\t\t";
	// printf("%'" PRIdMAX, ll);
	if constexpr (!is_same_v<T, __int128>)
	{
		ostringstream strm;
		strm.imbue(locale(""));
		strm << ll;
		cout << "\n\tLocale:\t\t\t\t" << strm.str();
	}

	/* cout << "\n\n\tC (printf)\n";
	cout << "\t\tOctal (Base 8):\t\t";
	printf("%" SCNoMAX, ll);
	cout << "\n\t\tDecimal (Base 10):\t";
	printf("%" SCNdMAX, ll);
	cout << "\n\t\tHexadecimal (Base 16):\t";
	printf("%" SCNxMAX, ll);

	cout << "\n\n\tC++ (cout)\n";
	cout << "\t\tBinary (Base 2):\t\t" << bitset<8>{ll};
	cout << "\n\t\tOctal (Base 8):\t\t" << oct << ll;
	cout << "\n\t\tDecimal (Base 10):\t" << dec << ll;
	cout << "\n\t\tHexadecimal (Base 16):\t" << hex << ll; */

	cout << "\n\n\tBinary (Base 2):\t\t" << outputbase(ll, 2, uppercase);
	cout << "\n\tTernary (Base 3):\t\t" << outputbase(ll, 3, uppercase);
	cout << "\n\tQuaternary (Base 4):\t\t" << outputbase(ll, 4, uppercase);
	cout << "\n\tQuinary (Base 6):\t\t" << outputbase(ll, 6, uppercase);
	cout << "\n\tOctal (Base 8):\t\t\t" << outputbase(ll, 8, uppercase);
	cout << "\n\tDecimal (Base 10):\t\t" << outputbase(ll, 10, uppercase);
	cout << "\n\tDuodecimal (Base 12):\t\t" << outputbase(ll, 12, uppercase);
	cout << "\n\tHexadecimal (Base 16):\t\t" << outputbase(ll, 16, uppercase);
	cout << "\n\tVigesimal (Base 20):\t\t" << outputbase(ll, 20, uppercase);
	// cout << "\n\tBase 36:\t\t\t" << outputbase(ll, 36, uppercase);

	cout << "\n";
	for (short i = 2; i <= 36; ++i)
		cout << "\n\tBase " << i << ":\t\t\t" << (i < 10 ? "\t" : "") << outputbase(ll, i, uppercase);

	cout << "\n\n\tInternational System of Units (SI):\t\t\t" << outputunit(ll, scale_SI, true);
	cout << "\n\tInternational Electrotechnical Commission (IEC):\t" << outputunit(ll, scale_IEC, true);
	cout << "\n\tInternational Electrotechnical Commission (IEC):\t" << outputunit(ll, scale_IEC_I, true);

	cout << "\n\n\tRoman Numerals:\t\t\t" << outputroman(ll, unicode, true);

	cout << "\n\n\tGreek Numerals:\t\t\t" << outputgreek(ll, uppercase, true);

	cout << "\n\n\tMorse code:\t\t\t" << outputmorsecode(ll, unicode);
	/* for (size_t i = 0; i < size(morsecode); ++i)
		cout << "\n\t\tStyle " << i << ":\t\t\t" << outputmorsecode(ll, i); */

	cout << "\n\n\tBraille:\t\t\t" << outputbraille(ll);

	cout << "\n\n\tText:\t\t\t\t" << outputtext(ll, special);

	cout << "\n\n\tPrime Factors:\t\t\t" << outputfactors(ll, print_exponents, unicode, true);
	cout << "\n\tDivisors:\t\t\t" << outputdivisors(ll, true);
	cout << "\n\tAliquot sum:\t\t\t" << outputaliquot(ll, true);
	cout << "\n\tPrime or composite:\t\t" << outputprime(ll, true) << "\n";
}

// Output all for arbitrary-precision integer numbers
#if HAVE_GMP
void outputall(const mpz_class &num, const bool print_exponents, const bool unicode, const bool uppercase, const bool special)
{
	// cout << "\n\tLocale:\t\t\t\t";
	// gmp_printf("%'Zd", num.get_mpz_t());
	ostringstream strm;
	strm.imbue(locale(""));
	strm << num;
	cout << "\n\tLocale:\t\t\t\t" << strm.str();

	/* cout << "\n\n\tC (printf)\n";
	cout << "\t\tOctal (Base 8):\t\t";
	gmp_printf("%Zo", num.get_mpz_t());
	cout << "\n\t\tDecimal (Base 10):\t";
	gmp_printf("%Zd", num.get_mpz_t());
	cout << "\n\t\tHexadecimal (Base 16):\t";
	gmp_printf("%Zx", num.get_mpz_t());

	cout << "\n\n\tC++ (cout)\n";
	cout << "\t\tBinary (Base 2):\t\t" << bitset<8>{num};
	cout << "\n\t\tOctal (Base 8):\t\t" << oct << num;
	cout << "\n\t\tDecimal (Base 10):\t" << dec << num;
	cout << "\n\t\tHexadecimal (Base 16):\t" << hex << num; */

	cout << "\n\n\tBinary (Base 2):\t\t" << num.get_str(uppercase ? -2 : 2);
	cout << "\n\tTernary (Base 3):\t\t" << num.get_str(uppercase ? -3 : 3);
	cout << "\n\tQuaternary (Base 4):\t\t" << num.get_str(uppercase ? -4 : 4);
	cout << "\n\tQuinary (Base 6):\t\t" << num.get_str(uppercase ? -6 : 6);
	cout << "\n\tOctal (Base 8):\t\t\t" << num.get_str(uppercase ? -8 : 8);
	cout << "\n\tDecimal (Base 10):\t\t" << num.get_str(uppercase ? -10 : 10);
	cout << "\n\tDuodecimal (Base 12):\t\t" << num.get_str(uppercase ? -12 : 12);
	cout << "\n\tHexadecimal (Base 16):\t\t" << num.get_str(uppercase ? -16 : 16);
	cout << "\n\tVigesimal (Base 20):\t\t" << num.get_str(uppercase ? -20 : 20);
	// cout << "\n\tBase 36:\t\t\t" << num.get_str(uppercase ? -36 : 36);

	cout << "\n";
	for (int i = 2; i <= 36; ++i)
		cout << "\n\tBase " << i << ":\t\t\t" << (i < 10 ? "\t" : "") << num.get_str(uppercase ? -i : i);

	cout << "\n\n\tMorse code:\t\t\t" << outputmorsecode(num, unicode);
	/* for (size_t i = 0; i < size(morsecode); ++i)
		cout << "\n\t\tStyle " << i << ":\t\t\t" << outputmorsecode(num, i); */

	cout << "\n\n\tBraille:\t\t\t" << outputbraille(num);

	cout << "\n\n\tText:\t\t\t\t" << outputtext(num, special);

	cout << "\n\n\tPrime Factors:\t\t\t" << outputfactors(num, print_exponents, unicode, true);
	cout << "\n\tDivisors:\t\t\t" << outputdivisors(num, true);
	cout << "\n\tAliquot sum:\t\t\t" << outputaliquot(num, true);
	cout << "\n\tPrime or composite:\t\t" << outputprime(num, true) << "\n";
}
#endif

// Convert floating point number to string
template <typename T>
string floattostring(T arg)
{
	ostringstream strm;
	strm.precision(numeric_limits<T>::digits10);
	strm << arg;
	return strm.str();
}

// Output all for floating point numbers
void outputall(const long double ld)
{
	// cout << "\n\tLocale:\t\t\t\t";
	// printf("%'.*Lg", LDBL_DIG, ld);
	ostringstream strm;
	strm.imbue(locale(""));
	strm << setprecision(LDBL_DIG) << ld;
	cout << "\n\tLocale:\t\t\t\t" << strm.str();

	cout << "\n\n\tInternational System of Units (SI):\t\t\t" << outputunit(ld, scale_SI, true);
	cout << "\n\tInternational Electrotechnical Commission (IEC):\t" << outputunit(ld, scale_IEC, true);
	cout << "\n\tInternational Electrotechnical Commission (IEC):\t" << outputunit(ld, scale_IEC_I, true);

	cout << "\n\n\tFractions and constants:\t" << outputfraction(ld) << "\n";
}

// Handle integer numbers
int integers(const char *const token, const int frombase, const short tobase, const bool unicode, const bool uppercase, const bool special, const bool print_exponents, const scale_type scale_to, const int arg)
{
	char *p;
	intmax_t ll = strtoimax(token, &p, frombase);
	if (*p)
	{
		cerr << "Error: Invalid integer number: " << quoted(token) << ".\n";
		return 1;
	}
	if (errno == ERANGE)
	{
		errno = 0;
		__int128 i128 = strtoi128(token, &p, frombase);
		if (*p)
		{
			cerr << "Error: Invalid integer number: " << quoted(token) << ".\n";
			return 1;
		}
		if (errno == ERANGE)
		{
#if HAVE_GMP
			char const *str = token;
			// Skip initial '+'.
			if (*str == '+')
				++str;
			try
			{
				mpz_class num(str, frombase);
				cout << num << ": ";
				if (dev_debug)
					cerr << "[using arbitrary-precision arithmetic] ";
				if (tobase)
					if (tobase == 16 and arg == 't')
						cout << outputhextext(num);
					else
						cout << num.get_str(uppercase ? -tobase : tobase);
				else
					switch (arg)
					{
					case 'a':
						outputall(num, print_exponents, unicode, uppercase, special);
						break;
					case 'e':
						// gmp_printf("%'Zd", num.get_mpz_t());
						{
							ostringstream strm;
							strm.imbue(locale(""));
							strm << num;
							cout << strm.str();
						}
						break;
					case 'm':
						cout << outputmorsecode(num, unicode);
						break;
					case BRAILLE_OPTION:
						cout << outputbraille(num);
						break;
					case 't':
						cout << outputtext(num, special);
						break;
					case 'p':
						cout << outputfactors(num, print_exponents, unicode);
						break;
					case 'd':
						cout << outputdivisors(num);
						break;
					case 's':
						cout << outputaliquot(num);
						break;
					case 'n':
						cout << outputprime(num);
						break;
					default:
						cerr << "Error: Option not available for arbitrary-precision integer numbers.\n";
						return 1;
					}
				cout << endl;
			}
			catch (const invalid_argument &ex)
			{
				cerr << "Error: Invalid integer number: " << quoted(token) << " (" << ex.what() << ").\n";
				return 1;
			}
#else
			cerr << "Error: Integer number too large to input: " << quoted(token) << " (" << strerror(errno) << "). Program does not support arbitrary-precision integer numbers, because it was not built with GNU Multiple Precision (GMP).\n";
			return 1;
#endif
		}
		else
		{
			cout << outputbase(i128) << ": ";
			if (dev_debug)
				cerr << "[using single-precision arithmetic] ";
			if (tobase)
				if (tobase == 16 and arg == 't')
					cout << outputhextext(i128);
				else
					cout << outputbase(i128, tobase, uppercase);
			else
				switch (arg)
				{
				case 'a':
					outputall(i128, print_exponents, unicode, uppercase, special);
					break;
				case TO_OPTION:
					cout << outputunit(i128, scale_to);
					break;
				case 'r':
					cout << outputroman(i128, unicode);
					break;
				case 'g':
					cout << outputgreek(i128, uppercase);
					break;
				case 'm':
					cout << outputmorsecode(i128, unicode);
					break;
				case BRAILLE_OPTION:
					cout << outputbraille(i128);
					break;
				case 't':
					cout << outputtext(i128, special);
					break;
				case 'p':
					cout << outputfactors(i128, print_exponents, unicode);
					break;
				case 'd':
					cout << outputdivisors(i128);
					break;
				case 's':
					cout << outputaliquot(i128);
					break;
				case 'n':
					cout << outputprime(i128);
					break;
				default:
					cerr << "Error: Option not available for 128-bit integer numbers.\n";
					return 1;
				}
			cout << endl;
		}
	}
	else
	{
		cout << ll << ": ";
		if (dev_debug)
			cerr << "[using single-precision arithmetic] ";
		if (tobase)
			if (tobase == 16 and arg == 't')
				cout << outputhextext(ll);
			else
				cout << outputbase(ll, tobase, uppercase);
		else
			switch (arg)
			{
			case 'a':
				outputall(ll, print_exponents, unicode, uppercase, special);
				break;
			case 'e':
				// printf("%'" PRIdMAX, ll);
				{
					ostringstream strm;
					strm.imbue(locale(""));
					strm << ll;
					cout << strm.str();
				}
				break;
			case TO_OPTION:
				cout << outputunit(ll, scale_to);
				break;
			case 'r':
				cout << outputroman(ll, unicode);
				break;
			case 'g':
				cout << outputgreek(ll, uppercase);
				break;
			case 'm':
				cout << outputmorsecode(ll, unicode);
				break;
			case BRAILLE_OPTION:
				cout << outputbraille(ll);
				break;
			case 't':
				cout << outputtext(ll, special);
				break;
			case 'p':
				cout << outputfactors(ll, print_exponents, unicode);
				break;
			case 'd':
				cout << outputdivisors(ll);
				break;
			case 's':
				cout << outputaliquot(ll);
				break;
			case 'n':
				cout << outputprime(ll);
				break;
			}
		cout << endl;
	}

	return 0;
}

// Handle floating point numbers
int floats(const char *const token, const scale_type scale_to, const int arg)
{
	char *p;
	const long double ld = strtold(token, &p);
	if (*p)
	{
		cerr << "Error: Invalid floating point number: " << quoted(token) << ".\n";
		return 1;
	}
	if (errno == ERANGE)
	{
		cerr << "Error: Floating point number too large to input: " << quoted(token) << " (" << strerror(errno) << ").\n";
		return 1;
	}

	cout << floattostring(ld) << ": ";
	switch (arg)
	{
	case 'a':
		outputall(ld);
		break;
	case 'e':
		// printf("%'.*Lg", LDBL_DIG, ld);
		{
			ostringstream strm;
			strm.imbue(locale(""));
			strm << setprecision(LDBL_DIG) << ld;
			cout << strm.str();
		}
		break;
	case TO_OPTION:
		cout << outputunit(ld, scale_to);
		break;
	case 'c':
		cout << outputfraction(ld);
		break;
	}
	cout << endl;

	return 0;
}

// Output usage
void usage(const char *const programname)
{
	cerr << "Usage:  " << programname << R"( [OPTION(S)]... [NUMBER(S)]...
or:     )"
		 << programname << R"d( <OPTION>
If any of the NUMBERS are negative, the first must be preceded by a --. If none are specified on the command line, read them from standard input. NUMBERS can be in Octal, Decimal or Hexadecimal. Use --from-base to specify a different base. See examples below.

Options:
    Mandatory arguments to long options are mandatory for short options too.
    -i, --int           Integer numbers (default)
        -e, --locale        Output in Locale format with digit grouping (same as 'printf "%'d" <NUMBER>' or 'numfmt --grouping')
            --grouping      
            --from-base <BASE> Input in bases 2 - 36
                                   Supports arbitrary-precision/bignums.
        -b, --to-base <BASE>   Output in bases 2 - 36
                                   Supports arbitrary-precision/bignums.
                --binary           Output in Binary      (same as --to-base 2)
                --ternary          Output in Ternary     (same as --to-base 3)
                --quaternary       Output in Quaternary  (same as --to-base 4)
                --quinary          Output in Quinary     (same as --to-base 6)
            -o, --octal            Output in Octal       (same as --to-base 8)
                --decimal          Output in Decimal     (same as --to-base 10)
                --duo              Output in Duodecimal  (same as --to-base 12)
            -x, --hex              Output in Hexadecimal (same as --to-base 16)
                --viges            Output in Vigesimal   (same as --to-base 20)
            --to <UNIT>     Auto-scale output numbers to <UNIT> (similar to 'numfmt --to=<UNIT>', but with more precision)
                                Run 'numfmt --help' for UNIT options.
        -r, --roman         Output as Roman numerals
                                Numbers 1 - 3999.
        -g, --greek         Output as Greek numerals
                                Numbers 1 - 9999, implies --unicode.
        -m, --morse         Output as Morse code
                                Supports arbitrary-precision/bignums.
            --braille       Output as Braille
                                Implies --unicode, supports arbitrary-precision/bignums.
        -t, --text          Output as text
                                Supports arbitrary-precision/bignums.
                --special       Use special words, including: pair, dozen, baker's dozen, score, gross and great gross.
        -p, --factors       Output prime factors (similar to 'factor')
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.
            -h, --exponents     Output repeated factors in form p^e unless e is 1 (similar to 'factor --exponents')
        -d, --divisors      Output divisors
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.
        -s, --aliquot       Output aliquot sum (sum of all divisors) and if it is perfect, deficient or abundant
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.
        -n, --prime         Output if it is prime or composite
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.)d"
#ifndef FACTOR
		 << R"(
        -w, --prove-primality Run probabilistic tests instead of proving the primality of factors
                                Only affects --factors, --divisors, --aliquot and --prime.)"
#endif
		 << R"(
        -a, --all           Output all of the above (default)
        Except when otherwise noted above, this program supports all Integer numbers )"
		 << outputbase(INT128_MIN) << " - " << outputbase(INT128_MAX) << R"d(.

    -f, --float         Floating point numbers
        -e, --locale        Output in Locale format with digit grouping (same as 'printf "%'g" <NUMBER>' or 'numfmt --grouping')
            --grouping      
            --to <UNIT>     Auto-scale output numbers to <UNIT> (similar to 'numfmt --to=<UNIT>', but with more precision)
                                Run 'numfmt --help' for UNIT options.
        -c, --fracts        Convert fractions and mathematical constants to Unicode characters
                                Supports all Unicode fractions, Pi and e constants, implies --unicode.
        -a, --all           Output all of the above (default)
        Except when otherwise noted above, this program supports all Floating point numbers )d"
		 << LDBL_MIN << " - " << LDBL_MAX << R"d(.

        --ascii         ASCII (default)
    -u, --unicode       Unicode
                            Only affects --roman, --morse and --factors.
    -l, --lower         Lowercase
                            Only affects --to-base (with <BASE> > 10) and --greek.
        --upper         Uppercase (default)

        --help          Display this help and exit
        --version       Output version information and exit

Examples:
    Output everything for -1234
    $ )d" << programname
		 << R"d( -- -1234

    Output 0361100 (octal), 123456 and 0x1E240 (hexadecimal) in binary
    $ )d" << programname
		 << R"d( --binary 0361100 123456 0x1E240

    Output 11110001001000000 (binary) in base 36
    $ )d" << programname
		 << R"d( --from-base 2 --to-base 36 11110001001000000

    Output 123456 in all the bases (Bash syntax)
    $ for i in {2..36}; do echo "Base $i: $()d"
		 << programname << R"d( --to-base "$i" 123456 | sed -n 's/^.*: //p')"; done

    Output 1234 as Unicode Roman numerals
    $ )d" << programname
		 << R"( --roman --unicode 1234

    Convert 1T from ‘SI’ to ‘IEC’ scales
    $ numfmt --from=si 1T | )"
		 << programname << R"d( --to=iec-i

    Output the current time (hour and minute) as text
    $ date +%l%n%M | )d"
		 << programname << R"( --from-base 10 --text | sed -n 's/^.*: //p'

    Output the aliquot sum for 6, 28, 496, 8128, 33550336, 8589869056 and 137438691328
    $ )" << programname
		 << R"( --aliquot 6 28 496 8128 33550336 8589869056 137438691328

    Output if 3, 7, 31, 127, 8191, 131071 and 524287 are prime or composite
    $ )" << programname
		 << R"( --prime 3 7 31 127 8191 131071 524287

    Output 1234.25 with Unicode fractions
    $ )" << programname
		 << R"( --float --fracts 1234.25

)";
}

int main(int argc, char *argv[])
{
	bool integer = true;
	int frombase = 0;
	int tobase = 0;
	bool unicode = false;
	bool uppercase = true;
	bool special = false;
	bool print_exponents = false;
	enum scale_type scale_to = scale_none;
	int arg = 'a';

	setlocale(LC_ALL, "");

	// https://stackoverflow.com/a/38646489

	static struct option long_options[] = {
		{"int", no_argument, nullptr, 'i'},
		{"locale", no_argument, nullptr, 'e'},
		{"grouping", no_argument, nullptr, 'e'},
		{"from-base", required_argument, nullptr, FROM_BASE_OPTION},
		{"to-base", required_argument, nullptr, 'b'},
		{"binary", no_argument, nullptr, BINARY_OPTION},		 // 2
		{"ternary", no_argument, nullptr, TERNARY_OPTION},		 // 3
		{"quaternary", no_argument, nullptr, QUATERNARY_OPTION}, // 4
		{"quinary", no_argument, nullptr, QUINARY_OPTION},		 // 6
		{"octal", no_argument, nullptr, 'o'},
		{"decimal", no_argument, nullptr, DECIMAL_OPTION}, // 1
		{"duo", no_argument, nullptr, DUO_OPTION},		   // j
		{"hex", no_argument, nullptr, 'x'},
		{"viges", no_argument, nullptr, VIGES_OPTION}, // k
		// {"base36", no_argument, NULL, BASE36_OPTION}, // q
		{"roman", no_argument, nullptr, 'r'},
		{"greek", no_argument, nullptr, 'g'},
		{"morse", no_argument, nullptr, 'm'},
		{"braille", no_argument, nullptr, BRAILLE_OPTION},
		{"text", no_argument, nullptr, 't'},
		{"special", no_argument, nullptr, SPECIAL_OPTION},
		{"to", required_argument, nullptr, TO_OPTION},
		{"factors", no_argument, nullptr, 'p'},
		{"exponents", no_argument, nullptr, 'h'},
		{"prove-primality", no_argument, nullptr, 'w'},
		{"divisors", no_argument, nullptr, 'd'},
		{"aliquot", no_argument, nullptr, 's'},
		{"prime", no_argument, nullptr, 'n'},
		{"all", no_argument, nullptr, 'a'},
		{"float", no_argument, nullptr, 'f'},
		{"fracts", no_argument, nullptr, 'c'},
		{"ascii", no_argument, nullptr, ASCII_OPTION}, // y
		{"unicode", no_argument, nullptr, 'u'},
		{"lower", no_argument, nullptr, 'l'},
		{"upper", no_argument, nullptr, UPPER_OPTION}, // z
		// {"-debug", no_argument, nullptr, DEV_DEBUG_OPTION},
		{"verbose", no_argument, nullptr, 'v'},
		{"-debug", no_argument, nullptr, 'v'},
		{"help", no_argument, nullptr, GETOPT_HELP_CHAR},
		{"version", no_argument, nullptr, GETOPT_VERSION_CHAR},
		{nullptr, 0, nullptr, 0}};

	int option_index = 0;
	int c = 0;

	while ((c = getopt_long(argc, argv, "ab:cdefghilmnoprstuvwx", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		// case 0:
		// break;
		case 'a':
		case 'd':
		case 'e':
		case 'n':
		case 'p':
		case 'r':
		case 's':
		case 't':
		case BRAILLE_OPTION:
			arg = c;
			break;
		case 'c':
		case 'g':
		case 'm':
			arg = c;
			unicode = true;
			break;
		case DECIMAL_OPTION:
			tobase = 10;
			break;
		case BINARY_OPTION:
			tobase = 2;
			break;
		case TERNARY_OPTION:
			tobase = 3;
			break;
		case QUATERNARY_OPTION:
			tobase = 4;
			break;
		case QUINARY_OPTION:
			tobase = 6;
			break;
		case FROM_BASE_OPTION:
			frombase = strtol(optarg, nullptr, 0);
			if (frombase < 2 or frombase > 36)
			{
				cerr << "Error: <BASE> must be 2 - 36.\n";
				return 1;
			}
			break;
		case 'b':
			tobase = strtol(optarg, nullptr, 0);
			if (tobase < 2 or tobase > 36)
			{
				cerr << "Error: <BASE> must be 2 - 36.\n";
				return 1;
			}
			break;
		case 'f':
			integer = false;
			break;
		case 'h':
			print_exponents = true;
			break;
		case 'i':
			integer = true;
			break;
		case DUO_OPTION:
			tobase = 12;
			break;
		case VIGES_OPTION:
			tobase = 20;
			break;
		case 'l':
			uppercase = false;
			break;
		case 'o':
			tobase = 8;
			break;
		// case BASE36_OPTION:
		// tobase = 36;
		// break;
		case TO_OPTION:
			arg = c;
			scale_to = xargmatch("--to", optarg, scale_to_args, size(scale_to_args), scale_to_types);
			break;
		case 'u':
			unicode = true;
			break;
		// case DEV_DEBUG_OPTION:
		case 'v':
			dev_debug = true;
			break;
		case 'w':
			flag_prove_primality = false;
			break;
		case 'x':
			tobase = 16;
			break;
		case ASCII_OPTION:
			unicode = false;
			break;
		case UPPER_OPTION:
			uppercase = true;
			break;
		case SPECIAL_OPTION:
			special = true;
			break;
		case GETOPT_HELP_CHAR:
			usage(argv[0]);
			return 0;
		case GETOPT_VERSION_CHAR:
			cout << "Numbers 1.0\n\n";
			return 0;
		case '?':
			cerr << "Try '" << argv[0] << " --help' for more information.\n";
			return 1;
		default:
			abort();
		}
	}

	if (integer)
	{
		if (arg == 'c')
		{
			cerr << "Usage: Option not available for integer numbers.\n";
			return 1;
		}
	}
	else
	{
		if (frombase or tobase or arg == 'r' or arg == 'g' or arg == 'm' or arg == BRAILLE_OPTION or arg == 't' or arg == 'p' or arg == 'd' or arg == 's' or arg == 'n')
		{
			cerr << "Usage: Option not available for floating point numbers.\n";
			return 1;
		}
	}

	if (special and arg != 'a' and arg != 't')
	{
		cerr << "Usage: --special is only available for integer numbers with --all and --text\n";
		return 1;
	}

	if (print_exponents and arg != 'a' and arg != 'p')
	{
		cerr << "Usage: --exponents is only available for integer numbers with --all and --factors\n";
		return 1;
	}

	if (optind < argc)
	{
		for (int i = optind; i < argc; ++i)
		{
			if (integer)
				integers(argv[i], frombase, tobase, unicode, uppercase, special, print_exponents, scale_to, arg);
			else
				floats(argv[i], scale_to, arg);
		}
	}
	else
	{
		string token;
		while (cin >> token)
		{
			if (integer)
				integers(token.c_str(), frombase, tobase, unicode, uppercase, special, print_exponents, scale_to, arg);
			else
				floats(token.c_str(), scale_to, arg);
		}
	}

	return 0;
}
