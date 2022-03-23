dnl
dnl check clang version 
dnl
AC_DEFUN([AC_CHECK_CLANGVVD_I386_VERSION],[
    AC_ARG_WITH(clangvvd,
    [  --with-clangvvd=executable-path path where the CLANG vvd is installed ],
    [
       ac_clangvvd="$withval"
    ])
dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_clangvvd" = x; then
   CLANG_TO_BE_CHECKED=""
else
   CLANG_TO_BE_CHECKED=$ac_clangvvd;
fi

echo "looking for clang vvd..."
for compiler in $CLANG_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for clang
      I386_CLANGVVD_VERSION=`$compiler --version |grep clang|grep -v InstalledDir|awk -F' ' '{print $[3]}'| awk -F'-' '{print $[1]}'`
      AS_VERSION_COMPARE($1, [7.0.0], MIN_CLANGVVD=[7.0.0], MIN_CLANGVVD=$1, MIN_CLANGVVD=$1)
      AS_VERSION_COMPARE([8.0.0], $2, MAX_CLANGVVD=[8.0.0], MAX_CLANGVVD=$2, MAX_CLANGVVD=$2)
      AS_VERSION_COMPARE($I386_CLANGVVD_VERSION, $MIN_CLANGVVD, echo "checking $compiler >= $MIN_CLANGVVD... no"; min=no, echo "checking $compiler >= $MIN_CLANGVVD... yes"; min=yes, echo "checking $compiler >= $MIN_CLANGVVD... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_CLANGVVD_VERSION, $MAX_CLANGVVD, echo "checking $compiler < $MAX_CLANGVVD... yes"; max=yes, echo "checking $compiler < $MAX_CLANGVVD... no"; max=no, echo "checking $compiler < $MAX_CLANGVVD... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_CLANGVVD_EXE=$compiler;
      clang_file=`basename $I386_CLANGVVD_EXE`
      clang_dir=`dirname $I386_CLANGVVD_EXE`

      llvm_config=`echo $clang_file | sed s/clang/llvm-config/`
      I386_LLVM_CONFIGVVD_EXE=$clang_dir/$llvm_config
      LLVMVVD_CXXFLAGS=`$I386_LLVM_CONFIGVVD_EXE --cxxflags`
      I386_LLVMVVD_CXXFLAGS="$LLVMVVD_CXXFLAGS -std=c++11 $3"
      if test "x$I386_LLVMVVD_CXXFLAGS" = "x"; then
         echo "checking CLANG/LLVM plugin support... no. Package llvm-vvd missing?"
         break;
      fi
      echo "llvm cxxflags...$I386_LLVMVVD_CXXFLAGS"
      cpp=`echo $clang_file | sed s/clang/clang-cpp/`
      I386_CLANG_CPPVVD_EXE=$clang_dir/$cpp
      if test -f $I386_CLANG_CPPVVD_EXE; then
         echo "checking cpp...$I386_CLANG_CPPVVD_EXE"
      else
         echo "checking cpp...no"
         I386_CLANGVVD_EXE=""
         continue
      fi
      clangpp=`echo $clang_file | sed s/clang/clang\+\+/`
      I386_CLANGPPVVD_EXE=$clang_dir/$clangpp
      if test -f $I386_CLANGPPVVD_EXE; then
         echo "checking clang++...$I386_CLANGPPVVD_EXE"
      else
         echo "checking clang++...no"
         continue
      fi
      llvm_link=`echo $clang_file | sed s/clang/llvm-link/`
      I386_LLVMVVD_LINK_EXE=$clang_dir/$llvm_link
      if test -f $I386_LLVMVVD_LINK_EXE; then
         echo "checking llvm-link...$I386_LLVMVVD_LINK_EXE"
      else
         echo "checking llvm-link...no"
         continue
      fi
      llvm_opt=`echo $clang_file | sed s/clang/opt/`
      I386_LLVMVVD_OPT_EXE=$clang_dir/$llvm_opt
      if test -f $I386_LLVMVVD_OPT_EXE; then
         echo "checking llvm-opt...$I386_LLVMVVD_OPT_EXE"
      else
         echo "checking llvm-opt...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_CLANGVVD_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_CLANGVVD_M32=yes,I386_CLANGVVD_M32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_CLANGVVD_M32" == xyes; then
         AC_DEFINE(HAVE_I386_CLANGVVD_M32,1,[Define if clang vvd supports -m32 ])
         echo "checking support to -m32... yes"
      else
         echo "checking support to -m32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_CLANGVVD_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_CLANGVVD_MX32=yes,I386_CLANGVVD_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_CLANGVVD_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_CLANGVVD_MX32,1,[Define if clang vvd supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_CLANGVVD_EXE
      CFLAGS="-m64"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_CLANGVVD_M64=yes,I386_CLANGVVD_M64=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_CLANGVVD_M64" == xyes; then
         AC_DEFINE(HAVE_I386_CLANGVVD_M64,1,[Define if clang vvd supports -m64 ])
         echo "checking support to -m64... yes"
      else
         echo "checking support to -m64... no"
      fi
      cat > plugin_test.cpp <<PLUGIN_TEST
//===- PrintFunctionNames.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
//
//===----------------------------------------------------------------------===//
#ifdef _WIN32
int check;
#else
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

namespace {

class PrintFunctionsConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  std::set<std::string> ParsedTemplates;

public:
  PrintFunctionsConsumer(CompilerInstance &Instance,
                         std::set<std::string> ParsedTemplates)
      : Instance(Instance), ParsedTemplates(ParsedTemplates) {}

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      const Decl *D = *i;
      if (const NamedDecl *ND = dyn_cast<NamedDecl>(D))
        llvm::errs() << "top-level-decl: \"" << ND->getNameAsString() << "\"\n";
    }

    return true;
  }

  void HandleTranslationUnit(ASTContext& context) override {
    if (!Instance.getLangOpts().DelayedTemplateParsing)
      return;

    // This demonstrates how to force instantiation of some templates in
    // -fdelayed-template-parsing mode. (Note: Doing this unconditionally for
    // all templates is similar to not using -fdelayed-template-parsig in the
    // first place.)
    // The advantage of doing this in HandleTranslationUnit() is that all
    // codegen (when using -add-plugin) is completely finished and this can't
    // affect the compiler output.
    struct Visitor : public RecursiveASTVisitor<Visitor> {
      const std::set<std::string> &ParsedTemplates;
      Visitor(const std::set<std::string> &ParsedTemplates)
          : ParsedTemplates(ParsedTemplates) {}
      bool VisitFunctionDecl(FunctionDecl *FD) {
        if (FD->isLateTemplateParsed() &&
            ParsedTemplates.count(FD->getNameAsString()))
          LateParsedDecls.insert(FD);
        return true;
      }

      std::set<FunctionDecl*> LateParsedDecls;
    } v(ParsedTemplates);
    v.TraverseDecl(context.getTranslationUnitDecl());
    clang::Sema &sema = Instance.getSema();
    for (const FunctionDecl *FD : v.LateParsedDecls) {
      clang::LateParsedTemplate &LPT =
          *sema.LateParsedTemplateMap.find(FD)->second;
      sema.LateTemplateParser(sema.OpaqueParser, LPT);
      llvm::errs() << "late-parsed-decl: \"" << FD->getNameAsString() << "\"\n";
    }   
  }
};

class PrintFunctionNamesAction : public PluginASTAction {
  std::set<std::string> ParsedTemplates;
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return llvm::make_unique<PrintFunctionsConsumer>(CI, ParsedTemplates);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "PrintFunctionNames arg = " << args.at(i) << "\n";

      // Example error handling.
      DiagnosticsEngine &D = CI.getDiagnostics();
      if (args.at(i) == "-an-error") {
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                            "invalid argument '%0'");
        D.Report(DiagID) << args.at(i);
        return false;
      } else if (args.at(i) == "-parse-template") {
        if (i + 1 >= e) {
          D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                     "missing -parse-template argument"));
          return false;
        }
        ++i;
        ParsedTemplates.insert(args.at(i));
      }
    }
    if (!args.empty() && args.at(0) == "help")
      PrintHelp(llvm::errs());

    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for PrintFunctionNames plugin goes here\n";
  }

  PluginASTAction::ActionType getActionType() override {
  return AddAfterMainAction;
  }
};

}

static FrontendPluginRegistry::Add<PrintFunctionNamesAction>
X1("print-fns", "print function names");
#endif
PLUGIN_TEST
      for plugin_compiler in $I386_CLANGPPVVD_EXE; do
         plugin_option=
         case $host_os in
           mingw*) 
             echo plugin_option="-shared -Wl,--export-all-symbols -Wl,--start-group -lclangAST -lclangASTMatchers -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangEdit -lclangRewrite -lclangRewriteFrontend -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangCrossTU -lclangIndex -lclangSerialization -lclangToolingCore -lclangTooling -lclangFormat -Wl,--end-group -lversion `$I386_LLVM_CONFIGVVD_EXE --ldflags --libs --system-libs`"
           ;;
           darwin*)
             plugin_option='-fPIC -shared -undefined dynamic_lookup '
           ;;
           *)
             plugin_option='-fPIC -shared '
           ;;
         esac
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         echo "compiling plugin $plugin_compiler -I$TOPSRCDIR/etc/clang_plugin/ $I386_LLVMVVD_CXXFLAGS -c plugin_test.cpp -o plugin_test.o -std=c++11 -fPIC"
         case $host_os in
           mingw*) 
             I386_CLANGVVD_PLUGIN_COMPILER=$plugin_compiler
             ;;
           *)
             $plugin_compiler -I$TOPSRCDIR/etc/clang_plugin/ $I386_LLVMVVD_CXXFLAGS -c plugin_test.cpp -o plugin_test.o -std=c++11 -fPIC 2> /dev/null
             $plugin_compiler plugin_test.o $plugin_option -o plugin_test.so 2> /dev/null
             if test ! -f plugin_test.so; then
               echo "checking $plugin_compiler plugin_test.o $plugin_option -o plugin_test.so ... no... Package libclang-vvd-dev missing?"
              continue
             fi
             echo "checking $plugin_compiler plugin_test.o $plugin_option -o plugin_test.so ... yes"
             ac_save_CC="$CC"
             ac_save_CFLAGS="$CFLAGS"
             CC=$I386_CLANGVVD_EXE
             CFLAGS="-fplugin=$BUILDDIR/plugin_test.so -Xclang -add-plugin -Xclang print-fns"
             AC_LANG_PUSH([C])
             AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
             I386_CLANGVVD_PLUGIN_COMPILER=$plugin_compiler,I386_CLANGVVD_PLUGIN_COMPILER=)
             AC_LANG_POP([C])
             CC=$ac_save_CC
             CFLAGS=$ac_save_CFLAGS
             #If plugin compilation fails, skip this executable
             if test "x$I386_CLANGVVD_PLUGIN_COMPILER" = x; then
               echo "plugin compilation does not work... $I386_CLANGVVD_EXE -fplugin=$BUILDDIR/plugin_test.so -Xclang -add-plugin -Xclang print-fns ?"
              continue
             fi
           ;;
         esac
         echo "OK, we have found the compiler"
         build_I386_CLANGVVD=yes;
         build_I386_CLANGVVD_EMPTY_PLUGIN=yes;
         build_I386_CLANGVVD_SSA_PLUGIN=yes;
         build_I386_CLANGVVD_SSA_PLUGINCPP=yes;
         build_I386_CLANGVVD_EXPANDMEMOPS_PLUGIN=yes;
         build_I386_CLANGVVD_GEPICANON_PLUGIN=yes;
         build_I386_CLANGVVD_CSROA_PLUGIN=yes;
         build_I386_CLANGVVD_TOPFNAME_PLUGIN=yes;
         build_I386_CLANGVVD_ASTANALYZER_PLUGIN=yes;
      done
      if test "x$I386_CLANGVVD_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done
if test x$I386_CLANGVVD_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_CLANGVVD_EMPTY_PLUGIN=clangvvd_plugin_dumpGimpleEmpty
  I386_CLANGVVD_SSA_PLUGIN=clangvvd_plugin_dumpGimpleSSA
  I386_CLANGVVD_SSA_PLUGINCPP=clangvvd_plugin_dumpGimpleSSACpp
  I386_CLANGVVD_EXPANDMEMOPS_PLUGIN=clangvvd_plugin_expandMemOps
  I386_CLANGVVD_GEPICANON_PLUGIN=clangvvd_plugin_GepiCanon
  I386_CLANGVVD_CSROA_PLUGIN=clangvvd_plugin_CSROA
  I386_CLANGVVD_TOPFNAME_PLUGIN=clangvvd_plugin_topfname
  I386_CLANGVVD_ASTANALYZER_PLUGIN=clangvvd_plugin_ASTAnalyzer
  AC_SUBST(I386_CLANGVVD_EMPTY_PLUGIN)
  AC_SUBST(I386_CLANGVVD_SSA_PLUGIN)
  AC_SUBST(I386_CLANGVVD_SSA_PLUGINCPP)
  AC_SUBST(I386_CLANGVVD_EXPANDMEMOPS_PLUGIN)
  AC_SUBST(I386_CLANGVVD_GEPICANON_PLUGIN)
  AC_SUBST(I386_CLANGVVD_CSROA_PLUGIN)
  AC_SUBST(I386_CLANGVVD_TOPFNAME_PLUGIN)
  AC_SUBST(I386_CLANGVVD_ASTANALYZER_PLUGIN)
  AC_SUBST(I386_LLVMVVD_CXXFLAGS)
  AC_SUBST(I386_CLANGVVD_EXE)
  AC_SUBST(I386_CLANGVVD_VERSION)
  AC_SUBST(I386_CLANGVVD_PLUGIN_COMPILER)
  AC_SUBST(I386_LLVM_CONFIGVVD_EXE)
  AC_SUBST(I386_CLANG_CPPVVD_EXE)
  AC_DEFINE(HAVE_I386_CLANGVVD_COMPILER, 1, "Define if CLANG vvd I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_EXE, "${I386_CLANGVVD_EXE}", "Define the plugin clang")
  AC_DEFINE_UNQUOTED(I386_CLANG_CPPVVD_EXE, "${I386_CLANG_CPPVVD_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_CLANGPPVVD_EXE, "${I386_CLANGPPVVD_EXE}", "Define the plugin clang++")
  AC_DEFINE_UNQUOTED(I386_LLVMVVD_LINK_EXE, "${I386_LLVMVVD_LINK_EXE}", "Define the plugin clang++")
  AC_DEFINE_UNQUOTED(I386_LLVMVVD_OPT_EXE, "${I386_LLVMVVD_OPT_EXE}", "Define the plugin clang++")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_EMPTY_PLUGIN, "${I386_CLANGVVD_EMPTY_PLUGIN}", "Define the filename of the CLANG PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_SSA_PLUGIN, "${I386_CLANGVVD_SSA_PLUGIN}", "Define the filename of the CLANG PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_SSA_PLUGINCPP, "${I386_CLANGVVD_SSA_PLUGINCPP}", "Define the filename of the CLANG PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_EXPANDMEMOPS_PLUGIN, "${I386_CLANGVVD_EXPANDMEMOPS_PLUGIN}", "Define the filename of the CLANG PandA expandMemOps plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_GEPICANON_PLUGIN, "${I386_CLANGVVD_GEPICANON_PLUGIN}", "Define the filename of the CLANG PandA GepiCanon plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_CSROA_PLUGIN, "${I386_CLANGVVD_CSROA_PLUGIN}", "Define the filename of the CLANG PandA CSROA plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_TOPFNAME_PLUGIN, "${I386_CLANGVVD_TOPFNAME_PLUGIN}", "Define the filename of the CLANG PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_ASTANALYZER_PLUGIN, "${I386_CLANGVVD_ASTANALYZER_PLUGIN}", "Define the filename of the CLANG PandA ASTAnalyzer plugin")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_VERSION, "${I386_CLANGVVD_VERSION}", "Define the clang version")
  AC_DEFINE_UNQUOTED(I386_CLANGVVD_PLUGIN_COMPILER, "${I386_CLANGVVD_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])