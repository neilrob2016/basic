   10 ' The INPUT commands will be interrupted when the TERMSIZE jump happens.
   20 ' This shows how to use the $interrupted system variable to solve that.
   30 ON TERMSIZE GOSUB 100
   40 REPEAT 
   50     PRINT "Type a string>";: INPUT str
   60 UNTIL NOT $interrupted
   70 PRINT "You typed: ",str
   80 GOTO 40
   90 ' 
  100 PRINT "Term size = ",$term_cols,"x",$term_rows
  110 RETURN 
