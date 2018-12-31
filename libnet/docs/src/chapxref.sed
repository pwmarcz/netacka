s/comment --- transform a list of nodenames followed by   ---/&/
s/comment --- chapter, section or subsection name, into a ---/&/
s/comment --- Sed script which changes node names into    ---/&/
s/comment --- chapter numbers in cross-references.        ---/&/

/^@node[ 	]/ {
  N
  s/\n/!!=!!/
  /^@node  *Top[^a-z]/d
  s/^@node  *\([^,][^,]*\),.*!!=!!@\([a-z][a-z]*\)[ 	][ 	]*\([0-9]\(\.*[0-9]\)*\).*/\
    s|@xref{\1}|See \2 \3: \1|g\
    s|@ref{\1}|\2 \3: \1|g\
    s|@pxref{\1}|see \2 \3: \1|g\
  /
  s/chapter \([0-9][0-9.]*\)/Chapter \1/g
  s/subsubsection \([0-9][0-9.]*\)/Subsubsec \1/g
  s/subsection \([0-9][0-9.]*\)/Subsec \1/g
  s/section \([0-9][0-9.]*\)/Section \1/g
  s/^@node  *\([^,][^,]*\),.*!!=!!@\([a-z][a-z]*\)[ 	][ 	]*\(.*\)/\
    s|@xref{\1}|See \2 \3: \1|g\
    s|@ref{\1}|\2 \3: \1|g\
    s|@pxref{\1}|see \2 \3: \1|g\
  /
  s/unnumberedsec /unnumsec/g
  s/unnumbered /unnum/g
}
/^@chapter[ 	]/d
/^@section[ 	]/d
/^@subsection[ 	]/d
/^@subsubsection[ 	]/d
/^@unnumbered[ 	]/d
/^@unnumberedsec[ 	]/d
