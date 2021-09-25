   10 ' Demonstrates how data behaves with a REDIM   
   20 DATA "hello",1,"cruel",3,"world",5,"out",7,"there",9
   30 RESTORE 20
   40 DIM a(10)
   50 DIM i,j
   60 PRINT "Before: "
   70 FOR i = 1 TO 10
   80     READ a(i)
   90     PRINT "a(",i,") = ",a(i)
  100 NEXT 
  110 ' 
  120 PRINT : PRINT "After REDIM(2,5), arrsize = ",arrsize(a)
  130 REDIM a(2,5)
  140 FOR i = 1 TO 2
  150     FOR j = 1 TO 5
  160         PRINT "a(",i,",",j,") = ",a(i,j)
  170     NEXT 
  180 NEXT 
  190 ' 
  200 REDIM a(3,4)
  210 PRINT : PRINT "After REDIM(3,4), arrsize = ",arrsize(a)
  220 FOR i = 1 TO 3
  230     FOR j = 1 TO 4
  240         PRINT "a(",i,",",j,") = ",a(i,j)
  250     NEXT 
  260 NEXT 
