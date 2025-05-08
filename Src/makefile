PROGRAMDIR = ../examples/bin

.PHONY: all docs clean buildDirs

all: 
	make -f makefileBack && make -f makefileFront && make -f makefileBackFront && make -f makefileMiddle && make -f makefileBackSpu
	cp build/backBuild/bin/backEnd 				$(PROGRAMDIR)/backEnd
	cp build/frontBuild/bin/frontEnd 			$(PROGRAMDIR)/frontEnd 
	cp build/middleBuild/bin/middleEnd 			$(PROGRAMDIR)/middleEnd  	
	cp build/backFrontBuild/bin/backFrontEnd	$(PROGRAMDIR)/backFrontEnd
	cp build/backBuildSpu/bin/backEndSpu		$(PROGRAMDIR)/backEndSpu
	cp build/preprocessorBuild/bin/preprocessor $(PROGRAMDIR)/preprocessor

clean:
	make -f makefileBack clean && make -f makefileFront clean && \
	make -f makefileBackFront clean && make -f makefileMiddle clean \
	make -f makefilePreprocessor clean

buildDirs:
	mkdir -p build
	mkdir -p ../examples/bin/
	make -f makefileBack buildDirs && make -f makefileFront buildDirs &&      \
	make -f makefileBackFront buildDirs && make -f makefileMiddle buildDirs && \
	make -f makefileBackSpu buildDirs && make -f makefilePreprocessor 
