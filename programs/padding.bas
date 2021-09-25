   10 ' Demonstrates left and right padding  
   20 DIM i
   30 FOR i = 0 TO 9
   40     PRINT "Lpad = ",i,": ",lpad$("X","-",i)
   50 NEXT 
   60 FOR i = 0 TO 9
   70     PRINT "Rpad = ",i,": ",rpad$("X","-",i)
   80 NEXT 
