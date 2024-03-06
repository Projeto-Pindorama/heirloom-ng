#!/usr/5bin/sed -nf
# sedtris.sed - sed tetris
# 26th of May, 2008
# Julia Jomantaite <julia.jomantaite@gmail.com>
#
# Changes by Gunnar Ritter <gunnarr@acm.org> to make it work with Heirloom sed;
# not yet completed
#
# Sccsid @(#)sedtris.sed	1.2 (gritter) 8/15/09
#
# from http://uuner.doslash.org/forfun
#
# This is free software. You may distribute it under the terms of the
# Poetic License. http://genaud.net/2005/10/poetic-license/
#
# (c) 2008 Julia Jomantaite
#
# This work `as-is' we provide.
# No warranty, express or implied.
# We've done our best,
# to debug and test.
# Liability for damages denied.
#
# Permission is granted hereby,
# to copy, share, and modify.
# Use as is fit,
# free or for profit.
# On this notice these rights rely.
#
1{
s/.*/ 2a2a2a2a|3a3a3a3a3a3a3a3a3a3a|2a2a2a2a~/
s/[^~]*~$/&&&/
s/.*/& 2a2a2a2a|0a0a0a0a0a0a0a0a0a0a|2a2a2a2a~/
s/[^~]*~$/&&&&&&&&&&&&&&&&&&&&/
s// 2a2a2a2a|4a4a4a4a4a4a4a4a4a4a|2a2a2a2a~/
s/[^~]*~$/&&&/
s/.*/~&/
s/$/#....,,,~/
s/$/#AAAA AAAA APPA APPA~/
s/$/#AAAA ANAA LPOA AHAA~/
s/$/#AAAA BKIA FPFA CKEA~/
s/$/#AAAA EDCA GPJA IMBA~/
s/$/#ACIA EGME BDJB ACIA~/
s/$/#AAAA IKEA FPFA BKCA~/
s/$/#AAAA CGBA MPDA EJIA~/
s/$/#NEXTAAAA ANAA LPOA AHAA 5~/
s/$/#SCORE0~/
h
b display
}

/1/{s/1//;x;s/#\.\.*/&./;s/#\.\{8\}/#./;x
}

/^w/b rotate
/^a/b left
/^d/b right
/^s/b nothing
/^\[A/b rotate
/^\[D/b left
/^\[C/b right
/^\[B/b nothing
/^$/b nothing
/^[z ]/{
g
s/#\.*/&.../
s/#\.\{8\}/#./
s/$/#FALL~/
h
b nothing
}
b display

:rotate
g
s/#\.*/&./
s/#\.\{8\}/#./
/[14-9x][e-h]/ !{/2[e-h]/!y/abcdefghijklmnop/acegikmobdfhjlnp/
}
h
b display

:left
g
s/#\.*/&../
s/#\.\{8\}/#./
/|[0-9x][i-p]/ !{/[14-9x][a-p][0-9x][i-p]/ !{
s/\([0-9x]\)a\(|\{0,1\}\)\([0-9x]\)\([b-p]\)/\1\4\2\3a/g
s/\([b-p]\)\(|\{0,1\}\)\([0-9x]\)a\(|\{0,1\}\)\([0-9x]\)\([b-p]\)/\1\2\3\6\4\5a/g
s/\([b-p]\)\(|\{0,1\}\)\([0-9x]\)a\(|\{0,1\}\)\([0-9x]\)\([b-p]\)/\1\2\3\6\4\5a/g
s/\([b-p]\)\(|\{0,1\}\)\([0-9x]\)a\(|\{0,1\}\)\([0-9x]\)\([b-p]\)/\1\2\3\6\4\5a/g
}
}
h
b display

:right
g
s/#\.*/&./
s/#\.\{8\}/#./
/[i-p]|/ !{/[i-p][14-9x]/!{
s/\([0-9x]\)\([b-p]\)\(|\{0,1\}\)\([0-9x]\)a/\1a\3\4\2/g
s/\([0-9x]\)\([b-p]\)\(|\{0,1\}\)\([0-9x]\)a\(|\{0,1\}\)\([0-9x]\)\([b-p]\)/\1a\3\4\2\5\6\7/g
s/\([0-9x]\)\([b-p]\)\(|\{0,1\}\)\([0-9x]\)a\(|\{0,1\}\)\([0-9x]\)\([b-p]\)/\1a\3\4\2\5\6\7/g
s/\([0-9x]\)\([b-p]\)\(|\{0,1\}\)\([0-9x]\)a\(|\{0,1\}\)\([0-9x]\)\([b-p]\)/\1a\3\4\2\5\6\7/g
}
}
h
b display

:nothing
g

#New brick
/~ \(2a\)*|\([15-9x]a\)*|\(2a\)*~/ !{
/[b-p]/ !{
#Set % before all 4-letter "words"
t e
:e
s/\(#NEXT\(\( \)\{0,1\}%[A-P]\)*\)\( \)\{0,1\}\([A-P]\)/\1\4%\5/
t e

#Set @ before probable new letters
s/^\(~[^~]*~[^~]*~.\{17\}\)\(..\)\(..\)\(..\)\(..\)/\1@\2@\3@\4@\5/
s/^\(~[^~]*~[^~]*~[^~]*~.\{17\}\)\(..\)\(..\)\(..\)\(..\)/\1@\2@\3@\4@\5/
t blank2
:blank2
/#NEXT\([^%]*%\)\{13\}[A-H]%[A-H]%[A-H]%[A-H]/{
s/^\(~[^~]*~[^~]*~[^~]*~[^~]*~.\{17\}\)\(..\)\(..\)\(..\)\(..\)/\1@\2@\3@\4@\5/
/#NEXT\([^%]*%\)\{9\}[A-H]%[A-H]%[A-H]%[A-H]/{
s/^\(~[^~]*~[^~]*~[^~]*~[^~]*~[^~]*~.\{17\}\)\(..\)\(..\)\(..\)\(..\)/\1@\2@\3@\4@\5/
b v2
}
b v1
}
s/^\(~.\{17\}\)\(..\)\(..\)\(..\)\(..\)/\1@\2@\3@\4@\5/
:v1
s/^\(~[^~]*~.\{17\}\)\(..\)\(..\)\(..\)\(..\)/\1@\2@\3@\4@\5/
:v2

#Add real new letters (capital)
t f
:f
/^\([^@]*\)[15-9x]@.\([^%]*\)%[I-P]/s/$/#OVER~/
s/^\([^@]*\)@.\([^%]*\)%\([A-P]\)/\1\3\2\3/
t f

#Decapitalization
h
s/^\(~[^~]*\)\{6\}~//
x
s/^\(\(~[^~]*\)\{6\}\)\(.*\)$/\1/
y/ABCDEFGHIJKLMNOP/abcdefghijklmnop/
x
H
#Add "Current"
g
s/\n/~/
s/#CURRENT.~//g
s/\(#NEXT[^~]*\)\([0-9x]\)\(.*\)$/\1\2\3#CURRENT\2~/

#Add NEXT
:g
/\./{
s/#/$/
s/\./@/
b g
}
s/@/./g
s/^\([^#]*\)#\([^~]*\)\(.*\)\(#NEXT[^~]*\)/\1#\2\3#NEXT\2/

s/\([^.]\)\(\.\)\([^.]*\)\(#NEXT[^~]*\)/\1\2\3\4 1/
s/\([^.]\)\(\.\{2\}\)\([^.]*\)\(#NEXT[^~]*\)/\1\2\3\4 5/
s/\([^.]\)\(\.\{3\}\)\([^.]*\)\(#NEXT[^~]*\)/\1\2\3\4 6/
s/\([^.]\)\(\.\{4\}\)\([^.]*\)\(#NEXT[^~]*\)/\1\2\3\4 7/
s/\([^.]\)\(\.\{5\}\)\([^.]*\)\(#NEXT[^~]*\)/\1\2\3\4 8/
s/\([^.]\)\(\.\{6\}\)\([^.]*\)\(#NEXT[^~]*\)/\1\2\3\4 9/
s/\([^.]\)\(\.\{7\}\)\([^.]*\)\(#NEXT[^~]*\)/\1\2\3\4 x/
s/\$/#/g

h
s/\(,*\)\([^,]*#NEXT[^~]*\)/\1\2\1/
s/.*#NEXT\([^~]*\)~.*/\1/

:g2
/,/{
s/\,/@/
y/ABCDEFGHIJKLMNOP/ACEGIKMOBDFHJLNP/
b g2
}
s/@//g
H
g
s/\n/~/
s/\(#NEXT\)[^~]*\(~.*\)~\([^~]*\)$/\1\3\2/
h
b display
}
}

#Eat that line
t d
:d
/~ \(2a\)*|\([15-9x]a\)*|\(2a\)*~/ {
s/~ \(2a\)*|\([15-9x]a\)*|\(2a\)*//
s/^~\([^~]*~\)\{3\}/& 2a2a2a2a|0a0a0a0a0a0a0a0a0a0a|2a2a2a2a~/
s/#SCORE[^~]*/&A/
b d
}
#Scoring
/#SCORE[^~]*A/{
s/\(#SCORE[^~]*\)AAAA/\1+1200z/
s/\(#SCORE[^~]*\)AAA/\1+300z/
s/\(#SCORE[^~]*\)AA/\1+100z/
s/\(#SCORE[^~]*\)A/\1+40z/

/#SCORE[0-9]*+[0-9]*z/{
s/+/@+/
:finnul
/0z/{
/#SCORE@/b endsum
s/\(.\)@/@\1/
s/0z/z/
/+z/b endsum
b finnul
}
t incr
:incr
s/9\(%*\)@/%\1@/
t incr
s/#SCORE\(%*\)@/#SCORE0\1@/
s/8\(%*\)@/9\1@/
s/7\(%*\)@/8\1@/
s/6\(%*\)@/7\1@/
s/5\(%*\)@/6\1@/
s/4\(%*\)@/5\1@/
s/3\(%*\)@/4\1@/
s/2\(%*\)@/3\1@/
s/1\(%*\)@/2\1@/
s/0\(%*\)@/1\1@/
s/%/0/g
t blank
:blank
s/9z/8z/
t incr
s/8z/7z/
t incr
s/7z/6z/
t incr
s/6z/5z/
t incr
s/5z/4z/
t incr
s/4z/3z/
t incr
s/3z/2z/
t incr
s/2z/1z/
t incr
s/1z/0z/
t finnul
:endsum
s/@//
s/#SCORE\([0-9]*\)+\([0-9]*\)z/#SCORE\2\1/
}
h
b display
}

#Falling down
:fall
/~[^~]*[03][i-p].\{38\}[14-9x]/ !{
:c
#Find the lowest line with the...
s/\(~[^~]*[b-p][^~]*\)~\([^b-p~]*~\)/\1z\2/
s/~\([^~b-p]*\)\([b-p]\)\([^~]\{39\}\)[a-p]/~\1a\3\2/
s/~\([^~b-p]*\)\([b-p]\)\([^~]\{39\}\)[a-p]/~\1a\3\2/
s/~\([^~b-p]*\)\([b-p]\)\([^~]\{39\}\)[a-p]/~\1a\3\2/
s/~\([^~b-p]*\)\([b-p]\)\([^~]\{39\}\)[a-p]/~\1a\3\2/
s/z/~/
/[^~]*[b-p][^~]*~[^~b-p]*~[^~]*[b-p][^~]*/b c
/#FALL~$/b fall
h
b display
}
s/#FALL~//g

#Freezing
s/0[b-h]/0a/g
t i
:i
s/\(0[i-p]\)\(.*#CURRENT\)\(.\)/\3a\2\3/
t i

s/\([15-9x]\)[b-p]/\1a/g
s/2[b-p]/2a/g
s/3[b-h]/3a/g
s/4[b-p]/4a/g

#if it's over
/3[i-p]/ s/$/#OVER~/
/[15-9x][i-p]/{
s/\([15-9x]\)\([i-p]\)/\1a/g
s/$/#OVER~/
}
h

#Display
:display
g
s/#\.*/&./
s/#\.\{8\}/#./
s/,\{1,\}/&,/
s/,\{5\}/,/
s/#FALL~//g
h

s/^~/~ +----------+~/
s/~[^~]*~$/& +----------+/
s/~[^~]*|[^~]*3[^~]*//g
s/~[^~]*|[^~]*4[^~]*//g
s/2[a-h]//g
s/\([15-9x]\)[i-p]/\1a/g

/#OVER/s/^\(\(~[^~]*\)\{6\}~\)\([^~]*~\)/\1 |GAME OVER!|~/

/#CURRENT1/s/0[i-p]/[1;46;36mO[m/g
/#CURRENT5/s/0[i-p]/[1;44;34mO[m/g
/#CURRENT6/s/0[i-p]/[1;45;35mO[m/g
/#CURRENT7/s/0[i-p]/[1;42;32mO[m/g
/#CURRENT8/s/0[i-p]/[1;41;31mO[m/g
/#CURRENT9/s/0[i-p]/[1;43;33mO[m/g
/#CURRENTx/s/0[i-p]/[0;47;30mO[m/g
s/0[a-h]/ /g
#s/[+|\-]/[7m&[m/g
s/1[a-h]/[1;46;36mX[m/g
s/5[a-h]/[1;44;34mX[m/g
s/6[a-h]/[1;45;35mX[m/g
s/7[a-h]/[1;42;32mX[m/g
s/8[a-h]/[1;41;31mX[m/g
s/9[a-h]/[1;43;33mX[m/g
s/x[a-h]/[0;47;30mX[m/g

:decap
/#NEXT[^~]*[A-P]/{
s/\(#NEXT[^~]*\)[A-H]/\1 /
s/\(#NEXT[^~]*\)[I-P]/\1X/
b decap
}
s/\(#NEXT[^~]*\)     /\1/

s/^\(\(~[^~]*\)\{2\}\)/&   Next:/
s/^\(\(~[^~]*\)\{3\}\)\(.*\)\(#NEXT\([^~]\{4\}\)\)/\1    \5\3\4/
s/^\(\(~[^~]*\)\{4\}\)\(.*\)\(#NEXT[^~]\{4\} \([^~]\{4\}\)\)/\1    \5\3\4/
s/^\(\(~[^~]*\)\{5\}\)\(.*\)\(#NEXT\([^~]\{4\} \)\{2\}\([^~]\{4\}\)\)/\1    \6\3\4/
s/^\(\(~[^~]*\)\{6\}\)\(.*\)\(#NEXT\([^~]\{4\} \)\{3\}\([^~]\{4\}\)\)/\1    \6\3\4/
s/^\(\(~[^~]*\)\{7\}\)\(.*\)\(#SCORE\([^~]*\)\)/\1   Score:\5 \3\4/
s/ \(X\{1,4\}\)/ [7m\1[m /g
s/^\(\(~[^~]*\)\{9\}\)/&  "w" or up - rotate/
s/^\(\(~[^~]*\)\{10\}\)/&  "a" or left - left/
s/^\(\(~[^~]*\)\{11\}\)/&  "d" or right - right/
s/^\(\(~[^~]*\)\{12\}\)/&  "s" or down - one step down/
s/^\(\(~[^~]*\)\{13\}\)/&  "z" or space - drop down/

#uncomment the line below to switch colours off
#s/\[\([0-9]*;\)*[0-9]*m//g
s/#[^~]*~//g
s/^~//
s/~/\
/g
i\
[2J[H
p
/GAME OVER!/q
