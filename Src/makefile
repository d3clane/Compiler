CXX = g++
CXXFLAGS = -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations	  \
		   -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts 		  \
		   -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal      \
		   -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op \
		   -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self \
		   -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel 		  \
		   -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods 				  \
		   -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand 		  \
		   -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix   \
		   -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs 			  \
		   -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow 	  \
		   -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie  \
		   -fPIE -Werror=vla 

HOME = $(shell pwd)
CXXFLAGS += -I $(HOME)

OBJECTDIR  = build/backBuild
PROGRAMDIR = build/backBuild/bin
TARGET 	   = backEnd.exe

DOXYFILE = Others/Doxyfile

TREE_DIR = Tree
TREE_CPP = DSL.cpp Tree.cpp	
TREE_OBJ = $(TREE_CPP:%.cpp=$(OBJECTDIR)/%.o)

TREE_NAME_TABLE_DIR = Tree/NameTable
TREE_NAME_TABLE_CPP = ArrayFuncs.cpp HashFuncs.cpp NameTable.cpp
TREE_NAME_TABLE_OBJ = $(TREE_NAME_TABLE_CPP:%.cpp=$(OBJECTDIR)/TREE_%.o)

COMMON_DIR = Common
COMMON_CPP = DoubleFuncs.cpp Log.cpp StringFuncs.cpp
COMMON_OBJ = $(COMMON_CPP:%.cpp=$(OBJECTDIR)/%.o)

FAST_INPUT_DIR = FastInput
FAST_INPUT_CPP = InputOutput.cpp StringFuncs.cpp
FAST_INPUT_OBJ = $(FAST_INPUT_CPP:%.cpp=$(OBJECTDIR)/$(FAST_INPUT_DIR)_%.o)

BACK_END_IR_DIR	      = BackEnd/IR
BACK_END_IR_CPP		  = IRRegisters.cpp
BACK_END_IR_OBJ 	  = $(BACK_END_IR_CPP:%.cpp=$(OBJECTDIR)/%.o)

BACK_END_IR_BUILD_DIR = BackEnd/IR/IRBuild
BACK_END_IR_BUILD_CPP = IRBuild.cpp
BACK_END_IR_BUILD_OBJ = $(BACK_END_IR_BUILD_CPP:%.cpp=$(OBJECTDIR)/%.o)

BACK_END_IR_LIST_DIR = BackEnd/IR/IRList
BACK_END_IR_LIST_CPP = IR.cpp
BACK_END_IR_LIST_OBJ = $(BACK_END_IR_LIST_CPP:%.cpp=$(OBJECTDIR)/%.o)

IR_LABEL_TABLE_DIR = BackEnd/IR/IRBuild/LabelTable
IR_LABEL_TABLE_CPP = LabelTable.cpp LabelTableArrayFuncs.cpp LabelTableHashFuncs.cpp
IR_LABEL_TABLE_OBJ = $(IR_LABEL_TABLE_CPP:%.cpp=$(OBJECTDIR)/%.o)

BACK_END_DIR = BackEnd
BACK_END_CPP = main.cpp
BACK_END_OBJ = $(BACK_END_CPP:%.cpp=$(OBJECTDIR)/%.o)

BACK_END_TRANSLATE_X64_DIR 	= BackEnd/TranslateFromIR/x64
BACK_END_TRANSLATE_X64_CPP	= x64Translate.cpp x64Encode.cpp
BACK_END_TRANSLATE_X64_OBJ	= $(BACK_END_TRANSLATE_X64_CPP:%.cpp=$(OBJECTDIR)/%.o)

BACK_END_TRANSLATE_X64_RODATA_IMM_DIR 	= BackEnd/TranslateFromIR/x64/RodataInfo/RodataImmediates
BACK_END_TRANSLATE_X64_RODATA_IMM_CPP	= RodataImmediates.cpp RodataImmediatesArrayFuncs.cpp \
										  RodataImmediatesHashFuncs.cpp
BACK_END_TRANSLATE_X64_RODATA_IMM_OBJ	= $(BACK_END_TRANSLATE_X64_RODATA_IMM_CPP:%.cpp=$(OBJECTDIR)/%.o)

BACK_END_TRANSLATE_X64_RODATA_STR_DIR 	= BackEnd/TranslateFromIR/x64/RodataInfo/RodataStrings
BACK_END_TRANSLATE_X64_RODATA_STR_CPP	= RodataStrings.cpp RodataStringsArrayFuncs.cpp \
										  RodataStringsHashFuncs.cpp
BACK_END_TRANSLATE_X64_RODATA_STR_OBJ	= $(BACK_END_TRANSLATE_X64_RODATA_STR_CPP:%.cpp=$(OBJECTDIR)/%.o)

.PHONY: all docs clean buildDirs

all: $(PROGRAMDIR)/$(TARGET)
	rm -rf ../Program/$(TARGET)
	cp $(PROGRAMDIR)/$(TARGET) ../Program/

$(PROGRAMDIR)/$(TARGET): $(TREE_OBJ) $(TREE_NAME_TABLE_OBJ) $(COMMON_OBJ) 			\
						 $(BACK_END_OBJ) $(BACK_END_IR_OBJ) $(IR_LABEL_TABLE_OBJ) 	\
						 $(BACK_END_IR_BUILD_OBJ) $(BACK_END_IR_LIST_OBJ)		 	\
						 $(BACK_END_TRANSLATE_X64_OBJ)								\
						 $(BACK_END_TRANSLATE_X64_RODATA_STR_OBJ)					\
						 $(BACK_END_TRANSLATE_X64_RODATA_IMM_OBJ)					\
						 $(FAST_INPUT_OBJ)
	$(CXX) $^ -o $(PROGRAMDIR)/$(TARGET) $(CXXFLAGS)

$(OBJECTDIR)/%.o : $(TREE_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/TREE_%.o : $(TREE_NAME_TABLE_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(COMMON_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/$(FAST_INPUT_DIR)_%.o : $(FAST_INPUT_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(BACK_END_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(BACK_END_TRANSLATE_X64_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(BACK_END_TRANSLATE_X64_RODATA_STR_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(BACK_END_TRANSLATE_X64_RODATA_IMM_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(BACK_END_IR_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(BACK_END_IR_LIST_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(BACK_END_IR_BUILD_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : $(IR_LABEL_TABLE_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

docs: 
	doxygen $(DOXYFILE)

clean:
	rm -rf $(OBJECTDIR)/*.o

buildDirs:
	mkdir build
	mkdir $(OBJECTDIR)
	mkdir $(PROGRAMDIR)