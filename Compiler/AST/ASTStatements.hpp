//
//  ASTStatements.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTStatements_hpp
#define ASTStatements_hpp

#include <utility>
#include "Scoping/SemanticScopeStats.hpp"
#include "ASTExpr.hpp"
#include "Scoping/IDScoper.hpp"

namespace EmojicodeCompiler {

class FunctionAnalyser;
class FunctionCodeGenerator;

class ASTStatement : public ASTNode {
public:
    virtual void generate(FunctionCodeGenerator *) const = 0;
    virtual void analyse(FunctionAnalyser *) = 0;
    virtual void analyseMemoryFlow(MFFunctionAnalyser *) = 0;

    void setParagraph() { paragraph_ = true; }
    bool paragraph() const { return paragraph_; }
protected:
    using ASTNode::ASTNode;
    bool paragraph_ = false;
};

class ASTBlock final : public ASTStatement {
public:
    explicit ASTBlock(const SourcePosition &p) : ASTStatement(p) {}

    void appendNode(std::unique_ptr<ASTStatement> node) { stmts_.emplace_back(std::move(node)); }
    /// Inserts a statement before the return statement in this block.
    /// @pre Calling this method is only valid if the last statement that is compiled is a return statement.
    ///      stopsAtReturn() returns true in this case.
    /// @see stopsAtReturn()
    void appendNodeBeforeReturn(std::unique_ptr<ASTStatement> node);

    void prependNode(std::unique_ptr<ASTStatement> node) {
        stmts_.emplace(stmts_.begin(), std::move(node));
    }

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

    /// Prints the code that goes between the block delimiters.
    void innerToCode(PrettyStream &pretty) const;
    bool returnedCertainly() const { return returnedCertainly_; }

    const SemanticScopeStats& scopeStats() { return scopeStats_; }
    void setScopeStats(SemanticScopeStats stats) { scopeStats_ = stats; }

    void popScope(FunctionAnalyser *analyser);

    /// Determines whether the last statement of the block that is compiled (i.e. that is not dead code) is a return
    /// or raise statement.
    /// @note This method can only be called before Memory Flow Analysis.
    bool stopsAtReturn() const;
    
private:
    std::vector<std::unique_ptr<ASTStatement>> stmts_;
    bool returnedCertainly_ = false;
    size_t stop_ = 0;
    SemanticScopeStats scopeStats_;
};

class ASTExprStatement final : public ASTStatement {
public:
    void analyse(FunctionAnalyser *analyser) override;

    void generate(FunctionCodeGenerator *fg) const override {
        expr_->generate(fg);
    }

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

    ASTExprStatement(std::shared_ptr<ASTExpr> expr, const SourcePosition &p) : ASTStatement(p), expr_(std::move(expr)) {}
private:
    std::shared_ptr<ASTExpr> expr_;
};

class ASTReturn : public ASTStatement {
public:
    ASTReturn(std::shared_ptr<ASTExpr> value, const SourcePosition &p) : ASTStatement(p), value_(std::move(value)) {}

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;

    void toCode(PrettyStream &pretty) const override;
    void analyseMemoryFlow(MFFunctionAnalyser *analyser) override;

    /// Informs the expression that it is used to return the initialized object from an object initializer.
    void setIsInitReturn() { initReturn_ = true; }

protected:
    std::shared_ptr<ASTExpr> value_;
    bool initReturn_ = false;
};

class ASTRaise final : public ASTReturn {
public:
    using ASTReturn::ASTReturn;

    void analyse(FunctionAnalyser *analyser) override;
    void generate(FunctionCodeGenerator *) const override;
    void toCode(PrettyStream &pretty) const override;
private:
    bool boxed_ = false;
};

} // namespace EmojicodeCompiler

#endif /* ASTStatements_hpp */
