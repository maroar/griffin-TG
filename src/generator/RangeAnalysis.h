/******************************************************************************
 * Copyright (c) 2017 Marcus Rodrigues de Araújo (demaroar@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 *****************************************************************************/

#ifndef PSYCHE_RANGEANALYSIS_H__
#define PSYCHE_RANGEANALYSIS_H__

#include "ASTVisitor.h"
#include "Range.h"
#include "VersionedMap.h"
#include <stack>
#include <map>

namespace psyche {

struct ArrayInfo {
    const CPlusPlus::Symbol* name_;
    std::map<int, Range> dimensionRange_;
    std::vector<bool> dimensionIsFixed_;

    ArrayInfo(const CPlusPlus::Symbol* name);
    void addRange(int dimension, Range& range);
    void addRangeReal(int dimension, Range &range);
    std::unique_ptr<AbstractValue> dimensionLength(int dimension);
    std::string name();
    void print();
};

class RangeAnalysis final : public CPlusPlus::ASTVisitor
{
public:
    RangeAnalysis(CPlusPlus::TranslationUnit *unit);

    void run(CPlusPlus::TranslationUnitAST *ast, CPlusPlus::Namespace *global);

//private:
    /*!
     * \brief switchScope
     * \param scope
     * \return
     *
     * Convenience to switch current scope.
     */
    const CPlusPlus::Scope *switchScope(const CPlusPlus::Scope *scope);

    /*!
     * \brief switchSymbol
     * \param symbol
     * \return
     */
    const CPlusPlus::Symbol* switchSymbol(const CPlusPlus::Symbol* symbol);

    /*!
     * \brief resolve
     * \param name
     *
     * Resolve a name to a symbol.
     */
    void resolve(const CPlusPlus::Name* name);

    /*!
     * \brief make the union of two relations
     * \param name
     *
     * given two range maps this method merge then.
     */
    void mapUnion(VersionedMap<const CPlusPlus::Symbol*, Range> *a,
                       VersionedMap<const CPlusPlus::Symbol*, Range> *b);

    void wideRanges(std::map<const CPlusPlus::Symbol*, Range> &refValues,
                    std::map<const CPlusPlus::Symbol*, std::list<Range> > &history);
    /*!
     * \brief returns the range of a given symbol
     * \param symbol
     *
     * returns the range of a given symbol.
     */
    std::unique_ptr<Range> getRangeOfSymbol(const CPlusPlus::Symbol* symbol);

    /*!
     * \brief save all table of ranges in a given point of the program
     * \param statement
     *
     */
    void saveState(const CPlusPlus::StatementAST* stmt);

    // auxiliary functions to extract information from boolean expressions that are
    // composed by two symbols
    // example: "if (va < vb) then St else Sf"
    //          va : [la, ua] | vb : [lb, ub]
    //          Rt = (R \ va : [la, min(ub − 1, ua)]) \ vb : [max(la + 1, lb), ub]
    //          Rf = (R \ va : [max(la, lb), ua]) \ vb : [lb, min(ua, ub)]
    //          rg(Rt, St) = Rt'
    //          rg(Rf, Sf) = Rf'
    //          R' = Rt' U Rf'
    //          -----------------
    //          rg(R, "if(va < vb) then St else Sf") = R'
    Range rangeForAWhenTrue(Range *lrange, Range *rrange,unsigned op); // va : [la, min(ub − 1, ua)])
    Range rangeForBWhenTrue(Range *lrange, Range *rrange,unsigned op); // vb : [max(la + 1, lb), ub]
    Range rangeForAWhenFalse(Range *lrange, Range *rrange,unsigned op); // va : [max(la, lb), ua]
    Range rangeForBWhenFalse(Range *lrange, Range *rrange,unsigned op); // vb : [lb, min(ua, ub)]

    bool lowerIsDecreasing(const std::list<Range> &ranges);
    bool upperIsGrowing(const std::list<Range> &ranges);

    // Top-level visitation entry points.
    void visitStatement(CPlusPlus::StatementAST *ast);
    void visitDeclaration(CPlusPlus::DeclarationAST *ast);
    void visitConditionWhenTrue(CPlusPlus::ExpressionAST *ast);
    void visitConditionWhenFalse(CPlusPlus::ExpressionAST *ast);
    void visitLoopBody(CPlusPlus::StatementAST *ast);
    bool visitLoop(CPlusPlus::StatementAST *ast);
    void visitLoopConditionWhenTrue(CPlusPlus::StatementAST *ast);
    void visitLoopConditionWhenFalse(CPlusPlus::StatementAST *ast);

    // Declarations
    bool visit(CPlusPlus::FunctionDefinitionAST *ast) override;
    bool visit(CPlusPlus::ParameterDeclarationAST *ast) override;
    bool visit(CPlusPlus::SimpleDeclarationAST *ast) override;

    // Declarator
    bool visit(CPlusPlus::DeclaratorAST *ast) override;
    bool visit(CPlusPlus::DeclaratorIdAST *ast) override;
    bool visit(CPlusPlus::ParameterDeclarationClauseAST *ast) override;

    // PostfixDeclarator
    bool visit(CPlusPlus::FunctionDeclaratorAST *ast) override;
    bool visit(CPlusPlus::ArrayDeclaratorAST *ast) override;
    bool visit(CPlusPlus::ArrayAccessAST *ast) override;
    bool visit(CPlusPlus::PointerAST *ast) override;

    // Expressions
    bool visit(CPlusPlus::BinaryExpressionAST* ast) override;
    bool visit(CPlusPlus::ConditionalExpressionAST* ast) override;
    bool visit(CPlusPlus::IdExpressionAST* ast) override;
    bool visit(CPlusPlus::MemberAccessAST* ast) override;
    bool visit(CPlusPlus::NumericLiteralAST* ast) override;
    bool visit(CPlusPlus::PostIncrDecrAST* ast) override;
    bool visit(CPlusPlus::CallAST* ast) override;
    bool visit(CPlusPlus::UnaryExpressionAST* ast) override;

    // Statements
    bool visit(CPlusPlus::CompoundStatementAST *ast) override;
    bool visit(CPlusPlus::DeclarationStatementAST *ast) override;
    bool visit(CPlusPlus::ExpressionStatementAST *ast) override;
    bool visit(CPlusPlus::IfStatementAST *ast) override;
    bool visit(CPlusPlus::WhileStatementAST *ast) override;
    bool visit(CPlusPlus::ForStatementAST *ast) override;

    void dumpRanges() const;
    void checkForPointerDefinition(CPlusPlus::ExpressionAST *ast);
    void insertOrAssign(const CPlusPlus::Symbol*, Range*);
    void insertOrAssign(const CPlusPlus::Symbol*, Range);
    void insertDefinitionToPointer(const CPlusPlus::Symbol*,
                                   CPlusPlus::ExpressionAST*,
                                   const CPlusPlus::StatementAST*);
    void insertAccessToPointer(const CPlusPlus::Symbol*,
                               unsigned,
                               CPlusPlus::ExpressionAST*,
                               const CPlusPlus::StatementAST*);
    void printResult();
    void dumpPointerIsArray();

    //! Scope we're in.
    const CPlusPlus::Scope *scope_;

    //! Symbol just looked-up.
    const CPlusPlus::Symbol* symbol_;

    //! Range stack.
    std::stack<Range> stack_;

    //! Enclosing statement
    CPlusPlus::StatementAST* enclosingStmt_;

    std::unordered_map<const CPlusPlus::StatementAST*, int32_t> revisionMap_;
    VersionedMap<const CPlusPlus::Symbol*, Range> rangeMap_;

    // deal with loops
    bool savingStateEnable_;
    void enteringInsideTheLoop(const CPlusPlus::StatementAST*);
    void goingOutFromTheLoop();

    // dealing with array
    const CPlusPlus::Symbol* currentArrayIdentifierSymbol_;
    int arrayAccessDepth_, currentArrayAccessIndex_;
    std::map<const CPlusPlus::Symbol*, bool> pointerIsArray_;
    std::map<const CPlusPlus::Symbol*, ArrayInfo> arrayInfoMap_;
    // array constraints
    std::map<const CPlusPlus::Symbol*,
             std::list<std::pair<CPlusPlus::ExpressionAST*,
                                 const CPlusPlus::StatementAST*> > > arrayDefinitions_;
    std::map<std::pair<const CPlusPlus::Symbol*, unsigned>,
             std::list<std::pair<CPlusPlus::ExpressionAST*,
                                 const CPlusPlus::StatementAST*> > > arrayAccesses_;

    // used for loop bounds analysis
    CPlusPlus::TranslationUnit* unit_;

    // range analysis values/programPoint
    std::map<const CPlusPlus::StatementAST*,
             std::list<std::pair<const CPlusPlus::Symbol*, Range> > > rangeAnalysis_;
    std::list<const CPlusPlus::StatementAST*> statementsOrder_;

    // boolean used to initialize just parameters values (function parameters
    // and global variables)
    bool parameterScop_;
};

} // namespace psyche

#endif
