   10 ' 
   20 ' *** Sieve of Eratosthenes v1, July 2021 ***         
   30 ' 
   40 PRINT "Sieve size> ";
   50 INPUT size
   60 IF NOT isnumstr(size) THEN 
   70     PRINT "*** ERROR ***"
   80     GOTO 40
   90 FI 
  100 ' 
  110 ' Set the sieve      
  120 ' 
  130 start_time = time()
  140 size = tonum(size)
  150 sqr = sqrt(size): ' Only have to go as far as the square root   
  160 CDIM sieve(size)
  170 ' 
  180 FOR i = 3 TO sqr STEP 2
  190     ' Don't set the 'i' position itself, only multiples of it         
  200     index = i + i
  210     IF index > size THEN CONTLOOP FI 
  220     IF sieve(index) THEN CONTLOOP FI 
  230     REPEAT 
  240         sieve(index) = 1
  250         index = index + i
  260     UNTIL index > size
  270 NEXT 
  280 end_time = time()
  290 ' 
  300 ' Print the primes      
  310 ' 
  320 cnt = 2
  330 PRINT 1
  340 PRINT 2
  350 FOR i = 3 TO size STEP 2
  360     IF NOT sieve(i) THEN 
  370         PRINT i
  380         cnt = cnt + 1
  390     FI 
  400 NEXT 
  410 ' 
  420 PRINT "Count    : ",cnt
  430 PRINT "Calc time: ",end_time - start_time," secs"
  440 GOTO 40
