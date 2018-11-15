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

#include "RangeAnalysis.h"
#include "AST.h"
#include "Assert.h"
#include "Debug.h"
#include "Literals.h"
#include "LoopAnalyser.h"
#include "Lookup.h"
#include "Scope.h"
#include "Symbols.h"
#include "TranslationUnit.h"
#include "Utils.h"
#include <cstdlib>
#include <memory>
#include <set>

#define VISITOR_NAME "RangeAnalysis"

using namespace CPlusPlus;
using namespace psyche;
using namespace std;

RangeAnalysis::RangeAnalysis(TranslationUnit *unit)
    : ASTVisitor(unit),
      unit_(unit)
{}

void RangeAnalysis::run(TranslationUnitAST *ast, Namespace *global)
{
    // initialization variables
    savingStateEnable_ = true;
    arrayAccessDepth_ = 0;
    currentArrayAccessIndex_ = 0;
    parameterScop_ = true;  // local variables should not be initialized... just global, and parameters...

    switchScope(global);
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next)
        accept(it->value);

    if (debugVisit) {
        printResult();
        dumpPointerIsArray();
    }
}

void RangeAnalysis::dumpRanges() const
{
    // TEMP...
    for (auto p : rangeMap_) {
        if (p.first->name()
                && p.first->name()->asNameId()) {
            Range range = p.second;
            std::cout << p.first->name()->asNameId()->identifier()->chars()
                      << ": " << range << std::endl;
        }
    }
}

void RangeAnalysis::insertOrAssign(const CPlusPlus::Symbol* symbol, Range *range)
{
    rangeMap_.insertOrAssign(symbol, std::move(*range));
}

void RangeAnalysis::insertOrAssign(const CPlusPlus::Symbol* symbol, Range range)
{
    rangeMap_.insertOrAssign(symbol, std::move(range));
}

void RangeAnalysis::insertDefinitionToPointer(const Symbol* sym, ExpressionAST* exp,
                                              const StatementAST* stmt)
{
    arrayDefinitions_[sym].push_back(std::make_pair(exp, stmt));
}

void RangeAnalysis::insertAccessToPointer(const Symbol* sym, unsigned dimension,
                                          ExpressionAST* exp, const StatementAST* stmt)
{
    auto access = std::make_pair(sym, dimension);
    arrayAccesses_[access].push_back(std::make_pair(exp, stmt));
}

const Scope *RangeAnalysis::switchScope(const Scope *scope)
{
    PSYCHE_ASSERT(scope, return nullptr, "scope must be valid");
    std::swap(scope_, scope);
    return scope;
}

const CPlusPlus::Symbol *RangeAnalysis::switchSymbol(const CPlusPlus::Symbol *symbol)
{
    PSYCHE_ASSERT(symbol, return nullptr, "symbol must be valid");
    std::swap(symbol_, symbol);
    return symbol;
}

// Extract the identifier of a name. Use this only when it's guaranteed that
// the underlying name is indeed a simple one.
std::string extractId(const Name* name)
{
    PSYCHE_ASSERT(name && name->isNameId(),
                  return std::string(),
                  "expected simple name");

    const Identifier *id = name->asNameId()->identifier();
    return std::string (id->chars(), id->size());
}

void RangeAnalysis::resolve(const Name *name)
{
    // We expect to see only simple names. Also, since range analysis assumes
    // well-formed code, symbol lookup must succeed.
    PSYCHE_ASSERT(name && name->asNameId(), return, "expected simple name");
// resolvename
//std::cout << ". "<< std::endl;
    std::string sym = extractId(name);
    if (debugVisit)
        std::cout << "resolve: " << sym << std::endl;
    symbol_ = lookupValueSymbol(name, scope_);
    PSYCHE_ASSERT(symbol_, return, "expected successful lookup");
//std::cout << ". "<< std::endl;
}

void RangeAnalysis::saveState(const CPlusPlus::StatementAST* stmt)
{
    for (auto it : rangeMap_) {
        rangeAnalysis_[stmt].push_back(std::make_pair(it.first, it.second));
    }

    statementsOrder_.push_back(stmt);
}

void RangeAnalysis::printResult()
{
    unsigned line;
    for (auto point : statementsOrder_) {
        translationUnit()->getTokenPosition(point->lastToken(), &line);
        auto infoTab = rangeAnalysis_.find(point);
        if (infoTab != rangeAnalysis_.end()) {
            std::cout << "------- " << " line: " << line << " ------- " << std::endl;
            for (auto info : infoTab->second) {
                std::cout << extractId(info.first->name());
                std::cout << " : " << info.second << "" << std::endl;
            }
            std::cout << "------------------------- \n" << std::endl;
        }
    }
}

void RangeAnalysis::dumpPointerIsArray()
{
    std::cout << "Array Info:" << std::endl;
    for (auto itPointerIsArray : pointerIsArray_) {
        if (itPointerIsArray.second) {
            auto itArrayInfoMap = arrayInfoMap_.find(itPointerIsArray.first);
            if (itArrayInfoMap != arrayInfoMap_.end()) {
                std::cout << itArrayInfoMap->second.name() << ":" << std::endl;
                for (auto itRanges : itArrayInfoMap->second.dimensionRange_) {
                    std::cout << "  " << itRanges.first << " : " << itRanges.second << " "
                              << "(";
                    std::cout << itArrayInfoMap->second.dimensionLength(itRanges.first)->toString();
                    std::cout << ")" << std::endl;
                }
                std::cout << "------------" << std::endl;
            }
        }
    }
}

void RangeAnalysis::visitStatement(StatementAST *ast)
{
    accept(ast);
}

void RangeAnalysis::visitDeclaration(DeclarationAST *ast)
{
    accept(ast);
}

bool isRelational(unsigned op)
{
    switch (op) {
    case T_LESS:
        return true;
    case T_LESS_EQUAL:
        return true;
    case T_GREATER:
        return true;
    case T_GREATER_EQUAL:
        return true;
    case T_EQUAL_EQUAL:
        return true;
    case T_EXCLAIM_EQUAL:
        return true;
    default:
        return false;
    }
    return false;
}

void RangeAnalysis::visitConditionWhenTrue(ExpressionAST *ast)
{
    DEBUG_VISIT(visitConditionWhenTrue);

    if (ast->asBinaryExpression()) {
        unsigned op = tokenKind(ast->asBinaryExpression()->binary_op_token);
        ExpressionAST* leftExpression = ast->asBinaryExpression()->left_expression;
        ExpressionAST* rightExpression = ast->asBinaryExpression()->right_expression;

        if (isRelational(op)) { // op == '<' || '>' || '=='...
            bool aIsSymbol = false, bIsSymbol = false;
            // va
            const CPlusPlus::Symbol* leftSymbol;
            // Ra
            std::unique_ptr<Range> rangeA;
            // vb
            const CPlusPlus::Symbol* rightSymbol;
            // Rb
            std::unique_ptr<Range> rangeB;

            if (leftExpression->asIdExpression()) { // get the range of va (or left expression)
                aIsSymbol = true;
                leftSymbol = lookupValueSymbol(leftExpression->asIdExpression()->name->name, scope_);
                rangeA = getRangeOfSymbol(leftSymbol);
            } else {
                accept(leftExpression);
                rangeA = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }

            if (rightExpression->asIdExpression()) { // get the range of vb (or right expression)
                bIsSymbol = true;
                rightSymbol = lookupValueSymbol(rightExpression->asIdExpression()->name->name, scope_);
                rangeB = getRangeOfSymbol(rightSymbol);
            } else {
                accept(rightExpression);
                rangeB = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }

            Range rangeAWhenTrue(rangeForAWhenTrue(rangeA.get(), rangeB.get(), op));
            Range rangeBWhenTrue(rangeForBWhenTrue(rangeA.get(), rangeB.get(), op));

            // Rt'
            if (aIsSymbol)
                insertOrAssign(leftSymbol, rangeForAWhenTrue(rangeA.get(), rangeB.get(), op));
            if (bIsSymbol)
                insertOrAssign(rightSymbol, rangeForBWhenTrue(rangeA.get(), rangeB.get(), op));
        }
    } else {
        accept(ast);
    }
}

void RangeAnalysis::visitConditionWhenFalse(CPlusPlus::ExpressionAST *ast)
{
    DEBUG_VISIT(visitConditionWhenFalse);

    if (ast->asBinaryExpression()) {
        unsigned op = tokenKind(ast->asBinaryExpression()->binary_op_token);
        ExpressionAST* leftExpression = ast->asBinaryExpression()->left_expression;
        ExpressionAST* rightExpression = ast->asBinaryExpression()->right_expression;

        if (isRelational(op)) { // op == '<' || '>' || '=='...
            bool aIsSymbol = false, bIsSymbol = false;
            // va
            const CPlusPlus::Symbol* leftSymbol;
            // Ra
            std::unique_ptr<Range> rangeA;
            // vb
            const CPlusPlus::Symbol* rightSymbol;
            // Rb
            std::unique_ptr<Range> rangeB;

            if (leftExpression->asIdExpression()) { // get the range of va (or left expression)
                aIsSymbol = true;
                leftSymbol = lookupValueSymbol(leftExpression->asIdExpression()->name->name, scope_);
                rangeA = getRangeOfSymbol(leftSymbol);
            } else {
                accept(leftExpression);
                rangeA = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }

            if (rightExpression->asIdExpression()) { // get the range of vb (or right expression)
                bIsSymbol = true;
                rightSymbol = lookupValueSymbol(rightExpression->asIdExpression()->name->name, scope_);
                rangeB = getRangeOfSymbol(rightSymbol);
            } else {
                accept(rightExpression);
                rangeB = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }

            Range rangeAWhenFalse(rangeForAWhenTrue(rangeA.get(), rangeB.get(), op));
            Range rangeBWhenFalse(rangeForBWhenTrue(rangeA.get(), rangeB.get(), op));

            // Rt'
            if (aIsSymbol) {
                Range valueAfterCondition(*getRangeOfSymbol(leftSymbol));
                insertOrAssign(leftSymbol, Range(valueAfterCondition.rangeUnion(rangeAWhenFalse)));
            }

            if (bIsSymbol) {
                Range valueBfterCondition(*getRangeOfSymbol(rightSymbol));
                insertOrAssign(rightSymbol, Range(valueBfterCondition.rangeUnion(rangeBWhenFalse)));
            }
        }
    }
}

bool RangeAnalysis::visit(ParameterDeclarationClauseAST *ast)
{
    DEBUG_VISIT(ParameterDeclarationClauseAST);

    for (ParameterDeclarationListAST *iter = ast->parameter_declaration_list; iter;
         iter = iter->next) {
        accept(iter->value);
    }

    return false;
}

bool RangeAnalysis::visit(ParameterDeclarationAST *ast)
{
    DEBUG_VISIT(ParameterDeclarationAST);

    if (ast->declarator)
        accept(ast->declarator);

    return false;
}

// PostfixDeclarator

bool RangeAnalysis::visit(FunctionDeclaratorAST *ast)
{
    DEBUG_VISIT(FunctionDeclaratorAST);

    // set the name of function as something that has a type

    Function* func = ast->symbol;

    if (ast->parameter_declaration_clause) {
        const Scope *previousScope = switchScope(func->asScope());
        visit(ast->parameter_declaration_clause);
        switchScope(previousScope);
    }

    return false;
}

bool RangeAnalysis::visit(ArrayDeclaratorAST *ast)
{
    DEBUG_VISIT(ArrayDeclaratorAST);

    pointerIsArray_[symbol_] = true;

    currentArrayAccessIndex_++;

    if (ast->expression) {
        accept(ast->expression);
        auto it = arrayInfoMap_.find(symbol_);
        if (it == arrayInfoMap_.end()) {
            ArrayInfo arrayInfo(symbol_);
            arrayInfo.addRangeReal(currentArrayAccessIndex_, stack_.top());
            arrayInfoMap_.insert(std::make_pair(symbol_, arrayInfo));
        } else {
            it->second.addRangeReal(currentArrayAccessIndex_, stack_.top());
        }

        stack_.pop();
    }

    return false;
}

bool RangeAnalysis::visit(ArrayAccessAST *ast)
{
    DEBUG_VISIT(ArrayAccessAST);

    arrayAccessDepth_++;

    accept(ast->base_expression);
    const Symbol* sym = symbol_;

    currentArrayAccessIndex_++;

    insertAccessToPointer(sym, currentArrayAccessIndex_, ast->expression, enclosingStmt_);

    accept(ast->expression);

    auto it = arrayInfoMap_.find(currentArrayIdentifierSymbol_);

    if (it != arrayInfoMap_.end()) {
        it->second.addRange(currentArrayAccessIndex_, stack_.top());
    }

    arrayAccessDepth_--;
    if (!arrayAccessDepth_) {
        currentArrayAccessIndex_ = 0;
        stack_.push(*getRangeOfSymbol(sym));
    }

    symbol_ = sym;

    return false;
}

// Declarator

bool RangeAnalysis::visit(PointerAST *ast)
{
    DEBUG_VISIT(PointerAST);

    return false;
}

bool RangeAnalysis::visit(DeclaratorAST *ast)
{
    DEBUG_VISIT(DeclaratorAST);

    accept(ast->core_declarator);
    const Symbol* sym = symbol_;

    // T**...*
    if (ast->ptr_operator_list) {
        pointerIsArray_[symbol_] = true;

        unique_ptr<ArrayInfo> arrayInfo;
        auto it = arrayInfoMap_.find(symbol_);
        if (it != arrayInfoMap_.end())
            arrayInfo = make_unique<ArrayInfo>(it->second);
        else
            arrayInfo = make_unique<ArrayInfo>(symbol_);

        int dimension = 1;
        IntegerValue zero(0);
        Range rg(zero.evaluate(), zero.evaluate());
        for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
            arrayInfo->addRange(dimension, rg);
            dimension++;
        }
        arrayInfoMap_.insert(std::make_pair(symbol_, *arrayInfo));
    }

    for (PostfixDeclaratorListAST *iter = ast->postfix_declarator_list; iter;
         iter = iter->next) {
        symbol_ = sym; //
        accept(iter->value);
    }

    // if the declarator is an A[m][n],
    // the currentArrayAccessIndex_ will be 2
    currentArrayAccessIndex_ = 0;

    if (tokenKind(ast->equal_token) == T_EQUAL) {
        checkForPointerDefinition(ast->initializer);
        stack_.pop();
        const CPlusPlus::Symbol* lhsSymbol = symbol_;
        accept(ast->initializer);
        insertOrAssign(lhsSymbol, std::move(stack_.top()));
        stack_.pop();
    }

    return false;
}

bool RangeAnalysis::visit(DeclaratorIdAST *ast)
{
    DEBUG_VISIT(DeclaratorIdAST);

    resolve(ast->name->name);

    // We must push the range for this symbol onto the stack. If there's already
    // a range associated to it, we use it. Otherwise a symbolic range is created.
    const auto it = rangeMap_.find(symbol_);
    if (it != rangeMap_.end()) {
        std::cout << " in map" << std::endl;
    } else {
        Range range(std::make_unique<SymbolValue>(symbol_),
                    std::make_unique<SymbolValue>(symbol_));
        stack_.push(std::move(range));
        if (parameterScop_)
            insertOrAssign(symbol_, std::move(stack_.top()));
    }

    return false;
}

// Declarations

bool RangeAnalysis::visit(FunctionDefinitionAST *ast)
{
    DEBUG_VISIT(FunctionDefinitionAST);

    Function* func = ast->symbol;
    const Scope *previousScope = switchScope(func->asScope());

    accept(ast->declarator->asDeclarator());
    parameterScop_ = false; // local variables should not be initialized
    visitStatement(ast->function_body);
    parameterScop_ = true;
    switchScope(previousScope);
    return false;
}

bool RangeAnalysis::visit(SimpleDeclarationAST *ast)
{
    DEBUG_VISIT(SimpleDeclarationAST);

    for (DeclaratorListAST *iter = ast->declarator_list; iter; iter = iter->next) {
        accept(iter->value);
    }

    return false;
}

// Expressions

bool RangeAnalysis::visit(CPlusPlus::UnaryExpressionAST* ast)
{
    DEBUG_VISIT(UnaryExpressionAST);

    auto tokenk = tokenKind(ast->unary_op_token);

    if (tokenk == T_MINUS) {
        accept(ast->expression);
        Range range1 = std::move(stack_.top());
        stack_.pop();
        IntegerValue iv(-1);
        Range range2 = Range(*range1.lower_ * iv, *range1.upper_ * iv);
        stack_.push(std::move(range2));
    } else if (tokenk == T_MINUS_MINUS) {
        accept(ast->expression);
        Range range1 = std::move(stack_.top());
        stack_.pop();
        IntegerValue iv(1);
        Range range2 = Range(*range1.lower_ - iv, *range1.upper_ - iv);
        stack_.push(std::move(range2.evaluate()));
        insertOrAssign(symbol_, range2);
    } else if (tokenk == T_PLUS_PLUS) {
        accept(ast->expression);
        Range range1 = std::move(stack_.top());
        stack_.pop();
        IntegerValue iv(1);
        Range range2 = Range(*range1.lower_ + iv, *range1.upper_ + iv);
        stack_.push(std::move(range2.evaluate()));
        insertOrAssign(symbol_, range2);
    } else if (tokenk == T_STAR) {
        accept(ast->expression);
    }

    return false;
}

bool RangeAnalysis::visit(PostIncrDecrAST *ast)
{
    DEBUG_VISIT(PostIncrDecrAST);

    accept(ast->base_expression);

    Range range = std::move(stack_.top());
    stack_.pop();

    if (tokenKind(ast->incr_decr_token) == T_PLUS_PLUS) {
        insertOrAssign(symbol_, Range(*range.lower_ + 1, *range.upper_ + 1));
    } else if (tokenKind(ast->incr_decr_token) == T_MINUS_MINUS) {
        insertOrAssign(symbol_, Range(*range.lower_ - 1, *range.upper_ - 1));
    }

    return false;
}

bool RangeAnalysis::visit(NumericLiteralAST *ast)
{
    DEBUG_VISIT(NumericLiteralAST);

    const NumericLiteral *numLit = numericLiteral(ast->literal_token);
    PSYCHE_ASSERT(numLit, return false, "numeric literal must exist");

    // We're interested only on natural numbers. In the case this literal
    // is a floating-point, our interpretation is to truncate it.
    auto value = std::atoi(numLit->chars());
    Range range(std::make_unique<IntegerValue>(value),
                std::make_unique<IntegerValue>(value));
    stack_.push(std::move(range));

    return false;
}

bool RangeAnalysis::visit(IdExpressionAST *ast)
{
    DEBUG_VISIT(IdExpressionAST);

    resolve(ast->name->name);

    // verify if we are inside an identifier of an array access
    if (arrayAccessDepth_ && !currentArrayAccessIndex_) {
        if (!pointerIsArray_[symbol_])
            pointerIsArray_[symbol_] = true;

        auto it = arrayInfoMap_.find(symbol_);
        if (it == arrayInfoMap_.end()) {
            arrayInfoMap_.insert(std::make_pair(symbol_, ArrayInfo(symbol_)));
        }
        currentArrayIdentifierSymbol_ = symbol_;
    }

    // When the symbol is a scope, we only need to enter it.
    if (symbol_->type()->asClassType()) {
        switchScope(symbol_->type()->asClassType());
        return false;
    }

    // We must push the range for this symbol onto the stack. If there's already
    // a range associated to it, we use it. Otherwise a symbolic range is created.
    const auto it = rangeMap_.find(symbol_);
    if (it != rangeMap_.end()) {
        stack_.push(it->second);
    } else {
        Range range(std::make_unique<SymbolValue>(symbol_),
                    std::make_unique<SymbolValue>(symbol_));
        stack_.push(std::move(range));
    }

    return false;
}



void RangeAnalysis::checkForPointerDefinition(ExpressionAST *ast)
{
    auto pointerIsArrayIt = pointerIsArray_.find(symbol_);
    if (pointerIsArrayIt != pointerIsArray_.end() &&
            pointerIsArrayIt->second == true)  { // collect all the asingnments to pointers
        insertDefinitionToPointer(symbol_, ast, enclosingStmt_);
    }
}

bool RangeAnalysis::visit(BinaryExpressionAST *ast)
{
    DEBUG_VISIT(BinaryExpressionAST);

    unsigned op = tokenKind(ast->binary_op_token);

    accept(ast->left_expression);

    // In the case of an assignment, we drop the left-hand-side's range, since
    // it will be overwritten by the one from the right-hand-side.
    if (op == T_EQUAL) {//if (op == T_EQUAL && ast->left_expression->asIdExpression()) {
        stack_.pop();
        const CPlusPlus::Symbol* lhsSymbol = symbol_;
        checkForPointerDefinition(ast->right_expression);
        accept(ast->right_expression);
        insertOrAssign(lhsSymbol, std::move(stack_.top()));
        stack_.pop();
        return false;
    }

    // When two expressions are added, we must correspondigly add their ranges
    // and push the result onto the stack.

    // (Va + Vb) : [la + lb, ua + ub]
    if (op == T_PLUS) {
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        auto lower = *lhsRange.lower_ + *rhsRange.lower_;
        auto upper = *lhsRange.upper_ + *rhsRange.upper_;
        stack_.push(Range(std::move(lower), std::move(upper)));

        return false;
    }

    // Va += Vb -> Va : [la + lb, ua + ub]
    if (op == T_PLUS_EQUAL) {
        const CPlusPlus::Symbol* lhsSymbol = symbol_;
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        Range range(*lhsRange.lower_ + *rhsRange.lower_,
                    *lhsRange.upper_ + *rhsRange.upper_);
        insertOrAssign(lhsSymbol, range);

        return false;
    }

    // (Va - Vb) : [la - ub, ua - lb]]
    if (op == T_MINUS) {
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        auto lower = *lhsRange.lower_ - *rhsRange.upper_;
        auto upper = *lhsRange.upper_ - *rhsRange.lower_;
        stack_.emplace(std::move(lower), std::move(upper));

        return false;
    }

    // Va -= Vb -> Va : [la - ub, ua - lb]
    if (op == T_MINUS_EQUAL) {
        const CPlusPlus::Symbol* lhsSymbol = symbol_;
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        Range range(*lhsRange.lower_ - *rhsRange.upper_, *lhsRange.upper_ - *rhsRange.lower_);
        insertOrAssign(lhsSymbol, range);

        return false;
    }

    // (Va * Vb) : [la * lb, ua * ub]]
    if (op == T_STAR) {
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        auto lower = *lhsRange.lower_ * *rhsRange.lower_;
        auto upper = *lhsRange.upper_ * *rhsRange.upper_;
        stack_.emplace(std::move(lower), std::move(upper));

        return false;
    }

    // Va *= Vb -> Va : [la * lb, ua * ub]]
    if (op == T_STAR_EQUAL) {
        const CPlusPlus::Symbol* lhsSymbol = symbol_;
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        Range range(*lhsRange.lower_ * *rhsRange.lower_, *lhsRange.upper_ * *rhsRange.upper_);
        insertOrAssign(lhsSymbol, range);

        return false;
    }

    ////////////////////////////////////////////
    // (Va / Vb) : [la / ub, ua / lb]]
    if (op == T_SLASH) {
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        auto lower = *lhsRange.lower_ / *rhsRange.upper_;
        auto upper = *lhsRange.upper_ / *rhsRange.lower_;
        stack_.emplace(std::move(lower), std::move(upper));
        return false;
    }

    // Va /= Vb -> Va : [la / ub, ua / lb]]
    if (op == T_SLASH_EQUAL) {
        const CPlusPlus::Symbol* lhsSymbol = symbol_;
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        Range range(*lhsRange.lower_ / *rhsRange.upper_, *lhsRange.upper_ / *rhsRange.lower_);
        insertOrAssign(lhsSymbol, range);

        return false;
    }

    // Va << Vb -> Va : [la << ub, ua << ub]]
    if (op == T_LESS_LESS) {
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        auto lower = *lhsRange.lower_ << *rhsRange.upper_;
        auto upper = *lhsRange.upper_ << *rhsRange.upper_;
        stack_.emplace(std::move(lower), std::move(upper));

        return false;
    }

    // Va >> Vb -> Va : [la >> ub, ua >> ub]]
    if (op == T_GREATER_GREATER) {
        Range lhsRange = std::move(stack_.top());
        stack_.pop();
        accept(ast->right_expression);
        Range rhsRange = std::move(stack_.top());
        stack_.pop();
        auto lower = *lhsRange.lower_ >> *rhsRange.upper_;
        auto upper = *lhsRange.upper_ >> *rhsRange.upper_;
        stack_.emplace(std::move(lower), std::move(upper));

        return false;
    }

    if (op == T_COMMA) {
        accept(ast->right_expression);
    }

    return false;
}

bool RangeAnalysis::visit(ConditionalExpressionAST *ast)
{
    if (ast->condition->asExpression() &&
            ast->condition->asExpression()->asBinaryExpression()) {
        unsigned op = tokenKind(ast->condition->asExpression()->asBinaryExpression()->binary_op_token);
        ExpressionAST* leftExpression = ast->condition->asExpression()->asBinaryExpression()->left_expression;
        ExpressionAST* rightExpression = ast->condition->asExpression()->asBinaryExpression()->right_expression;

        if (isRelational(op)) { // op == '<' || '>' || '=='...
            bool aIsSymbol = false, bIsSymbol = false;
            // va
            const CPlusPlus::Symbol* leftSymbol;
            // Ra
            std::unique_ptr<Range> rangeA;
            // vb
            const CPlusPlus::Symbol* rightSymbol;
            // Rb
            std::unique_ptr<Range> rangeB;

            if (leftExpression->asIdExpression()) { // get the range of va (or left expression)
                aIsSymbol = true;
                leftSymbol = lookupValueSymbol(leftExpression->asIdExpression()->name->name, scope_);
                rangeA = getRangeOfSymbol(leftSymbol);
            } else {
                accept(leftExpression);
                rangeA = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }

            if (rightExpression->asIdExpression()) { // get the range of vb (or right expression)
                bIsSymbol = true;
                rightSymbol = lookupValueSymbol(rightExpression->asIdExpression()->name->name, scope_);
                rangeB = getRangeOfSymbol(rightSymbol);
            } else {
                accept(rightExpression);
                rangeB = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }
            Range rangeAWhenTrue(rangeForAWhenTrue(rangeA.get(), rangeB.get(), op));
            Range rangeBWhenTrue(rangeForBWhenTrue(rangeA.get(), rangeB.get(), op));
            Range rangeAWhenFalse(rangeForAWhenFalse(rangeA.get(), rangeB.get(), op));
            Range rangeBWhenFalse(rangeForBWhenFalse(rangeA.get(), rangeB.get(), op));
            const auto revision = rangeMap_.revision();
            // Rt'
            if (aIsSymbol) {
                insertOrAssign(leftSymbol, rangeForAWhenTrue(rangeA.get(), rangeB.get(), op));
            }
            if (bIsSymbol) {
                insertOrAssign(rightSymbol, rangeForBWhenTrue(rangeA.get(), rangeB.get(), op));
            }

            accept(ast->left_expression);
            Range rangeLeft(stack_.top());
            stack_.pop();

            VersionedMap<const CPlusPlus::Symbol*, Range> ifTrueRangeMap(rangeMap_);

            rangeMap_.applyRevision(revision);

            // Rf'
            if (aIsSymbol)
                insertOrAssign(leftSymbol, rangeForAWhenFalse(rangeA.get(), rangeB.get(), op));
            if (bIsSymbol)
                insertOrAssign(rightSymbol, rangeForBWhenFalse(rangeA.get(), rangeB.get(), op));

            accept(ast->right_expression);
            Range rangeRight(stack_.top());
            stack_.pop();

            VersionedMap<const CPlusPlus::Symbol*, Range> ifFalseRangeMap(rangeMap_);

            rangeMap_.applyRevision(revision);

            mapUnion(&ifTrueRangeMap, &ifFalseRangeMap);
            stack_.push(rangeLeft.rangeUnion(rangeRight));

            return false;
        }
    }

    accept(ast->condition);

    const auto revision = rangeMap_.revision();
    accept(ast->left_expression);
    Range rangeLeft(stack_.top());
    stack_.pop();
    VersionedMap<const CPlusPlus::Symbol*, Range> ifRangeMap(rangeMap_);
    rangeMap_.applyRevision(revision);
    accept(ast->right_expression);
    Range rangeRight(stack_.top());
    stack_.pop();
    VersionedMap<const CPlusPlus::Symbol*, Range> elseRangeMap(rangeMap_);
    rangeMap_.applyRevision(revision);

    mapUnion(&ifRangeMap, &elseRangeMap);
    stack_.push(rangeLeft.rangeUnion(rangeRight));

    return false;
}

bool RangeAnalysis::visit(MemberAccessAST *ast)
{
    DEBUG_VISIT(MemberAccessAST);

    const Scope* prevScope = scope_;
    accept(ast->base_expression);
    resolve(ast->member_name->name);
    switchScope(prevScope);
    return false;
}

bool RangeAnalysis::visit(CPlusPlus::CallAST* ast)
{
    DEBUG_VISIT(CallAST);

    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        accept(it->value);
    }

    accept(ast->base_expression);

    return false;
}

// Statements

bool RangeAnalysis::visit(CompoundStatementAST *ast)
{
    DEBUG_VISIT(CompoundStatementAST);

    const Scope *previousScope = switchScope(ast->symbol);
    for (StatementListAST *it = ast->statement_list; it; it = it->next) {
        enclosingStmt_ = it->value;
        visitStatement(it->value);
        if (savingStateEnable_)
            saveState(it->value);
    }
    switchScope(previousScope);

    return false;
}

bool RangeAnalysis::visit(DeclarationStatementAST *ast)
{
    DEBUG_VISIT(DeclarationStatementAST);

    visitDeclaration(ast->declaration);

    return false;
}

bool RangeAnalysis::visit(ExpressionStatementAST *ast)
{
    DEBUG_VISIT(ExpressionStatementAST);

    accept(ast->expression);
    revisionMap_.insert(std::make_pair(ast, rangeMap_.revision()));

    return false;
}

void RangeAnalysis::mapUnion(VersionedMap<const CPlusPlus::Symbol*, Range> *a,
                   VersionedMap<const CPlusPlus::Symbol*, Range> *b) {
    std::set<const CPlusPlus::Symbol*> keys;
    for (auto it = a->begin(); it != a->end(); ++it) {
        keys.insert(it->first);
    }
    for (auto it = b->begin(); it != b->end(); ++it) {
        keys.insert(it->first);
    }

    for (auto it : keys) {
        const auto ifV = a->find(it);
        const auto elseV = b->find(it);

        if (ifV == a->end()) {
            PSYCHE_ASSERT(elseV != b->end(), return , "expected that the map contains the key value - a");
            Range range(elseV->second);
            insertOrAssign(it, std::move(range));
            continue;
        }
        if (elseV == b->end()) {
            PSYCHE_ASSERT(ifV != a->end(), return , "expected that the map contains the key value - b");
            Range range(ifV->second);
            insertOrAssign(it, std::move(range));
            continue;
        }
        Range rangeIf(ifV->second);
        Range rangeElse(elseV->second);
        Range range(std::make_unique<NAryValue>(
                        rangeIf.lower_->clone(),
                        rangeElse.lower_->clone(),
                        Operation::Minimum),
                    std::make_unique<NAryValue>(
                        rangeIf.upper_->clone(),
                        rangeElse.upper_->clone(),
                        Operation::Maximum));
       insertOrAssign(it, range.evaluate());
    }
}

std::unique_ptr<Range> RangeAnalysis::getRangeOfSymbol(const CPlusPlus::Symbol* symbol) {
    auto it = rangeMap_.find(symbol);
    if (it != rangeMap_.end()) {
        stack_.push(it->second);
    } else {
        Range range(std::make_unique<SymbolValue>(symbol),
                    std::make_unique<SymbolValue>(symbol));
        stack_.push(std::move(range));
    }
    Range range(stack_.top());
    stack_.pop();
    return std::make_unique<Range>(range);
}

bool RangeAnalysis::visit(IfStatementAST *ast)
{
    DEBUG_VISIT(IfStatementAST);
    // RELATED TO: Condition expression
    if (ast->condition->asExpression() &&
            ast->condition->asExpression()->asBinaryExpression()) {
        unsigned op = tokenKind(ast->condition->asExpression()->asBinaryExpression()->binary_op_token);
        ExpressionAST* leftExpression = ast->condition->asExpression()->asBinaryExpression()->left_expression;
        ExpressionAST* rightExpression = ast->condition->asExpression()->asBinaryExpression()->right_expression;

        if (isRelational(op)) { // op == '<' || '>' || '=='...
            bool aIsSymbol = false, bIsSymbol = false;
            // va
            const CPlusPlus::Symbol* leftSymbol;
            // Ra
            std::unique_ptr<Range> rangeA;
            // vb
            const CPlusPlus::Symbol* rightSymbol;
            // Rb
            std::unique_ptr<Range> rangeB;

            if (leftExpression->asIdExpression()) { // get the range of va (or left expression)
                aIsSymbol = true;
                leftSymbol = lookupValueSymbol(leftExpression->asIdExpression()->name->name, scope_);
                rangeA = getRangeOfSymbol(leftSymbol);
            } else {
                accept(leftExpression);
                rangeA = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }

            if (rightExpression->asIdExpression()) { // get the range of vb (or right expression)
                bIsSymbol = true;
                rightSymbol = lookupValueSymbol(rightExpression->asIdExpression()->name->name, scope_);
                rangeB = getRangeOfSymbol(rightSymbol);
            } else {
                accept(rightExpression);
                rangeB = std::make_unique<Range>(stack_.top());
                stack_.pop();
            }
            Range rangeAWhenTrue(rangeForAWhenTrue(rangeA.get(), rangeB.get(), op));
            Range rangeBWhenTrue(rangeForBWhenTrue(rangeA.get(), rangeB.get(), op));
            Range rangeAWhenFalse(rangeForAWhenFalse(rangeA.get(), rangeB.get(), op));
            Range rangeBWhenFalse(rangeForBWhenFalse(rangeA.get(), rangeB.get(), op));
            const auto revision = rangeMap_.revision();
            // Rt'
            if (aIsSymbol) {
                insertOrAssign(leftSymbol, rangeForAWhenTrue(rangeA.get(), rangeB.get(), op));
            }
            if (bIsSymbol) {
                insertOrAssign(rightSymbol, rangeForBWhenTrue(rangeA.get(), rangeB.get(), op));
            }

            accept(ast->statement);

            VersionedMap<const CPlusPlus::Symbol*, Range> ifTrueRangeMap(rangeMap_);

            rangeMap_.applyRevision(revision);

            // Rf'
            if (aIsSymbol)
                insertOrAssign(leftSymbol, rangeForAWhenFalse(rangeA.get(), rangeB.get(), op));
            if (bIsSymbol)
                insertOrAssign(rightSymbol, rangeForBWhenFalse(rangeA.get(), rangeB.get(), op));

            if (ast->else_statement)
                accept(ast->else_statement);

            VersionedMap<const CPlusPlus::Symbol*, Range> ifFalseRangeMap(rangeMap_);

            rangeMap_.applyRevision(revision);

            mapUnion(&ifTrueRangeMap, &ifFalseRangeMap);

            return false;
        }
    }

    accept(ast->condition);

    const auto revision = rangeMap_.revision();
    if (ast->statement) {
        accept(ast->statement);
    }
    VersionedMap<const CPlusPlus::Symbol*, Range> ifRangeMap(rangeMap_);
    rangeMap_.applyRevision(revision);
    if (ast->else_statement) {
        accept(ast->else_statement);
    }
    VersionedMap<const CPlusPlus::Symbol*, Range> elseRangeMap(rangeMap_);
    rangeMap_.applyRevision(revision);

    mapUnion(&ifRangeMap, &elseRangeMap);

    return false;
}

void RangeAnalysis::wideRanges(std::map<const CPlusPlus::Symbol*, Range> &refValues,
                               std::map<const CPlusPlus::Symbol*, std::list<Range> > &history)
{
    for (auto const& itHist : history) {
        bool ld = lowerIsDecreasing(itHist.second);
        bool ug = upperIsGrowing(itHist.second);
        if (ld && ug) {
            // [-Inf, +Inf]
            Range newRange(std::make_unique<InfinityValue>(Negative), std::make_unique<InfinityValue>(Positive));

            const auto itRef = refValues.find(itHist.first);
            if (itRef != refValues.end()) {
                // make the union of current value with the old value
                Range rangeUnion(itRef->second.rangeUnion(newRange));

                if (rangeUnion != itRef->second) {
                    refValues.insert(std::make_pair(itHist.first, rangeUnion));
                    insertOrAssign(itHist.first, std::move(rangeUnion));
                }
            } else {
                refValues.insert(std::make_pair(itHist.first, newRange));
                insertOrAssign(itHist.first, std::move(newRange));
            }
        } else if (ld) {
            // [-Inf, u]
            Range newRange(std::make_unique<InfinityValue>(Negative), itHist.second.back().upper_->clone());

            const auto itRef = refValues.find(itHist.first);
            if (itRef != refValues.end()) {
                // make the union of current value with the old value
                Range rangeUnion(itRef->second.rangeUnion(newRange));

                if (rangeUnion != itRef->second) {
                    refValues.insert(std::make_pair(itHist.first, rangeUnion));
                    insertOrAssign(itHist.first, std::move(rangeUnion));
                }
            } else {
                refValues.insert(std::make_pair(itHist.first, newRange));
                insertOrAssign(itHist.first, std::move(newRange));
            }
        } else if (ug) {
            // [l, +Inf]
            Range newRange(itHist.second.back().lower_->clone(), std::make_unique<InfinityValue>(Positive));

            const auto itRef = refValues.find(itHist.first);
            if (itRef != refValues.end()) {
                // make the union of current value with the old value
                Range rangeUnion(itRef->second.rangeUnion(newRange));

                if (rangeUnion != itRef->second) {
                    refValues.insert(std::make_pair(itHist.first, rangeUnion));
                    insertOrAssign(itHist.first, std::move(rangeUnion));
                }
            } else {
                refValues.insert(std::make_pair(itHist.first, newRange));
                insertOrAssign(itHist.first, std::move(newRange));
            }
        }
    }
}

void printHistory(std::map<const CPlusPlus::Symbol*, std::list<Range> > const history)
{
    std::cout << " history - begin ///////////////////////////////////////// " << std::endl;
    for (auto &a : history) {
        std::cout << a.first->name()->asNameId()->identifier()->chars() << ": ";
        for (auto &b : a.second) {
            std::cout << b << " ";
        }
        std::cout << std::endl;
    }
    std::cout << " history - end   ///////////////////////////////////////// " << std::endl;
}

void RangeAnalysis::visitLoopBody(StatementAST *ast)
{
    if (ast->asWhileStatement()) {
        accept(ast->asWhileStatement()->statement);
    } else if (ast->asForStatement()) {
        accept(ast->asForStatement()->statement);
        accept(ast->asForStatement()->expression);
    } else {
        PSYCHE_ASSERT(true, return , "ERROR: visitLoopBody, visit function for loop body not implemented");
    }
}

void RangeAnalysis::visitLoopConditionWhenTrue(StatementAST *ast)
{
    if (ast->asWhileStatement()) {
        visitConditionWhenTrue(ast->asWhileStatement()->condition->asExpression());
    } else if (ast->asForStatement()) {
        visitConditionWhenTrue(ast->asForStatement()->condition->asExpression());
    } else {
        PSYCHE_ASSERT(true, return , "ERROR: visitLoopCondition, visit function for loop condition not implemented");
    }
}

void RangeAnalysis::visitLoopConditionWhenFalse(StatementAST *ast)
{
    if (ast->asWhileStatement()) {
        visitConditionWhenFalse(ast->asWhileStatement()->condition->asExpression());
    } else if (ast->asForStatement()) {
        visitConditionWhenFalse(ast->asForStatement()->condition->asExpression());
    } else {
        PSYCHE_ASSERT(true, return , "ERROR: visitLoopConditionWhenFalse, visit function for loop condition not implemented");
    }
}

bool RangeAnalysis::visitLoop(StatementAST *ast)
{
    if (!savingStateEnable_)
        return false;

    if (ast->asForStatement())
        accept(ast->asForStatement()->initializer);

    const auto revision = rangeMap_.revision();

    std::map<const CPlusPlus::Symbol*, Range> refValues; // reference values used to see if some value was changed
    std::map<const CPlusPlus::Symbol*, std::list<Range> > history; // map with the list of modifications of the ranges of each symbol
    std::list<const CPlusPlus::Symbol*> symbolsBeforeLoop; // symbols with a range before the loop

    for (auto const& it : rangeMap_) {
        refValues.insert(std::make_pair(it.first, it.second));
        history[it.first].push_back(it.second);
        symbolsBeforeLoop.push_back(it.first);
    }

    savingStateEnable_ = false;
    visitLoopBody(ast);
    savingStateEnable_ = true;

    for (auto const& it : rangeMap_) {
        history[it.first].push_back(it.second);
    }

    for (auto &it : refValues) {
        bool lowerIsDecreasing = *it.second.lower_ > (*history[it.first].back().lower_.get());
        bool upperIsGrowing = *it.second.upper_ < (*history[it.first].back().upper_.get());
        bool upperIsTheSame = *it.second.upper_ == (*history[it.first].back().upper_.get());
        bool lowerIsTheSame = *it.second.lower_ == (*history[it.first].back().lower_.get());

        if (lowerIsDecreasing && upperIsGrowing) {
            // [-Inf, +Inf]
            Range newRange(std::make_unique<InfinityValue>(Negative),
                           std::make_unique<InfinityValue>(Positive));
            insertOrAssign(it.first, std::move(newRange));
        } else if (lowerIsDecreasing) {
            // [-Inf, u]
            Range newRange(std::make_unique<InfinityValue>(Negative),
                           history[it.first].back().upper_->clone());
            Range rangeUnion(it.second.rangeUnion(newRange));
            insertOrAssign(it.first, std::move(rangeUnion));
        } else if (upperIsGrowing) {
            // [l, +Inf]
            Range newRange(history[it.first].back().lower_->clone(),
                    std::make_unique<InfinityValue>(Positive));
            Range rangeUnion(it.second.rangeUnion(newRange));
            insertOrAssign(it.first, std::move(rangeUnion));
        } else { // can't handle the values
            std::unique_ptr<AbstractValue> lower;
            if(lowerIsTheSame) lower = it.second.lower_->clone();
            else lower = std::make_unique<InfinityValue>(Negative);

            std::unique_ptr<AbstractValue> upper;
            if(upperIsTheSame) upper = it.second.upper_->clone();
            else upper = std::make_unique<InfinityValue>(Positive);

            Range newRange(std::move(lower),
                           std::move(upper));
            insertOrAssign(it.first, std::move(newRange));
        }
    }
    /*if (ast->asForStatement())
        LoopAnalyser loopAnalyser(unit_, ast, history, scope_);
    else if (ast->asWhileStatement())
        LoopAnalyser loopAnalyser(unit_, ast, history, scope_);
    else if (ast->asDoStatement()) {
        cout << "ERROR: bool RangeAnalysis::visitLoop(StatementAST *ast):"
                " do while loop not implemented yet!" << endl;
        exit(1);
    } else {
        cout << "ERROR: bool RangeAnalysis::visitLoop(StatementAST *ast):"
                " unexpected loop!" << endl;
        exit(1);
    }//*/

    /*// Reinitialization of the variants' ranges to [v, v]
    for (auto const& it : rangeMap_) {
        bool upperIsTheSame = *it.second.upper_ == (*history[it.first].back().upper_.get());
        bool lowerIsTheSame = *it.second.lower_ == (*history[it.first].back().lower_.get());
        if (!upperIsTheSame || !lowerIsTheSame) {
            std::unique_ptr<AbstractValue> symb = SymbolValue(it.first).evaluate();
            Range newRange(symb->clone(), symb->clone());
            insertOrAssign(it.first, std::move(newRange));
        }
    }*/

    visitLoopConditionWhenTrue(ast);
    visitLoopBody(ast);

    // Creating list of Abstract Value for each bound
    std::map<const CPlusPlus::Symbol*, std::set<AV> > bounds;

    std::map<const CPlusPlus::Symbol*, Range> whenTrue;
    std::list<const CPlusPlus::Symbol*> whenTrueSymbols;
    for (auto const& it : rangeMap_) {
        whenTrue.insert(std::make_pair(it.first, it.second));
        whenTrueSymbols.push_back(it.first);
    }

    rangeMap_.applyRevision(revision);

    visitLoopConditionWhenFalse(ast);

    std::map<const CPlusPlus::Symbol*, Range> whenFalse;
    for (auto const& it : rangeMap_) {
        whenFalse.insert(std::make_pair(it.first, it.second));
    }

    std::list<const CPlusPlus::Symbol*> diff(whenTrueSymbols);
    for (auto it = diff.begin(); it != diff.end();) {
        bool found = (std::find(symbolsBeforeLoop.begin(), symbolsBeforeLoop.end(), *it) != symbolsBeforeLoop.end());
        if (found)
            it = diff.erase(it);
        else
            it++;
    }

    for (auto it : symbolsBeforeLoop) {
        auto itT = whenTrue.find(it);
        if (itT == whenTrue.end()) {
            r("ERROR: range should exists!");
            exit(1);
        }

        auto itF = whenFalse.find(it);
        if (itT == whenTrue.end()) {
            r("ERROR: range should exists!");
            exit(1);
        }

        Range rTrue(itT->second);
        Range rFalse(itF->second);

        Range rangeUnion(rTrue.rangeUnion(rFalse));
        insertOrAssign(it, std::move(rangeUnion));
    }

    for (auto it : diff) {
        auto itM = whenTrue.find(it);
        if (itM == whenTrue.end()) {
            r("ERROR: range should exists!");
            exit(1);
        }

        Range range(itM->second);
        insertOrAssign(it, move(range));
    }

    return false;
}

bool RangeAnalysis::visit(WhileStatementAST *ast)
{
    DEBUG_VISIT(WhileStatementAST);

    return visitLoop(ast);
}

bool RangeAnalysis::visit(ForStatementAST *ast)
{
    DEBUG_VISIT(ForStatementAST);

    return visitLoop(ast);
}

// ----------------------------------------------------------------
// ----------------------------------------------------------------
// ----------------------------------------------------------------
// ----------------------------------------------------------------
// ----------------------------------------------------------------
// ----------------------------------------------------------------
// ----------------------------------------------------------------

bool RangeAnalysis::lowerIsDecreasing(const std::list<Range> &ranges)
{
    if (ranges.size() <= 2)
        return false;

    auto it1 = ranges.begin();
    auto it2 = std::next(ranges.begin());

    while(it2 != ranges.end() && it1 != it2) {
        if (*it1->lower_ < (*it2->lower_.get())) {
            return false;
        }
        std::swap(it1, it2);
        it2 = std::next(it2);
    }

    return true;
}

bool RangeAnalysis::upperIsGrowing(const std::list<Range> &ranges)
{
    if (ranges.size() <= 2)
        return false;

    auto it1 = ranges.begin();
    auto it2 = std::next(ranges.begin());

    while(it2 != ranges.end() && it1 != it2) {
        if (*it1->upper_ > (*it2->lower_.get())) {
            return false;
        }
        std::swap(it1, it2);
        it2 = std::next(it2);
    }

    return true;
}

Range RangeAnalysis::rangeForAWhenTrue(Range *rangeA, Range *rangeB,
                                                       unsigned op)
{
    IntegerValue one(1);

    switch (op) {
    case T_LESS:
        // va : [la, min(ub − 1, ua)]
        return Range(rangeA->lower_->clone(), // la
                     NAryValue( // min(ub − 1, ua)
                       rangeA->upper_->clone(), // ua
                       *rangeB->upper_ - one, // ub - 1
                       Operation::Minimum).evaluate()); // min(...)
    case T_LESS_EQUAL:
        // va : [la, min(ub, ua)]
        return Range(rangeA->lower_->clone(), // la
                     NAryValue( // min(ub, ua)
                       rangeA->upper_->clone(), // ua
                       rangeB->upper_->clone(), // ub
                       Operation::Minimum).evaluate()); // min(...)
    case T_GREATER:
        // va : [max(la, lb + 1), ua]
        return Range(NAryValue( // max(la, lb + 1)
                       rangeA->lower_->clone(), // la
                       *rangeB->lower_ + one, // lb + 1
                       Operation::Maximum).evaluate(), // max(...)
                     rangeA->upper_->clone()); // ua
    case T_GREATER_EQUAL:
        // va : [max(la, lb), ua]
        return Range(NAryValue( // max(la, lb)
                       rangeA->lower_->clone(), // la
                       rangeB->lower_->clone(), // lb
                       Operation::Maximum).evaluate(), // max(...)
                     rangeA->upper_->clone()); // ua
    case T_EQUAL_EQUAL:
        // va : [la, ua] `intersection` [lb, ub]
        return rangeA->rangeIntersection(rangeB);
    case T_EXCLAIM_EQUAL:
        // va = [la, ua]
        return Range(rangeA->lower_->clone(), rangeA->upper_->clone());
    }
}

Range RangeAnalysis::rangeForBWhenTrue(Range *rangeA, Range *rangeB,
                                                        unsigned op)
{
    IntegerValue one(1);

    switch (op) {
    case T_LESS:
        // vb : [max(la + 1, lb), ub]
        return Range(NAryValue( // max(la + 1, lb)
                         *rangeA->lower_ + one, // la + 1
                         rangeB->lower_->clone(), // lb
                         Operation::Maximum).evaluate(), // max(...)
                     rangeB->upper_->clone()); // ub
    case T_LESS_EQUAL:
        // vb : [max(la, lb), ub]
        return Range(NAryValue( // max(la, lb)
                         rangeA->lower_->clone(), // la
                         rangeB->lower_->clone(), // lb
                         Operation::Maximum).evaluate(), // max(...)
                     rangeB->upper_->clone()); // ub
    case T_GREATER:
        // vb : [lb, min(ua - 1, ub)]
        return Range(rangeB->lower_->clone(), // lb
                     NAryValue( // min(ua - 1, ub)
                           *rangeA->upper_ - one, // ua - 1
                           rangeB->upper_->clone(), // ub
                           Operation::Minimum).evaluate()); // min(...)
    case T_GREATER_EQUAL:
        // vb : [lb, min(ua, ub)]
        return Range(rangeB->lower_->clone(), // lb
                     NAryValue( // min(ua, ub)
                           rangeA->upper_->clone(), // ua
                           rangeB->upper_->clone(), // ub
                           Operation::Minimum).evaluate()); // min(...)
    case T_EQUAL_EQUAL:
        // vb : [la, ua] `intersection` [lb, ub]
        return rangeB->rangeIntersection(rangeA);
    case T_EXCLAIM_EQUAL:
        // vb = [lb, ub]
        return Range(rangeB->lower_->clone(), rangeB->upper_->clone());
    }
}

Range RangeAnalysis::rangeForAWhenFalse(Range *rangeA, Range *rangeB,
                                                        unsigned op)
{
    IntegerValue one(1);

    switch (op) {
    case T_LESS:
        // va : [max(la, lb), ua]
        return Range(NAryValue( // max(la, lb)
                         rangeA->lower_->clone(), // la
                         rangeB->lower_->clone(), // lb
                         Operation::Maximum).evaluate(), // max(...)
                     rangeA->upper_->clone()); // ua
    case T_LESS_EQUAL:
        // va : [max(la, lb + 1), ua]
        return Range(NAryValue( // max(la, lb + 1)
                         rangeA->lower_->clone(), // la
                         *rangeB->lower_ + one, // lb + 1
                         Operation::Maximum).evaluate(), // max(...)
                     rangeA->upper_->clone()); // ua
    case T_GREATER:
        // va : [la, min(ub, ua)]
        return Range(rangeA->lower_->clone(), // la
                     NAryValue( // min(ua, ub)
                           rangeB->upper_->clone(), // ub
                           rangeA->upper_->clone(), // ua
                           Operation::Minimum).evaluate()); // min(...)
    case T_GREATER_EQUAL:
        // va : [la, min(ub − 1, ua)]
        return Range(rangeA->lower_->clone(), // la
                     NAryValue( // min(ub - 1, ua)
                            *rangeB->upper_ - one, // ub - 1
                            rangeA->upper_->clone(), // ua
                            Operation::Minimum).evaluate()); // min(...)
    case T_EQUAL_EQUAL:
        // va = [la, ua]
        return Range(rangeA->lower_->clone(), rangeA->upper_->clone());
    case T_EXCLAIM_EQUAL:
        // va : [la, ua] `intersection` [lb, ub]
        return rangeA->rangeIntersection(rangeB);
    }
}

Range RangeAnalysis::rangeForBWhenFalse(Range *rangeA, Range *rangeB,
                                                         unsigned op)
{
    IntegerValue one(1);

    // va
    switch (op) {
    case T_LESS:
        // vb : [lb, min(ua, ub)]
        return Range(rangeB->lower_->clone(), // lb
                     NAryValue( // min(ua, ub)
                         rangeA->upper_->clone(), // ua
                         rangeB->upper_->clone(), // ub
                         Operation::Minimum).evaluate()); // min(...)
    case T_LESS_EQUAL:
        // vb : [lb, min(ua - 1, ub)
        return Range(rangeB->lower_->clone(), // lb
                     NAryValue( // min(ua - 1, ub)
                            *rangeA->upper_ - one, // ua - 1
                            rangeA->upper_->clone(), // ub
                            Operation::Minimum).evaluate()); // min(...)
    case T_GREATER:
        // vb : [max(la, lb), ub]
        return Range(NAryValue( // max(la, lb)
                             rangeA->lower_->clone(), // la
                             rangeB->lower_->clone(), // lb
                             Operation::Maximum).evaluate(), // max(...)
                           rangeB->upper_->clone()); // ub
    case T_GREATER_EQUAL:
        // vb : [max(la + 1, lb), ub]
        return Range(NAryValue( // max(la + 1, lb)
                         *rangeA->lower_ + one, // la + 1
                            rangeB->lower_->clone(), // lb
                            Operation::Maximum).evaluate(), // max(...)
                         rangeB->upper_->clone()); // ub
    case T_EQUAL_EQUAL:
        // vb : [lb, ub]
        return Range(rangeB->lower_->clone(), rangeB->upper_->clone());
    case T_EXCLAIM_EQUAL:
        // vb : [la, ua] `intersection` [lb, ub]
        return rangeB->rangeIntersection(rangeA);
    }
}

// ArraySymbol

std::string ArrayInfo::name()
{
    return extractId(name_->name());
}

ArrayInfo::ArrayInfo(const CPlusPlus::Symbol *name)
    : name_(name)
    , dimensionIsFixed_(10, false)
{}

void ArrayInfo::addRange(int dimension, Range &range)
{
    if(dimensionIsFixed_[dimension])
            return;

    auto it = dimensionRange_.find(dimension);
    if (it == dimensionRange_.end()) {
        dimensionRange_.insert(std::make_pair(dimension, range));
    } else {
        Range rangeUnion(it->second.rangeUnion(range));
        it->second = rangeUnion;
    }
}

void ArrayInfo::addRangeReal(int dimension, Range &range)
{
    dimensionIsFixed_[dimension] = true;
    auto it = dimensionRange_.find(dimension);
    if (it == dimensionRange_.end()) {
        dimensionRange_.insert(std::make_pair(dimension, range));
    } else {
        it->second = range;
    }
}

std::unique_ptr<AbstractValue> ArrayInfo::dimensionLength(int dimension)
{
    auto it = dimensionRange_.find(dimension);

    if (it == dimensionRange_.end()) {
        std::cout << "ArrayInfo::dimensionLength::ERROR: expected a dimension!" << std::endl;
        exit(1);
    }

    if (dimensionIsFixed_[dimension]) {
        return it->second.upper();
    } else {
        IntegerValue one(1);
        return *it->second.upper() + one;
    }
}

void ArrayInfo::print()
{
    std::cout << name() << ":" << std::endl;
    for (auto it : dimensionRange_) {
        std::cout << it.first << ": " << it.second << std::endl;
    }
}
