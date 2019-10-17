#!/bin/bash

# Teal Dulcet
# Outputs the prime factors, divisors and aliquot sum of a number.
# ./divisors.sh <number>

# set -e

if [[ "$#" -ne 1 ]]; then
	echo "Usage: $0 <number>" >&2
	exit 1
fi

if ! command -v bc >/dev/null; then
	echo "Error: bc command is required to support arbitrary precision numbers." >&2
	exit 1
fi

n=$1

# if [[ "$n" -lt 1 ]]; then
if [[ $(bc <<< "$n < 1") -ne 0 ]]; then
	echo "Error: <number> must be a number greater than 0." >&2
	exit 1
fi

echo "$n:"
echo -e "\tLocale:\t\t\t\t$(printf "%'d" "$n")\n"

if ! FACTORS=$(factor "$n" 2>&1); then
	echo "Error: $FACTORS"
	exit 1
fi
FACTORS=( $(echo "$FACTORS" | sed -n 's/^.*: //p') )
echo -e "\tPrime Factors:\t\t\t${FACTORS[*]}"
DIVISORS=()
temp=$(((2**${#FACTORS[@]})-1))
for (( i = 0; i < temp; ++i )); do
	idx=$i
	divisor=1
	for (( j = 0; j < ${#FACTORS[@]}; ++j )); do
		if (( idx % 2 )); then
			divisor=$(bc <<< "$divisor * ${FACTORS[$j]}")
			# ((divisor *= ${FACTORS[$j]}))
		fi
		((idx>>=1))
	done
	DIVISORS+=( "$divisor" )
done
DIVISORS=( $(printf "%s\n" "${DIVISORS[@]}" | sort -n -u) )
echo -e "\tDivisors:\t\t\t${DIVISORS[*]}"
asum=0
for k in "${DIVISORS[@]}"; do
	asum=$(bc <<< "$asum + $k")
	# ((asum += k))
done
echo -e "\tAliquot sum (sum of all divisors):\t$asum"
