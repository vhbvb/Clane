CLANG_LEVEL := ../../../..
TOOLNAME = clang-rename
include $(CLANG_LEVEL)/../../Makefile.config
LINK_COMPONENTS := $(TARGETS_TO_BUILD) asmparser bitreader support mc option
USEDLIBS = clangRename.a clangFrontend.a clangSerialization.a clangDriver.a \
           clangTooling.a clangToolingCore.a \
	   clangParse.a clangSema.a clangIndex.a \
           clangStaticAnalyzerFrontend.a clangStaticAnalyzerCheckers.a \
           clangStaticAnalyzerCore.a clangAnalysis.a clangRewriteFrontend.a \
           clangRewrite.a clangEdit.a clangAST.a clangLex.a \
           clangAPINotes.a clangBasic.a clangIndex.a

include $(CLANG_LEVEL)/Makefile

