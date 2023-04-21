   10 ' An example of using labels with goto and gosub
   20 DATA "one","two","three"
   30 AUTORESTORE 20
   40 ' 
   50 LABEL "loop"
   60 READ a
   70 GOSUB a
   80 GOTO "loop"
   90 ' 
  100 LABEL "one"
  110 PRINT "SUB ONE"
  120 RETURN 
  130 ' 
  140 LABEL "two"
  150 PRINT "SUB TWO"
  160 RETURN 
  170 ' 
  180 LABEL "three"
  190 PRINT "SUB THREE"
  200 RETURN 
