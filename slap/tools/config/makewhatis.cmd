# Determine the comand-line prefix used to update the manpage indexes of
# a named manpage directory.
# Print it as a Makefile definition to stdout, and set equivalent
# environment variables.
#

echo "determining how to update manpage indexes" 1>&2


# assume worst, until proven otherwise
#
prog="tools/nothing"

# build list of directories to search
#
list="`echo ${PATH} | sed -e 's/:/ /g'`"
list="$list /bin /usr/bin /usr/sbin /sbin"

# search for likely program:
#
for d in $list ; do
	if [ -f ${d}/catman ] && [ -x $d/catman ] ; then
		prog="$d/catman -w -M"
	elif [ -f $d/makewhatis ] && [ -x $d/makewhatis ] ; then
		prog="$d/makewhatis"
	fi
done

cat << EOT

# Command-prefix used to update manpage index for a named directory:
#
MAKEWHATIS=$prog

EOT

export MAKEWHATIS
