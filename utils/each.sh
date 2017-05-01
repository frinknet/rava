for x in $@; do
	if [ "$1" != "$x" ]; then
		echo "$1\t$x"
 fi
done
