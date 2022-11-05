[![Build Status](https://travis-ci.com/tdulcet/Numbers-Tool.svg?branch=master)](https://travis-ci.com/tdulcet/Numbers-Tool)
[![Actions Status](https://github.com/tdulcet/Numbers-Tool/workflows/CI/badge.svg?branch=master)](https://github.com/tdulcet/Numbers-Tool/actions)

# Numbers Tool

Outputs numbers in various representations

Copyright © 2019 Teal Dulcet

This program outputs numbers in various representations, including:

* Integer numbers
	* Locale format with digit grouping (same as `printf "%'d" <NUMBER>` or `numfmt --grouping <NUMBER>`)
	* \*Arbitrary bases 2 - 36
	* Auto-scale to unit (similar to `numfmt --to=<UNIT> <NUMBER>`, but with more precision)
	* [Roman numerals](https://en.wikipedia.org/wiki/Roman_numerals)
	* [Greek numerals](https://en.wikipedia.org/wiki/Greek_numerals)
	* \*[Morse code](https://en.wikipedia.org/wiki/Morse_code)
	* \*[Braille](https://en.wikipedia.org/wiki/English_Braille#Formatting_marks)
	* [Text](https://github.com/tdulcet/sliding-text-plus-plus) (spelled out)
	* \*[Prime factors](https://en.wikipedia.org/wiki/Integer_factorization#Prime_decomposition) (same as `factor <NUMBER>`)
	* \*[Divisors](https://en.wikipedia.org/wiki/Divisor)
	* \*[Aliquot sum](https://en.wikipedia.org/wiki/Aliquot_sum) (sum of all divisors) and if it is [perfect](https://en.wikipedia.org/wiki/Perfect_number), [deficient](https://en.wikipedia.org/wiki/Deficient_number) or [abundant](https://en.wikipedia.org/wiki/Abundant_number)
	* \*If it is [prime](https://en.wikipedia.org/wiki/Prime_number) or [composite](https://en.wikipedia.org/wiki/Composite_number)
* Floating point numbers
	* Locale format with digit grouping (same as `printf "%'g" <NUMBER>` or `numfmt --grouping <NUMBER>`)
	* Auto-scale to unit (similar to `numfmt --to=<UNIT> <NUMBER>`, but with more precision)
	* Convert [fractions](https://en.wikipedia.org/wiki/Number_Forms) and [mathematical constants](https://en.wikipedia.org/wiki/Mathematical_constant) to Unicode characters

\* Supports arbitrary-precision/bignums

It is designed as an extension to the existing [factor](https://www.gnu.org/software/coreutils/manual/html_node/factor-invocation.html) and [numfmt](https://www.gnu.org/software/coreutils/manual/html_node/numfmt-invocation.html) commands from GNU Coreutils.

❤️ Please visit [tealdulcet.com](https://www.tealdulcet.com/) to support this program and my other software development.

## Usage

Requires support for C++11 and [GNU Coreutils](https://www.gnu.org/software/coreutils/), which is included with most Linux distributions.

Support for arbitrary-precision integers requires the [GNU Multiple Precision](https://gmplib.org/) (GMP) library. On Ubuntu and Debian, run `sudo apt-get update` and `sudo apt-get install libgmp3-dev`.

Compile without GMP:

GCC: `g++ -Wall -g -O3 -flto numbers.cpp -o numbers`\
Clang: `clang++ -Wall -g -O3 -flto numbers.cpp -o numbers`

Compile with GMP:

GCC: `g++ -Wall -g -O3 -flto numbers.cpp -o numbers -DHAVE_GMP -lgmpxx -lgmp`\
Clang: `clang++ -Wall -g -O3 -flto numbers.cpp -o numbers -DHAVE_GMP -lgmpxx -lgmp`

Run: `./numbers [OPTION(S)]... [NUMBER(S)]...`\
If any of the `NUMBERS` are negative, the first must be preceded by a `--`. See [Help](#help) below for full usage information.

If you want this program to be available for all users, install it. Run: `sudo mv numbers /usr/local/bin/numbers` and `sudo chmod +x /usr/local/bin/numbers`.

On Linux distributions with GNU Coreutils older than 9.0, including [Ubuntu](https://bugs.launchpad.net/ubuntu/+source/coreutils/+bug/696618) and [Debian](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=608832), the factor command (part of GNU Coreutils) is built without arbitrary-precision/bignum support. If this is the case on your system and you are compiling this program with GMP, you will also need to build the factor command with GMP. You can check by running this and checking for any "too large" errors (note that if it was built with arbitrary-precision/bignum support, this may take a few minutes to complete):

```bash
factor 9223372036854775807 18446744073709551615 170141183460469231731687303715884105727 340282366920938463463374607431768211455 57896044618658097711785492504343953926634992332820282019728792003956564819967 115792089237316195423570985008687907853269984665640564039457584007913129639935
```

The above is `factor` 2<sup>63</sup> - 1, 2<sup>64</sup> - 1, 2<sup>127</sup> - 1, 2<sup>128</sup> - 1, 2<sup>255</sup> - 1 and 2<sup>256</sup> - 1.

Requires Make and the GNU C compiler.

### Build GNU Coreutils

<details>
  <summary>Instructions</summary>

```bash
# Save current directory
DIRNAME=$PWD

cd /tmp/
git clone --depth 1 https://github.com/coreutils/coreutils.git
git clone https://github.com/coreutils/gnulib.git
GNULIB_SRCDIR="$PWD/gnulib"
cd coreutils/
./bootstrap --gnulib-srcdir="$GNULIB_SRCDIR"
./configure # If the next command fails, try rerunning this command with the --disable-gcc-warnings flag
make -j "$(nproc)" CFLAGS="-g -O3 -flto"
make -j "$(nproc)" check CFLAGS="-g -O3 -flto" RUN_EXPENSIVE_TESTS=yes RUN_VERY_EXPENSIVE_TESTS=yes

# Copy factor command to starting directory
cp ./src/factor "$DIRNAME/"
```
</details>

### Build uutils coreutils

[uutils coreutils](https://github.com/uutils/coreutils) is a cross-platform Rust rewrite of the GNU Coreutils, but [does not currently support arbitrary-precision/bignums](https://github.com/uutils/coreutils/issues/1559).

<details>
  <summary>Instructions</summary>

Requires Rust: `curl https://sh.rustup.rs -sSf | sh`

```bash
# Save current directory
DIRNAME=$PWD

cd /tmp/
git clone --depth 1 https://github.com/uutils/coreutils.git
cd coreutils/
make PROFILE=release
make -j "$(nproc)" test

# Copy factor command to starting directory
cp ./target/release/factor "$DIRNAME/"
```
</details>

---

Then change the `FACTOR` variable near the top of the [numbers.cpp](numbers.cpp) file to the new factor commands location, for example: "./factor".

## Help

```
$ numbers --help
Usage:  numbers [OPTION(S)]... [NUMBER(S)]...
or:     numbers <OPTION>
If any of the NUMBERS are negative, the first must be preceded by a --. If none are specified on the command line, read them from standard input. NUMBERS can be in Octal, Decimal or Hexadecimal. Use --from-base to specify a different base. See examples below.

Options:
    Mandatory arguments to long options are mandatory for short options too.
    -i, --int           Integer numbers (default)
        -e, --locale        Output in Locale format with digit grouping (same as 'printf "%'d" <NUMBER>' or 'numfmt --grouping <NUMBER>')
            --grouping
            --from-base <BASE> Input in bases 2 - 36
                                   Supports arbitrary-precision/bignums
        -b, --to-base <BASE>   Output in bases 2 - 36
                                   Supports arbitrary-precision/bignums
                --binary           Output in Binary      (same as --to-base 2)
                --ternary          Output in Ternary     (same as --to-base 3)
                --quaternary       Output in Quaternary  (same as --to-base 4)
                --quinary          Output in Quinary     (same as --to-base 6)
            -o, --octal            Output in Octal       (same as --to-base 8)
                --decimal          Output in Decimal     (same as --to-base 10)
                --duo              Output in Duodecimal  (same as --to-base 12)
            -x, --hex              Output in Hexadecimal (same as --to-base 16)
                --viges            Output in Vigesimal   (same as --to-base 20)
            --to <UNIT>     Auto-scale output numbers to <UNIT> (similar to 'numfmt --to=<UNIT> <NUMBER>', but with more precision)
                                Run 'numfmt --help' for UNIT options
        -r, --roman         Output as Roman numerals
                                Numbers 1 - 3999
        -g, --greek         Output as Greek numerals
                                Numbers 1 - 9999, implies --unicode
        -m, --morse         Output as Morse code
                                Supports arbitrary-precision/bignums
            --braille       Output as Braille
                                Implies --unicode, supports arbitrary-precision/bignums
        -t, --text          Output as text
                --special       Use special words, including: pair, dozen, baker's dozen, score, gross and great gross
        -p, --factors       Output prime factors (same as 'factor <NUMBER>')
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision
            -h, --exponents     Output repeated factors in form p^e unless e is 1 (same as 'factor --exponents <NUMBER>')
        -d, --divisors      Output divisors
                                Numbers > 0, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision
        -s, --aliquot       Output aliquot sum (sum of all divisors) and if it is perfect, deficient or abundant
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision
        -n, --prime         Output if it is prime or composite
                                Numbers > 1, supports arbitrary-precision/bignums if factor command was also built with GNU Multiple Precision
        -a, --all           Output all of the above (default)

    -f, --float         Floating point numbers
        -e, --locale        Output in Locale format with digit grouping (same as 'printf "%'g" <NUMBER>' or 'numfmt --grouping <NUMBER>')
            --grouping
            --to <UNIT>     Auto-scale output numbers to <UNIT> (similar to 'numfmt --to=<UNIT> <NUMBER>', but with more precision)
                                Run 'numfmt --help' for UNIT options
        -c, --fracts        Convert fractions and mathematical constants to Unicode characters
                                Supports all Unicode fractions, Pi and e constants, implies --unicode
        -a, --all           Output all of the above (default)

        --ascii         ASCII (default)
    -u, --unicode       Unicode
                            Only affects --roman, --morse and --factors
    -l, --lower         Lowercase
                            Only affects --to-base (with <BASE> > 10) and --greek
        --upper         Uppercase (default)

        --help          Display this help and exit
        --version       Output version information and exit

Examples:
    Output everything for -1234
    $ numbers -- -1234

    Output 0361100 (octal), 123456 and 0x1E240 (hexadecimal) in binary
    $ numbers --binary 0361100 123456 0x1E240

    Output 11110001001000000 (binary) in base 36
    $ numbers --from-base 2 --to-base 36 11110001001000000

    Output 123456 in all the bases (Bash syntax)
    $ for i in {2..36}; do echo "Base $i: $(numbers --to-base "$i" 123456 | sed -n 's/^.*: //p')"; done

    Output 1234 as Unicode Roman numerals
    $ numbers --roman --unicode 1234

    Convert 1T from ‘SI’ to ‘IEC’ scales
    $ numfmt --from=si 1T | numbers --to=iec-i

    Output the current time (hour and minute) as text
    $ date +%l%n%M | numbers --from-base 10 --text | sed -n 's/^.*: //p'

    Output the aliquot sum for 6, 28, 496, 8128, 33550336, 8589869056 and 137438691328
    $ numbers --aliquot 6 28 496 8128 33550336 8589869056 137438691328

    Output if 3, 7, 31, 127, 8191, 131071 and 524287 are prime or composite
    $ numbers --prime 3 7 31 127 8191 131071 524287

    Output 1234.25 with Unicode fractions
    $ numbers --float --fracts 1234.25

```

### Comparison of `--to` option

This program vs the [numfmt](https://www.gnu.org/software/coreutils/manual/html_node/numfmt-invocation.html) command from GNU Coreutils. This program uses up to 7 characters, while `numfmt` uses up to 5.

Number | This program | `numfmt`
--- | ---: | ---:
1234 | 1.235K | 1.3K
12345 | 12.35K | 13K
123456 | 123.5K | 124K
999999 | 1000K | 1.0M
1000000 | 1M | 1.0M
1000001 | 1.000M | 1.1M

The examples above use `--to=si`.

## Contributing

Pull requests welcome! Ideas for contributions:

* Add more options
	* Output if the number is a [Mersenne prime](https://en.wikipedia.org/wiki/Mersenne_prime) (suggested by Daniel Connelly)
* Add more examples
* Improve the performance
	* Parallelize the [factor](https://www.gnu.org/software/coreutils/manual/html_node/factor-invocation.html) command from GNU Coreutils to improve its performance on large numbers
* Support outputting Roman numerals greater than 3999 and Greek numerals greater than 9999
* Add tests
* Submit an enhancement request to [GNU Coreutils](https://www.gnu.org/software/coreutils/) to get this included as an extension to the existing [factor](https://www.gnu.org/software/coreutils/manual/html_node/factor-invocation.html) and [numfmt](https://www.gnu.org/software/coreutils/manual/html_node/numfmt-invocation.html) commands
* Port to other languages (C, Rust, etc.)

Thanks to [Daniel Connelly](https://github.com/Danc2050) for providing feedback on the usage information!
