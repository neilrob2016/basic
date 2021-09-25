   10 ' Print the first 100 users and groups   
   20 DIM i,j,info
   30 ' 
   40 PRINT "-------- USERS ----------"
   50 FOR i = 1 TO 100
   60     info = getuserbyid$(i)
   70     IF info THEN PRINT info FI 
   80 NEXT 
   90 ' 
  100 PRINT 
  110 PRINT "-------- GROUPS ---------"
  120 FOR i = 1 TO 100
  130     info = getgroupbyid$(i)
  140     IF info THEN 
  150         PRINT element$(info,1),": ",element$(info,2)
  160         FOR j = 3 TO elementcnt(info)
  170             PRINT "    ",j - 2,": ",element$(info,j)
  180         NEXT 
  190     FI 
  200 NEXT 
