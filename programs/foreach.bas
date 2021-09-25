   10 DIM i,k,v,m#
   20 DATA "hello","world","blah","test","bored",123,"wibble","wobble"
   30 RESTORE 20
   40 WHILE havedata()
   50     READ k,v
   60     m(k) = v
   70 WEND 
   80 ' 
   90 FOREACH k,v IN m
  100     REM Deleting the key shifts everything up one so foreach loop skips   
  110     REM one of the pairs     
  120     IF haskey(m,"hello") THEN 
  130         DELKEY m,"hello"
  140     FI 
  150     PRINT "Key = ",k,", val = ",v
  160 NEXTEACH 
  170 PRINT "Press return:"
  180 INPUT k
  190 GOTO 90
