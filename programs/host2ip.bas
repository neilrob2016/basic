   10 PRINT "Enter hostname>";
   20 INPUT host
   30 ip = host2ip$(host)
   40 IF ip = "" THEN 
   50     PRINT "Host not found."
   60     GOTO 10
   70 FI 
   80 ' host2ip$() returns a space seperated list of the iP addresses
   90 FOR i = 1 TO elementcnt(ip)
  100     PRINT i,": ",element$(ip,i)
  110 NEXT 
  120 GOTO 10
