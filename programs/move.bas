   10 ' A rather contrived example of using MOVE in a program. 
   20 ' Can't use FOR-NEXT, WHILE-WEND etc as all loop constructs are reset 
   30 ' when MOVE is used.     
   40 linenum = 75
   50 i = 1
   60 PRINT "----"
   75 PRINT "ONE. Linenum = ",$prog_line
   80 PRINT 2
   90 PRINT 3
  100 PRINT 4
  110 PRINT 5
  120 MOVE linenum TO linenum + 10
  130 linenum = linenum + 10
  140 i = i + 1
  150 IF i < 6 THEN GOTO 60 FI 
