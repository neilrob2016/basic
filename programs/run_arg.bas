   10 ' Shows use of the $run_arg system variable which is set by passing     
   20 ' an extra parameter to RUN or CHAIN     
   30 PRINT "Run arg = ",$run_arg
   40 DIM hello = 0
   50 IF isstr($run_arg) AND upper$($run_arg) = "HELLO" THEN 
   60     hello = 1
   70 FIALL 
   80 IF hello THEN 
   90     PRINT "Hello!"
  100 ELSE 
  110     PRINT "Not very friendly are you"
  120 FI 
