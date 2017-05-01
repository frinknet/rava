for x in $@; do
	if [ "$1" != "$x" ]; then
		echo $1 $x
 fi
done
