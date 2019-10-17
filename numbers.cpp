// Teal Dulcet

// Requires GNU Coreutils

// Support for arbitrary-precision integers requires the GNU Multiple Precision (GMP) library
// sudo apt-get update
// sudo apt-get install libgmp3-dev

// Compile without GMP: g++ -Wall -g -O3 numbers.cpp -o numbers
// or:                  g++ -std=c++11 -Wall -g -O3 numbers.cpp -o numbers

// Compile with GMP: g++ -Wall -g -O3 numbers.cpp -o numbers -DHAVE_GMP -lgmpxx -lgmp
// or:               g++ -std=c++11 -Wall -g -O3 numbers.cpp -o numbers -DHAVE_GMP -lgmpxx -lgmp

// Run: ./numbers [OPTION(S)]... <NUMBER(S)>...
// If any of the <NUMBERS> are negative, the first must be preceded by a --.

// On many Linux distributions, including Ubuntu (https://bugs.launchpad.net/ubuntu/+source/coreutils/+bug/696618) and Debian (https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=608832), the factor command (part of GNU Coreutils) is built without arbitrary-precision/bignum support. If this is the case on your system and you are compiling this program with GMP, you will also need to build the factor command with GMP.

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
	make -j "$(nproc)" CFLAGS="-g -O3"
	make -j "$(nproc)" check CFLAGS="-g -O3" RUN_EXPENSIVE_TESTS=yes RUN_VERY_EXPENSIVE_TESTS=yes

	Copy factor command to starting directory
	cp ./src/factor "$DIRNAME/" */

/* Build uutils coreutils (Cross-platform Rust rewrite of the GNU Coreutils, but does not currently support arbitrary-precision/bignums (https://github.com/uutils/coreutils/pull/201))

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
#include <cmath>
#include <climits>
#include <cfloat>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <clocale>
#include <vector>
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
	UPPER_OPTION
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
	{"I", "IV", "V", "IX", "X", "XL", "L", "XC", "C", "CD", "D", "CM", "M"}, //ASCII
	{"Ⅰ", "ⅠⅤ", "Ⅴ", "ⅠⅩ", "Ⅹ", "ⅩⅬ", "Ⅼ", "ⅩⅭ", "Ⅽ", "ⅭⅮ", "Ⅾ", "ⅭⅯ", "Ⅿ"}  //Unicode
};

const short romanvalues[] = {1, 4, 5, 9, 10, 40, 50, 90, 100, 400, 500, 900, 1000};

const char *const greek[][36] = {
	{"α", "β", "γ", "δ", "ε", "ϛ", "ζ", "η", "θ", "ι", "κ", "λ", "μ", "ν", "ξ", "ο", "π", "ϟ", "ρ", "σ", "τ", "υ", "φ", "χ", "ψ", "ω", "ϡ", "α", "β", "γ", "δ", "ε", "ϛ", "ζ", "η", "θ"}, //Unicode lowercase
	{"Α", "Β", "Γ", "Δ", "Ε", "Ϛ", "Ζ", "Η", "Θ", "Ι", "Κ", "Λ", "Μ", "Ν", "Ξ", "Ο", "Π", "Ϟ", "Ρ", "Σ", "Τ", "Υ", "Φ", "Χ", "Ψ", "Ω", "Ϡ", "Α", "Β", "Γ", "Δ", "Ε", "Ϛ", "Ζ", "Η", "Θ"}  //Unicode uppercase
};

const short greekvalues[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000};

const char *const ONES[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};

const char *const TEENS[] = {"", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"};

const char *const TENS[] = {"", "ten", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};

// https://en.wikipedia.org/wiki/Names_of_large_numbers
const char *const thousandpowers[] = {"", "thousand", "million", "billion", "trillion", "quadrillion", "quintillion", "sextillion", "septillion", "octillion", "nonillion", "decillion", "undecillion"};

const uintmax_t thousandpowersindex = log(UINTMAX_MAX) / log(1000);
const uintmax_t thousandpowersscale = pow(1000, thousandpowersindex);

const char *const morsecode[][11] = {
	// 0-9, -
	{"- - - - -", ". - - - -", ". . - - -", ". . . - -", ". . . . -", ". . . . .", "- . . . .", "- - . . .", "- - - . .", "- - - - .", "- . . . . -"},																				   //ASCII
	{"− − − − −", "• − − − −", "• • − − −", "• • • − −", "• • • • −", "• • • • •", "− • • • •", "− − • • •", "− − − • •", "− − − − •", "− • • • • −"},																				   //Bullet and minus sign
	{"– – – – –", "· – – – –", "· · – – –", "· · · – –", "· · · · –", "· · · · ·", "– · · · ·", "– – · · ·", "– – – · ·", "– – – – ·", "– · · · · –"},																				   //Middle dot and en dash
	{"▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄ ▄ ▄▄▄▄ ▄▄▄▄", "▄ ▄ ▄ ▄ ▄▄▄▄", "▄ ▄ ▄ ▄ ▄", "▄▄▄▄ ▄ ▄ ▄ ▄", "▄▄▄▄ ▄▄▄▄ ▄ ▄ ▄", "▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄ ▄", "▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄▄▄▄ ▄", "▄▄▄▄ ▄ ▄ ▄ ▄ ▄▄▄▄"} //Blocks
};

const char *const gap = "   ";

const char *const braille[] = {"⠀", "⠁", "⠂", "⠃", "⠄", "⠅", "⠆", "⠇", "⠈", "⠉", "⠊", "⠋", "⠌", "⠍", "⠎", "⠏", "⠐", "⠑", "⠒", "⠓", "⠔", "⠕", "⠖", "⠗", "⠘", "⠙", "⠚", "⠛", "⠜", "⠝", "⠞", "⠟", "⠠", "⠡", "⠢", "⠣", "⠤", "⠥", "⠦", "⠧", "⠨", "⠩", "⠪", "⠫", "⠬", "⠭", "⠮", "⠯", "⠰", "⠱", "⠲", "⠳", "⠴", "⠵", "⠶", "⠷", "⠸", "⠹", "⠺", "⠻", "⠼", "⠽", "⠾", "⠿"};

const short brailleindexes[] = {26, 1, 3, 9, 25, 17, 11, 27, 19, 10}; // 0-9

const char *const fractions[] = {"¼", "½", "¾", "⅐", "⅑", "⅒", "⅓", "⅔", "⅕", "⅖", "⅗", "⅘", "⅙", "⅚", "⅛", "⅜", "⅝", "⅞"};

const long double fractionvalues[] = {1.0L / 4.0L, 1.0L / 2.0L, 3.0L / 4.0L, 1.0L / 7.0L, 1.0L / 9.0L, 1.0L / 10.0L, 1.0L / 3.0L, 2.0L / 3.0L, 1.0L / 5.0L, 2.0L / 5.0L, 3.0L / 5.0L, 4.0L / 5.0L, 1.0L / 6.0L, 5.0L / 6.0L, 1.0L / 8.0L, 3.0L / 8.0L, 5.0L / 8.0L, 7.0L / 8.0L};

#define XARGMATCH(Context, Arg, Arglist, Argsize, Vallist) ((Vallist)[xargmatch(Context, Arg, Arglist, Argsize)])

// Check if the argument is in the argument list
// Adapted from: https://github.com/coreutils/gnulib/blob/master/lib/argmatch.c
ptrdiff_t xargmatch(const char *const context, const char *const arg, const char *const *arglist, size_t argsize)
{
	const size_t arglen = strlen(arg);

	for (size_t i = 0; i < argsize; ++i)
		if (!strncmp(arglist[i], arg, arglen) and strlen(arglist[i]) == arglen)
			return i;

	cerr << "Error: Invalid argument ‘" << arg << "’ for ‘" << context << "’\n";
	exit(1);

	return -1;
}

// Auto-scale number to unit
// Adapted from: https://github.com/coreutils/coreutils/blob/master/src/numfmt.c
string outputunit(long double number, enum scale_type scale, const bool all)
{
	char buf[128];
	const size_t buf_size = sizeof(buf);
	int num_size = 0;

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
			exit(1);
		}

		num_size = snprintf(buf, buf_size, "%.*Lg", LDBL_DIG, number);
		if (num_size < 0 or num_size >= (int)buf_size)
		{
			cerr << "Error: Failed to prepare number '" << number << "' for printing\n";
			exit(1);
		}
		return buf;
	}

	if (x > 27 - 1)
	{
		if (all)
			return "N/A";
		else
		{
			cerr << "Error: Number too large to be printed: '" << number << "' (cannot handle numbers > 999Y)\n";
			exit(1);
		}
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

	const long double anumber = abs(number) + 0.5;

	if (number != 0 and anumber < 1000 and power > 0)
	{
		num_size = snprintf(buf, buf_size - 1, "%Lg", number);
		if (num_size < 0 or num_size >= (int)buf_size - 1)
		{
			cerr << "Error: Failed to prepare number '" << number << "' for printing\n";
			exit(1);
		}

		const int length = 5 + (number < 0 ? 1 : 0);
		if (num_size > length)
		{
			const int prec = anumber < 10 ? 3 : (anumber < 100 ? 2 : (anumber < 1000 ? 1 : 0));
			num_size = snprintf(buf, buf_size - 1, "%.*Lf", prec, number);
			if (num_size < 0 or num_size >= (int)buf_size - 1)
			{
				cerr << "Error: Failed to prepare number '" << number << "' for printing\n";
				exit(1);
			}
		}
	}
	else
	{
		num_size = snprintf(buf, buf_size - 1, "%.0Lf", number);
		if (num_size < 0 or num_size >= (int)buf_size - 1)
		{
			cerr << "Error: Failed to prepare number '" << number << "' for printing\n";
			exit(1);
		}
	}

	string str = buf;

	str += power < 9 ? suffix_power_char[power] : "(error)";

	if (scale == scale_IEC_I and power > 0)
		str += "i";

	return str;
}

// Output number in bases 2 - 36
string outputbase(const intmax_t number, const short base, const bool uppercase)
{
	if (base < 2 or base > 36)
	{
		cerr << "Error: <BASE> must be 2 - 36.\n";
		exit(1);
	}

	uintmax_t anumber = abs(number);

	string str;

	do
	{
		char digit = anumber % base;

		if (digit < 10)
			digit += '0';
		else
			digit += (uppercase ? 'A' : 'a') - 10;

		str = digit + str;

		anumber /= base;

	} while (anumber > 0);

	if (number < 0)
		str = "-" + str;

	return str;
}

// Output numbers 1 - 3999 as Roman numerals
string outputroman(const intmax_t number, const bool unicode, const bool all)
{
	uintmax_t anumber = abs(number);

	if (anumber < 1 or anumber > 3999)
	{
		if (all)
			return "N/A";
		else
		{
			cerr << "Error: Number must be between 1 - 3999\n";
			exit(1);
		}
	}

	string str;

	if (number < 0)
		str = "-";

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
string outputgreek(const intmax_t number, const bool uppercase, const bool all)
{
	uintmax_t anumber = abs(number);

	if (anumber < 1 or anumber > 9999)
	{
		if (all)
			return "N/A";
		else
		{
			cerr << "Error: Number must be between 1 - 9999\n";
			exit(1);
		}
	}

	string str;

	if (number < 0)
		str = "-";

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
	const T n = abs(number);

	string str;
	string text = tostring(n);

	if (number < 0)
	{
		str = morsecode[style][10];
		str += gap;
	}

	for (unsigned int i = 0; i < text.length(); ++i)
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
	const T n = abs(number);

	string str;
	string text = tostring(n);

	if (number < 0)
	{
		str = braille[16];
		str += braille[36];
	}

	str += braille[60]; // Number indicator

	for (unsigned int i = 0; i < text.length(); ++i)
		str += braille[brailleindexes[text[i] - '0']];

	return str;
}

// Output number as text
// Adapted from: https://rosettacode.org/wiki/Number_names
string outputtext(const intmax_t number, const bool special)
{
	uintmax_t n = abs(number);

	string str;

	if (number < 0)
		// str = "-";
		str = "negative ";
	if (special and n <= 1728)
	{
		if (n == 2)
		{
			str += "pair";
			return str;
		}
		else if (n == 13)
		{
			str += "baker's dozen";
			return str;
		}
		else if (n == 20)
		{
			str += "score";
			return str;
		}
		else if (n % 12 == 0)
		{
			uintmax_t temp = n / 12;
			if (temp == 1)
			{
				str += "dozen";
				return str;
			}
			else if (temp == 12)
			{
				str += "gross";
				return str;
			}
			else if (temp == 144)
			{
				str += "great gross";
				return str;
			}
		}
	}
	if (n < 10)
	{
		str += ONES[n];
		return str;
	}
	uintmax_t index = thousandpowersindex;
	for (uintmax_t scale = thousandpowersscale; scale > 0; scale /= 1000, --index)
	{
		if (n >= scale)
		{
			uintmax_t h = n / scale;
			if (h > 99)
			{
				str += ONES[h / 100];
				str += " hundred";
				h %= 100;
				if (h)
					// str += " ";
					str += " and ";
			}
			if (h >= 20 or h == 10)
			{
				str += TENS[h / 10];
				h %= 10;
				if (h)
					str += "-";
			}
			if (h < 20 and h > 10)
				str += TEENS[h - 10];
			else if (h < 10 and h > 0)
				str += ONES[h];
			if (index > 0)
			{
				str += " ";
				str += thousandpowers[index];
			}
			n %= scale;
			if (n)
				// str += " ";
				str += n < 100 ? " and " : ", ";
		}
	}
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
		while (fgets(buffer, sizeof(buffer), pipe) != NULL)
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
vector<T> factor(const T &number)
{
	string cmd = tostring(FACTOR) + " \"" + tostring(number) + "\" 2>&1";

	string result = "";
	if (exec(cmd.c_str(), result))
	{
		cerr << "Error: " << result << "\n";
		exit(1);
	}

	regex re("^.*: ");
	result = regex_replace(result, re, "");

	istringstream strm(result);
	vector<T> factors;

	T temp;
	while (strm >> temp)
		factors.push_back(temp);

	return factors;
}

// Output prime factors of number
template <typename T>
string outputfactors(const T &number, const bool unicode, const bool all)
{
	if (number < 1)
	{
		if (all)
			return "N/A";
		else
		{
			cerr << "Error: Number must be > 0\n";
			exit(1);
		}
	}

	const T n = abs(number);

	ostringstream strm;
	vector<T> factors = factor(n);

	for (size_t i = 0; i < factors.size(); ++i)
	{
		if (i > 0)
			strm << " " << (unicode ? "×" : "*") << " ";
		// strm << " ";
		strm << factors[i];
	}

	return strm.str();
}

// Get divisors of number
template <typename T>
vector<T> divisor(const T &number)
{
	vector<T> factors = factor(number);
	vector<T> divisors;

	T temp = pow(2, factors.size()) - 1;
	for (T i = 0; i < temp; ++i)
	{
		T idx = i;
		T divisor = 1;
		for (size_t j = 0; j < factors.size(); ++j)
		{
			// if (idx % 2)
			if (idx % 2 != 0)
				divisor *= factors[j];
			idx >>= 1;
		}
		divisors.push_back(divisor);
	}

	sort(divisors.begin(), divisors.end());
	divisors.erase(unique(divisors.begin(), divisors.end()), divisors.end());

	return divisors;
}

// Output divisors of number
template <typename T>
string outputdivisors(const T &number, const bool all)
{
	if (number < 1)
	{
		if (all)
			return "N/A";
		else
		{
			cerr << "Error: Number must be > 0\n";
			exit(1);
		}
	}

	ostringstream strm;
	const T n = abs(number);

	vector<T> divisors = divisor(n);

	for (size_t i = 0; i < divisors.size(); ++i)
	{
		if (i > 0)
			strm << " ";
		strm << divisors[i];
	}

	return strm.str();
}

// Output aliquot sum of number
template <typename T>
string outputaliquot(const T &number, const bool all)
{
	if (number < 2)
	{
		if (all)
			return "N/A";
		else
		{
			cerr << "Error: Number must be > 1\n";
			exit(1);
		}
	}

	ostringstream strm;
	const T n = abs(number);

	vector<T> divisors = divisor(n);
	T sum = 0;

	for (size_t i = 0; i < divisors.size(); ++i)
		sum += divisors[i];

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
string outputprime(const T &number, const bool all)
{
	if (number < 2)
	{
		if (all)
			return "N/A";
		else
		{
			cerr << "Error: Number must be > 1\n";
			exit(1);
		}
	}

	string str;
	const T n = abs(number);

	vector<T> divisors = divisor(n);

	if (divisors.size() == 1)
		str = "Prime!";
	else
		str = "Composite (Not prime)";

	return str;
}

// Convert fractions and constants to Unicode characters
// Adapted from: https://github.com/tdulcet/Tables-and-Graphs/blob/master/graphs.hpp
string outputlabel(const long double number)
{
	bool output = false;

	long double intpart = 0;
	long double fractionpart = abs(modf(number, &intpart));

	ostringstream strm;

	for (unsigned int i = 0; i < (sizeof fractions / sizeof fractions[0]) and !output; ++i)
	{
		if (abs(fractionpart - fractionvalues[i]) < DBL_EPSILON)
		{
			if (intpart != 0)
				strm << intpart;

			strm << fractions[i];

			output = true;
		}
	}

	if (abs(number) >= DBL_EPSILON)
	{
		if (!output and fmod(number, M_PI) == 0)
		{
			const char symbol[] = "π";

			intpart = number / M_PI;

			if (intpart == -1)
				strm << "-";
			else if (intpart != 1)
				strm << intpart;

			strm << symbol;

			output = true;
		}
		else if (!output and fmod(number, M_E) == 0)
		{
			const char symbol[] = "e";

			intpart = number / M_E;

			if (intpart == -1)
				strm << "-";
			else if (intpart != 1)
				strm << intpart;

			strm << symbol;

			output = true;
		}
	}

	if (!output)
		strm << number;

	return strm.str();
}

// Output all for integer numbers
void outputall(const intmax_t ll, const bool unicode, const bool uppercase, const bool special)
{
	cout << "\n\tLocale:\t\t\t\t";
	printf("%'" PRIdMAX, ll);

	/* cout << "\n\n\tC (printf)\n";
	cout << "\t\tOctal (Base 8):\t\t";
	printf("%" SCNoMAX, ll);
	cout << "\n\t\tDecimal (Base 10):\t";
	printf("%" SCNdMAX, ll);
	cout << "\n\t\tHexadecimal (Base 16):\t";
	printf("%" SCNxMAX, ll);

	cout << "\n\n\tC++ (cout)\n";
	cout << "\t\tOctal (Base 8):\t\t" << oct << ll;
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
	for (int i = 2; i <= 36; ++i)
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

	cout << "\n\n\tPrime Factors:\t\t\t" << outputfactors(ll, unicode, true);
	cout << "\n\tDivisors:\t\t\t" << outputdivisors(ll, true);
	cout << "\n\tAliquot sum:\t\t\t" << outputaliquot(ll, true);
	cout << "\n\tPrime or composite:\t\t" << outputprime(ll, true) << "\n";
}

// Output all for arbitrary-precision integer numbers
#if HAVE_GMP
void outputall(const mpz_class &num, const bool unicode, const bool uppercase)
{
	cout << "\n\tLocale:\t\t\t\t";
	gmp_printf("%'Zd", num.get_mpz_t());

	/* cout << "\n\n\tC (printf)\n";
	cout << "\t\tOctal (Base 8):\t\t";
	gmp_printf("%Zo", num.get_mpz_t());
	cout << "\n\t\tDecimal (Base 10):\t";
	gmp_printf("%Zd", num.get_mpz_t());
	cout << "\n\t\tHexadecimal (Base 16):\t";
	gmp_printf("%Zx", num.get_mpz_t());

	cout << "\n\n\tC++ (cout)\n";
	cout << "\t\tOctal (Base 8):\t\t" << oct << num;
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

	cout << "\n\n\tPrime Factors:\t\t\t" << outputfactors(num, unicode, true);
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
	typedef numeric_limits<T> dbl;
	strm.precision(dbl::digits10);
	strm << arg;
	return strm.str();
}

// Output all for floating point numbers
void outputall(const long double ld)
{
	cout << "\n\tLocale:\t\t\t\t";
	printf("%'.*Lg", LDBL_DIG, ld);

	cout << "\n\n\tInternational System of Units (SI):\t\t\t" << outputunit(ld, scale_SI, true);
	cout << "\n\tInternational Electrotechnical Commission (IEC):\t" << outputunit(ld, scale_IEC, true);
	cout << "\n\tInternational Electrotechnical Commission (IEC):\t" << outputunit(ld, scale_IEC_I, true);

	cout << "\n\n\tFractions and constants:\t" << outputlabel(ld) << "\n";
}

// Output usage
void usage(const char *const programname)
{
	cerr << "Usage:  " << programname << " [OPTION(S)]... <NUMBER(S)>...\n\
or:     " << programname
		 << " <OPTION>\n\
If any of the <NUMBERS> are negative, the first must be preceded by a --. <NUMBERS> can be in Octal, Decimal or Hexadecimal. Use --from-base to specify a different base. See examples below.\n\
\n\
Options:\n\
    Mandatory arguments to long options are mandatory for short options too.\n\
    -i, --int           Integer numbers (default)\n\
        -e, --locale        Output in Locale format with digit grouping (same as 'printf \"%'d\" <NUMBER>' or 'numfmt --grouping <NUMBER>')\n\
            --grouping      \n\
            --from-base <BASE> Input in bases 2 - 36\n\
                                   Supports arbitrary-precision/bignums\n\
        -b, --to-base <BASE>   Output in bases 2 - 36\n\
                                   Supports arbitrary-precision/bignums\n\
                --binary           Output in Binary      (same as --to-base 2)\n\
                --ternary          Output in Ternary     (same as --to-base 3)\n\
                --quaternary       Output in Quaternary  (same as --to-base 4)\n\
                --quinary          Output in Quinary     (same as --to-base 6)\n\
            -o, --octal            Output in Octal       (same as --to-base 8)\n\
                --decimal          Output in Decimal     (same as --to-base 10)\n\
                --duo              Output in Duodecimal  (same as --to-base 12)\n\
            -x, --hex              Output in Hexadecimal (same as --to-base 16)\n\
                --viges            Output in Vigesimal   (same as --to-base 20)\n\
            --to <UNIT>     Auto-scale output numbers to <UNIT> (similar to 'numfmt --to=<UNIT> <NUMBER>', but with more precision)\n\
                                Run 'numfmt --help' for UNIT options\n\
        -r, --roman         Output as Roman numerals\n\
                                Numbers 1 - 3999\n\
        -g, --greek         Output as Greek numerals\n\
                                Numbers 1 - 9999, implies --unicode\n\
        -m, --morse         Output as Morse code\n\
                                Supports arbitrary-precision/bignums\n\
            --braille       Output as Braille\n\
                                Implies --unicode, supports arbitrary-precision/bignums\n\
        -t, --text          Output as text\n\
                --special       Use special words, including: pair, dozen, baker's dozen, score, gross and great gross\n\
        -p, --factors       Output prime factors (same as 'factor <NUMBER>')\n\
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision\n\
        -d, --divisors      Output divisors\n\
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision\n\
        -s, --aliquot       Output aliquot sum (sum of all divisors) and if it is perfect, deficient or abundant\n\
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision\n\
        -n, --prime         Output if it is prime or composite\n\
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision\n\
        -a, --all           Output all of the above (default)\n\
        Except when otherwise noted above, this program supports all Integer numbers "
		 << INTMAX_MIN << " - " << INTMAX_MAX << ".\n\n\
    -f, --float         Floating point numbers\n\
        -e, --locale        Output in Locale format with digit grouping (same as 'printf \"%'g\" <NUMBER>' or 'numfmt --grouping <NUMBER>')\n\
            --grouping      \n\
            --to <UNIT>     Auto-scale output numbers to <UNIT> (similar to 'numfmt --to=<UNIT> <NUMBER>', but with more precision)\n\
                                Run 'numfmt --help' for UNIT options\n\
        -c, --fracts        Convert fractions and mathematical constants to Unicode characters\n\
                                Supports all Unicode fractions, Pi and e constants, implies --unicode\n\
        -a, --all           Output all of the above (default)\n\
        Except when otherwise noted above, this program supports all Floating point numbers "
		 << LDBL_MIN << " - " << LDBL_MAX << ".\n\
\n\
        --ascii         ASCII (default)\n\
    -u, --unicode       Unicode\n\
                            Only affects --roman, --morse and --factors\n\
    -l, --lower         Lowercase\n\
                            Only affects --to-base (with <BASE> > 10) and --greek\n\
        --upper         Uppercase (default)\n\
\n\
        --help          Display this help and exit\n\
        --version       Output version information and exit\n\
\n\
Examples:\n\
    Output everything for -1234\n\
    $ " << programname
		 << " -- -1234\n\
\n\
    Output 0361100 (octal), 123456 and 0x1E240 (hexadecimal) in binary\n\
    $ " << programname
		 << " --binary 0361100 123456 0x1E240\n\
\n\
    Output 11110001001000000 (binary) in base 36\n\
    $ " << programname
		 << " --from-base 2 --to-base 36 11110001001000000\n\
\n\
    Output 123456 in all the bases (Bash syntax)\n\
    $ for i in {2..36}; do echo \"Base $i: $("
		 << programname << " --to-base \"$i\" 123456 | sed -n 's/^.*: //p')\"; done\n\
\n\
    Output 1234 as Unicode Roman numerals\n\
    $ " << programname
		 << " --roman --unicode 1234\n\
\n\
    Convert 1T from ‘SI’ to ‘IEC’ scales (Bash syntax)\n\
    $ " << programname
		 << " --to=iec-i \"$(numfmt --from=si 1T)\"\n\
\n\
    Output the current time (hour and minute) as text (Bash syntax)\n\
    $ " << programname
		 << " --from-base 10 --text \"$(date +%l)\" \"$(date +%M)\" | sed -n 's/^.*: //p'\n\
\n\
    Output the aliquot sum for 6, 28, 496, 8128, 33550336, 8589869056 and 137438691328\n\
    $ " << programname
		 << " --aliquot 6 28 496 8128 33550336 8589869056 137438691328\n\
\n\
    Output if 3, 7, 31, 127, 8191, 131071 and 524287 are prime or composite\n\
    $ " << programname
		 << " --prime 3 7 31 127 8191 131071 524287\n\
\n\
    Output 1234.25 with Unicode fractions\n\
    $ " << programname
		 << " --float --fracts 1234.25\n\
\n";
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
	enum scale_type scale_to = scale_none;
	int arg = 'a';

	setlocale(LC_ALL, "");

	// https://stackoverflow.com/a/38646489

	static struct option long_options[] = {
		{"int", no_argument, NULL, 'i'},
		{"locale", no_argument, NULL, 'e'},
		{"grouping", no_argument, NULL, 'e'},
		{"from-base", required_argument, NULL, FROM_BASE_OPTION},
		{"to-base", required_argument, NULL, 'b'},
		{"binary", no_argument, NULL, BINARY_OPTION},		  // 2
		{"ternary", no_argument, NULL, TERNARY_OPTION},		  // 3
		{"quaternary", no_argument, NULL, QUATERNARY_OPTION}, // 4
		{"quinary", no_argument, NULL, QUINARY_OPTION},		  // 6
		{"octal", no_argument, NULL, 'o'},
		{"decimal", no_argument, NULL, DECIMAL_OPTION}, // 1
		{"duo", no_argument, NULL, DUO_OPTION},			// j
		{"hex", no_argument, NULL, 'x'},
		{"viges", no_argument, NULL, VIGES_OPTION}, // k
		// {"base36", no_argument, NULL, BASE36_OPTION}, // q
		{"roman", no_argument, NULL, 'r'},
		{"greek", no_argument, NULL, 'g'},
		{"morse", no_argument, NULL, 'm'},
		{"braille", no_argument, NULL, BRAILLE_OPTION}, // w
		{"text", no_argument, NULL, 't'},
		{"special", no_argument, NULL, SPECIAL_OPTION},
		{"to", required_argument, NULL, TO_OPTION},
		{"factors", no_argument, NULL, 'p'},
		{"divisors", no_argument, NULL, 'd'},
		{"aliquot", no_argument, NULL, 's'},
		{"prime", no_argument, NULL, 'n'},
		{"all", no_argument, NULL, 'a'},
		{"float", no_argument, NULL, 'f'},
		{"fracts", no_argument, NULL, 'c'},
		{"ascii", no_argument, NULL, ASCII_OPTION}, // y
		{"unicode", no_argument, NULL, 'u'},
		{"lower", no_argument, NULL, 'l'},
		{"upper", no_argument, NULL, UPPER_OPTION}, // z
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}};

	int option_index = 0;
	int c = 0;

	while ((c = getopt_long(argc, argv, "ab:cdefgilmnoprstux", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		// case 0:
		// break;
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
		case 'a':
			arg = c;
			break;
		case FROM_BASE_OPTION:
			frombase = atoi(optarg);
			if (frombase < 2 or frombase > 36)
			{
				cerr << "Error: <BASE> must be 2 - 36.\n";
				return 1;
			}
			break;
		case 'b':
			tobase = atoi(optarg);
			if (tobase < 2 or tobase > 36)
			{
				cerr << "Error: <BASE> must be 2 - 36.\n";
				return 1;
			}
			break;
		case 'c':
			arg = c;
			unicode = true;
			break;
		case 'd':
			arg = c;
			break;
		case 'e':
			arg = c;
			break;
		case 'f':
			integer = false;
			break;
		case 'g':
			arg = c;
			unicode = true;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
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
		case 'm':
			arg = c;
			unicode = true;
			break;
		case 'n':
			arg = c;
			break;
		case 'o':
			tobase = 8;
			break;
		case 'p':
			arg = c;
			break;
		// case BASE36_OPTION:
		// tobase = 36;
		// break;
		case TO_OPTION:
			arg = c;
			scale_to = XARGMATCH("--to", optarg, scale_to_args, (sizeof scale_to_args / sizeof scale_to_args[0]), scale_to_types);
			break;
		case 'r':
			arg = c;
			break;
		case 's':
			arg = c;
			break;
		case 't':
			arg = c;
			break;
		case 'u':
			unicode = true;
			break;
		case 'v':
			cout << "Numbers 1.0\n\n";
			return 0;
		case BRAILLE_OPTION:
			arg = c;
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
			cerr << "Error: Option not available for integer numbers.\n";
			return 1;
		}
	}
	else
	{
		if (frombase or tobase or arg == 'r' or arg == 'g' or arg == 'm' or arg == BRAILLE_OPTION or arg == 't' or arg == 'p' or arg == 'd' or arg == 's' or arg == 'n')
		{
			cerr << "Error: Option not available for floating point numbers.\n";
			return 1;
		}
	}

	if (special and arg != 'a' and arg != 't')
	{
		cerr << "Error: --special is only available for integer numbers with --all and --text\n";
		return 1;
	}

	if (optind < argc)
	{
		if (integer)
		{
			for (int i = optind; i < argc; ++i)
			{
				const intmax_t ll = strtoimax(argv[i], NULL, frombase);
				if (errno == ERANGE)
				{
#if HAVE_GMP
					mpz_class num(argv[i], frombase);
					cout << num << ": ";
					if (tobase)
						mpz_out_str(stdout, uppercase ? -tobase : tobase, num.get_mpz_t());
					else if (arg == 'a')
						outputall(num, unicode, uppercase);
					else if (arg == 'e')
						gmp_printf("%'Zd", num.get_mpz_t());
					else if (arg == 'm')
						cout << outputmorsecode(num, unicode);
					else if (arg == BRAILLE_OPTION)
						cout << outputbraille(num);
					else if (arg == 'p')
						cout << outputfactors(num, unicode, false);
					else if (arg == 'd')
						cout << outputdivisors(num, false);
					else if (arg == 's')
						cout << outputaliquot(num, false);
					else if (arg == 'n')
						cout << outputprime(num, false);
					else
					{
						cerr << "Error: Option not available for arbitrary-precision integer numbers.\n";
						// return 1;
					}
					cout << endl;
#else
					cerr << "Error: Integer number too large to input: '" << argv[i] << "' (" << strerror(errno) << "). Program does not support arbitrary-precision integer numbers, because it was not built with GNU Multiple Precision (GMP).\n";
					return 1;
#endif
				}
				else
				{
					cout << ll << ": ";
					if (tobase)
						cout << outputbase(ll, tobase, uppercase);
					else if (arg == 'a')
						outputall(ll, unicode, uppercase, special);
					else if (arg == 'e')
						printf("%'" PRIdMAX, ll);
					else if (arg == TO_OPTION)
						cout << outputunit(ll, scale_to, false);
					else if (arg == 'r')
						cout << outputroman(ll, unicode, false);
					else if (arg == 'g')
						cout << outputgreek(ll, uppercase, false);
					else if (arg == 'm')
						cout << outputmorsecode(ll, unicode);
					else if (arg == BRAILLE_OPTION)
						cout << outputbraille(ll);
					else if (arg == 't')
						cout << outputtext(ll, special);
					else if (arg == 'p')
						cout << outputfactors(ll, unicode, false);
					else if (arg == 'd')
						cout << outputdivisors(ll, false);
					else if (arg == 's')
						cout << outputaliquot(ll, false);
					else if (arg == 'n')
						cout << outputprime(ll, false);
					cout << endl;
				}
			}
		}
		else
		{
			for (int i = optind; i < argc; ++i)
			{
				const long double ld = strtold(argv[i], NULL);
				if (errno == ERANGE)
				{
					cerr << "Error: Floating point number too large to input: '" << argv[i] << "' (" << strerror(errno) << ")\n";
					return 1;
				}

				cout << ld << " (" << floattostring(ld) << "): ";
				if (arg == 'a')
					outputall(ld);
				else if (arg == 'e')
					printf("%'.*Lg", LDBL_DIG, ld);
				else if (arg == TO_OPTION)
					cout << outputunit(ld, scale_to, false);
				else if (arg == 'c')
					cout << outputlabel(ld);
				cout << endl;
			}
		}
	}
	else
	{
		cerr << "Error: No <NUMBERS> were provided.\n";
		return 1;
	}

	return 0;
}
