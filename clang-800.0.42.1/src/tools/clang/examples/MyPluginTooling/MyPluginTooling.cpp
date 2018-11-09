#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTImporter.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

#include "clang/Driver/Options.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace std;
using namespace llvm;

namespace
{
    
    class MyPluginVisitor : public RecursiveASTVisitor<MyPluginVisitor>
    {
    private:
        CompilerInstance &Instance;
        ASTContext *Context;
        
    public:
        
        void setASTContext (ASTContext &context)
        {
            this -> Context = &context;
        }
        
        MyPluginVisitor (CompilerInstance &Instance)
        :Instance(Instance)
        {
            
        }
        
        // OC 类声明
        bool VisitObjCInterfaceDecl(ObjCInterfaceDecl *declaration)
        {
            if (isUserSourceCode(declaration -> getSourceRange()))
            {
                checkFormatForObjcInterface(declaration);
            }
            return true;
        }
        
        // OC 类扩展声明
        bool VisitObjCCategoryDecl(ObjCCategoryDecl *declaration)
        {
            if (isUserSourceCode(declaration -> getSourceRange()))
            {
                checkFormatForObjcInterface(declaration);
            }
            return true;
        }
        
        // OC 类扩展实现声明
        bool VisitObjCCategoryImplDecl(ObjCCategoryImplDecl *declaration)
        {
            if (isUserSourceCode(declaration -> getSourceRange()))
            {
                checkFormatForObjcInterface(declaration);
            }
            return true;
        }
        
        // OC 类实现声明
        bool VisitObjCImplementationDecl(ObjCImplementationDecl *declaration)
        {
            if (isUserSourceCode(declaration -> getSourceRange()))
            {
                checkFormatForObjcInterface(declaration);
            }
            return true;
        }
        
        // OC 协议声明
        bool VisitObjCProtocolDecl(ObjCProtocolDecl *declaration)
        {
            if (isUserSourceCode(declaration -> getSourceRange()))
            {
                checkFormatForObjcInterface(declaration);
            }
            return true;
        }
        
        // 方法声明
        bool VisitObjCMethodDecl(ObjCMethodDecl *declaration)
        {
            if (isUserSourceCode(declaration -> getSourceRange()))
            {
                checkThreadOperationInInitialize(declaration);
            }
            return true;
        }
        
        // If节点
        bool VisitIfStmt(IfStmt *S)
        {
            if (isUserSourceCode(S -> getSourceRange()))
            {
                checkSpaceBehindKeyWord(S);
            }
            
            return true;
        }
        
        // Switch节点
        bool VisitSwitchStmt(SwitchStmt *S)
        {
            if (isUserSourceCode(S -> getSourceRange()))
            {
                checkSpaceBehindKeyWord(S);
            }
            
            return true;
        }
        
        // While节点
        bool VisitWhileStmt(WhileStmt *S)
        {
            if (isUserSourceCode(S -> getSourceRange()))
            {
                checkSpaceBehindKeyWord(S);
            }
            
            return true;
        }
        
        // Do节点
        bool VisitDoStmt(DoStmt *S)
        {
            if (isUserSourceCode(S -> getSourceRange()))
            {
                checkSpaceBehindKeyWord(S);
            }
            
            return true;
        }
        
        // For节点
        bool VisitForStmt(ForStmt *S)
        {
            if (isUserSourceCode(S -> getSourceRange()))
            {
                checkSpaceBehindKeyWord(S);
            }
            
            return true;
        }
        
        // For in 节点
        bool VisitObjCForCollectionStmt(ObjCForCollectionStmt *S)
        {
            if (isUserSourceCode(S -> getSourceRange()))
            {
                checkSpaceBehindKeyWord(S);
            }
            
            return true;
        }
        
        // 运算符遍历节点
        bool VisitBinaryOperator(const BinaryOperator *Node)
        {
            if (isUserSourceCode(Node ->getSourceRange()))
            {
                checkBinaryOperator(Node);
            }
            
            return true;
        }
        
        //条件中的逻辑运算符（&&、||、!、>、>=、<、<=、==）两边需要各保留一个空格
        void checkBinaryOperator(const BinaryOperator *Node)
        {
            SourceLocation loc = Node -> getLocStart();
            const char *code = Instance.getSourceManager().getCharacterData(loc);
            std::string opcodeStr = Node -> getOpcodeStr().str();
            
            size_t offset = strstr(code, opcodeStr.c_str()) - code;
            
            size_t offsetL;
            for (offsetL = offset -1 ; offsetL >= 0; offsetL--)
            {
                if (code[offsetL] != ' ')
                {
                    break;
                }
            }
            
            size_t offsetR;
            for (offsetR = offset+strlen(opcodeStr.c_str()); offsetR <= strlen(code); offsetR++)
            {
                if (code[offsetR] != ' ')
                {
                    break;
                }
            }
            
            if (offset - offsetL != 2)
            {
                FixItHint fixItHint;
                if (offset - offsetL == 1)
                {
                    fixItHint = FixItHint::CreateInsertion(loc.getLocWithOffset(offset), StringRef(" "));
                }
                else
                {
                    fixItHint = FixItHint::CreateRemoval(SourceRange(loc.getLocWithOffset(offsetL+1), loc.getLocWithOffset(offsetR-1)));
                }
                
                DiagnosticsEngine &D = Instance.getDiagnostics();
                
                char buf[1024];
                std::string msg = "A blank space required on left side of the \"" + opcodeStr + "\"";
                memset(buf, 0, 1024);
                memcpy(buf, msg.c_str(), msg.size());
                
                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, buf);
                D.Report(loc.getLocWithOffset(offset), diagID).AddFixItHint(fixItHint);
            }
            
            if (offsetR - offset != strlen(opcodeStr.c_str()) + 1)
            {
                FixItHint fixItHint;
                if (offsetR - offset == strlen(opcodeStr.c_str()))
                {
                    fixItHint = FixItHint::CreateInsertion(loc.getLocWithOffset(offsetR), StringRef(" "));
                }
                else
                {
                    fixItHint = FixItHint::CreateRemoval(SourceRange(loc.getLocWithOffset(offset+strlen(opcodeStr.c_str())), loc.getLocWithOffset(offsetR-1)));
                }
                
                DiagnosticsEngine &D = Instance.getDiagnostics();
                
                char buf[1024];
                std::string msg = "A blank space required on right side of the \"" + opcodeStr + "\"";
                memset(buf, 0, 1024);
                memcpy(buf, msg.c_str(), msg.size());
                
                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, buf);
                D.Report(loc.getLocWithOffset(offset), diagID).AddFixItHint(fixItHint);
            }
        }
        
        // 对于类定义中得关键字与类名称需要保留一个空格，以及与基类之间的：号两边需要保留一个空格
        void checkFormatForObjcInterface(ObjCContainerDecl *declaration) {
            
//            SourceLocation loc = declaration -> getLocStart();
//            const char *code = Instance.getSourceManager().getCharacterData(loc);
            
//            const char *className;
//            switch (declaration -> getKind())
//            {
//                case Decl::ObjCInterface:
//                case Decl::ObjCImplementation:
//                case Decl::ObjCProtocol:
//                    className = declaration -> getName().data();
//                    break;
//                case Decl::ObjCCategory:
//                {
//                    ObjCCategoryDecl *dec = (ObjCCategoryDecl *)declaration;
//                    className = dec -> getClassInterface()-> getName().data();
//                }
//                    break;
//                case Decl::ObjCCategoryImpl:
//                {
//                    ObjCCategoryImplDecl *dec = (ObjCCategoryImplDecl *)declaration;
//                    className = dec -> getClassInterface()-> getName().data();
//                }
//                    break;
//                    
//                default:
//                    break;
//            }
            
//            if ( strlen(className) > 0 )
//            {
//                std::string cs[3] = {"(","<",(std::string)className};
//                size_t endPoint = strstr(code, "@end") - code;
//                
//                for (int i = 0; i<3; i++)
//                {
//                    std::string c = cs[i];
//                    size_t offset = strstr(code, c.c_str()) - code;
//                    
//                    //只能有一个空格
//                    if (offset < endPoint && (code[offset-1] != ' ' || code[offset-2] == ' '))
//                    {
//                        size_t offsetL;
//                        
//                        for (offsetL = offset - 1; offsetL > 0 ; offsetL--)
//                        {
//                            if (code[offsetL] != ' ')
//                            {
//                                break;
//                            }
//                        }
//                        
//                        FixItHint fixItHint;
//                        
//                        if(offset - offsetL == 1)
//                        {
//                            fixItHint = FixItHint::CreateInsertion(loc.getLocWithOffset(offset), StringRef(" "));
//                        }
//                        else
//                        {
//                            fixItHint = FixItHint::CreateRemoval(SourceRange(loc.getLocWithOffset(offsetL+1), loc.getLocWithOffset(offset-1)));
//                        };
//                        
//                        DiagnosticsEngine &D = Instance.getDiagnostics();
//                        
//                        char buf[1024];
//                        std::string msg = "Only one blank space required in front of  \"" + c + "\"";
//                        memset(buf, 0, 1024);
//                        memcpy(buf, msg.c_str(), msg.size());
//                        
//                        int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, buf);
//                        D.Report(loc.getLocWithOffset(offset), diagID).AddFixItHint(fixItHint);
//                    }
//                }
//                
//                // : 两边要且只要一个空格
//                size_t offsetSymbol = strstr(code, ":") - code;
//                
//                if (offsetSymbol < endPoint)
//                {
//                    size_t offsetSymbolL;
//                    for (offsetSymbolL = offsetSymbol -1 ; offsetSymbolL >= 0; offsetSymbolL--)
//                    {
//                        if (code[offsetSymbolL] != ' ')
//                        {
//                            break;
//                        }
//                    }
//                    
//                    size_t offsetSymbolR;
//                    for (offsetSymbolR = offsetSymbol+1; offsetSymbolR <= strlen(code); offsetSymbolR++)
//                    {
//                        if (code[offsetSymbolR] != ' ')
//                        {
//                            break;
//                        }
//                    }
//                    
//                    if (offsetSymbol - offsetSymbolL != 2)
//                    {
//                        FixItHint fixItHint;
//                        
//                        if (offsetSymbol - offsetSymbolL == 1)
//                        {
//                            fixItHint = FixItHint::CreateInsertion(loc.getLocWithOffset(offsetSymbol), StringRef(" "));
//                        }
//                        else
//                        {
//                            fixItHint = FixItHint::CreateRemoval(SourceRange(loc.getLocWithOffset(offsetSymbolL+1), loc.getLocWithOffset(offsetSymbol-1)));
//                        }
//                        
//                        DiagnosticsEngine &D = Instance.getDiagnostics();
//                        
//                        int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "A blank space required on left of \":\"");
//                        D.Report(loc.getLocWithOffset(offsetSymbol), diagID).AddFixItHint(fixItHint);
//                    }
//                    
//                    if (offsetSymbolR - offsetSymbol != 2)
//                    {
//                        FixItHint fixItHint;
//                        
//                        if (offsetSymbolR - offsetSymbol == 1)
//                        {
//                            fixItHint = FixItHint::CreateInsertion(loc.getLocWithOffset(offsetSymbolR), StringRef(" "));
//                        }
//                        else
//                        {
//                            fixItHint = FixItHint::CreateRemoval(SourceRange(loc.getLocWithOffset(offsetSymbol+1), loc.getLocWithOffset(offsetSymbolR-1)));
//                        }
//                        
//                        DiagnosticsEngine &D = Instance.getDiagnostics();
//                        
//                        int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "A blank space required on right of \":\"");
//                        D.Report(loc.getLocWithOffset(offsetSymbol), diagID).AddFixItHint(fixItHint);
//                    }
//                    
//                }
//                
//                // { 需要换行
//                size_t braceLoc = std::string(code).find("{");
//                if (braceLoc < endPoint)
//                {
//                    bool needFix = false;
//                    size_t index = braceLoc;
//                    while (index--)
//                    {
//                        if (code[index] == ' ')
//                        {
//                            continue;
//                        }
//                        else if (code[index] == '\n')
//                        {
//                            break;
//                        }
//                        else
//                        {
//                            needFix = true;
//                        }
//                    }
//                    
//                    if (needFix)
//                    {
//                        FixItHint fixItHint= FixItHint::CreateInsertion(loc.getLocWithOffset(braceLoc), StringRef("\n"));;
//                        DiagnosticsEngine &D = Instance.getDiagnostics();
//                        
//                        int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "A \"\n\" required in front of \"{\"");
//                        D.Report(loc.getLocWithOffset(braceLoc), diagID).AddFixItHint(fixItHint);
//                    }
//                }
//            }
            
//            // 协议左对齐
//            unsigned protocolSize = 0;
//            
//            switch (declaration -> getKind())
//            {
//                case Decl::ObjCInterface:
//                {
//                    ObjCInterfaceDecl *dec = (ObjCInterfaceDecl *)declaration;
//                    const ObjCList<ObjCProtocolDecl> &Protocols = dec -> getReferencedProtocols();
//                    protocolSize = Protocols.size();
//                }
//                    break;
//                case Decl::ObjCProtocol:
//                {
//                    ObjCProtocolDecl *dec = (ObjCProtocolDecl *)declaration;
//                    const ObjCList<ObjCProtocolDecl> &Protocols = dec -> getReferencedProtocols();
//                    protocolSize = Protocols.size();
//                }
//                    break;
//                case Decl::ObjCCategory:
//                {
//                    ObjCCategoryDecl *dec = (ObjCCategoryDecl *)declaration;
//                    const ObjCList<ObjCProtocolDecl> &Protocols = dec -> getReferencedProtocols();
//                    protocolSize = Protocols.size();
//                }
//                    break;
//                    
//                default:
//                    break;
//            }
//            
//            if (protocolSize > 1)
//            {
//                size_t lOffset = strstr(code, "<") - code;
//                size_t rOffset = strstr(code, ">") - code;
//                SourceLocation B = loc.getLocWithOffset(lOffset);
//                SourceLocation E = loc.getLocWithOffset(rOffset);
//                
//                bool needFix = false;
//                if (code[lOffset+1] == ' ' || code[lOffset+1] == '\n')
//                {
//                    needFix = true;
//                }
//                else
//                {
//                    const char *sep = strstr(code, ",");
//                    
//                    while (sep - code < rOffset)
//                    {
//                        size_t p = sep - code;
//                        
//                        if (code[p-1] == ' ' || code[p-1] == '\n')
//                        {
//                            needFix = true;
//                            break;
//                        }
//                        
//                        for (int i=p+1; i<rOffset; i++)
//                        {
//                            if (code[i] == '\n')
//                            {
//                                for (int j=1;; j++)
//                                {
//                                    
//                                    if (code[i+j] != '\n')
//                                    {
//                                        needFix = true;
//                                        break;
//                                    }
//                                    
//                                    if (code[i+j] != ' ')
//                                    {
//                                        if (j != lOffset + 1)
//                                        {
//                                            needFix = true;
//                                            break;
//                                        }
//                                    }
//                                }
//                            }
//                            else if (code[i] == ' ')
//                            {
//                                continue;
//                            }
//                            else
//                            {
//                                needFix = true;
//                            }
//                        }
//                        
//                        if (needFix)
//                        {
//                            break;
//                        }
//                        else
//                        {
//                            sep = strstr(sep + 1, ",");
//                        }
//                    }
//                }
//                
//                if (needFix)
//                {
//                    char replace[1024];
//                    
//                    size_t j = 0;
//                    for (size_t i = 0; i<= rOffset - lOffset; i++)
//                    {
//                        if (code[i+lOffset] != ' ' && code[i+lOffset] != '\n')
//                        {
//                            replace[++j] = code[i+lOffset];
//                            
//                            if (code[i+lOffset]==',')
//                            {
//                                replace[++j] = '\n';
//                                for (size_t tmp = 0; tmp < lOffset + 1; tmp++)
//                                {
//                                    replace[++j] = ' ';
//                                }
//                            }
//                        }
//                    }
//                    
//                    SourceRange range = SourceRange(loc.getLocWithOffset(lOffset), loc.getLocWithOffset(rOffset));
//                    
//                    FixItHint fixItHint = FixItHint::CreateReplacement(range, StringRef(replace+1));
//                    
//                    DiagnosticsEngine &D = Instance.getDiagnostics();
//                    
//                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, "Protocol Format Invalid");
//                    D.Report(loc.getLocWithOffset(lOffset), diagID).AddFixItHint(fixItHint);
//                }
//            }
        };
        
        /**
         检查 关键字后面是否有一个空格，} 换行
         */
        void checkSpaceBehindKeyWord(const Stmt *Node)
        {
            SourceLocation loc = Node -> getLocStart();
            const char *p = Instance.getSourceManager().getCharacterData(loc);
            
            int strLength;
            
            int i=0 ;
            for (i=0; i<100; i++)
            {
                if (p[i] < 'a' || p[i] > 'z')
                {
                    strLength = i;
                    break;
                }
            }
            
            char c = p[strLength];
            
            char str[100];
            memset(str, 0, strLength+1);
            memcpy(str, p, strLength);
            std::string keyWord = str;
            
            if (p[strLength] != ' ' || p[strLength+1] == ' ')
            {
                int offset;
                
                for (offset = 0; offset<strlen(p); offset++)
                {
                    if (p[strLength+offset] == '(')
                    {
                        break;
                    }
                }
                StringRef replacement(keyWord + " (");
                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(loc, loc.getLocWithOffset(strLength + offset)), replacement);
                DiagnosticsEngine &D = Instance.getDiagnostics();
                
                char buf[1024];
                std::string msg = "Only one blank space required behind \"" + keyWord + "\"";
                memset(buf, 0, 1024);
                memcpy(buf, msg.c_str(), msg.size());
                
                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, buf);
                D.Report(loc, diagID).AddFixItHint(fixItHint);
            }
            
            // { 另起一行检查
            
            int j;
            
            int offsetL;
            int offsetR;
            
            bool needWarn = false;
            
            for (j = 0; j<strlen(p); j++)
            {
                if (p[j] == '\n')
                {
                    break;
                }
                if (p[j] == ')')
                {
                    offsetL = j;
                }
                if (p[j] == '{')
                {
                    needWarn = true;
                    offsetR = j;
                    break;
                }
            }
            
            if (needWarn)
            {
                StringRef replacement(")\n{");
                FixItHint fixItHint = FixItHint::CreateReplacement(SourceRange(loc.getLocWithOffset(offsetL), loc.getLocWithOffset(offsetR)), replacement);
                DiagnosticsEngine &D = Instance.getDiagnostics();
                
                char buf[1024];
                std::string msg = "\"{\" need a new line";
                memset(buf, 0, 1024);
                memcpy(buf, msg.c_str(), msg.size());
                
                int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, buf);
                D.Report(loc, diagID).AddFixItHint(fixItHint);
            }
        }
        
        
        void checkThreadOperationInInitialize (ObjCMethodDecl *decl)
        {
            Selector sel = decl -> getSelector();
            int selectorPartCount = decl -> getNumSelectorLocs();
            for (int i = 0; i < selectorPartCount; i++)
            {
                StringRef selName = sel.getNameForSlot(i);
                
                if (decl -> isClassMethod() && decl -> hasBody())
                {
                    Stmt *methodBody = decl -> getBody();
                    std::string srcCode;
                    //                    srcCode.assign(Instance.getSourceManager().getCharacterData(methodBody->getLocStart()),
                    //                                   methodBody->getLocEnd().getRawEncoding() - methodBody->getLocStart().getRawEncoding() + 1);
                    
                    if (strcmp(selName.str().c_str(), "initialize") == 0 && decl -> isClassMethod())
                    {
                        //                        checkThreadOperation(methodBody->getLocStart(), srcCode.c_str(), srcCode.c_str(), "dispatch_sync", "Avoid thread operation in class method \"initialize\"");
                        //                        checkThreadOperation(methodBody->getLocStart(), srcCode.c_str(), srcCode.c_str(), "dispatch_async", "Avoid thread operation in class method \"initialize\"");
                    }
                }
            }
        }
        
        
        bool checkThreadOperation(SourceLocation startLoc, const char *sourceCode, const char *targetBody, const char *target, std::string hintMsg)
        {
            const char *targetFuntion = strstr(targetBody, target);
            
            if (targetFuntion != NULL)
            {
                if (!isCommentedCode(sourceCode, targetFuntion))
                {
                    DiagnosticsEngine &D = Instance.getDiagnostics();
                    char buf[1024];
                    memset(buf, 0, 1024);
                    memcpy(buf, hintMsg.c_str(), hintMsg.size());
                    int diagID = D.getCustomDiagID(DiagnosticsEngine::Warning, buf);
                    //                    "Avoid thread operation in class method \"initialize\""
                    D.Report(startLoc.getLocWithOffset(targetFuntion-sourceCode), diagID);
                }
                
                checkThreadOperation(startLoc, sourceCode, targetFuntion + strlen(target), target, hintMsg);
            }
            
            return false;
        }
        
        // 检查是不是注释 orz~~~
        bool isCommentedCode (const char *body, const char *target)
        {
            size_t pos = target - body;
            std::string bodyStr = body;
            
            size_t m0Begin = bodyStr.rfind("//",pos);
            size_t m0End = bodyStr.rfind("\n",pos);
            size_t m1Begin = bodyStr.rfind("/*",pos);
            size_t m1End = bodyStr.rfind("*/",pos);
            
            if (m0Begin != bodyStr.npos && (m0End == bodyStr.npos || m0End < m0Begin))
            {
                return true;
            }
            else if (m1Begin != bodyStr.npos && (m1End == bodyStr.npos || m1End < m1Begin))
            {
                return true;
            }
            
            return false;
        }
        
        
        /**
         判断是否为用户源码
         
         @param decl 声明
         @return true 为用户源码，false 非用户源码
         */
        bool isUserSourceCode (SourceRange range)
        {
            string filename = Instance.getSourceManager().getFilename(range.getBegin()).str();
            
            if (filename.empty())
                return false;
            
            //非XCode中的源码都认为是用户源码
            if(filename.find("/Users/max/Documents/Xcode.app") == 0)
                return false;
            
            return true;
        }
    };
    
    class MyPluginConsumer : public ASTConsumer
    {
        CompilerInstance &Instance;
        std::set<std::string> ParsedTemplates;
    public:
        MyPluginConsumer(CompilerInstance &Instance,
                         std::set<std::string> ParsedTemplates)
        : Instance(Instance), ParsedTemplates(ParsedTemplates), visitor(Instance) {}
        
        bool HandleTopLevelDecl(DeclGroupRef DG) override
        {
            return true;
        }
        
        void HandleTranslationUnit(ASTContext& context) override
        {
            visitor.setASTContext(context);
            visitor.TraverseDecl(context.getTranslationUnitDecl());
        }
    private:
        MyPluginVisitor visitor;
        
    };
    
    class MyPluginASTAction : public ASTFrontendAction
    {
        std::set<std::string> ParsedTemplates;
    protected:
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                       llvm::StringRef) override
        {
            return llvm::make_unique<MyPluginConsumer>(CI, ParsedTemplates);
        }
        
//        bool ParseArgs(const CompilerInstance &CI,
//                       const std::vector<std::string> &args) override {
//            return true;
//        }
    };
}

static llvm::cl::OptionCategory OptsCategory("MyPluginTooling");

int main(int argc, const char **argv) {
    clang::tooling::CommonOptionsParser op(argc, argv, OptsCategory);
    clang::tooling::ClangTool Tool(op.getCompilations(), op.getSourcePathList());
    int result = Tool.run(clang::tooling::newFrontendActionFactory<MyPluginASTAction>().get());
    return result;
}

