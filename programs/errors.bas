   10 DIM i,str
   20 str = error$(i)
   30 IF str = "" THEN STOP FI 
   40 PRINT i,": ",str
   50 i = i + 1
   60 GOTO 20
