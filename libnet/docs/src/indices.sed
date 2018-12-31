s/comment --- extract Info indices to temporary files ---/&/

/^[0-9 .]*Topic Index/,/^File:/ {
   s/^\* Menu://
   s/^\(\* \)\(.*\)/\2/w concepts.tmp
}
/^[0-9 .]*Program Index/,/^Tag Table:/ {
   s/^\* Menu://
   s/^\(\* \)\(.*\)/\2/w programs.tmp
}
