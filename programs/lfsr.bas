   10 PRINT "Press RETURN for Fibonacci LFSR:"
   20 INPUT a
   30 cnt = 1
   40 reg = 0xACE1
   50 REPEAT 
   60     bit = reg ^ (reg >> 2) ^ (reg >> 3) ^ (reg >> 5)
   70     reg = ((reg >> 1) | (bit << 15)) & 0xFFFF
   80     GOSUB 230
   90     cnt = cnt + 1
  100 UNTIL reg = 0xACE1
  110 ' 
  120 PRINT "Press RETURN for Galois LFSR:"
  130 INPUT a
  140 cnt = 1
  150 reg = 0xACE1
  160 REPEAT 
  170     reg = (reg >> 1) ^ (-(reg & 1) & 0xB400)
  180     GOSUB 230
  190     cnt = cnt + 1
  200 UNTIL reg = 0xACE1
  210 STOP 
  220 ' 
  230 hstr = "0x" + lpad$(hex$(reg),"0",4)
  240 bstr = "0b" + lpad$(bin$(reg),"0",16)
  250 PRINT rpad$(tostr$(cnt)," ",5),": ",rpad$(tostr$(reg)," ",5),", ",hstr,", ",bstr
  260 RETURN 
