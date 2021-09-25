   10 DIM i,a(2)
   20 a(1) = "EVEN"
   30 a(2) = "ODD"
   40 FOR i = 0 TO 0xff
   50     PRINT i,",",bin$(i),": ",a(parity(i) + 1)
   60 NEXT 
