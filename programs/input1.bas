   10 DIM name,c(10),i
   20 PRINT "Enter your name: "
   30 INPUT name
   40 PRINT "Hello ",name,", enter 10 characters "
   50 FOR i = 1 TO 10
   60     CINPUT c(i),1.5
   70     IF c(i) = "" THEN 
   80         PRINT "Times up!"
   90         GOTO 130
  100     FI 
  110     PRINT c(i);
  120 NEXT 
  130 PRINT "-----------"
  140 FOR i = 1 TO 10
  150     PRINT i,": ",c(i)
  160 NEXT 
