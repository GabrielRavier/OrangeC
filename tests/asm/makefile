all: alltests

asmcmp.exe : asmcmp.cpp
	occ $^

%.o: %.nas
	oasm /b $^

%.tst: %.o %1.cmpx
	asmcmp $* > $*.tst
	fc /b $*.tst $*.cmpx

farbranch.tst: farbranch.o farbranch1.cmpx
	fc /b farbranch.o farbranch1.cmpx

alltests: asmcmp.exe addr16.tst addr32.tst addr64.tst int.tst float.tst sse.tst farbranch.tst