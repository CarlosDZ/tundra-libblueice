# The exported symbols are the released ones, no more and no fewer.
lib=$1
dir=$(dirname "$0")
got=$(nm -D --defined-only "$lib" | awk '{print $3}' | LC_ALL=C sort)
if [ "$got" = "$(LC_ALL=C sort "$dir/symbols")" ]; then
	echo "ok - exported symbols match tests/abi/symbols"
	echo "# 1 passed, 0 failed"
else
	echo "not ok - exported symbols differ from tests/abi/symbols"
	echo "$got" | sed 's/^/#   /'
	echo "# 0 passed, 1 failed"
	exit 1
fi
