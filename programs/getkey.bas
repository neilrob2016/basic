   10 ' Demonstrates use of the GETKEY() function  
   20 DIM m#
   30 DIM i,k,v
   40 DATA "hello","world","this",123,"is","a","long",456,"test",789
   50 RESTORE 40
   60 WHILE havedata()
   70     READ k,v: m(k) = v
   80 WEND 
   90 ' 
  100 FOR i = 1 TO mapsize(m)
  110     k = getkey$(m,i)
  120     PRINT "Key #",i," = '",k,"', value = '",m(k),"'"
  130 NEXT 
