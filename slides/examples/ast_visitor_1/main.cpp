#include <format>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/CommandLine.h>
#include <utility>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <memory>
#include <thread>

namespace ct = clang::tooling;

class MyAstVisitor : public clang::RecursiveASTVisitor<MyAstVisitor> {
public:
	MyAstVisitor(clang::ASTContext& astContext, std::string filename)
        : astContext_(&astContext), filename_(std::move(filename)) {
    }

	bool VisitVarDecl(clang::VarDecl* varDecl)  {
        const auto &fileId = astContext_->getSourceManager().getFileID(
                varDecl->getLocation());
        const auto &decl = *varDecl;
        const auto &parents = astContext_->getParents(decl);
        bool isSingleParent = parents.size() == 1 && parents[0].get<clang::TranslationUnitDecl>() == astContext_->getTranslationUnitDecl();
        auto type = varDecl->getType();
        bool containsDots = varDecl->getQualifiedNameAsString().find("::") != std::string::npos; // check for namespace variable (e.g. std::cout)
        if (fileId == astContext_->getSourceManager().getMainFileID()
            && varDecl->getParentFunctionOrMethod() == nullptr // check parent
            && !type.isConstQualified() // check for const
            && isSingleParent // check parent
            && !varDecl->isLocalVarDeclOrParm() // check if variable is not in function (same as check parent)
            && !varDecl->isStaticLocal() && !varDecl->isStaticDataMember() // static variables are not suitable
            && varDecl->getLanguageLinkage() != clang::LanguageLinkage::CLanguageLinkage // check for extern
            && !containsDots) {
            names_.push_back(varDecl->getQualifiedNameAsString());
        }
        return true;
    }

    const std::vector<std::string>& getNames() {
        return names_;
    }
private:
	clang::ASTContext* astContext_;
    std::vector<std::string> names_;
    std::string filename_;
};

class MyAstConsumer : public clang::ASTConsumer {
public:
    MyAstConsumer(std::shared_ptr<std::ofstream> out, std::string filename) : out(std::move(out)), filename_(filename) {
        int last_slash_pos = filename.rfind('/');
        if (last_slash_pos != std::string::npos) {
            filename_ = filename_.substr(last_slash_pos + 1);
        }
    }

	void HandleTranslationUnit(clang::ASTContext& astContext) final {
		clang::TranslationUnitDecl* tuDecl =
		  astContext.getTranslationUnitDecl();
		MyAstVisitor visitor(astContext, filename_);
		visitor.TraverseDecl(tuDecl);
        flushToFile(visitor.getNames());
	}

    void flushToFile(std::vector<std::string> names) {
        std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b){
            for (size_t i = 0; i < a.size() && i < b.size(); i++) {
                if (std::tolower(a[i]) != std::tolower(b[i]))
                    return (std::tolower(a[i]) < std::tolower(b[i]));
            }
            return a.size() < b.size();
        });
        *out << filename_;
        for (int i = 0; i < names.size(); i++) {
            *out << " " << names[i];
        }
        *out << "\n";
    }
private:
    std::string filename_;
    std::shared_ptr<std::ofstream> out;
};

class MyFrontendAction : public clang::ASTFrontendAction {
public:
    MyFrontendAction(std::shared_ptr<std::ofstream> out) : out(std::move(out)) {}

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
	  clang::CompilerInstance&, clang::StringRef filename) final {
		return std::unique_ptr<clang::ASTConsumer>{new MyAstConsumer(out, std::string(filename))};
	}
private:
    std::shared_ptr<std::ofstream> out;
};

static llvm::cl::OptionCategory toolOptions("Tool Options");

class SimpleFrontendActionFactory : public ct::FrontendActionFactory {
public:
    SimpleFrontendActionFactory(std::shared_ptr<std::ofstream> out) : out(std::move(out)) {}

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<MyFrontendAction>(out);
    }
private:
    std::shared_ptr<std::ofstream> out;
};

std::unique_ptr<ct::FrontendActionFactory> newFrontendActionFactory(std::shared_ptr<std::ofstream> out) {
    return std::unique_ptr<ct::FrontendActionFactory>(
            new SimpleFrontendActionFactory(std::move(out)));
}

void run_tool(ct::CommonOptionsParser* parser, const std::vector<std::string>& sources, const std::shared_ptr<std::ofstream>& out) {
    ct::ClangTool tool(parser->getCompilations(), sources);
    int status = tool.run(
            newFrontendActionFactory(out).get());
    llvm::outs() << "tool exited with " << status << " status\n";
    out->flush();
}

int main(int argc, char** argv) {
	auto expectedOptionsParser = ct::CommonOptionsParser::create(argc,
	  const_cast<const char**>(argv), toolOptions);
	if (!expectedOptionsParser) {
		llvm::errs() << std::format("Unable to create option parser ({}).\n",
		  llvm::toString(expectedOptionsParser.takeError()));
		return 1;
	}
	ct::CommonOptionsParser& optionsParser = *expectedOptionsParser;
	const std::vector<std::string>& sources = optionsParser.getSourcePathList();
    const int kThreadsCount = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreadsCount);

    const int chunk_size = static_cast<int>(sources.size()) / kThreadsCount;

    std::string filenames_in_line;

    for (int i = 0; i < kThreadsCount; i++) {
        int begin_index = i * chunk_size;
        int end_index = i == kThreadsCount - 1 ? static_cast<int>(sources.size()) : (i + 1) * chunk_size;
        std::string filename = std::string("threaded_output_") + std::to_string(i) + ".txt";
        filenames_in_line += filename + " ";

        std::shared_ptr<std::ofstream> stream_ptr = std::make_shared<std::ofstream>(filename);
        std::vector<std::string> vec(sources.begin() + begin_index, sources.begin() + end_index);
        std::thread thr(
                run_tool,
                &optionsParser,
                vec,
                stream_ptr);
        threads.push_back(std::move(thr));
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    std::string command = std::format("cat {} | sort > output.txt", filenames_in_line); // unite files
    system(command.c_str());
    return 0;
}
