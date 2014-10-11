NOTICE:
=======

These programs need to be able to run on a wide variety of UNIX, UNIX-derived
and UNIX-like systems, including some as old as 7th Edition UNIX (1979).

Thus they are written in to use ONLY the most minimal, generic, available and
portable set of UNIX command-line facilities, ie: conforming to the "sh" manpage
from 7th Edition UNIX ("original Bourne shell"), and excluding programs that
have (at one time or another) ever been considered "optional".

Some facilities that should be avoided by *these* scripts, in order to meet
the above portability objectives, include:

	non-Bourne-compatible shells or syntax, eg:
		syntax/facilities specific to the Bourne-Again-Shell
		Kornish syntax/facilities
		syntax-facilities specific to C-Shell, zsh, ash, posh, etc
		syntax introduced in new ports of Bourne's shell, eg:
			the "setvar" builtin in the FreeBSD 6.x /bin/sh,
			and so on.

	basename with multiple arguments
	???shift with arguments (ie: use "shift ; shift" instead of "shift 2")
	multiple arguments to the "." shell-builtin command
	sed -e option not followed by whitespace, ie:
		"sed -e's/...//'" is NOT portable, use
		"sed -e 's/...//'" instead.
	set address-pattern seperators other than a slash
	unset
	type, file
	typeset
	${name:...}	# colon-form not supported by 7th Edition UNIX sh
	cut, paste, join
	awk facilities beyond the most basic (ie: beyond 7th Edition UNIX)
	nawk (and gawk, mawk, etc)
	m4 (7th Edition UNIX has this, but many other UNIXen view m4 as an
		"optional" component which is not installed by default).
	shell-functions
	dynamic reassignments of IFS within for loops - works OK in 7th Ed.
		/bin/sh, but some more recent Bourne derivatives
		don't handle this properly (eg: FreeBSD 6.3)
	the "return" statement anywhere, even in ".-included" scripts
	arguments to the "." command
	nested shell command-substitutions
	getopts, getopt
	extended regular-expressions (in any context)
	dirname
	dirs, pushd, popd
	true, false
	install
	assuming support for filenames longer than 14 characters (it is OK to
		*handle* longer filenames, but not to *require* them).
	pax, cpio, tp
	tar with pathnames longer than 99 characters
	tar flags other than -c -v -f -x -t
	echo with flags or metacharacters
	cat with flags
	the [ command (use "test" instead)
	diff3
	dircmp
	symbolic file-access permissions
	explicit file-access permissions outside the range 0-0777
	xargs
	perl
	printf
	print
	select
	mkdir with flags
	cp with flags
	ln with flags
	rmdir with flags
	rm with flags other than -r and -f
	rdist, rcp, rsh
	split, csplit
	tsort
	sort with flags other than +#, -# and -b

The above list of "things to be avoided" is not complete: it just lists the
*most obvious* "introduced since 7th Edition UNIX" facilities, those ones most
likely to be used "by accident".

Particular things you *can* rely on include:

	* the file argument to the "." command is subject to the same
	  $PATH-search as is used for external commands.

	* basic non-internationalised pattern-matched sed substitution
	  operations can be performed, including tagging, eg:
	  	sed -e 's/:\([-+_A-Za-z0-9].*\):/:/g'

	* 
