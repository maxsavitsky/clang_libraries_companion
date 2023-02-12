#include <cassert>
#include <format>
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

namespace ct = clang::tooling;
namespace cam = clang::ast_matchers;

std::vector<std::string> getPackTypeNames(const clang::TemplateArgument& arg) {
	std::vector<std::string> names;
	clang::PrintingPolicy pp{clang::LangOptions()};
	pp.SuppressTagKeyword = true;
	pp.FullyQualifiedName = true;
	for (auto packIter = arg.pack_begin(); packIter != arg.pack_end();
	  ++packIter) {
		names.push_back({});
		llvm::raw_string_ostream outStream(names.back());
		packIter->print(pp, outStream, false);
	}
	return names;
}

struct MyMatchCallback : public cam::MatchFinder::MatchCallback {
	void run(const cam::MatchFinder::MatchResult& result) override;
};

void MyMatchCallback::run(const cam::MatchFinder::MatchResult& result) {
	const clang::SourceManager& sourceManager = *result.SourceManager;
	auto tempDecl =
	  result.Nodes.getNodeAs<clang::ClassTemplateSpecializationDecl>("c");
	auto varDecl = result.Nodes.getNodeAs<clang::VarDecl>("v");
	assert(tempDecl && varDecl);
	clang::SourceRange sourceRange;
	bool nonparm = (llvm::dyn_cast<clang::ParmVarDecl>(varDecl) == nullptr);
	const clang::TemplateArgumentList& args = tempDecl->getTemplateArgs();
	if (args.size() != 1) {
		llvm::errs() << "tuple does not have one template parameter\n";
		return;
	}
	const clang::TemplateArgument& arg = args.get(0);
	if (arg.getKind() != clang::TemplateArgument::ArgKind::Pack) {
		llvm::errs() << "tuple template parameter is not a pack\n";
		return;
	}
	std::vector<std::string> names = getPackTypeNames(arg);
	clang::SourceLocation loc = varDecl->getLocation();
	assert(tempDecl->getQualifiedNameAsString() == "std::tuple");
	llvm::outs() << std::format(
	  "variable {} of type {} with {} template arguments\n",
	  varDecl->getName(), tempDecl->getQualifiedNameAsString(),
	  arg.pack_size());
	for (auto i : names) {llvm::outs() << std::format("    {}\n", i);}
}

AST_MATCHER(clang::ClassTemplateSpecializationDecl, isPartialSpecialization)
  {return llvm::dyn_cast<clang::ClassTemplatePartialSpecializationDecl>(&Node);}

AST_MATCHER(clang::VarDecl, isParmDecl)
  {return llvm::dyn_cast<clang::ParmVarDecl>(&Node);}

cam::DeclarationMatcher getMatcher() {
	using namespace cam;
	return varDecl(unless(isParmDecl()),
	  hasType(classTemplateSpecializationDecl(hasName("std::tuple"),
	  unless(isPartialSpecialization())).bind("c"))).bind("v");
}

int main(int argc, const char **argv) {
	static llvm::cl::OptionCategory optionCategory("Tool options");
	auto expectedParser = ct::CommonOptionsParser::create(argc, argv,
	  optionCategory);
	if (!expectedParser) {
		llvm::errs() << llvm::toString(expectedParser.takeError());
		return 1;
	}
	ct::CommonOptionsParser& optionsParser = expectedParser.get();
	ct::ClangTool tool(optionsParser.getCompilations(),
	  optionsParser.getSourcePathList());
	cam::DeclarationMatcher matcher = getMatcher();
	MyMatchCallback matchCallback;
	cam::MatchFinder matchFinder;
	matchFinder.addMatcher(matcher, &matchCallback);
	return !tool.run(ct::newFrontendActionFactory(&matchFinder).get()) ? 0 : 1;
}
