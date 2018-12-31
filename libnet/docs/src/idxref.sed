s/comment --- transform a list of nodenames followed by chapter ---/&/
s/comment --- or section name, into a Sed script which changes  ---/&/
s/comment --- node names into chapter numbers in nodenames.     ---/&/

1i\
/^\\* Menu:$/,$ {

/^@node[ 	]/ {
  N
  s/\n/!!=!!/
  /^@node  *Top/d
  s/^@node  *\([^,][^,]*\),.*!!=!!@\([a-z][a-z]*\)[ 	][ 	]*\([0-9]\(\.*[0-9]\)*\).*/s|:  *\1\\.$|: \2 \3.|/
  s/chapter \([0-9][0-9.]*\)/Chapter \1/
  s/section \([0-9][0-9.]*\)/Section \1/
  s/subSection \([0-9][0-9.]*\)/Subsection \1/
  s/subSubsection \([0-9][0-9.]*\)/Subsubsection \1/
  s/^@node  *\([^,][^,]*\),.*!!=!!@\([a-z][a-z]*\)[ 	][ 	]*\(.*\)/s|ref{\1,|ref{\2 \3,|/
  s/unnumberedsec //
  s/unnumbered //
  s/``/"/g
  s/''/"/g
}

/^@chapter[ 	]/d
/^@section[ 	]/d
/^@subsection[ 	]/d
/^@subsubsection[ 	]/d
/^@unnumbered[ 	]/d
/^@unnumberedsec[ 	]/d

$a\
s/^\\* Menu:$//\
}

