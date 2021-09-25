   10 DATA 1,2,3,4
   20 DATA "hello","world": DATA : DATA "wibble"
   30 AUTORESTORE 10
   40 DIM a(10)
   50 DIM i
   60 FOR i=1 TO 10
   70     READ a(i)
   80 NEXT 
   90 FOR i=1 TO 10
  100     PRINT "a(",i,") = ",a(i)
  110 NEXT 
