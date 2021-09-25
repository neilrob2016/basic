   10 DIM etype
   20 IF instr($build_options,"DES_ONLY",1) = -1 THEN 
   30     PRINT "SHA-512:"
   40     etype = "SHA512"
   50 ELSE 
   60     PRINT "DES:"
   70     etype = "DES"
   80 FI 
   90 PRINT crypt$("hello world","xy",etype)
