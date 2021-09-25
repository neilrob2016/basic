   10 ' Shows user defined expressions  
   20 DEFEXP add = a + b
   30 DEFEXP mult = a * b
   40 ' 
   50 DIM a,b
   60 FOR a = 1 TO 3
   70     FOR b = 1 TO 3
   80         PRINT a," + ",b," = ",!add
   90         PRINT a," * ",b," = ",!mult
  100         PRINT "add * mult = ",!add * !mult
  110         PRINT 
  120     NEXT 
  130 NEXT 
