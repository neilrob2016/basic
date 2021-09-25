   10 ' 
   20 ' *** Sieve of Eratosthenes v2, July 2021 ***           
   30 ' 
   40 ' This uses half the memory of v1 but runs at about 1/3 to 1/4 the speed
   50 ' 
   60 DEFEXP index2 = (index - 1) / 2
   70 ' 
   80 PRINT "Sieve size> ";
   90 INPUT size
  100 IF NOT isnumstr(size) THEN 
  110     PRINT "*** ERROR ***"
  120     GOTO 80
  130 FI 
  140 ' 
  150 ' Set the sieve. Only need to store the odd numbers hence size / 2 
  160 ' and where the memory saving comes from compared to sieve1.  
  170 ' 
  180 start_time = time()
  190 size = tonum(size)
  200 sqr = sqrt(size): ' Only have to go as far as the square root      
  210 CDIM sieve(size / 2)
  220 ' 
  230 FOR i = 3 TO sqr STEP 2
  240     ' Don't set the 'i' position itself, only multiples of it           
  250     index = i + i
  260     IF index > size THEN CONTLOOP FI 
  270     ' If already set then all multiples will have been set too          
  280     IF (index % 2) AND sieve(!index2) THEN CONTLOOP FI 
  290     REPEAT 
  300         ' Only set for odd numbers, we know evens are never prime         
  310         IF (index % 2) THEN sieve(!index2) = 1 FI 
  320         index = index + i
  330     UNTIL index > size
  340 NEXT 
  350 end_time = time()
  360 ' 
  370 ' Print the primes. In sieve 1,2,3 = 3,5,7 etc hence i * 2 + 1         
  380 ' 
  390 cnt = 2
  400 size = size / 2 - 1
  410 PRINT 1
  420 PRINT 2
  430 FOR i = 1 TO size
  440     IF NOT sieve(i) THEN 
  450         PRINT i * 2 + 1
  460         cnt = cnt + 1
  470     FI 
  480 NEXT 
  490 ' 
  500 PRINT "Count    : ",cnt
  510 PRINT "Calc time: ",end_time - start_time," secs"
  520 GOTO 80
