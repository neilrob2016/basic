   10 ' This shows how to deal with interrupted INPUT commands. Resize the
   20 ' terminal window to see it in operation.
   30 ON TERMSIZE GOSUB 120
   40 str = ""
   50 PRINT ">",str;: INPUT a
   60 str = str + a
   70 IF $interrupted THEN 
   80     GOTO 50
   90 FI 
  100 PRINT "--",str,"--"
  110 GOTO 40
  120 PRINT "Term ",$term_cols,"x",$term_rows
  130 RETURN 
