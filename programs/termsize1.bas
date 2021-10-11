   10 ON TERMSIZE GOSUB 140
   20 PRINT date$(time(),"%Y-%m-%d %H:%M:%S")
   30 ' 
   40 ' SLEEP will be interrupted if the TERMSIZE jump is triggered so
   50 ' make sure we sleep for the correct length of time.
   60 slen = 1
   70 before = time()
   80 REPEAT 
   90     SLEEP slen
  100     slen = time() - before
  110 UNTIL slen >= 1
  120 GOTO 20
  130 ' 
  140 PRINT "Term size = ",$term_cols,"x",$term_rows
  150 RETURN 
