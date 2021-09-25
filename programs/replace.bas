   10 DIM str = "The quick brown fox jumps over the lazy dog"
   20 DIM i
   30 FOR i = asc("a") TO asc("z")
   40     PRINT str
   50     str = replace$(str,chr$(i),chr$(i - 1))
   60     SLEEP 0.1
   70 NEXT 
