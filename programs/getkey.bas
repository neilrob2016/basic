   10 ' Demonstrates use of the GETKEY() function  
   20 DATA "hello","world","this",123,"is","a","long",456,"test",789
   30 RESTORE 20
   40 WHILE havedata()
   50     READ k,v: m(k) = v
   60 WEND 
   70 ' 
   80 FOR i = 1 TO mapsize(m)
   90     k = getkey$(m,i)
  100     PRINT "Key #",i," = '",k,"', value = '",m(k),"'"
  110 NEXT 
