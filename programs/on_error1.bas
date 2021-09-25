   10 ON ERROR GOSUB 80
   20 PRINT "Using undefined variable..."
   30 PRINT a
   40 PRINT "Resetting error handler to default behaviour ..."
   50 ON ERROR BREAK : ' Resetting to default behaviour 
   60 GOTO 20
   70 ' 
   80 PRINT ">> Oh dear, we had an error on line ",$error_line
   90 PRINT ">> The error was number ",$error," (",error$($error),")"
  100 RETURN 
