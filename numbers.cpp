// Teal Dulcet

// Requires GNU Coreutils

// Support for arbitrary-precision integers requires the GNU Multiple Precision (GMP) library
// sudo apt-get update
// sudo apt-get install libgmp-dev

// Compile without GMP: g++ -Wall -g -O3 -flto numbers.cpp -o numbers

// Compile with GMP: g++ -Wall -g -O3 -flto numbers.cpp -o numbers -DHAVE_GMP -lgmpxx -lgmp

// Run: ./numbers [OPTION(S)]... [NUMBER(S)]...
// If any of the NUMBERS are negative, the first must be preceded by a --.

// On Linux distributions with GNU Coreutils older than 9.0, including Ubuntu (https://bugs.launchpad.net/ubuntu/+source/coreutils/+bug/696618) and Debian (https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=608832), the factor command (part of GNU Coreutils) is built without arbitrary-precision/bignum support. If this is the case on your system and you are compiling this program with GMP, you will also need to build the factor command with GMP.

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

// Change the FACTOR variable below to the new factor commands location, for example: "./factor"

#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <climits>
#include <cfloat>
#include <limits>
#include <cstdio>
#include <clocale>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cinttypes>
#include <regex>
#include <getopt.h>
#if HAVE_GMP
#include <gmpxx.h>
#endif

using namespace std;

// factor command
const char *const FACTOR = "factor";

enum
{
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

const char *const suffix_power_char[] = {"", "K", "M", "G", "T", "P", "E", "Z", "Y"};

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

const char *const ONES[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
const char *const TEENS[] = {"", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};
const char *const TENS[] = {"", "ten", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
// https://en.wikipedia.org/wiki/Names_of_large_numbers
const char *const THOUSANDPOWERS[] = {"", "thousand", "million", "billion", "trillion", "quadrillion", "quintillion", "sextillion", "septillion", "octillion", "nonillion", "decillion", "undecillion"};

const char *const HEXONES[] = {ONES[0], ONES[1], ONES[2], ONES[3], ONES[4], ONES[5], ONES[6], ONES[7], ONES[8], ONES[9], "ann", "bet", "chris", "dot", "ernest", "frost"};
const char *const HEXTEENS[] = {TEENS[0], TEENS[1], TEENS[2], TEENS[3], TEENS[4], TEENS[5], TEENS[6], TEENS[7], TEENS[8], TEENS[9], "annteen", "betteen", "christeen", "dotteen", "ernesteen", "frosteen"};
const char *const HEXTENS[] = {TENS[0], TENS[1], TENS[2], TENS[3], TENS[4], TENS[5], TENS[6], TENS[7], TENS[8], TENS[9], "annty", "betty", "christy", "dotty", "ernesty", "frosty"};

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

const long double max_bit = scalbn(1.0L, LDBL_MANT_DIG - 1);
const long double MAX = max_bit + (max_bit - 1);

template <typename T>
using T2 = typename conditional<is_integral<T>::value, make_unsigned<T>, common_type<T>>::type::type;

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

	unsigned int x = 0;
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

	if (x > 27 - 1)
	{
		if (all)
			return "N/A";

		cerr << "Error: Number too large to be printed: '" << number << "' (cannot handle numbers > 999Y)\n";
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

	unsigned int power = 0;
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

	if (number != 0 and anumber < 1000 and power > 0)
	{
		strm << setprecision(LDBL_DIG) << number;
		str = strm.str();

		const unsigned int length = 5 + (number < 0 ? 1 : 0);
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

	str += power < 9 ? suffix_power_char[power] : "(error)";

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
string outputroman(const intmax_t number, const bool unicode, const bool all = false)
{
	// uintmax_t anumber = abs(number);
	uintmax_t anumber = number;
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

	for (int i = (sizeof romanvalues / sizeof romanvalues[0]) - 1; anumber > 0; --i)
	{
		uintmax_t div = anumber / romanvalues[i];
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
string outputgreek(const intmax_t number, const bool uppercase, const bool all = false)
{
	// uintmax_t anumber = abs(number);
	uintmax_t anumber = number;
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

	for (int i = (sizeof greekvalues / sizeof greekvalues[0]) - 1; anumber > 0; --i)
	{
		if (anumber / greekvalues[i])
		{
			anumber %= greekvalues[i];
			if (greekvalues[i] >= 1000)
				str += "͵"; // lower left keraia
			str += greek[uppercase][i];
			if (greekvalues[i] < 1000 and anumber == 0)
				str += "ʹ"; // keraia
		}
	}

	return str;
}

// Convert number to string
template <typename T>
string tostring(T arg)
{
	ostringstream strm;
	strm << arg;
	return strm.str();
}

// Output number as Morse code
template <typename T>
string outputmorsecode(const T &number, const unsigned int style)
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
		if (i > 0)
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

// Output number as text
// Adapted from: https://rosettacode.org/wiki/Number_names
string outputtext(const intmax_t number, const bool special)
{
	// uintmax_t n = abs(number);
	uintmax_t n = number;
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
			intmax_t temp = n / 12;
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
		str += ONES[n];
		return str;
	}

	string astr;

	const uintmax_t scale = 1000;
	for (uintmax_t index = 0; n > 0; ++index)
	{
		uintmax_t h = n % scale;
		n /= scale;
		if (h)
		{
			string aastr;
			if (n)
				// aastr += ' ';
				aastr += astr.empty() and (h < 100 or h % 100 == 0) ? " and " : ", ";
			if (h >= 100)
			{
				aastr += ONES[h / 100];
				aastr += " hundred";
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
			{
				aastr += ' ';
				aastr += THOUSANDPOWERS[index];
			}
			astr = aastr + astr;
		}
	}

	str += astr;
	return str;
}

// Output hexadecimal number as text
string outputhextext(const intmax_t number)
{
	// uintmax_t n = abs(number);
	uintmax_t n = number;
	n = number < 0 ? -n : n;

	string str;

	const uintmax_t scale = 0x100;
	do
	{
		uintmax_t h = n % scale;
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

// Execute factor command to get prime factors of number
template <typename T>
map<T2<T>, size_t> factor(const T &number)
{
	const string cmd = string(FACTOR) + R"( ")" + tostring(number) + R"(" 2>&1)";

	string result;
	if (exec(cmd.c_str(), result))
	{
		cerr << "Error: " << result /*  << "\n" */;
		return {};
	}

	result = regex_replace(result, re, "");

	istringstream strm(result);
	map<T2<T>, size_t> counts;

	T2<T> temp;
	while (strm >> temp)
		++counts[temp];

	return counts;
}

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
	const T2<T> &n = number;
	// n = number < 0 ? -n : n;

	ostringstream strm;
	map<T2<T>, size_t> counts = factor(n);

	for (const auto &acount : counts)
	{
		const T2<T> &prime = acount.first;
		const size_t exponent = acount.second;
		for (size_t j = 0; j < exponent; ++j)
		{
			if (strm.tellp())
				strm << ' ' << (unicode ? "×" : "*") << ' ';
			// strm << ' ';
			strm << prime;
			if (print_exponents and exponent > 1)
			{
				if (unicode)
					strm << outputexponent(exponent);
				else
					strm << "^" << exponent;
				break;
			}
		}
	}

	return strm.str();
}

// Get divisors of number
template <typename T>
vector<T2<T>> divisor(const T &number)
{
	map<T2<T>, size_t> counts = factor(number);
	vector<T2<T>> divisors{1};

	for (const auto &acount : counts)
	{
		const T2<T> &prime = acount.first;
		const size_t exponent = acount.second;
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
		if (i > 0)
			strm << ' ';
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

	strm << sum << " (";

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

		for (size_t i = 0; i < (sizeof fractions / sizeof fractions[0]) and !output; ++i)
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
			for (size_t i = 0; i < (sizeof constants / sizeof constants[0]) and !output; ++i)
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
void outputall(const intmax_t ll, const bool print_exponents, const bool unicode, const bool uppercase, const bool special)
{
	// cout << "\n\tLocale:\t\t\t\t";
	// printf("%'" PRIdMAX, ll);
	ostringstream strm;
	strm.imbue(locale(""));
	strm << ll;
	cout << "\n\tLocale:\t\t\t\t" << strm.str();

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
	/* for (unsigned int i = 0; i < (sizeof morsecode / sizeof morsecode[0]); ++i)
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
void outputall(const mpz_class &num, const bool print_exponents, const bool unicode, const bool uppercase)
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

	cout << "\n\n\tBinary (Base 2):\t\t";
	mpz_out_str(stdout, uppercase ? -2 : 2, num.get_mpz_t());
	cout << "\n\tTernary (Base 3):\t\t";
	mpz_out_str(stdout, uppercase ? -3 : 3, num.get_mpz_t());
	cout << "\n\tQuaternary (Base 4):\t\t";
	mpz_out_str(stdout, uppercase ? -4 : 4, num.get_mpz_t());
	cout << "\n\tQuinary (Base 6):\t\t";
	mpz_out_str(stdout, uppercase ? -6 : 6, num.get_mpz_t());
	cout << "\n\tOctal (Base 8):\t\t\t";
	mpz_out_str(stdout, uppercase ? -8 : 8, num.get_mpz_t());
	cout << "\n\tDecimal (Base 10):\t\t";
	mpz_out_str(stdout, uppercase ? -10 : 10, num.get_mpz_t());
	cout << "\n\tDuodecimal (Base 12):\t\t";
	mpz_out_str(stdout, uppercase ? -12 : 12, num.get_mpz_t());
	cout << "\n\tHexadecimal (Base 16):\t\t";
	mpz_out_str(stdout, uppercase ? -16 : 16, num.get_mpz_t());
	cout << "\n\tVigesimal (Base 20):\t\t";
	mpz_out_str(stdout, uppercase ? -20 : 20, num.get_mpz_t());
	// cout << "\n\tBase 36:\t\t\t";
	// mpz_out_str(stdout, uppercase ? -36 : 36, num.get_mpz_t());

	cout << "\n";
	for (int i = 2; i <= 36; ++i)
	{
		cout << "\n\tBase " << i << ":\t\t\t" << (i < 10 ? "\t" : "");
		mpz_out_str(stdout, uppercase ? -i : i, num.get_mpz_t());
	}

	cout << "\n\n\tMorse code:\t\t\t" << outputmorsecode(num, unicode);
	/* for (unsigned int i = 0; i < (sizeof morsecode / sizeof morsecode[0]); ++i)
		cout << "\n\t\tStyle " << i << ":\t\t\t" << outputmorsecode(num, i); */

	cout << "\n\n\tBraille:\t\t\t" << outputbraille(num);

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
	const intmax_t ll = strtoimax(token, &p, frombase);
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
		mpz_class num(str, frombase);
		cout << num << ": ";
		if (tobase)
			mpz_out_str(stdout, uppercase ? -tobase : tobase, num.get_mpz_t());
		else
			switch (arg)
			{
			case 'a':
				outputall(num, print_exponents, unicode, uppercase);
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
#else
		cerr << "Error: Integer number too large to input: " << quoted(token) << " (" << strerror(errno) << "). Program does not support arbitrary-precision integer numbers, because it was not built with GNU Multiple Precision (GMP).\n";
		return 1;
#endif
	}
	else
	{
		cout << ll << ": ";
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
		cerr << "Error: Invalid floating point number: " << quoted(token) << "\n";
		return 1;
	}
	if (errno == ERANGE)
	{
		cerr << "Error: Floating point number too large to input: " << quoted(token) << " (" << strerror(errno) << ")\n";
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
                --special       Use special words, including: pair, dozen, baker's dozen, score, gross and great gross.
        -p, --factors       Output prime factors (similar to 'factor')
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.
            -h, --exponents     Output repeated factors in form p^e unless e is 1 (similar to 'factor --exponents')
        -d, --divisors      Output divisors
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.
        -s, --aliquot       Output aliquot sum (sum of all divisors) and if it is perfect, deficient or abundant
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.
        -n, --prime         Output if it is prime or composite
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision.
        -a, --all           Output all of the above (default)
        Except when otherwise noted above, this program supports all Integer numbers )d"
		 << INTMAX_MIN << " - " << INTMAX_MAX << R"d(.

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
	if (argc < 2)
	{
		usage(argv[0]);
		return 1;
	}

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
		{"braille", no_argument, nullptr, BRAILLE_OPTION}, // w
		{"text", no_argument, nullptr, 't'},
		{"special", no_argument, nullptr, SPECIAL_OPTION},
		{"to", required_argument, nullptr, TO_OPTION},
		{"factors", no_argument, nullptr, 'p'},
		{"exponents", no_argument, nullptr, 'h'},
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
		{"help", no_argument, nullptr, GETOPT_HELP_CHAR},
		{"version", no_argument, nullptr, GETOPT_VERSION_CHAR},
		{nullptr, 0, nullptr, 0}};

	int option_index = 0;
	int c = 0;

	while ((c = getopt_long(argc, argv, "ab:cdefghilmnoprstux", long_options, &option_index)) != -1)
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
			scale_to = xargmatch("--to", optarg, scale_to_args, (sizeof scale_to_args / sizeof scale_to_args[0]), scale_to_types);
			break;
		case 'u':
			unicode = true;
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
