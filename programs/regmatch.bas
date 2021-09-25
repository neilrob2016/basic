   10 ' Eg: pattern = ".*el.*", string = "hello"  
   20 ON ERROR GOTO 140
   30 PRINT "Enter pattern>";
   40 INPUT pat
   50 PRINT "Enter string>";
   60 INPUT str
   70 m = regmatch(str,pat)
   80 IF m THEN 
   90     PRINT "MATCH"
  100 ELSE 
  110     PRINT "NO MATCH"
  120 FI 
  130 GOTO 50
  140 IF $error = 83 THEN 
  150     PRINT "Invalid regex"
  160     GOTO 30
  170 FI 
  180 PRINT "BASIC ERROR: ",error$($error)
