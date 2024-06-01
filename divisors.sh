#!/bin/bash

# Teal Dulcet
# Outputs the prime factors, divisors and aliquot sum of a number.
# ./divisors.sh <number>

# set -e

if [[ $# -ne 1 ]]; then
	echo "Usage: $0 <number>" >&2
	exit 1
fi

if ! command -v bc >/dev/null; then
	echo "Error: bc command is required to support arbitrary precision numbers." >&2
	exit 1
fi

# $RANDOM
n=$1

# if [[ $n -lt 1 ]]; then
if (($(bc <<<"$n < 1"))); then
	echo "Error: <number> must be a number greater than 0." >&2
	exit 1
fi

echo "$n:"
printf "\tLocale:\t\t\t\t%'d\n\n" "$n"

if ! FACTORS=$(factor "$n" 2>&1); then
	echo "Error: $FACTORS"
	exit 1
fi
FACTORS=($(echo "$FACTORS" | sed -n 's/^.*: //p'))
echo -e "\tPrime Factors:\t\t\t${FACTORS[*]}"
declare -A AFACTORS=()
for prime in "${FACTORS[@]}"; do
	((++AFACTORS[$prime]))
done
FACTORS=($(printf '%s\n' "${FACTORS[@]}" | uniq))
DIVISORS=(1)
for prime in "${FACTORS[@]}"; do
	exponent=${AFACTORS[$prime]}
	count=${#DIVISORS[*]}
	multiplier=1
	for ((j = 0; j < exponent; ++j)); do
		multiplier=$(bc <<<"$multiplier * $prime")
		# ((multiplier *= prime))
		DIVISORS+=($(for ((i = 0; i < count; ++i)); do echo "${DIVISORS[i]} * $multiplier"; done | bc))
		# for (( i = 0; i < count; ++i )); do
			# DIVISORS+=( $((DIVISORS[i] * multiplier)) )
		# done
	done
done
unset 'DIVISORS[-1]'
DIVISORS=($(printf '%s\n' "${DIVISORS[@]}" | sort -n))
echo -e "\tDivisors:\t\t\t${DIVISORS[*]}"
asum=0
for k in "${DIVISORS[@]}"; do
	asum=$(bc <<<"$asum + $k")
	# ((asum += k))
done
echo -e "\tAliquot sum (sum of all divisors):\t$asum"
