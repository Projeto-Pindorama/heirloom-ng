
#
# man command. Gunnar Ritter, Freiburg i. Br., Germany, August 2001
#

#
# Copyright (c) 2003 Gunnar Ritter
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute
# it freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source distribution.
#

# Sccsid @(#)man.ksh	1.47 (gritter) 4/25/05

if test "x$BASH_VERSION" != x
then
	shopt -s extglob
	printn="printf %s"
	printr="printf %s\\n"
	lowercase() {
		eval $1=\"`echo $2 | dd conv=lcase 2>/dev/null`\"
	}
else
	# Assume a Korn shell.
	printn="print -n"
	printr="print -r"
	lowercase() {
		case $KSH_VERSION in
		*"PD KSH"*)
			eval $1=\"`echo $2 | dd conv=lcase 2>/dev/null`\"
			;;
		*)
			eval typeset -l $1=\"$2\"
			;;
		esac
	}
fi
