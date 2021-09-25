   10 DIM i,s
   20 FOR i = 0 TO 360 STEP 30
   30     s = sin(i)
   40     PRINT "SIN(",i,") = ",s,", ABS = ",abs(s),", SGN = ",sgn(s)
   50 NEXT 
   60 PRINT "--------"
   70 FOR i = 0 TO 360 STEP 30
   80     PRINT "COS(",i,") = ",cos(i)
   90 NEXT 
  100 PRINT "--------"
  110 FOR i = 1 TO 16
  120     PRINT "SQRT(",i,") = ",sqrt(i)
  130 NEXT 
  140 PRINT "--------"
  150 DIM p
  160 FOR i = 1 TO 10
  170     p = pow(10,i)
  180     PRINT "POW(10,",i,") = ",p,", LOG10 = ",log10(p)
  190 NEXT 
  200 PRINT "--------"
  210 FOR i = 1 TO 8
  220     p = pow(2,i)
  230     PRINT "POW(2,",i,") = ",p,", LOG2 = ";
  240     IF instr($build_options,"NO_LOG2",1) > -1 THEN 
  250         PRINT "<not implemented>"
  260     ELSE 
  270         PRINT log2(i)
  280     FI 
  290 NEXT 
  300 PRINT "--------"
  310 FOR i = 1 TO 8
  320     p = pow($e,i)
  330     PRINT "POW(E,",i,") = ",p,", LOG = ",log(p)
  340 NEXT 
  350 PRINT "--------"
  360 PRINT "MAX(10,20,3,4) = ",max(10,20,3,4)
  370 PRINT "MIN(10,20,3,4) = ",min(10,20,3,4)
