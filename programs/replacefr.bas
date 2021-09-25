   10 s = "hello hello hello"
   20 last_s = ""
   30 len = strlen(s)
   40 FOR i = len TO 1 STEP -1
   50     new_s = replacefr$(s,"e","x",i)
   60     IF new_s <> last_s THEN 
   70         PRINT "Pos ",format$("%%",i),": ",new_s
   80         last_s = new_s
   90     FI 
  100 NEXT 
