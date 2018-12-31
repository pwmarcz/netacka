s/comment --- extract chapters' and sections' names ---/&/

s/^@subsubsection  */      /p
s/^@subsection  */    /p
s/^@section  */  /p
s/^@chapter  */\
/p
s/^@unnumbered  */\
-  /p
s/^@unnumberedsec  */   -  /p
