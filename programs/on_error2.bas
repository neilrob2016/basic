   10 ON ERROR GOSUB 60
   20 PRINT "A = ",a
   30 PRINT "*** DONE ***"
   40 PRINT "B = ",b
   50 ' 
   60 PRINT : PRINT "BASIC ERROR: ",$error
   70 GOSUB 120
   80 PRINT "Returning from error handler..."
   90 ' This return will eventually fail causing yet another error 
  100 RETURN 
  110 ' 
  120 PRINT "At line 200"
  130 RETURN 
  140 ' Will end up here after a return from the error handler which was 
  150 ' caused by an unexpected RETURN! 
