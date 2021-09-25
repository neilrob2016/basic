   10 DIM a="1234567890"
   20 DIM i
   30 FOR i=1 TO 10
   40     PRINT sub$(a,i,10)
   50 NEXT 
   60 FOR i=1 TO 10
   70     PRINT sub$(a,i,i*2)
   80 NEXT 
   90 FOR i=0 TO 9
  100     PRINT sub$(a,1,strlen(a)-i)
  110 NEXT 
