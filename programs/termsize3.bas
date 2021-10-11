   10 ' This shows how to get around the INPUT command losing all the text
   20 ' entered before it was interrupted.
   30 ON TERMSIZE GOTO 170
   40 REPEAT 
   50     str = ""
   60     PRINT "Type a string>",str;
   70     CINPUT c
   80     IF asc(c) = 10 THEN GOTO 130 FI 
   90     PRINT c;
  100     str = str + c
  110     GOTO 70
  120 UNTIL NOT $interrupted
  130 PRINT 
  140 PRINT "You typed: ",str
  150 GOTO 40
  160 ' 
  170 PRINT 
  180 PRINT "Term size = ",$term_cols,"x",$term_rows
  190 GOTO 60
