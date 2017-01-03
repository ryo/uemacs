/^\t/ {
	printf( "s/	%s[	 ]*}/	%s}/g\n", $1, $2 )
}

/^$/ {
	print( "" )
}
