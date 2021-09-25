   10 ' Examples of using the format$() function  
   20 DIM i,a
   30 FOR i = 0 TO 10000 STEP 1000.15
   40     ' ! at start means don't pad digits before decimal point. ! at end  
   50     ' means don't pad digits after decimal point. Hash # means pad  
   60     ' with zeroes 
   70     PRINT "i = ",format$("###,###.##",i)
   80     PRINT "i = ",format$("!###,###.##",i)
   90     PRINT "i = ",format$("!###,###.##!",i)
  100 NEXT 
  110 PRINT "Press a key: ";: INPUT a
  120 ' As above except uses '%' which means pad with spaces 
  130 FOR i = 0 TO 10000 STEP 1000.15
  140     PRINT "i = ",format$("%%%,%%%.%%",i)
  150     PRINT "i = ",format$("!%%%,%%%.%%",i)
  160     PRINT "i = ",format$("!%%%,%%%.%%!",i)
  170 NEXT 
