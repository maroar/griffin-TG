/******************************************************************************
 * Copyright (c) 2016 Leandro T. C. Melo (ltcmelo@gmail.com)
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

/*
 Contributors:
   - Marcus Rodrigues (demaroar@gmail.com)
*/

#include "Tester.h"
#include "AST.h"
#include "AstFixer.h"
#include "Bind.h"
#include "Control.h"
#include "Debug.h"
#include "DiagnosticCollector.h"
#include "Dumper.h"
#include "Literals.h"
#include "RangeAnalysis.h"
#include "TranslationUnit.h"
#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <sstream>
#include <string.h>

using namespace psyche;
using namespace CPlusPlus;
using namespace std;

namespace {

extern bool runingTests;

// Global vars used to setup an translation unit

static IntegerValue zero(0);
static IntegerValue one(1);
static IntegerValue two(2);
static InfinityValue pinf(Sign::Positive);
static InfinityValue minf(Sign::Negative);

} // anonymous

/*
 * The expected AST is obtained as follows:
 *  - The generator always generates a .dot file for the original program's AST
 *    and another .dot file for the "fixed" AST.
 *  - Create a PDF out of the fixed AST .dot file and manually inspect it to
 *    see whether the AST is correct and according to what we want.
 *  - If so, create a new test case and copy the contents of the fixed .dot
 *    file into it as a raw string.
 *
 * Follow the already existing examples for additional info.
 */
void Tester::checkAst(const std::string &source, std::string expected)
{
    StringLiteral name("testfile", strlen("testfile"));
    Control control;
    std::unique_ptr<TranslationUnit> program =
            analyseProgram(source, control, name, options_);
    PSYCHE_EXPECT_TRUE(program);

    std::ostringstream oss;
    Dumper(program.get()).dump(program->ast()->asTranslationUnit(), "test", oss);

    compareText(expected, oss.str());
}

/*
 * The expected contraints must be in accordance with Rodrigo's type inference
 * engine input.
 */
void Tester::checkConstraints(const std::string &source, std::string expected)
{
    StringLiteral name("testfile", strlen("testfile"));
    Control control;
    std::unique_ptr<TranslationUnit> program =
            analyseProgram(source, control, name, options_);
    PSYCHE_EXPECT_TRUE(program);

    compareText(expected, options_.constraints_);
}

std::string getRanges(const std::string& source) {
    StringLiteral name("asdas", strlen("asdas"));
    Control control;
    std::unique_ptr<CPlusPlus::TranslationUnit> program(new TranslationUnit(&control, &name));

    program->setSource(source.c_str(), source.length());

    DiagnosticCollector collector;
    control.setDiagnosticClient(&collector); // Collector is alive only within this scope.

    // Check whether the parser finished successfully.
    if (!program->parse()) {
        std::cout << "Parsing failed" << std::endl;
        return "";
    }

    // We only proceed if the source is free from syntax errors.
    if (!collector.isEmpty()) {
        std::cout << "Source has syntax errors" << std::endl;
        return "";
    }

    // If we have no AST, there's nothing to do.
    if (!program->ast() || !program->ast()->asTranslationUnit()) {
        std::cout << "No AST" << std::endl;
        return "";
    }

    TranslationUnitAST* ast = program->ast()->asTranslationUnit();
    Namespace* globalNs = control.newNamespace(0, nullptr);
    Bind bind(program.get());
    bind(ast, globalNs);

    // Disambiguate eventual ambiguities.
    AstFixer astFixer(program.get());
    astFixer.fix(ast);

    if (isProgramAmbiguous(program.get(), ast)) {
        std::cout << "Code has unresolved ambiguities" << std::endl;
        return nullptr;
    }

    RangeAnalysis rangeAnalysis(program.get());
    rangeAnalysis.run(ast->asTranslationUnit(), globalNs);

    std::string ret = "";
    auto itOrder = rangeAnalysis.statementsOrder_.back();
    for (auto const& it : rangeAnalysis.rangeAnalysis_[itOrder]) {
        std::string str = it.first->name()->asNameId()->identifier()->chars();
        ret += str + " : "
             + "[" + it.second.lower_->toString() + ", "
             + it.second.upper_->toString() + "] ";
    }

    return ret;
}

void Tester::compareText(std::string expected, std::string actual) const
{
    // Remove all spaces to avoid silly errors in comparisson. Use a lambda
    // instead of the function directly because isspace is overloaded as a
    // template.
    auto checkSpace = [](const char c) { return std::isspace(c) || c == 0; };
    expected.erase(std::remove_if(expected.begin(), expected.end(), checkSpace), expected.end());
    actual.erase(std::remove_if(actual.begin(), actual.end(), checkSpace), actual.end());

    std::string expectedSorted = expected;
    std::sort(expectedSorted.begin(), expectedSorted.end());
    std::string actualSorted = actual;
    std::sort(actualSorted.begin(), actualSorted.end());

    try {
        PSYCHE_EXPECT_STR_EQ(expectedSorted, actualSorted);
    } catch (...) {
        std::cout << "Expected:\n  " << expected << std::endl
                  << "Actual:\n  " << actual
                  << std::endl;
        throw TestFailed();
        return ;
    }
}

void Tester::reset()
{
    options_ = AnalysisOptions();
}

void Tester::testAll()
{
    runingTests = true;
    StringLiteral name("testfile", strlen("testfile"));
    Control control;
    std::unique_ptr<CPlusPlus::TranslationUnit> program_ = make_unique<TranslationUnit>(&control, &name);

    i_ = new Simbol("i", program_.get());
    j_ = new Simbol("j", program_.get());
    k_ = new Simbol("k", program_.get());
    m_ = new Simbol("m", program_.get());
    n_ = new Simbol("n", program_.get());
    x_ = new Simbol("x", program_.get());
    y_ = new Simbol("y", program_.get());
    z_ = new Simbol("z", program_.get());
    std::cout << "Running tests..." << std::endl;
    for (auto testData : tests_) {
        reset();
        currentTest_ = testData.second;
        testData.first(this);
        std::cout << ANSI_COLOR_GREEN << "\t" << currentTest_
                  << " passed!" << ANSI_COLOR_RESET << std::endl;
    }
    delete i_;
    delete j_;
    delete k_;
    delete m_;
    delete n_;
    delete x_;
    delete y_;
    delete z_;
    runingTests = false;
}

// Basics on Symbols

void Tester::testCaseSymbol0() // n + 1
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> ip1 = one + i;
    unique_ptr<AbstractValue> exp = *ip1 + two;
    std::string expected = "(i + 3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol1() // i + s
{
    SymbolValue si(i_);
    unique_ptr<AbstractValue> ipj = one + si;
    std::string expected = "(i+1)";

    compareText(expected, ipj->toString());
}

void Tester::testCaseSymbol2() // s + i
{
    SymbolValue si(i_);
    unique_ptr<AbstractValue> ipj = si + one;
    std::string expected = "(i+1)";

    compareText(expected, ipj->toString());
}

void Tester::testCaseSymbol3() // s + s
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> ipj = i + i;
    std::string expected = "(i*2)";

    compareText(expected, ipj->toString());
}

void Tester::testCaseSymbol4() // I + s
{
    SymbolValue si(i_);
    unique_ptr<AbstractValue> ipj = pinf + si;
    std::string expected = "+Inf";

    compareText(expected, ipj->toString());
}

void Tester::testCaseSymbol5() // s + I
{
    SymbolValue si(i_);
    unique_ptr<AbstractValue> ipj = si + pinf;
    std::string expected = "+Inf";

    compareText(expected, ipj->toString());
}

void Tester::testCaseSymbol6() // n + s
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> ipj = i + j;
    unique_ptr<AbstractValue> ipjPi = *ipj + i;
    std::string expected = "(j + (i * 2))";

    compareText(expected, ipjPi->toString());
}

void Tester::testCaseSymbol7() // s + n
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> ipj = i + j;
    unique_ptr<AbstractValue> iPipj = i + *ipj;
    std::string expected = "(j + (i * 2))";

    compareText(expected, iPipj->toString());
}

void Tester::testCaseSymbol8() // n + n
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> ipj = i + j;
    unique_ptr<AbstractValue> exp = *ipj + *ipj;

    std::string expected = "((j*2) + (i*2))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol9() // n1 + n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    unique_ptr<AbstractValue> ipj = i + j;
    unique_ptr<AbstractValue> kpm = k + m;
    unique_ptr<AbstractValue> exp = *ipj + *kpm;

    std::string expected = "(i + k + m + j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol10() // i + n
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> ip2 = i + two;
    unique_ptr<AbstractValue> ip4 = two + *ip2;
    unique_ptr<AbstractValue> exp = i + *ip4;
    std::string expected = "((i * 2) + 4)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol11() // i * s
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = one * i;
    std::string expected = "i";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol12() // s * i
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i * two;
    std::string expected = "(i*2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol13() // s * s
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i * i;
    std::string expected = "(i*i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol14() // I * s
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = pinf * i;
    std::string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol15() // s * I
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i * pinf;
    std::string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol16() // n * s
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> ipj = i + j; // i + j
    unique_ptr<AbstractValue> ipjTi = *ipj * i; // i * (i + j)
    unique_ptr<AbstractValue> ipjTiTj = *ipjTi * j; // i * j * (i + j)
    unique_ptr<AbstractValue> ipjTiTjTi = *ipjTiTj * i; // i * i * j * (i + j)
    unique_ptr<AbstractValue> jPipjTiTjTi = *ipjTiTjTi + j; // j+(i*i*j*(i + j))
    unique_ptr<AbstractValue> jt2PipjTiTjTi = *jPipjTiTjTi + j; // j+j+(i*i*j*(i+j))
    unique_ptr<AbstractValue> exp = *jt2PipjTiTjTi + *ipjTiTjTi;
        // j+j+(i*i*j*(i+j)) + (i * i * j * (i + j))
    std::string expected = "( (j*2) + (2*i*i*j*j) + (2*i*i*i*j) )";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol17() // s * n
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = k * *itj; // i * j * k
    unique_ptr<AbstractValue> exp = two * *itjtk; // 2 * i * j * k
    std::string expected = "(2*i*j*k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol18() // n * n
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = k * *itj; // i * j * k
    unique_ptr<AbstractValue> exp = *itjtk * *itjtk; // i * i  * j * j * k * k
    std::string expected = "(i*j*k*i*j*k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol19() // n1 * n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    unique_ptr<AbstractValue> ipj = i + j; // i + j
    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = k * *itj; // i * j * k
    unique_ptr<AbstractValue> exp = *ipj * *itjtk; // i * i  * j * j * k * k
    std::string expected = "((i*j*k*i) + (i*j*k*j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol20() // n * i
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = k * *itj; // i * j * k
    unique_ptr<AbstractValue> allSquare = *itjtk * *itjtk; // i * i  * j * j * k * k
    unique_ptr<AbstractValue> allSquaret2 = two * *allSquare; // 2*i*i*j*j*k*k
    unique_ptr<AbstractValue> allSquaret4 = *allSquaret2 * two; // 4*i*i*j*j*k*k

    std::string expected = "(i*j*k*i*j*k*4)";

    compareText(expected, allSquaret4->toString());
}

void Tester::testCaseSymbol20_1() // (n1/n2) * n3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue n(n_);

    unique_ptr<AbstractValue> num = *(i + j) + k; // i + j + k
    unique_ptr<AbstractValue> den = n * *num; // n * (i + j + k)
    unique_ptr<AbstractValue> q = *num / *den; // (i + j + k) / (n * (i + j + k))
    unique_ptr<AbstractValue> exp = *q * n;
    std::string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol20_2() // n3 * (n1/n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    unique_ptr<AbstractValue> n1 = *(i + j) + k; // i + j + k
    unique_ptr<AbstractValue> n2 = *(i * j) * k; // i * j * k
    unique_ptr<AbstractValue> n3 = j * k; // j * k
    unique_ptr<AbstractValue> q = *n1 / *n2; // n1 / n2

    unique_ptr<AbstractValue> exp = *n3 * *q;
    std::string expected = "(((j*k*j)+(i*k*j)+(k*k*j))/(k*j*i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol21() // i / s
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = one / i; // 1 / i

    std::string expected = "(1 / i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol22() // s / i
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i / two; // i / 2

    std::string expected = "(i / 2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_0() // s / s
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i / i; // 1

    std::string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_1() // s1 / s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> exp = i / j; // i / j

    std::string expected = "(i / j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_2() // s1 / s2*s1
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> exp = i / *itj; // i / i*j

    std::string expected = "(1 / j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_3() // s1*s2 / s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> exp = *itj / i; // i*j / i

    std::string expected = "j";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_4() // s1*s2*s3 / s4*s1*s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = *itj * k; // i * j * k
    unique_ptr<AbstractValue> itjtm = m * *itj; // i * j * m
    unique_ptr<AbstractValue> kom = *itjtk / *itjtm; // k / m

    std::string expected = "(k/m)";

    compareText(expected, kom->toString());
}

void Tester::testCaseSymbol23_5() // i1 / i2 // no mdc
{
    IntegerValue three(3);
    unique_ptr<AbstractValue> exp = two / three; // 2 / 3

    std::string expected = "(2/3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_6() // n1 / n1*n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> oneOj = i / *itj; // i / i * j

    std::string expected = "(1/j)";

    compareText(expected, oneOj->toString());
}

void Tester::testCaseSymbol23_7() // n1*n2 / n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> oneTj = *itj / i; // i * j / i

    std::string expected = "j";

    compareText(expected, oneTj->toString());
}

void Tester::testCaseSymbol23_8() // n1*n2*n3 / n4*n1*n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    unique_ptr<AbstractValue> n1 = i + j; // i + j
    unique_ptr<AbstractValue> n2 = i + k; // i + k
    unique_ptr<AbstractValue> n3 = i + m; // i + m
    unique_ptr<AbstractValue> n4 = m + k; // m + k

    unique_ptr<AbstractValue> n1Tn2 = *n1 * *n2; // (i+j) * (i+k)
    unique_ptr<AbstractValue> n = *n1Tn2 * *n3; // (i+j) * (i+k) * (i+m)
    unique_ptr<AbstractValue> d = *n4 * *n1Tn2; // (m+k) * (i+j) * (i+k)

    unique_ptr<AbstractValue> exp = *n / *d; // n1*n2*n3 / n4*n1*n2

    std::string expected = "(((j*k*m)+(i*k*m)+(j*i*m)+(i*i*m)+(j*k*i)+(i*k*i)+(j*i*i)+(i*i*i))/((k*j*k)+(m*j*k)+(k*i*k)+(m*i*k)+(k*j*i)+(m*j*i)+(k*i*i)+(m*i*i)))"; // (i+m)/(m+k)

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_9() // 4*n1 / 2*n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> n1 = i + j; // i + j
    unique_ptr<AbstractValue> n2 = i * i; // i * i

    unique_ptr<AbstractValue> na = two * *n1; // 2 * (i + j)
    unique_ptr<AbstractValue> da = two * *n2; // 2 * (i * i)

    unique_ptr<AbstractValue> n = *na * two; // 4 * (i + j)
    unique_ptr<AbstractValue> d = *da * two; // 4 * (i * i)

    unique_ptr<AbstractValue> exp = *n / *d; // 4*n1 / 4*n2

    std::string expected = "(((j*4)+(i*4))/(i*i*4))"; // n1/n2

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_10() // i1*n1 / i2*n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue five(5);

    unique_ptr<AbstractValue> n1 = i + j; // i + j
    unique_ptr<AbstractValue> n2 = i * i; // i * i

    unique_ptr<AbstractValue> na = two * *n1; // 2 * (i + j)
    unique_ptr<AbstractValue> da = two * *n2; // 2 * (i * i)

    unique_ptr<AbstractValue> n = *na * two; // 4 * (i + j)
    unique_ptr<AbstractValue> d = *da * five; // 10 * (i * i)

    unique_ptr<AbstractValue> exp = *n / *d; // 4*n1 / 10*n2

    std::string expected = "(((j*4)+(i*4))/(i*i*10))"; // n1/n2

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol23_11() // i1 / i2 // with mdc
{
    IntegerValue fiftySix(56);
    IntegerValue sixteen(16);

    unique_ptr<AbstractValue> exp = fiftySix / sixteen; // 56 / 16

    std::string expected = "(7/2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol24() // I / s
{
    // I still don't know how to handle this case
    // If you know, just tell me and I'll change it! =)

    /*SymbolValue i(i_);

    unique_ptr<AbstractValue> exp = pinf / i; // +Inf / i

    std::string expected = "+Inf";

    compareText(expected, exp->toString());*/
}

void Tester::testCaseSymbol25() // s / I
{
    SymbolValue i(i_);

    unique_ptr<AbstractValue> exp = i / pinf; // i / +Inf

    std::string expected = "0";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol26_1() // s1*s2*s3 / s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = k * *itj; // i * j * k

    unique_ptr<AbstractValue> exp = *itjtk / j; // i * j * k / j

    std::string expected = "(i*k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol26_2() // s1+s2 / s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> ipj = i + j; // i + j

    unique_ptr<AbstractValue> exp = *ipj / j; // (i + j) / j

    std::string expected = "((j+i)/j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol26_3() // s1*s2 / s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> itj = i * j; // i * j

    unique_ptr<AbstractValue> exp = *itj / j; // (i*j) / j

    std::string expected = "i";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol27_1() // s2 / s1*s2*s3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = *itj * k; // i * j * k

    unique_ptr<AbstractValue> exp = j / *itjtk ; // j / (i*j*k)

    std::string expected = "(1/(i*k))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol27_2() // s1 / (s1+s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> ipj = i + j; // i + j

    unique_ptr<AbstractValue> exp = j / *ipj; // j / (i+j)

    std::string expected = "(j/(i+j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol27_3() // s4 / s1*s2*s3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = *itj * k; // i * j * k

    unique_ptr<AbstractValue> exp = m / *itjtk; // (i*j*k) / j

    std::string expected = "(m/(i*j*k))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol28() // n / n
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    IntegerValue fourteen(14);
    IntegerValue six(6);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> itjtk = *itj * k; // i * j * k

    unique_ptr<AbstractValue> n = fourteen * *itjtk;
    unique_ptr<AbstractValue> d = six * *itjtk;
    unique_ptr<AbstractValue> exp = *n / *d;

    std::string expected = "(7/3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol29() // n1 / n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    IntegerValue fourteen(14);
    IntegerValue three(3);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> ipk = i + k; // i + k

    unique_ptr<AbstractValue> itjtk = *itj * k;
    unique_ptr<AbstractValue> itjTipk = *itj * *ipk;
    unique_ptr<AbstractValue> n = fourteen * *itjTipk;
    unique_ptr<AbstractValue> ipkT3 = *ipk * three;
    unique_ptr<AbstractValue> d = *ipkT3 + *ipkT3;

    unique_ptr<AbstractValue> exp = *n / *d;

    std::string expected = "(((k*j*i*14)+(i*j*i*14))/((k*6)+(i*6)))";

    compareText(expected, exp->toString());

}

void Tester::testCaseSymbol30() // (i1*i2*n) / i1*i3 = i2*n / i3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    IntegerValue fourteen(14);
    IntegerValue six(6);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> ipk = i + k; // i + k

    unique_ptr<AbstractValue> itjtk = *itj * k;
    unique_ptr<AbstractValue> itjTipk = *itj * *ipk;
    unique_ptr<AbstractValue> n = fourteen * *itjTipk;

    unique_ptr<AbstractValue> exp = *n / six;

    std::string expected = "(((k*j*i*14)+(i*j*i*14))/6)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol31() // i1*i2 / (n*i1*i3) = i2 / i3*n
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    IntegerValue fourteen(14);
    IntegerValue six(6);

    unique_ptr<AbstractValue> itj = i * j; // i * j
    unique_ptr<AbstractValue> ipk = i + k; // i + k

    unique_ptr<AbstractValue> itjtk = *itj * k;
    unique_ptr<AbstractValue> itjTipk = *itj * *ipk;
    unique_ptr<AbstractValue> n = fourteen * *itjTipk;

    unique_ptr<AbstractValue> exp = six / *n;

    std::string expected = "(6/((k*j*i*14)+(i*j*i*14)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol32() // (n+i2) / i2 = (n+i2) / i2
{
    SymbolValue i(i_);
    IntegerValue six(6);

    unique_ptr<AbstractValue> ipSix = i + six; // i + 6

    unique_ptr<AbstractValue> exp = *ipSix / six;

    std::string expected = "((i + 6) / 6)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol33() // i2 / (n+i2) = i2 / (n+i2)
{
    SymbolValue i(i_);
    IntegerValue fourteen(14);
    IntegerValue six(6);

    unique_ptr<AbstractValue> ipSix = six + i; // i + 6

    unique_ptr<AbstractValue> exp = fourteen / *ipSix;

    std::string expected = "(14 / (i + 6))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol34() // i1*i2 / -1*i1*i3 = -1*i2 / i3
{
    IntegerValue fourteen(14);
    IntegerValue mSix(-6);

    unique_ptr<AbstractValue> exp = fourteen / mSix;

    std::string expected = "(-7 / 3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol35() // s / (-1*i1)
{
    SymbolValue i(i_);
    IntegerValue mSix(-6);

    unique_ptr<AbstractValue> exp = i / mSix;

    std::string expected = "((i*-1) / 6)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol36() // i1 / (-1*i2)
{
    IntegerValue seven(7);
    IntegerValue mSix(-6);

    unique_ptr<AbstractValue> exp = seven / mSix;

    std::string expected = "(-7 / 6)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol37() // n1 / (-1*n2)
{
    SymbolValue i(i_);
    IntegerValue mSix(-6);

    AV ipi = i + i; // i + i = 2*i
    AV ipipi = i + *ipi; // i + 2*i = 3*i
    AV ipiTmSix = *ipi * mSix; // 2*i * (-6) = -12*i
    unique_ptr<AbstractValue> exp = *ipi / *ipiTmSix;

    std::string expected = "(-1/6)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol38() // n1*n2 / n1
{
    IntegerValue mThirtySix(-36);
    IntegerValue six(6);

    unique_ptr<AbstractValue> exp = mThirtySix / six;

    std::string expected = "-6";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol49() // (s1+s2) / (s1+1)*(s1+s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ipj = i + j; // i + j
    AV ipOne = i + one; // i + 1
    AV times = *ipj * *ipOne; // (i + j) * (i + 1)

    AV exp = *ipj / *times; // (i + j) / ((i + j) * (i + 1))

    std::string expected = "((j+i)/(j+i+(j*i)+(i*i)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol50() // min(n1, n2) / s3*min(n2, n1)*(s1+s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV minimum1 = NAryValue(i.evaluate(), j.evaluate(), Minimum).evaluate();
    AV minimum2 = NAryValue(j.evaluate(), i.evaluate(), Minimum).evaluate();
    AV d = k * *(*minimum2 * *(i + j));

    AV exp = *minimum1 / *d;

    std::string expected = "(min(i,j)/((j*min(j,i)*k)+(i*min(j,i)*k)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol51() // max(n1, n2) / max(n2, n1)*i1
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV maximum1 = NAryValue(i.evaluate(), j.evaluate(), Maximum).evaluate();
    AV maximum2 = NAryValue(j.evaluate(), i.evaluate(), Maximum).evaluate();
    AV d = *maximum2 * i;

    AV exp = *maximum1 / *d;

    std::string expected = "(1 / i  )";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol52() // (s1+1)*(s1+s2) / (s1+s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ipj = i + j; // i + j
    AV ipOne = i + one; // i + 1
    AV times = *ipj * *ipOne; // (i + j) * (i + 1) = ii + i + ij + j

    AV exp = *times / *ipj; // (ii + i + ij + j) / (i + j)

    std::string expected = "((j+i+(j*i)+(i*i))/(j+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol53() // s3*min(n2, n1)*(s1+s2) / min(n1, n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV minimum1 = NAryValue(i.evaluate(), j.evaluate(), Minimum).evaluate(); // min(i, j)
    AV minimum2 = NAryValue(j.evaluate(), i.evaluate(), Minimum).evaluate(); // min(j, i)
    AV n = k * *(*minimum2 * *(i + j)); // k*i*min2 + k*j*min2

    AV exp = *n / *minimum1; // (k*i*min + k*j*min) / min

    std::string expected = "(((j*min(j,i)*k)+(i*min(j,i)*k))/min(i,j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol54() // max(n2, n1)*i1 / max(n1, n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV maximum1 = NAryValue(j * two, i * two, Maximum).evaluate();
    AV maximum2 = NAryValue(i.evaluate(), j.evaluate(), Maximum).evaluate();
    AV d = *maximum2 * two;

    AV exp = *d / *maximum1;

    std::string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol55() // (i1/i2) / i3
{
    IntegerValue three(3);
    IntegerValue five(5);
    IntegerValue seven(7);

    AV d = three / five;
    AV exp = *d / seven;

    std::string expected = "(3/35)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol56() // (s1/s2) / s3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV d = i / j;
    AV exp = *d / k;

    std::string expected = "(i/(j*k))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol57() // (n1/n2) / n3
{

    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue n(n_);

    AV n1 = i + one;
    AV n2 = i * j;
    AV num = *n1 / *n2;
    AV n3 = two * *(k * n);
    AV exp = *num / *n3;

    std::string expected = "((1+i)/(j*i*n*k*2))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol58() // (n1/n2) / (n3/n4)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue n(n_);
    IntegerValue ten(10);
    IntegerValue three(3);

    AV n1 = *(i * ten) * k;
    AV n2 = *(n * j) * two;
    AV num = *n1 / *n2;

    AV n3 = i * j;
    AV n4 = *(three * n) * *(n * k);
    AV den = *n3 / *n4;

    AV exp = *num / *den;

    std::string expected = "((15*k*k*n)/(j*j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol59() // i3 / (i1/i2)
{
    IntegerValue five(5);
    IntegerValue seven(7);
    IntegerValue ten(10);
    IntegerValue three(3);

    AV n1 = three * ten;
    AV n2 = ten * seven;
    AV q = *n1 / *n2;

    AV exp = *q * five;

    std::string expected = "(15/7)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol60() // s3 / (s1/s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV q = i / j;

    AV exp = *q * k;

    std::string expected = "((i*k)/j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol61() // s1 / (n1/n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV n1 = i * j;
    AV n2 = *(i * j) * k;
    AV q = *n1 / *n2;

    AV exp = j / *q;

    std::string expected = "(k*j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol62() // s1 / (n1/n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV n1 = i * j;
    AV n2 = *(i * j) * k;
    AV q = *n2 / *n1;

    AV exp = j / *q;

    std::string expected = "(j/k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol63() // n3 / (n1/n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue n(n_);

    AV n1 = *(i * j) * n; // i*j*n
    AV n2 = *(i * j) * k; // i*j*k
    AV q = *n1 / *n2; // n/k

    AV exp = *(j * n) / *q; // j*n / n/k = j*n * (k/n) = (j*k)

    std::string expected = "(j*k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol64() // n3 / (n1/n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue n(n_);

    AV n1 = *(i * j) * n; // i*j*n
    AV n2 = *(i * j) * k; // i*j*k
    AV q = *n2 / *n1; // k/n

    AV exp = *(j * n) / *q; // j*n * (n/k) = (j*n*n) / k

    std::string expected = "((j*n*n)/k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol65() // n3 / (n1/n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue n(n_);

    AV n1 = *(i * j) * n; // i*j*n
    AV n2 = *(i * j) * k; // i*j*k
    AV q = *n2 / *n1; // k/n

    AV exp = *(j / n) / *q; // (j/n) * (n/k) = j / k

    std::string expected = "(j/k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol66() // n1/i3 + n2/i3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue n(n_);

    AV n1 = *(i * j) * n; // i*j*n
    AV n2 = *(i * j) * n; // i*j*n

    AV q1 = *n1 / two; // i*j*n/2
    AV q2 = *n2 / two; // i*j*n/2

    AV exp = *q1 + *q2; // (j/n) * (n/k) = j / k

    std::string expected = "(i*j*n)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol66_1() // n1/s3 + n2/s3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue n(n_);

    AV n1 = *(i * j) * n; // i*j*n
    AV n2 = *(i * j) * n; // i*j*n

    AV q1 = *n1 / k; // i*j*n/2
    AV q2 = *n2 / k; // i*j*n/2

    AV exp = *q1 + *q2; // (j/n) * (n/k) = j / k

    std::string expected = "((n*j*i*2)/k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol67() // i1/i3 + i2/i3
{
    IntegerValue three(3);
    IntegerValue five(5);
    IntegerValue seven(7);

    AV q1 = three / seven; // 3/7
    AV q2 = five / seven; // 5/7

    AV exp = *q1 + *q2; // (3/7) + (5/7) = 8 / 7

    std::string expected = "(8/7)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol68() // s1/s3 + s2/s3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV q1 = i / k; // i*j*n/2
    AV q2 = j / k; // i*j*n/2

    AV exp = *q1 + *q2; // (j/k) * (i/k) = (i+j) / k

    std::string expected = "((i+j)/k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol69() // n1/n3 + n2/n3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV n1 = NAryValue(i + one, j.evaluate(), Maximum).evaluate();
    AV n2 = NAryValue(i.evaluate(), j.evaluate(), Maximum).evaluate();
    AV n3 = *(i + j) + k;

    AV q1 = *n1 / *n3; // max(i+1, j) / (i + j + k)
    AV q2 = *n2 / *n3; // max(i, j) / (i + j + k)

    AV exp = *q1 + *q2; // (max(i+1, j) / (i + j + k)) + (max(i, j) / (i + j + k))

    std::string expected = "(max((1+(i*2)),(1+i+j),(j*2))/(j+i+k))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol70() // i1/n3 + i2/n3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV n3 = NAryValue(i.clone(), j.clone(), Minimum).evaluate();;

    AV q1 = two / *n3; // 2 / min(i, j)
    AV q2 = one / *n3; // 1 / min(i, j)

    AV exp = *q1 + *q2; // 3 / min(i, j)

    std::string expected = "(3/min(i,j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol71() // s1/n3 + s2/n3
{
    SymbolValue i(i_);
    SymbolValue k(k_);

    AV n3 = k << two; // 4*k
    AV q1 = *(two * i) / *n3; // 2*i / 4*k
    AV q2 = *(two * i) / *n3; // 2*i / 4*k

    AV exp = *q1 + *q2;  // i / k
    std::string expected = "(i/k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol72() // n1/n3 + n2/n3
{
    SymbolValue i(i_);
    SymbolValue k(k_);

    AV n3 = k >> two; // k/4
    AV q1 = *(two * i) / *n3; // 2*i / (k/4) = (2*i) * (4/k) = 8*i/k
    AV q2 = *(two * i) / *n3; // 2*i / (k/4) = (2*i) * (4/k) = 8*i/k

    AV exp = *q1 + *q2; // 16*i/k
    std::string expected = "((16*i)/k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSymbol39() // s1 == s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    string result;

    if (i == j)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseSymbol40() // s1 == s1
{
    SymbolValue i(i_);

    string result;

    if (i == i)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseSymbol41() // s1 == 1*s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp = j.evaluate();

    string result;

    if (i == *exp)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseSymbol42() // s1 == 1*s1
{
    SymbolValue i(i_);

    AV exp = i.evaluate();

    string result;

    if (i == *exp)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseSymbol43() // s1 == 1*s2 + 0
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV jTOne = j.evaluate();
    AV exp = *jTOne + zero;

    string result;

    if (i == *exp)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseSymbol44() // s1 == 1*s1 + 0
{
    SymbolValue i(i_);

    AV iTOne = i.evaluate();
    AV exp = *iTOne + zero;

    string result;

    if (i == *exp)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseSymbol45() // 5*s2 == s1
{
    IntegerValue five(5);
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp = j * five;

    string result;

    if (*exp == i)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseSymbol46() // 5*s1 == s1
{
    IntegerValue five(5);
    SymbolValue i(i_);

    AV exp = i * five;

    string result;

    if (*exp == i)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseSymbol47() // 1*s1 + 2 == s1
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV iTOne = i.evaluate();
    AV exp = *iTOne + two;

    string result;

    if (*exp == i)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseSymbol48() // 2*s1 + 0 == s1
{
    SymbolValue i(i_);

    AV iTTwo = i * two;
    AV exp = *iTTwo + zero;

    string result;

    if (*exp == i)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

// Symbolic Maths

void Tester::testCase1()
{
    SymbolValue si(i_);
    SymbolValue sj(j_);

    unique_ptr<AbstractValue> ipj = si + sj;

    std::string expected = "(i+j)";

    compareText(expected, ipj->toString());
}

void Tester::testCase2()
{
    unique_ptr<AbstractValue> exp = two + zero;
    std::string expected = "2";

    compareText(expected, exp->toString());
}

void Tester::testCase3()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> exp = i + two;
    std::string expected = "(i+2)";

    compareText(expected, exp->toString());
}

void Tester::testCase4()
{  
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i * two;
    std::string expected = "(i*2)";

    compareText(expected, exp->toString());
}

void Tester::testCase5()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> exp = i - two;
    std::string expected = "(i + -2)";

    compareText(expected, exp->toString());
}

void Tester::testCase6()
{   
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> exp = two - i;
    std::string expected = "((i*-1)+2)";

    compareText(expected, exp->toString());
}

void Tester::testCase7()
{  
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (*ip2 == *im2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase8()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (*ip2 < *im2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase9()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (*ip2 > *im2)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase10()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 == *ip2)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase11()
{    
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 > *ip2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase12()
{    
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 < *ip2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase13()
{   
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;
    std::unique_ptr<AbstractValue> it2 = i * two;

    string result;

    if (*ip2 > *it2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase14()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;
    std::unique_ptr<AbstractValue> it2 = i * two;

    string result;

    if (*ip2 < *it2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase15()
{    
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;
    std::unique_ptr<AbstractValue> it2 = i * two;

    string result;

    if (*ip2 == *it2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase16()
{    
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;
    std::unique_ptr<AbstractValue> it2 = i * two;

    string result;

    if (*ip2 == *ip2 and *it2 == *it2)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase17()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 > pinf)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase18()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 < pinf)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase19()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 == pinf)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase20()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 < minf)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase21()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 > minf)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase22()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 == minf)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase23()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (i < *ip2)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase24()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (i > *ip2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase25()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (i == *ip2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase26()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (i == *im2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase27()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (i > *im2)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase28()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (i < *im2)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase29()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (*im2 == i)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase30()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (*im2 > i)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase31()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> im2 = i - two;

    string result;

    if (*im2 < i) {
        result = "true";
    }
    else {
        result = "false";
    }

    compareText("true", result);
}

void Tester::testCase32()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 == i)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase33()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 > i)
        result = "true";
    else
        result = "false";

    compareText("true", result);
}

void Tester::testCase34()
{
    SymbolValue i(i_);
    std::unique_ptr<AbstractValue> ip2 = i + two;

    string result;

    if (*ip2 < i)
        result = "true";
    else
        result = "false";

    compareText("false", result);
}

void Tester::testCase35()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> exp = i * j;
    std::string expected = "(i*j)";

    compareText(expected, exp->toString());
}

void Tester::testCase36()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> it2 = i * two; // i * 2
    unique_ptr<AbstractValue> exp1 = two * *it2; // 2 * (2*i)
    unique_ptr<AbstractValue> exp = two * *exp1; // 2 * (4 * i)
    std::string expected = "(i*8)";

    compareText(expected, exp->toString());
}

void Tester::testCase37()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> ipj = i + j;
    unique_ptr<AbstractValue> it2 = i * *ipj; // i * (i + j)
    unique_ptr<AbstractValue> exp1 = two * *it2; // 2 * i * (i + j)
    unique_ptr<AbstractValue> exp2 = two * *exp1; // 4 * i * (i + j)
    unique_ptr<AbstractValue> exp = two * *exp2; // 8 * i * (i + j)
    std::string expected = "((j*i*8)+(i*i*8))";

    compareText(expected, exp->toString());
}

void Tester::testCase38()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> exp = i / j;
    std::string expected = "(i / j)";

    compareText(expected, exp->toString());
}

void Tester::testCase39()
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i / one;
    std::string expected = "i";

    compareText(expected, exp->toString());
}

void Tester::testCase40()
{
    unique_ptr<AbstractValue> exp = two / one;
    std::string expected = "2";

    compareText(expected, exp->toString());
}

void Tester::testCase41()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    unique_ptr<AbstractValue> exp1 = i / j;
    unique_ptr<AbstractValue> exp = *exp1 / k;
    std::string expected = "(i / (j*k))";

    compareText(expected, exp->toString());
}

void Tester::testCase42()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    unique_ptr<AbstractValue> exp1 = i / j;
    unique_ptr<AbstractValue> exp = *exp1 / one;
    std::string expected = "(i / j)";

    compareText(expected, exp->toString());
}

void Tester::testCase43()
{
    InfinityValue pInf(Positive);
    unique_ptr<AbstractValue> exp = pInf / one;
    std::string expected = "+Inf";

    compareText(expected, exp->toString());
}

void Tester::testCase44()
{
    unique_ptr<AbstractValue> exp = two << one;
    std::string expected = "4";

    compareText(expected, exp->toString());
}

void Tester::testCase45()
{
    unique_ptr<AbstractValue> exp = two >> one;
    std::string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCase46()
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = one >> i;
    std::string expected = "(1>>i)";

    compareText(expected, exp->toString());
}

void Tester::testCase47()
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = two << i;
    std::string expected = "(2<<i)";

    compareText(expected, exp->toString());
}

void Tester::testCase48()
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp1 =  two << i;
    unique_ptr<AbstractValue> exp2 =  one >> *exp1;
    unique_ptr<AbstractValue> exp =  two << *exp2;
    std::string expected = "(2 << (1 >> (2 << i)))";

    compareText(expected, exp->toString());
}

void Tester::testCase49()
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i >> two;
    std::string expected = "(i / 4)";

    compareText(expected, exp->toString());
}

void Tester::testCase50()
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> exp = i << two;
    std::string expected = "(i * 4)";

    compareText(expected, exp->toString());
}

void Tester::testCase51()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    unique_ptr<AbstractValue> exp1 =  i << two;
    unique_ptr<AbstractValue> exp2 =  j >> *exp1;
    unique_ptr<AbstractValue> exp =  k << *exp2;
    std::string expected = "(k << (j >> (i * 4)))";

    compareText(expected, exp->toString());
}

void Tester::testCase52()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    unique_ptr<AbstractValue> exp1 =  i >> two;
    unique_ptr<AbstractValue> exp2 =  j >> *exp1;
    unique_ptr<AbstractValue> exp =  k << *exp2;
    std::string expected = "(k << (j >> (i / 4)))";

    compareText(expected, exp->toString());
}

void Tester::testCase53()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);
    SymbolValue n(n_);
    SymbolValue x(x_);

    unique_ptr<AbstractValue> exp1 =  i << j; // (i << j)
    unique_ptr<AbstractValue> exp2 =  k >> m; // (k >> m)
    unique_ptr<AbstractValue> exp3 =  n << *exp2; // (n << (k >> m))
    unique_ptr<AbstractValue> exp =  x >> *exp3; // (x >> (n << (k >> m)))
    std::string expected = "(x >> (n << (k >> m)))";

    compareText(expected, exp->toString());
}

void Tester::testCase54()
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    unique_ptr<AbstractValue> exp1 = i + one; // i + 1
    unique_ptr<AbstractValue> exp2 = j * two; // j * 2
    unique_ptr<AbstractValue> exp3 = *exp2 * *exp1; // j * (i + 1) * 2
    unique_ptr<AbstractValue> exp = *exp2 >> *exp3; // ((j * 2) >> (j * (i + 1) * 2))
    std::string expected = "((j*2)>>((j*2)+(i*j*2)))";

    compareText(expected, exp->toString());
}

void Tester::testCase55()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue m(m_);
    unique_ptr<AbstractValue> four = two * two;

    unique_ptr<AbstractValue> exp1 =  i << j; // (i << j)
    unique_ptr<AbstractValue> exp2 =  *exp1 >> m; // ((i << j) >> m)
    unique_ptr<AbstractValue> exp3 =  *exp2 << *four; // (((i << j) >> m) * 16)
    unique_ptr<AbstractValue> exp =   *exp3 * *four; // (((i << j) >> m) * 64)
    std::string expected = "(((i << j) >> m) * 64)";

    compareText(expected, exp->toString());
}

void Tester::testCase56_1()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue m(m_);
    unique_ptr<AbstractValue> four = two * two;

    unique_ptr<AbstractValue> exp1 =  i << j; // (i << j)
    unique_ptr<AbstractValue> exp =  *exp1 >> *four; // ((i << j) / 16)

    std::string expected = "((i<<j)/16)";

    compareText(expected, exp->toString());
}

void Tester::testCase56_2()
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue m(m_);
    unique_ptr<AbstractValue> four = two * two;

    unique_ptr<AbstractValue> exp1 =  i << j; // (i << j)
    unique_ptr<AbstractValue> exp2 =  *exp1 >> *four; // ((i << j) / 16)
    unique_ptr<AbstractValue> exp3 =  *exp2 << m; // (((i << j) / 16) << m)
    unique_ptr<AbstractValue> exp =   *exp3 << *exp1; // ((((i << j) / 16) << m) << (i << j))
    std::string expected = "((((i << j) / 16) << m) << (i << j))";

    compareText(expected, exp->toString());
}

void Tester::testCase57()
{
    SymbolValue i(i_);
    unique_ptr<AbstractValue> three = two + one;

    unique_ptr<AbstractValue> exp1 = i + i; // i + i
    unique_ptr<AbstractValue> exp = *three * *exp1; // i + i

    std::string expected = "(i*6)";

    compareText(expected, exp->toString());
}

void Tester::testCaseR01() // i1 > i2
{
    string result;
    if (one > two)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR01_1() // i1 > (i2/i3) // when true
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    IntegerValue five(5);
    AV division = seventeen / four;

    string result;
    if (five > *division)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR01_2() // i1 > (i2/i3) // when false
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    AV division = seventeen / four;

    string result;
    if (four > *division)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR01_3() // (i2/i3) > i1 // when true
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    AV division = seventeen / four;

    string result;
    if (*division > four)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR01_4() // (i2/i3) > i1 // when false
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    IntegerValue five(5);
    AV division = seventeen / four;

    string result;
    if (*division > five)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR02() // i2 < i1
{
    string expected;

    if (one < two)
        expected = "true";
    else
        expected = "false";

    compareText(expected, "true");
}

void Tester::testCaseR02_1() // i1 < (i2/i3) // when true
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    AV division = seventeen / four;

    string result;
    if (four < *division)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR02_2() // i1 < (i2/i3) // when false
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    IntegerValue five(5);
    AV division = seventeen / four;

    string result;
    if (five < *division)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR02_3() // (i2/i3) < i1 // when true
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    IntegerValue five(5);
    AV division = seventeen / four;

    string result;
    if (*division < five)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR02_4() // (i2/i3) < i1 // when false
{
    IntegerValue seventeen(17);
    IntegerValue four(4);
    AV division = seventeen / four;

    string result;
    if (*division < four)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR03() // s1 > s1
{
    SymbolValue i(i_);

    string result;
    if (i > i)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR04() // s1 < s1
{
    SymbolValue i(i_);

    string result;
    if (i < i)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR05() // s1 < s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    string result;
    if (i < j)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR06() // s1 > s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    string result;
    if (i > j)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR07() // (s1 + 1) > s1
{
    SymbolValue i(i_);

    AV exp = i + one;

    string result;
    if (*exp > i)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR07_1() // (n1 - 1) > n1
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV division = i / j;
    AV divisionMinusOne = *division - one;

    string result;
    if (*divisionMinusOne > *division)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR08() // s1 > (s1 + 1)
{
    SymbolValue i(i_);

    AV exp = i + one;

    string result;
    if (i > *exp)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR08_1() // n1 > (n1 - 1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV mult = i * j;
    AV multMinusTwo = *mult - two;

    string result;
    if (*mult > *multMinusTwo)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR09() // (s1 + 1) < s1
{
    SymbolValue i(i_);

    AV exp = i + one;

    string result;
    if (*exp < i)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR09_1() // (s1 - 1) < s1
{
    SymbolValue i(i_);

    AV exp = i - one;

    string result;
    if (*exp < i)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR10() // s1 < (s1 + 1)
{
    SymbolValue i(i_);

    AV exp = i + one;

    string result;
    if (i < *exp)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR10_1() // s1 < (s1 - 1)
{
    SymbolValue i(i_);

    AV exp = i - one;

    string result;
    if (i < *exp)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR11() // (s1 + 2) < (s1 + 1)
{
    SymbolValue i(i_);

    AV exp1 = i + two;
    AV exp2 = i + one;

    string result;
    if (*exp1 < *exp2)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR11_1() // (s1 - 2) < (s1 - 1)
{
    SymbolValue i(i_);

    AV exp1 = i - two;
    AV exp2 = i - one;

    string result;
    if (*exp1 < *exp2)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR12() // (s1 + 2) > (s1 + 1)
{
    SymbolValue i(i_);

    AV exp1 = i + two;
    AV exp2 = i + one;

    string result;
    if (*exp1 > *exp2)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR12_1() // (s1 - 2) > (s1 + 1)
{
    SymbolValue i(i_);

    AV exp1 = i - two;
    AV exp2 = i + one;

    string result;
    if (*exp1 > *exp2)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR13() // (s1 + s2 + 2) > (s1 + 1 + s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp1a = i + j;
    AV exp1 = *exp1a + two;

    AV exp2a = i + one;
    AV exp2 = *exp2a + j;

    string result;
    if (*exp1 > *exp2)
        result = "true";
    else
        result = "false";

    string expected = "true";

    compareText(expected, result);
}

void Tester::testCaseR14() // (s1 + s2 + 2) < (s1 + 1 + s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp1a = i + j;
    AV exp1 = *exp1a + two;

    AV exp2a = i + one;
    AV exp2 = *exp2a + j;

    string result;
    if (*exp1 < *exp2)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR15() // (s1 - s2 + 2) > (s1 - 1 + s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp1a = i - j;
    AV exp1 = *exp1a + two;

    AV exp2a = i - one;
    AV exp2 = *exp2a + j;

    string result;
    if (*exp1 > *exp2)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR16() // (s1 - s2 + 2) < (s1 - 1 + s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp1a = i - j;
    AV exp1 = *exp1a + two;

    AV exp2a = i - one;
    AV exp2 = *exp2a + j;

    string result;
    if (*exp1 > *exp2)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR17() // (s1 + s2 + s3) > (s1 + s2 + s4)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV exp1a = i + j;
    AV exp1 = *exp1a + k;

    AV exp2a = i + j;
    AV exp2 = *exp2a + m;

    string result;
    if (*exp1 > *exp2)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseR18() // (s1 + s2 + s3) < (s1 + s2 + s4)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV exp1a = i + j;
    AV exp1 = *exp1a + k;

    AV exp2a = i + j;
    AV exp2 = *exp2a + m;

    string result;
    if (*exp1 < *exp2)
        result = "true";
    else
        result = "false";

    string expected = "false";

    compareText(expected, result);
}

void Tester::testCaseM01() // min(i1, i2)
{
    AV exp = NAryValue(one.clone(), two.clone(), Minimum).evaluate();

    string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCaseM02() // max(i1, i2)
{
    AV exp = NAryValue(one.clone(), two.clone(), Maximum).evaluate();

    string expected = "2";

    compareText(expected, exp->toString());
}

void Tester::testCaseM03() // min(i, i) = i
{
    AV exp = NAryValue(two.clone(), two.clone(), Minimum).evaluate();

    string expected = "2";

    compareText(expected, exp->toString());
}

void Tester::testCaseM04() // max(i, i) = i
{
    AV exp = NAryValue(two.clone(), two.clone(), Maximum).evaluate();

    string expected = "2";

    compareText(expected, exp->toString());
}

void Tester::testCaseM05() // max(s, i)
{
    SymbolValue i(i_);

    NAryValue nv(i.evaluate(), two.clone(), Maximum);
    AV exp = nv.evaluate();

    string expected = "max(i,2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseM06() // min(i, s)
{
    SymbolValue i(i_);

    AV exp = NAryValue(two.clone(), i.evaluate(), Minimum).evaluate();

    string expected = "min(i,2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseM07() // max(n, s)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV iDj = i / j;

    AV exp = NAryValue(iDj->clone(), k.evaluate(), Maximum).evaluate();

    string expected = "max((i/j),k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseM08() // min(s, n)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV iDj = i + j;

    AV exp = NAryValue(iDj->clone(), k.evaluate(), Minimum).evaluate();

    string expected = "min((i+j),k)";

    compareText(expected, exp->toString());
}

void Tester::testCaseM09() // max(+Inf, n)
{
    SymbolValue i(i_);

    AV exp = NAryValue(pinf.clone(), i.evaluate(), Maximum).evaluate();

    string expected = "+Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseM10() // max(n, -Inf)
{
    SymbolValue i(i_);

    AV exp = NAryValue(minf.clone(), i.evaluate(), Maximum).evaluate();

    string expected = "i";

    compareText(expected, exp->toString());
}

void Tester::testCaseM11() // min(+Inf, n)
{
    SymbolValue i(i_);

    AV exp = NAryValue(pinf.clone(), i.evaluate(), Minimum).evaluate();

    string expected = "i";

    compareText(expected, exp->toString());
}

void Tester::testCaseM12() // min(n, -Inf)
{
    SymbolValue i(i_);

    AV exp = NAryValue(i.evaluate(), minf.clone(), Minimum).evaluate();

    string expected = "-Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseM13() // min(s1+1, s1)
{
    SymbolValue i(i_);

    AV exp = NAryValue(i + one, i.evaluate(), Minimum).evaluate();

    string expected = "i";

    compareText(expected, exp->toString());
}

void Tester::testCaseM14() // max(s1+1, s1)
{
    SymbolValue i(i_);

    AV exp = NAryValue(i + one, i.evaluate(), Maximum).evaluate();

    string expected = "(i+1)";

    compareText(expected, exp->toString());
}

void Tester::testCaseM15() // min(s1-1, s1)
{
    SymbolValue i(i_);

    AV exp = NAryValue(i - one, i.evaluate(), Minimum).evaluate();

    string expected = "(-1+i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseM16() // max(s1-1, s1)
{
    SymbolValue i(i_);

    AV exp = NAryValue(i - one, i.evaluate(), Maximum).evaluate();

    string expected = "i";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA01() // max(min(a, b), a) = a
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    NAryValue minimum(i.evaluate(), j.evaluate(), Minimum);
    NAryValue maximum(minimum.evaluate(), j.evaluate(), Maximum);

    AV exp = maximum.evaluate();

    string expected = "j";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA02() // max(a, min(a, b)) = a
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    NAryValue minimum(i.evaluate(), j.evaluate(), Minimum);
    NAryValue maximum(j.evaluate(), minimum.evaluate(), Maximum);

    AV exp = maximum.evaluate();

    string expected = "j";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA03() // min(a, max(a, b)) = a
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    NAryValue maximum(i.evaluate(), j.evaluate(), Maximum);
    NAryValue minimum(j.evaluate(), maximum.evaluate(), Minimum);

    AV exp = minimum.evaluate();

    string expected = "j";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA04() // min(max(a, b), a) = a
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    NAryValue maximum(i.evaluate(), j.evaluate(), Maximum);
    NAryValue minimum(maximum.evaluate(), j.evaluate(), Minimum);

    AV exp = minimum.evaluate();

    string expected = "j";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA05() // max(a, a, b, a) = max(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    NAryValue maximum1(i.evaluate(), i.evaluate(), Maximum);
    NAryValue maximum2(maximum1.clone(), j.evaluate(), Maximum);
    NAryValue maximum(maximum2.clone(), i.evaluate(), Maximum);

    AV exp = maximum.evaluate();

    string expected = "max(i, j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA06() // min(a, a, b, a) = min(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    NAryValue minimum1(i.evaluate(), i.evaluate(), Minimum);
    NAryValue minimum2(minimum1.clone(), j.evaluate(), Minimum);
    NAryValue minimum(minimum2.clone(), i.evaluate(), Minimum);

    AV exp = minimum.evaluate();

    string expected = "min(i, j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA07() // min(a, min(min(b, c), d)) = min(a, b, c, d)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    NAryValue minimum1(j.evaluate(), k.evaluate(), Minimum);
    NAryValue minimum2(minimum1.clone(), m.evaluate(), Minimum);
    NAryValue minimum(i.evaluate(), minimum2.clone(), Minimum);

    AV exp = minimum.evaluate();

    string expected = "min(i, j, k, m)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA08() // max(a, max(max(c, d), e)) = max(a, b, c, d, e)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    NAryValue maximum1(j.evaluate(), k.evaluate(), Maximum);
    NAryValue maximum2(maximum1.clone(), m.evaluate(), Maximum);
    NAryValue maximum(i.evaluate(), maximum2.clone(), Maximum);

    AV exp = maximum.evaluate();

    string expected = "max(i, j, k, m)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA09() // min(a, b, a-1, a+1, a+2, b+2) = min(a-1, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV im1 = i - one;
    AV ip1 = i + one;
    AV ip2 = i + two;
    AV jp2 = j + two;

    NAryValue minimum1(j.evaluate(), i.evaluate(), Minimum);
    NAryValue minimum2(minimum1.clone(), im1->evaluate(), Minimum);
    NAryValue minimum3(minimum2.clone(), ip1->evaluate(), Minimum);
    NAryValue minimum4(ip2->evaluate(), minimum3.clone(), Minimum);
    NAryValue minimum(jp2->evaluate(), minimum4.clone(), Minimum);

    AV exp = minimum.evaluate();

    string expected = "min((-1+i), j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA10() // max(b, a-1, a, a+1, a+2, b+2) = max(a+2, b+2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV im1 = i - one;
    AV ip1 = i + one;
    AV ip2 = i + two;
    AV jp2 = j + two;

    NAryValue maximum1(j.evaluate(), im1->evaluate(), Maximum);
    NAryValue maximum2(maximum1.clone(), i.evaluate(), Maximum);
    NAryValue maximum3(maximum2.clone(), ip1->evaluate(), Maximum);
    NAryValue maximum4(ip2->evaluate(), maximum3.clone(), Maximum);
    NAryValue maximum(jp2->evaluate(), maximum4.clone(), Maximum);

    AV exp = maximum.evaluate();

    string expected = "max((i+2), (j+2))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA11() // max(a, b) + max(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(j.evaluate(), i.evaluate(), Maximum);

    AV exp = maximum + maximum;

    string expected = "max((2*i), (j+i), (2*j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA12() // min(a, b) + min(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);

    AV exp = minimum + minimum;

    string expected = "min((2*i), (j+i), (2*j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA13() // max(a, b) * max(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(j.evaluate(), i.evaluate(), Maximum);

    AV exp = maximum * maximum;

    string expected = "(max(i,j) * max(i,j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA14() // min(a, b) * min(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);

    AV exp = minimum * minimum;

    string expected = "(min(i,j) * min(i,j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA15() // max(a, b) - max(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(j.evaluate(), i.evaluate(), Maximum);

    AV exp = maximum - maximum;

    string expected = "min(max(0,(i+(j*-1))),max((j+(i*-1)),0))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA16() // min(a, b) - min(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);

    AV exp = minimum - minimum;

    string expected = "max(min(0,(i+(j*-1))),min((j+(i*-1)),0))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA17() // max(a, b) / max(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(j.evaluate(), i.evaluate(), Maximum);

    AV exp = maximum / maximum;

    string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA17_1() // max(a, b) / max(a, c)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    NAryValue maximum1(j.evaluate(), i.evaluate(), Maximum);
    NAryValue maximum2(k.evaluate(), i.evaluate(), Maximum);

    AV exp = maximum1 / maximum2;

    string expected = "(max(j,i)/max(k,i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA18() // min(a, b) / min(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);

    AV exp = minimum / minimum;

    string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA18_1() // min(a, b) / min(a, c)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    NAryValue minimum1(j.evaluate(), i.evaluate(), Minimum);
    NAryValue minimum2(k.evaluate(), i.evaluate(), Minimum);

    AV exp = minimum1 / minimum1;

    string expected = "1";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA19() // max(a, b) / min(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);
    NAryValue maximum(i.evaluate(), j.evaluate(), Maximum);

    AV exp = maximum / minimum;

    string expected = "(max(i,j)/min(j,i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA20() // min(a, b) / max(a, b)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);
    NAryValue maximum(i.evaluate(), j.evaluate(), Maximum);

    AV exp = minimum / maximum;

    string expected = "(min(j,i)/max(i,j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA21() // min(a, b) / max(c, d)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);
    NAryValue maximum(k.evaluate(), m.evaluate(), Maximum);

    AV exp = minimum / maximum;

    string expected = "(min(j,i)/max(k,m))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA22() // max(a, b) / min(c, d)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    NAryValue minimum(j.evaluate(), i.evaluate(), Minimum);
    NAryValue maximum(k.evaluate(), m.evaluate(), Maximum);

    AV exp = maximum / minimum;

    string expected = "(max(k,m)/min(j,i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA23() // max(s, i) + 2
{
    SymbolValue i(i_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = maximum + two;

    string expected = "max((2 + i), 3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA24() // min(i, s) + 2
{
    SymbolValue i(i_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = minimum + two;

    string expected = "min((2 + i), 3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA25() // max(i, s) - 2
{
    SymbolValue i(i_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = maximum - two;

    string expected = "max((-2 + i), -1)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA26() // min(s, i) - 2
{
    SymbolValue i(i_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = minimum - two;

    string expected = "min((-2 + i), -1)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA27() // max(s1, i) + s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = maximum + j;

    string expected = "max((i + j), (1 + j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA28() // min(i, s1) + s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = minimum + j;

    string expected = "min((i + j), (1 + j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA29() // max(i, s1) - s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = maximum - j;

    string expected = "max((i + (-1*j)), (1 + (-1*j)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA30() // min(s1, i) - s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = minimum - j;

    string expected = "min((i + (-1*j)), (1 + (-1*j)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA31() // max(s1, i) + s1
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = maximum + j;

    string expected = "max((i + j), (1 + j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA32() // min(i, s1) + s1
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = minimum + j;

    string expected = "min((i + j), (1 + j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA33() // max(i, s1) - s1
{
    SymbolValue i(i_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = maximum - i;

    string expected = "max(0, (1 + (i*-1)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA34() // min(s1, i) - s1
{
    SymbolValue i(i_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = minimum - i;

    string expected = "min(0, (1 + (i*-1)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA35() // max(n1, i) + n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n11 = i * j;
    AV n1 = *n11 / *(two*two);

    AV n2 = m + k;

    NAryValue maximum(n1->evaluate(), two.evaluate(), Maximum);

    AV exp = maximum + *n2;

    string expected = "max((((j*i)/4)+k+m),(2+k+m))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA36() // min(i, n1) + n2
{
    SymbolValue i(i_);
    IntegerValue four(4);

    AV n1 = i * two;

    AV n2 = i * four;

    NAryValue minimum(two.evaluate(), n1->evaluate(), Minimum);

    AV exp = minimum + *n2;

    string expected = "min((6*i),(2+(4*i)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA37() // max(i, n1) - n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue three(3);
    IntegerValue mone(-1);

    AV threeTi = three * i;
    AV twoTj = j * two;
    AV n1 = *threeTi + *twoTj;

    AV mThreeTi = mone * *threeTi;
    AV mTwoTj = *twoTj * mone;
    AV n2 = one + *(*mThreeTi + *mTwoTj);

    NAryValue maximum(two.evaluate(), n1->evaluate(), Maximum);

    AV exp = maximum + *n2;

    string expected = "max((3+(j*-2)+(i*-3)),1)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA38() // min(n1, i) - n2
{
    SymbolValue i(i_);
    IntegerValue four(4);

    AV n1 = i * two;

    AV n2 = i * four;

    NAryValue minimum(n1->evaluate(), one.evaluate(), Minimum);

    AV exp = minimum - *n2;

    string expected = "min((i*-2),(1+(i*-4)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA39() // max(n1, n2) + n1
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue three(3);
    IntegerValue mone(-1);

    AV threeTi = three * i;
    AV twoTj = j * two;
    AV n1 = *(*threeTi + *twoTj) + three;

    AV mi = mone * i;
    AV mTwoTj = j * *(mone * two);
    AV n2 = mone + *(*mi + *mTwoTj);

    NAryValue maximum(n1->evaluate(), n2->evaluate(), Maximum);

    AV exp = maximum + *n1;

    string expected = "max((6+(j*4)+(i*6)),(2+(i*2)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA40() // min(n1, n2) - n2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue three(3);
    IntegerValue mone(-1);

    AV threeTi = three * i;
    AV twoTj = j * two;
    AV n1 = *(*threeTi + *twoTj) + three; // 3i + 2j + 3

    AV mi = mone * i;
    AV mTwoTj = j * *(mone * two);
    AV n2 = mone + *(*mi + *mTwoTj); // -i -2j -1

    NAryValue minimum(n1->evaluate(), n2->evaluate(), Minimum);

    AV exp = minimum - *n2;

    string expected = "min((4+(j*4)+(i*4)),0)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA41() // max(n2, n1) - n3
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    IntegerValue three(3);
    IntegerValue seven(7);
    IntegerValue mone(-1);

    NAryValue maximum(i.evaluate(), j.evaluate(), Maximum);
    AV n1 = *(*(*(three * i) + *(j * two)) + three) + maximum; // 3i + 2j + 3 + max(i, j)

    AV n2 = *(i * j) / *(seven * k); // (i*j)/(7*k)

    AV n3 = *(*(mone * i) + *(j * two)) + three; // -i + 2j + 3

    NAryValue maximum2(n1->evaluate(), n2->evaluate(), Maximum);

    AV exp = maximum2 - *n3;

    string expected = "max((i*5),((i*4)+j),(((j*i)/(k*7))+-3+(j*-2)+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA42() // min(n1, n2) - n1
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    IntegerValue three(3);
    IntegerValue seven(7);
    IntegerValue mone(-1);

    NAryValue minimum1(i.evaluate(), j.evaluate(), Minimum);
    AV n1 = *(*(*(three * i) + *(j * two)) + three) + minimum1; // 3i + 2j + 3 + minimum(i, j)

    AV n2 = *(i * j) + *(seven * k); // (i*j)/(7*k)

    AV n3 = *(*(*(mone * i) + *(j * two)) + three) + *(three * k); // -i + 2j + 3

    NAryValue minimum2(n1->evaluate(), n2->evaluate(), Minimum);

    AV exp = minimum2 - *n3;

    string expected = "min(((k*-3)+(i*5)),((k*-3)+(i*4)+j),(-3+(j*i)+(j*-2)+i+(k*4)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA43() // 2 + max(s, i)
{
    SymbolValue i(i_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = two + maximum;

    string expected = "max((2 + i), 3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA44() // 2 + min(i, s)
{
    SymbolValue i(i_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = two + minimum;

    string expected = "min((2 + i), 3)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA45() // 2 - max(i, s)
{
    SymbolValue i(i_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = two - maximum;

    string expected = "min((2+(i*-1)),1)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA46() // 2 - min(s, i)
{
    SymbolValue i(i_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = two - minimum;

    string expected = "max((2+(i*-1)),1)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA47() // s2 + max(s1, i)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = j + maximum;

    string expected = "max((j+i),(1+j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA48() // s2 + min(i, s1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(one.evaluate(), i.evaluate(), Minimum);

    AV exp = j + minimum;

    string expected = "min((j+i),(1+j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA49() // s2 - max(i, s1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = j - maximum;

    string expected = "min(((-1*i)+j),(-1+j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA50() // s2 - min(s1, i)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = j - minimum;

    string expected = "max(((-1*i)+j),(-1+j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA51() // s1 + max(s1, i)
{
    SymbolValue i(i_);

    NAryValue maximum(i.evaluate(), one.evaluate(), Maximum);

    AV exp = i + maximum;

    string expected = "max((i*2),(1+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA52() // s1 + min(i, s1)
{
    SymbolValue i(i_);

    NAryValue minimum(i.evaluate(), one.evaluate(), Minimum);

    AV exp = i + minimum;

    string expected = "min((i*2),(1+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA53() // s1 - max(i, s1)
{
    SymbolValue i(i_);

    NAryValue maximum(one.evaluate(), i.evaluate(), Maximum);

    AV exp = i - maximum;

    string expected = "min((-1+i),0)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA54() // s1 - min(s1, i)
{
    SymbolValue i(i_);

    NAryValue minimum(one.evaluate(), i.evaluate(), Minimum);

    AV exp = i - minimum;

    string expected = "max((-1+i),0)";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA55() // n2 + max(n1, i)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2
    AV n2 = *(*(i * two) - j) + one; // 2i - j + 1

    NAryValue maximum(n1->evaluate(), one.evaluate(), Maximum);

    AV exp = *n2 + maximum;

    string expected = "max((3+(i*3)),(2+(j*-1)+(i*2)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA56() // n2 + min(i, n1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2
    AV n2 = *(*(i * two) - j) + one; // 2i - j + 1

    NAryValue minimum(n1->evaluate(), one.evaluate(), Minimum);

    AV exp = *n2 + minimum;

    string expected = "min((3+(i*3)),(2+(j*-1)+(i*2)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA57() // n2 - max(i, n1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2
    AV n2 = *(*(i * two) - j) + one; // 2i - j + 1

    NAryValue maximum(n1->evaluate(), one.evaluate(), Maximum);

    AV exp = *n2 - maximum;

    string expected = "min((-1+(j*-2)+i),((j*-1)+(i*2)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA58() // n2 - min(n1, i)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2
    AV n2 = *(*(i * two) - j) + one; // 2i - j + 1

    NAryValue minimum(n1->evaluate(), one.evaluate(), Minimum);

    AV exp = *n2 - minimum;

    string expected = "max((-1+(j*-2)+i),((j*-1)+(i*2)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA59() // n1 + max(n1, i)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2

    NAryValue maximum(n1->evaluate(), one.evaluate(), Maximum);

    AV exp = *n1 + maximum;

    string expected = "max((4+(j*2)+(i*2)),(3+j+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA60() // n1 + min(i, n1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2

    NAryValue minimum(n1->evaluate(), one.evaluate(), Minimum);

    AV exp = *n1 + minimum;

    string expected = "min((4+(j*2)+(i*2)),(3+j+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA61() // n1 - max(i, n1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2

    NAryValue minimum(n1->evaluate(), one.evaluate(), Minimum);

    AV exp = *n1 - minimum;

    string expected = "max(0,(1+j+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA62() // n1 - min(n1, i)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = *(i + j) + two; // i + j + 2

    NAryValue minimum(n1->evaluate(), one.evaluate(), Minimum);

    AV exp = *n1 - minimum;

    string expected = "max(0,(1+j+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseMA63() // min(s1, n1) + (s2 - min(s1, s2-1))/2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV R = i.evaluate();
    AV L = NAryValue(j.evaluate(), i - one, Minimum).evaluate();
    AV t2 = *(*R - *L) / two;
    AV t1 = NAryValue(j.evaluate(), i - one, Minimum).evaluate();

    AV exp = *t1 + *t2;

    string expected = "min(((max(((j*-1)+i),1)/2)+j),((max(((j*-1)+i),1)/2)+-1+i))";

    compareText(expected, exp->toString());
}

// tests with Undefined value

void Tester::testCaseU01() // U + i
{
    UndefinedValue U;
    AV exp = U + two;
    compareText("Undefined", exp->toString());
}

void Tester::testCaseU02() // U - s
{
    UndefinedValue U;
    SymbolValue i(i_);
    AV exp = U - i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU03() // U * Inf
{
    UndefinedValue U;
    AV exp = U * pinf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU04() // U / i
{
    UndefinedValue U;
    AV exp = U / two;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU05() // U + n
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV exp = U + *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU06() // U - n
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i << j;
    AV exp = U - *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU07() // U * n
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i / j;
    AV exp = U * *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU08() // U / n
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV exp = U / *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU09() // min(U, n)
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i - j;
    AV exp = NAryValue(U.evaluate(), n->evaluate(), Minimum).evaluate();

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU10() // max(U, n)
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i * j;
    AV exp = NAryValue(U.evaluate(), n->evaluate(), Maximum).evaluate();

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU11() // min(n, U)
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i - j;
    AV exp = NAryValue(n->evaluate(), U.evaluate(), Minimum).evaluate();

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU12() // max(n, U)
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i >> j;
    AV exp = NAryValue(n->evaluate(), U.evaluate(), Maximum).evaluate();

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU13() // U >> s
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = U >> i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU14() // U << i
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = U << i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU15() // U > -Inf
{
    UndefinedValue U;
    SymbolValue i(i_);

    string exp;
    if (U > minf)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU16() // U < +Inf
{
    UndefinedValue U;
    SymbolValue i(i_);

    string exp;
    if (U < minf)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU17() // s + U
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = i + U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU18() // i - U
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = i - U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU19() // Inf * U
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = pinf * U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU20() // i / U
{
    UndefinedValue U;

    AV exp = two / U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU21() // n + U
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i >> j;
    AV exp = *n + U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU22() // n - U
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i << j;
    AV exp = *n - U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU23() // n * U
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV exp = *n * U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU24() // n / U
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i - j;
    AV exp = *n / U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU25() // i >> U
{
    UndefinedValue U;

    AV exp = two >> U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU26() // s << U
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = i << U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU27() // s - U
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = i - U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU28() // i * U
{
    UndefinedValue U;

    AV exp = one * U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU29() // s * U
{
    UndefinedValue U;
    SymbolValue j(j_);

    AV exp = j * U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU30() // s / U
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = i / U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU31() // U + s
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = U + i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU32() // U * s
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = U * i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU33() // U / s
{
    UndefinedValue U;
    SymbolValue i(i_);

    AV exp = U / i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU34() // n >> U
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV exp = *n >> U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU35() // U >> n
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV exp = U >> *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU36() // n << U
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV exp = *n << U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU37() // U << n
{
    UndefinedValue U;
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV exp = U << *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU38() // +Inf > U
{
    UndefinedValue U;
    SymbolValue i(i_);

    string exp;
    if (pinf > U)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU39() // -Inf < U
{
    UndefinedValue U;
    SymbolValue i(i_);

    string exp;
    if (minf < U)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU40() // i >> A(U)
{
    SymbolValue i(i_);
    AV AU = UndefinedValue().evaluate();

    AV exp = i >> *AU;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU41() // i >> Inf
{
    AV exp = two >> pinf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU42() // i << A(U)
{
    SymbolValue i(i_);
    AV AU = UndefinedValue().evaluate();

    AV exp = i << *AU;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU43() // i << Inf
{
    AV exp = two << pinf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU44() // i / A(U)
{
    AV AU = UndefinedValue().evaluate();

    AV exp = two / *AU;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU45() // i < A(U)
{
    AV AU = UndefinedValue().evaluate();

    string exp;
    if (two < *AU)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU46() // i > A(U)
{
    AV AU = UndefinedValue().evaluate();

    string exp;
    if (two < *AU)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU47() // s >> A(U)
{
    SymbolValue i(i_);
    AV AU = UndefinedValue().evaluate();

    AV exp = i >> *AU;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU48() // s >> Inf
{
    SymbolValue i(i_);

    AV exp = i >> minf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU49() // s << A(U)
{
    SymbolValue i(i_);
    AV AU = UndefinedValue().evaluate();

    AV exp = i << *AU;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU50() // s << Inf
{
    SymbolValue i(i_);

    AV exp = i << minf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU51() // Inf >> A
{
    SymbolValue i(i_);
    AV pexp = i.evaluate();
    AbstractValue& av = *pexp;

    AV exp = pinf >> av;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU52() // Inf >> i
{
    AV exp = pinf >> two;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU53() // Inf >> s
{
    SymbolValue i(i_);

    AV exp = minf >> i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU54() // Inf >> Inf
{
    AV exp = minf >> pinf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU55() // Inf >> n
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;

    AV exp = pinf >> *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU56() // Inf << A
{
    SymbolValue i(i_);
    AV pexp = i.evaluate();
    AbstractValue& av = *pexp;

    AV exp = minf << av;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU57() // Inf << i
{
    AV exp = pinf << two;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU58() // Inf << s
{
    SymbolValue i(i_);

    AV exp = pinf << i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU59() // Inf << Inf
{
    AV exp = pinf << pinf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU60() // Inf << n
{
    SymbolValue i(i_);
    AV n = i * two;

    AV exp = pinf << *n;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU61() // Inf << U
{
    UndefinedValue U;

    AV exp = pinf << U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU62() // Inf >> U
{
    UndefinedValue U;

    AV exp = pinf >> U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU63() // Inf / s
{
    SymbolValue i(i_);

    AV exp = pinf / i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU64_0() // Inf * Inf
{
    AV exp = pinf * pinf;

    compareText("+Inf", exp->toString());
}

void Tester::testCaseU64_1() // -Inf * Inf
{
    AV exp = pinf * minf;

    compareText("-Inf", exp->toString());
}

void Tester::testCaseU64_2() // -Inf * -Inf
{
    AV exp = minf * minf;

    compareText("+Inf", exp->toString());
}

void Tester::testCaseU65() // Inf / A
{
    SymbolValue i(i_);
    AV pexp = i.evaluate();
    AbstractValue& av = *pexp;

    AV exp = minf / av;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU66() // Inf * s
{
    SymbolValue i(i_);

    AV exp = minf * i;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU67() // Inf / Inf
{
    AV exp = minf / pinf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU68() // +Inf < A(U)
{
    UndefinedValue U;
    AV av = U.evaluate();

    string exp;
    if (pinf < *av)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU69() // -Inf < A(U)
{
    UndefinedValue U;
    AV av = U.evaluate();

    string exp;
    if (minf < *av)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU70() // +Inf > A(U)
{
    UndefinedValue U;
    AV av = U.evaluate();

    string exp;
    if (pinf > *av)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU71() // -Inf > A(U)
{
    UndefinedValue U;
    AV av = U.evaluate();

    string exp;
    if (minf > *av)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU72() // Inf == A(U)
{
    UndefinedValue U;
    AV av = U.evaluate();

    string exp;
    if (pinf == *av)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU73() // n * Inf
{
    SymbolValue i(i_);
    AV n = i * two;

    AV exp = *n * pinf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU74() // n / U
{
    UndefinedValue U;
    SymbolValue i(i_);
    AV n = i * two;

    AV exp = *n / U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU75() // n >> U
{
    UndefinedValue U;
    SymbolValue i(i_);
    AV n = i * two;

    AV exp = *n >> U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU76() // n >> Inf
{
    SymbolValue i(i_);
    AV n = i + two;

    AV exp = *n >> minf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU77() // n << U
{
    UndefinedValue U;
    SymbolValue i(i_);
    AV n = i * two;

    AV exp = *n << U;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU78() // n << Inf
{
    SymbolValue i(i_);
    AV n = i + two;

    AV exp = *n << minf;

    compareText("Undefined", exp->toString());
}

void Tester::testCaseU79() // n < U
{
    UndefinedValue U;
    SymbolValue i(i_);
    AV n = i + two;

    string exp;
    if (*n < U)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU80() // n > U
{
    UndefinedValue U;
    SymbolValue i(i_);
    AV n = i + two;

    string exp;
    if (*n > U)
        exp = "true";
    else
        exp = "false";

    compareText("false", exp);
}

void Tester::testCaseU81() // s * inf
{
    SymbolValue i(i_);

    AV exp = i * minf; // i * -Inf

    string expected = "Undefined"; // 7i + ij
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseU82() // inf * A // A == i
{
    IntegerValue seven(7);
    AV av = seven.evaluate();

    AV exp = minf * *av; // -Inf * 7

    string expected = "-Inf";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseU83() // A * inf // A == i
{
    IntegerValue mSeven(-7);
    AV av = mSeven.evaluate();

    AV exp = minf * *av; // -Inf * -7

    string expected = "+Inf";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS01() // s1 * (i1 + s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue seven(7);

    AV sum = seven + j; // (7 + j)
    AV exp = i * *sum; // i * (7 + j)

    string expected = "((7*i) + (i*j))"; // 7i + ij
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS02() // (s1 + i1) * s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue seven(7);

    AV sum = seven + j; // (7 + j)
    AV exp = *sum * i; // i * (7 + j)

    string expected = "((7*i) + (i*j))"; // 7i + ij
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS03() // s3*s4*i1 * (s1 + s2 + i1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);
    IntegerValue seven(7);

    AV term1 = *(i * j) * seven; // i*j*7
    AV term2 = *(k + m) + two; // (k + m + 2)
    AV exp = *term1 * *term2; // 14*ij + 7*ijm + 7*ijk

    string expected = "((j*i*14)+(m*j*i*7)+(k*j*i*7))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS04() // (s1 + s2 + i1) * s3*s4*i1
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);
    SymbolValue n(n_);
    IntegerValue seven(7);

    AV av = m * n; // m*n
    AV term1 = *(i * j) * seven; // i*j*7
    AV term2 = *(k + *av) + two; // (k + m*n + 2)
    AV exp = *term2 * *term1; // (k + m*n + 2) * i*j*7

    string expected = "((j*i*14)+(m*n*j*i*7)+(k*j*i*7))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS05() // (s1*s2 + s3 + i1) * (s4 + i2 + s2*s5)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);
    SymbolValue n(n_);
    IntegerValue seven(7);

    AV av1 = m * n; // m*n
    AV term1 = *(k + *av1) + two; // k + m*n + 2

    AV av2 = i * k; // i*k
    AV term2 = *(*av2 + j) + seven; // i*k + j + 7
    AV exp = *term1 * *term2; // (k + m*n + 2) * (i*k + j + 7)
                              // = ikk + jk + 7k + mnik + mnj + 7mn + 2ik + 2j + 14

    string expected = "(14+(k*7)+(n*m*7)+(j*2)+(k*j)+(n*m*j)+(k*i*2)+(k*k*i)+(n*m*k*i))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS06() // (s1 + s2 + i1) * (s1 + s2 + i2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue seven(7);

    AV term1 = *(i + j) + two; // i + j + 2
    AV term2 = *(j + seven) + i; // j + i + 7

    AV exp = *term1 * *term2; // (i + j + 2) * (j + i + 7)
                              // = ij + ii + 7i + jj + ji + 7j + 2j + 2i + 14
                              // = 2ij + ii + 9i + jj + 9j + 14

    string expected = "(14+(j*j)+(i*i)+(j*9)+(i*9)+(i*j*2))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS07() // s1>>s2 * (s1>>s2 + s3 + s2>>s1)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    IntegerValue seven(7);

    AV term1 = i >> j; // i >> j
    AV term2 = *(*(j >> i) + k) + *term1; // j>>i + k + i>>j

    AV exp = *term1 * *term2; // (i>>j) * (j>>i + k + i>>j)
                              // = (i>>j)>>(j>>i) + (i>>j)*k + (i>>j)>>(i>>j)

    string expected = "(((j>>i)*(i>>j))+((i>>j)*(i>>j))+(k*(i>>j)))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS08() // (s1>>s2 + s3 + s2>>s1) * s1>>s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV isj = i >> j; // i >> j
    AV jsi = j >> i; // j >> i
    AV term1 = *(*isj + k) + *jsi; // (i>>j + k + j>>i)

    AV exp = *term1 * *isj; // (i>>j + k + j>>i) * i>>j
                            // = (i>>j)>>(j>>i) + (i>>j)*k + (i>>j)>>(i>>j)

    string expected = "(((j>>i)*(i>>j))+((i>>j)*(i>>j))+(k*(i>>j)))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS09() // min(s1, s2) * (s1 + i1 + s1*s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV term1 = NAryValue(i.evaluate(), j.evaluate(), Operation::Minimum).evaluate(); // min(i, j)
    AV term2 = *(j + two) + *(i * j); // j + 2 + ij

    AV exp = *term1 * *term2; // min(i, j) * (j + 2 + ij)
                              // = min(i, j)*j + min(2*i, 2*j) + ij*min(i, j)

    string expected = "(min((i*2),(j*2))+(j*min(i,j))+(j*i*min(i,j)))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS10() // (s1 + i1 + s1/s2) * max(s1, s2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV term1 = *(i + two) + *(i / j); // i + 2 + i/j
    AV term2 = NAryValue(i.evaluate(), j.evaluate(), Operation::Maximum).evaluate(); // max(i, j)

    AV exp = *term1 * *term2; // (i + 2 + i/j) * max(i, j)
                              // = max(i, j)*i + max(2*i, 2*j) + i/j*max(i, j)

    string expected = "(max((i*2),(j*2))+((i/j)*max(i,j))+(i*max(i,j)))";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS11() // s1*s2*i1 * s1*s3*s4
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV term1 = *(i * j) * two; // 2ij
    AV term2 = *(i * k) * m; // ikm

    AV exp = *term1 * *term2; // 2ij * ikm

    string expected = "(k*i*m*j*i*2)";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS12() // s1*s2*i1 * s1*s2*s4*i2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV term1 = *(i * j) * two; // 2ij
    AV term2 = *(*(i * j) * k) * two; // 2ijk

    AV exp = *term1 * *term2; // 2ij * 2ijk

    string expected = "(4*i*i*j*j*k)";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS13() // s1 * s1*s2*s4*i2
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV term1 = i.evaluate(); // i
    AV term2 = *(*(i * j) * k) * two; // 2ijk

    AV exp = *term1 * *term2; // i * 2ijk

    string expected = "(2*i*i*j*k)";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS14() // s1 * s2
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp = i * j; // i * j

    string expected = "(i*j)";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseS15() // s2*s4*i2 * s1
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV term1 = *(j * k) * two; // 2jk
    AV term2 = i.evaluate(); // i

    AV exp = *term1 * *term2; // 2jk * i

    string expected = "(2*i*j*k)";
    string result = exp->toString();

    compareText(expected, result);
}

void Tester::testCaseSQ01() // sqrt(i)
{
    IntegerValue nine(9);

    AV exp = UnaryValue(nine.evaluate(), SquareRoot).evaluate();

    string expected = "3";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ02() // sqrt(i')
{
    IntegerValue eight(8);

    AV exp = UnaryValue(eight.evaluate(), SquareRoot).evaluate();

    string expected = "2";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ03() // sqrt(s)
{
    SymbolValue i(i_);

    AV exp = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string expected = "sqrt(i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ04() // sqrt(+Inf)
{
    AV exp = UnaryValue(pinf.evaluate(), SquareRoot).evaluate();

    string expected = "+Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ04_1() // sqrt(-Inf)
{
    AV exp = UnaryValue(minf.evaluate(), SquareRoot).evaluate();

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ05() // sqrt(U)
{
    AV exp = UnaryValue(UndefinedValue().clone(), SquareRoot).evaluate();

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ06() // sqrt(n1 + n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n1 = *(i >> j) + k;
    AV n2 = *(k / *(m + *NAryValue(m*i, m<<j, Minimum).evaluate())) + k;

    AV exp = UnaryValue(*n1 + *n2, SquareRoot).evaluate();

    string expected = "sqrt(((k/min(((i*m)+m),((m<<j)+m)))+(i>>j)+(k*2)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ07() // sqrt(n1 * n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n1 = *(*(i * j) * k) + k;
    AV n2 = k * m;

    AV exp = UnaryValue(*n1 * *n2, SquareRoot).evaluate();

    string expected = "sqrt(((k*m*k)+(k*j*i*m*k)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ08() // sqrt(n1 / n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);

    AV n1 = i + j;
    AV n2 = k * k;

    AV exp = UnaryValue(*n1 / *n2, SquareRoot).evaluate();

    string expected = "sqrt(((j+i)/(k*k)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ09() // sqrt(n1 - n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n1 = *(i + j) * *(k + m);
    AV n2 = *(i + j) * *(k + i);

    AV exp = UnaryValue(*n1 - *n2, SquareRoot).evaluate();

    string expected = "sqrt(((j*i*-1)+(i*i*-1)+(j*m)+(i*m)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ10() // sqrt(min(n1, n2))
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n1 = *(i * j) + *(k << m);
    AV n2 = *(i / j) / *(k - i);

    AV exp = UnaryValue(NAryValue(move(n1), move(n2), Minimum).evaluate(), SquareRoot).evaluate();

    string expected = "sqrt(min(((k<<m)+(j*i)),(i/((i*j*-1)+(k*j)))))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ11() // sqrt(max(n1, n2))
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n1 = *(i << j) >> *(k << m);
    AV n2 = *(i << j) << *(k >> i);

    AV exp = UnaryValue(NAryValue(move(n1), move(n2), Minimum).evaluate(), SquareRoot).evaluate();

    string expected = "sqrt(min(((i<<j)>>(k<<m)),((i<<j)<<(k>>i))))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ12() // sqrt(n >> s)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n1 = *(i + j) + *(k + m);

    AV exp = UnaryValue(*n1 >> *(i - k), SquareRoot).evaluate();

    string expected = "sqrt(((m+k+j+i)>>((k*-1)+i)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ13() // sqrt(n1 << n2)
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    SymbolValue k(k_);
    SymbolValue m(m_);

    AV n1 = *(i + j) - *(k + m);

    AV exp = UnaryValue(*n1 << *(i + j), SquareRoot).evaluate();

    string expected = "sqrt((((m*-1)+(k*-1)+j+i)<<(j+i)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ14() // Un + i
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov + two;

    string expected = "(sqrt(i)+2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ15() // i + Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = two + *ov;

    string expected = "(sqrt(i)+2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ16() // s + Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = i + *ov;

    string expected = "(sqrt(i)+i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ17() // Un + s
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov + i;

    string expected = "(sqrt(i)+i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ18() // Un + n1
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov + *(i + i);

    string expected = "(sqrt(i)+(2*i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ19() // n1 + Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *(i + j) + *ov;

    string expected = "(sqrt(i)+i+j)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ19_1() // +Inf + Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = pinf + *ov;

    string expected = "+Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ19_2() // -Inf + Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = minf + *ov;

    string expected = "-Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ19_3() // Un + +Inf
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov + pinf;

    string expected = "+Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ19_4() // Un + -Inf
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov + minf;

    string expected = "-Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ20() // Un * i
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov + minf;

    string expected = "-Inf";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ21() // i * Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = two * *ov;

    string expected = "(2*sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ22() // Un * s
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov * i;

    string expected = "(i*sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ23() // s * Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = i * *ov;

    string expected = "(i*sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ24() // Un * n
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov * *(i + j);

    string expected = "((i*sqrt(i))+(j*sqrt(i)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ25() // n * Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *(i+j) * *ov;

    string expected = "((i*sqrt(i))+(j*sqrt(i)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ26() // Un / i
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov / two;

    string expected = "(sqrt(i)/2)";
}

void Tester::testCaseSQ27() // i / Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = two / *ov;

    string expected = "(2/sqrt(i))";
}

void Tester::testCaseSQ28() // Un / s
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov / i;

    string expected = "(sqrt(i)/i)";
}

void Tester::testCaseSQ29() // s / Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = i / *ov;

    string expected = "(i/sqrt(i))";
}

void Tester::testCaseSQ30() // Un / n
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov / *(i+j);

    string expected = "(sqrt(i)/(i+j))";
}

void Tester::testCaseSQ31() // n / Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *(i+j) / *ov;

    string expected = "((i+j)/sqrt(i))";
}

void Tester::testCaseSQ32() // min(i, Un)
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    AV exp = NAryValue(one.evaluate(), ov1->evaluate(), Minimum).evaluate();

    string expected = "min(1,sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ33() // min(s, Un)
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    AV exp = NAryValue(i.evaluate(), ov1->evaluate(), Minimum).evaluate();

    string expected = "min(i,sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ34() // min(n, Un)
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    AV exp = NAryValue(i + two, ov1->evaluate(), Minimum).evaluate();

    string expected = "min((i+2),sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ35() // max(i, Un)
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    AV exp = NAryValue(one.evaluate(), ov1->evaluate(), Maximum).evaluate();

    string expected = "max(1,sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ36() // max(s, Un)
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    AV exp = NAryValue(i.evaluate(), ov1->evaluate(), Maximum).evaluate();

    string expected = "max(i,sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ37() // max(n, Un)
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    AV exp = NAryValue(i + two, ov1->evaluate(), Maximum).evaluate();

    string expected = "max((i+2),sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ38() // Un >> i
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov >> two;

    string expected = "(sqrt(i)/4)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ39() // i >> Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = i >> *ov;

    string expected = "(i>>sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ40() // Un >> s
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov >> i;

    string expected = "(sqrt(i)>>i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ41() // s >> Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = i >> *ov;

    string expected = "(i>>sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ42() // Un >> n
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov >> *(i + one);

    string expected = "(sqrt(i)>>(1+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ43() // n >> Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *(i + i) >> *ov;

    string expected = "((i*2)>>sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ44() // Un << i
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov << one;

    string expected = "(sqrt(i)*2)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ45() // i << Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = one << *ov;

    string expected = "(1<<sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ46() // Un << s
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov << i;

    string expected = "(sqrt(i)<<i)";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ47() // s << Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = i << *ov;

    string expected = "(i<<sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ48() // Un << n
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *ov << *(i + one);

    string expected = "(sqrt(i)<<(1+i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ49() // n << Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();

    AV exp = *(i + i) << *ov;

    string expected = "((i*2)<<sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ50() // Un + U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *ov + *U;;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ51() // Un - U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *ov - *U;;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ52() // Un * U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *ov * *U;;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ53() // Un / U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *ov / *U;;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ54() // Un min U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = NAryValue(ov->evaluate(), U->evaluate(), Minimum).evaluate();

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ55() // Un max U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = NAryValue(ov->evaluate(), U->evaluate(), Maximum).evaluate();

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ56() // Un >> U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *ov >> *U;;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ57() // Un << U
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *ov << *U;;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ58() // U op Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *U / *ov;;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ59() // U op Un
{
    SymbolValue i(i_);

    AV ov = UnaryValue(i.clone(), SquareRoot).evaluate();
    AV U = UndefinedValue().evaluate();

    AV exp = *U + *ov;

    string expected = "Undefined";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ60() // Un == Un
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov3 = UnaryValue(two.evaluate(), SquareRoot).evaluate();
    AV ov4 = UnaryValue(one.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 == *ov2) {
        if (*ov4 == *ov3) {
            if (*ov3 == *ov3) {
                r = "true";
            } else {
                r = "sambanga";
            }
        } else {
            r = "sai fora... kkk";
        }
    } else {
        r = "aqui nao... kkk";
    }

    string expected = "true";

    compareText(expected, r);
}

void Tester::testCaseSQ61() // Un == Un'
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();
    AV ov3 = UnaryValue(*(j + i) + i, SquareRoot).evaluate();
    AV ov4 = UnaryValue(*(two * i) + j, SquareRoot).evaluate();

    string r = "";
    if (*ov1 == *ov2) {
        r = "sai fora irmão! kkkk";
    } else {
        if (*ov3 == *ov4) {
            if (*ov3 == *ov1) {
                r = "tem nada aqui n! kkkk";
            } else {
                r = "false";
            }
        } else {
            r = "aqui tb não! kkkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ62() // Un == i
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov3 = UnaryValue(*(j + i) + i, SquareRoot).evaluate();
    AV ov4 = UnaryValue(two + two, SquareRoot).evaluate();

    string r = "";
    if (*ov1 == two) {
        r = "aki nao... kkk";
    } else {
        if (two == *ov4) {
            if (*ov3 == *ov4) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ63() // Un == s
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov3 = UnaryValue(*(j + i) + i, SquareRoot).evaluate();
    AV ov4 = UnaryValue(two + two, SquareRoot).evaluate();

    string r = "";
    if (i == *ov4) {
        r = "aki nao... kkk";
    } else {
        if (two == *ov4) {
            if (*ov3 == j) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ64() // Un == n
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov3 = UnaryValue(*(j + i) + i, SquareRoot).evaluate();
    AV ov4 = UnaryValue(two + two, SquareRoot).evaluate();

    string r = "";
    if (*(i * j) == *ov4) {
        r = "aki nao... kkk";
    } else {
        if (two == *ov4) {
            if (*ov3 == *(i + j)) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ65() // Un == U
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov3 = UnaryValue(*(j + i) + i, SquareRoot).evaluate();
    AV ov4 = UnaryValue(two + two, SquareRoot).evaluate();

    string r = "";
    if (*(UndefinedValue().evaluate()) == *ov4) {
        r = "aki nao... kkk";
    } else {
        if (two == *ov4) {
            if (*ov3 == *(UndefinedValue().evaluate())) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ66() // Un == Inf
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(NAryValue(i+j, i*j, Minimum).evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (pinf == *ov1) {
        r = "aki nao... kkk";
    } else {
        if (two == *ov1) {
            r = "caramelo! kkk";
        } else {
            if (*ov2 == minf) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ67() // i == Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(NAryValue(i+j, i*j, Minimum).evaluate(), SquareRoot).evaluate();
    AV i1 = UnaryValue(two + two, SquareRoot).evaluate();

    string r = "";
    if (*i1 == *ov1) {
        r = "aki nao... kkk";
    } else {
        if (two == *i1) {
            if (*ov1 == *i1) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ68() // s == Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(two * i, SquareRoot).evaluate();
    AV ov2 = UnaryValue(i + i, SquareRoot).evaluate();

    string r = "";
    if (i == *ov1) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 == *ov2) {
            if (*ov1 == i) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ69() // n == Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n1 = i + i;

    AV ov1 = UnaryValue(i * i, SquareRoot).evaluate();
    AV ov2 = UnaryValue(i + i, SquareRoot).evaluate();

    string r = "";
    if (*n1 == *ov1) {
        r = "aki nao... kkk";
    } else {
        if (i == *ov1) {
            r = "caramelo! kkk";
        } else {
            if (*ov2 == *n1) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ70() // U == Un
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i * i, SquareRoot).evaluate();

    string r = "";
    if (*UndefinedValue().evaluate() == *ov1) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 == i) {
            r = "caramelo! kkk";
        } else {
            if (*ov1 == *UndefinedValue().evaluate()) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ71() // Inf == Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i / *(i + j), SquareRoot).evaluate();

    string r = "";
    if (pinf == *ov1) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 == i) {
            r = "caramelo! kkk";
        } else {
            if (*ov1 == minf) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ72() // Un > i
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue mone(-1);

    AV ov1 = UnaryValue(i / *(i + j), SquareRoot).evaluate();

    string r = "";
    if (mone > *ov1 ) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 > one) {
            r = "caramelo! kkk";
        } else {
            if (mone > *ov1) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ73() // Un > s
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i / *(i + j), SquareRoot).evaluate();

    string r = "";
    if (*ov1 > i) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 > j) {
            r = "caramelo! kkk";
        } else {
            if (j > *ov1) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ74() // Un > n
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV n = i + j;
    AV ov1 = UnaryValue(i / j, SquareRoot).evaluate();

    string r = "";
    if (*ov1 > *n) {
        r = "aki nao... kkk";
    } else {
        if (*n > *ov1) {
            r = "caramelo! kkk";
        } else {
            if (*ov1 > *(i*j)) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ75() // Un > U
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i / j, SquareRoot).evaluate();

    string r = "";
    if (*ov1 > *UndefinedValue().evaluate()) {
        r = "aki nao... kkk";
    } else {
        if (*UndefinedValue().evaluate() > *ov1) {
            r = "caramelo! kkk";
        } else {
            if (*ov1 > *UndefinedValue().evaluate()) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ76() // Un > Inf
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 > pinf) {
        r = "aki nao... kkk";
    } else {
        if (minf > *ov1) {
            r = "caramelo! kkk";
        } else {
            if (*ov1 > *(pinf + i)) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ77() // Un < i
{
    SymbolValue i(i_);
    IntegerValue mone(-1);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < one) {
        r = "aki nao... kkk";
    } else {
        if (mone < *ov1) {
            if (*ov1 < one) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ78() // Un < s
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < i) {
        r = "aki nao... kkk";
    } else {
        if (i < *ov1) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ79() // Un < n
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp = i + j;
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < *exp) {
        r = "aki nao... kkk";
    } else {
        if (*exp < *ov1) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ80() // Un < U
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < *UndefinedValue().evaluate()) {
        r = "aki nao... kkk";
    } else {
        if (*UndefinedValue().evaluate() < *ov1) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ81() // Un < Inf
{
    SymbolValue i(i_);
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < minf) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 < pinf) {
            if (pinf < *ov1) {
                r = "deu ruim! kkkk";
            } else {
                if (minf < *ov1) {
                    r = "true";
                } else {
                    r = "fudeu! kkkk";
                }
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "true";

    compareText(expected, r);
}

void Tester::testCaseSQ82() // i < Un
{
    SymbolValue i(i_);
    IntegerValue mone(-1);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < one) {
        r = "aki nao... kkk";
    } else {
        if (one < *ov1) {
            r = "caramelo! kkk";
        } else {
            if (mone < *ov1) {
                if (*ov1 < mone) {
                    r = "fudeu! kkkk";
                } else {
                    r = "true";
                }
            } else {
                r = "deu ruim! kkkk";
            }
        }
    }

    string expected = "true";

    compareText(expected, r);
}

void Tester::testCaseSQ83() // s < Un
{
    SymbolValue i(i_);
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < i) {
        r = "aki nao... kkk";
    } else {
        if (i < *ov1) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ84() // n < Un
{
    SymbolValue i(i_);

    AV exp = i + one;
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";

    if (*exp < *ov1) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 < *exp) {
            r = "caramelo! kkk";
        } else {
            r = "true";
        }
    }

    string expected = "true";

    compareText(expected, r);
}

void Tester::testCaseSQ85() // U < Un
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*UndefinedValue().evaluate() < *ov1) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 < *UndefinedValue().evaluate()) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ86() // Inf < Un
{
    SymbolValue i(i_);
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 < minf) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 < pinf) {
            if (pinf < *ov1) {
                r = "deu ruim! kkkk";
            } else {
                if (minf < *ov1) {
                    r = "true";
                } else {
                    r = "fudeu! kkkk";
                }
            }
        } else {
            r = "caramelo! kkk";
        }
    }

    string expected = "true";

    compareText(expected, r);
}

void Tester::testCaseSQ87() // i > Un
{
    SymbolValue i(i_);
    IntegerValue mone(-1);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 > one) {
        r = "aki nao... kkk";
    } else {
        if (one > *ov1) {
            r = "caramelo! kkk";
        } else {
            if (mone > *ov1) {
                r = "deu ruim! kkkk";
            } else {
                if (*ov1 > mone) {
                    r = "true";
                } else {
                    r = "fudeu! kkkk";
                }
            }
        }
    }

    string expected = "true";

    compareText(expected, r);
}

void Tester::testCaseSQ88() // s > Un
{
    SymbolValue i(i_);
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";

    if (*ov1 > i) {
        r = "aki nao... kkk";
    } else {
        if (i > *ov1) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ89() // n > Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV exp = i + j;
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 > *exp) {
        r = "aki nao... kkk";
    } else {
        if (*exp > *ov1) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ90() // U > Un
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*UndefinedValue().evaluate() > *ov1) {
        r = "aki nao... kkk";
    } else {
        if (*ov1 > *UndefinedValue().evaluate()) {
            r = "caramelo! kkk";
        } else {
            r = "false";
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ91() // Inf > Un
{
    SymbolValue i(i_);
    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();

    string r = "";
    if (*ov1 > minf) {
        if (*ov1 > pinf) {
            r = "caramelo! kkk";
        } else {
            if (pinf > *ov1) {
                if (minf > *ov1) {
                    r = "fudeu! kkkk";
                } else {
                    r = "false";
                }
            } else {
                r = "deu ruim! kkkk";
            }
        }
    } else {
        r = "aki nao... kkk";
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseSQ92() // Un + Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp = *ov1 + *ov2;

    string expected = "(sqrt(j) + sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ93() // Un - Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp = *ov1 - *ov2;

    string expected = "(sqrt(i)+(sqrt(j)*-1))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ94() // Un / Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp = *ov1 / *ov2;

    string expected = "(sqrt(i)/sqrt(j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ95() // Un * Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp1 = *ov1 * *ov2;
    AV exp2 = *ov1 * *ov1;

    AV exp = *exp1 + *exp2;

    string expected = "((sqrt(i)*sqrt(i))+(sqrt(j)*sqrt(i)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ96() // Un 'min' Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue mone(-1);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp1 = NAryValue(ov1->evaluate(), mone.evaluate(), Minimum).evaluate();
    AV exp2 = NAryValue(ov2->evaluate(), one.evaluate(), Minimum).evaluate();;

    AV exp = *(*exp1 + *exp2) - *ov2;

    string expected = "min((sqrt(j)+-1+(sqrt(j)*-1)),(sqrt(j)*-1))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ97() // Un 'max' Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);
    IntegerValue mone(-1);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp1 = NAryValue(ov1->evaluate(), mone.evaluate(), Maximum).evaluate();
    AV exp2 = NAryValue(ov2->evaluate(), one.evaluate(), Maximum).evaluate();;

    AV exp = *ov2 - *(*exp1 + *exp2);

    string expected = "min((sqrt(j)+(sqrt(j)*-1)+(sqrt(i)*-1)),(sqrt(j)+-1+(sqrt(i)*-1)))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ98() // Un(Un)
{
    SymbolValue i(i_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV exp = UnaryValue(ov1->evaluate(), SquareRoot).evaluate();

    string expected = "sqrt(sqrt(i))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ99() // Un >> Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp = *ov1 >> *ov2;

    string expected = "(sqrt(i)>>sqrt(j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ100() // Un << Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j.evaluate(), SquareRoot).evaluate();

    AV exp = *ov1 << *ov2;

    string expected = "(sqrt(i)<<sqrt(j))";

    compareText(expected, exp->toString());
}

void Tester::testCaseSQ101() // Un > Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j + i, SquareRoot).evaluate();
    AV ov3 = UnaryValue(i + one, SquareRoot).evaluate();

    string r = "";
    if (*ov1 > *ov2) {
        r = "aki nao... kkk";
    } else {
        if (*ov2 > *ov3) {
            r = "caramelo! kkk";
        } else {
            if (*ov3 > *ov1) {
                r = "true";
            } else {
                r = "opa! sai fora... kkk";
            }
        }
    }

    string expected = "true";

    compareText(expected, r);
}

void Tester::testCaseSQ102() // Un < Un
{
    SymbolValue i(i_);
    SymbolValue j(j_);

    AV ov1 = UnaryValue(i.evaluate(), SquareRoot).evaluate();
    AV ov2 = UnaryValue(j + i, SquareRoot).evaluate();
    AV ov3 = UnaryValue(i + one, SquareRoot).evaluate();

    string r = "";
    if (*ov1 < *ov2) {
        r = "aki nao... kkk";
    } else {
        if (*ov2 < *ov3) {
            r = "caramelo! kkk";
        } else {
            if (*ov3 < *ov1) {
                r = "opa! sai fora... kkk";
            } else {
                r = "false";
            }
        }
    }

    string expected = "false";

    compareText(expected, r);
}

void Tester::testCaseRA1()
{
    std::string source = R"raw(
void f() {
    int a = 1;
}
        )raw";

    std::string expected = R"raw(
a : [1, 1]
f : [f, f]
        )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA2()
{
    std::string source = R"raw(
int* init();

int foo() {
  int* v = init();
}
        )raw";

    std::string expected = R"raw(
v : [init, init]
foo : [foo, foo]
init : [init, init]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA3()
{
    std::string source = R"raw(
 int foo(int* v, int n)
 {
   int i;

   for (i = 5; i < n; i++) {
     v[i];
   }
 }
        )raw";

    std::string expected = R"raw(
i:[5,max(n,5)]
n:[n, n]
v:[v,v]
foo:[foo,foo]
)raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA4()
{
    std::string source = R"raw(
 int fm(int fn);

 int foo(int* v, int n)
 {
   int i;

   i = fm(n);
   for (; i < n; i++) {
     v[i];
   }
 }
        )raw";

    std::string expected = R"raw(
i:[fm, max(n, fm)]
v:[v, v]
foo:[foo, foo]
fn:[fn, fn]
n:[n, n]
fm:[fm, fm]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA5()
{
    std::string source = R"raw(
 int init(int* v, int n);
 int md(int n);

 int foo(int* v, int* v2, int n) {
     init(v2, n);
     v2[n];
     int m = md(n);
     v2[m];
 }
        )raw";

    std::string expected = R"raw(
m:[md, md]
n:[n, n]
v2:[v2, v2]
v:[v, v]
n:[n, n]
init:[init, init]
v:[v, v]
md:[md, md]
foo:[foo, foo]
n:[n, n]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA6()
{
    std::string source = R"raw(
 int get();
 int* init(int n);

 int foo(int n) {
     int *v1, *v2, *v3;
     int m1, m2;
     if (n < 5) {
         v1 = init(n);
         m1 = get();
         v1[m1];
     }
     else {
         v2 = init(42);
         v2[41];
     }
     v3 = v1;
     m2 = get();
     v3[m2];
 }
        )raw";

    std::string expected = R"raw(
v2:[init,init]
v1:[init,init]
v3:[init,init]
n:[n,n]
m2:[get,get]
foo:[foo,foo]
n:[n,n]
m1:[get,get]
init:[init,init]
get:[get,get]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA7()
{
    std::string source = R"raw(
 int get();
 int* init();
 int* init2();

 int foo(int n, int m) {
     int* v;
         v = init();
     v[50];
     int i = get();
     v[i];
     int* v2;
         v2 = v;
     for (i = 0; i < n; i++) {
         v2[i];
     }
     v[50 - get()];
 }
        )raw";

    std::string expected = R"raw(
v2:[init, init]
i:[0, max(n, 0)]
v:[init, init]
m:[m, m]
foo:[foo, foo]
n:[n, n]
init:[init, init]
init2:[init2, init2]
get:[get, get]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA8()
{
    std::string source = R"raw(
 int get();
 int* malloc(int a3);

 int f(int n4) {
   int m4 = n4 + get();
   return m4;
 }

 int g() {
   int a1 = 5;
   while (a1 < 5) {
     a1 = a1 + f(a1);
   }
   int* v2;
   v2 = malloc(42);
   v2[a1];
 }
        )raw";

    std::string expected = R"raw(
v2:[malloc, malloc]
a1:[-Inf, max((f+4), 5)]
g:[g, g]
m4:[(get+n4), (get+n4)]
n4:[n4, n4]
f:[f, f]
a3:[a3, a3]
malloc:[malloc, malloc]
get:[get, get]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA9()
{
    std::string source = R"raw(
 int g();

 int f() {
   int a;
   a = 30;
   while (a < 25) {
     a = a + g();
   }
 }
        )raw";

    std::string expected = R"raw(
a:[-Inf,max((g+24),30)]
f:[f,f]
g:[g,g]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA10()
{
    std::string source = R"raw(
 int m(int n);
 int* init(int n);

 int foo(int* v, int n)
 {
     v = init(n);
     int i;
     for (i = m(n); i < n; i++) {
         v[i];
     }
 }
        )raw";

    std::string expected = R"raw(
i:[m,max(n,m)]
n:[n,n]
foo:[foo,foo]
v:[init,init]
n:[n,n]
n:[n,n]
init:[init,init]
m:[m,m]
)raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA11()
{
    std::string source = R"raw(
 int* init(int n);

 int foo(int* v, int n)
 {
     v = init(n);
     int i;
     for (i = 5; i < n; i++) {
         v[i];
     }
 }
        )raw";

    std::string expected = R"raw(
i:[5,max(n,5)]
v:[init,init]
foo:[foo,foo]
n:[n,n]
n:[n,n]
init:[init,init]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA12()
{
    std::string source = R"raw(
 int m(int n);
 int init(int* v, int n);

 int foo(int* v, int n)
 {
     init(v, n);
     v[n];
     v[m(n)];
 }
        )raw";

    std::string expected = R"raw(
n:[n,n]
v:[v,v]
n:[n,n]
m:[m,m]
foo:[foo,foo]
v:[v,v]
n:[n,n]
init:[init,init]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA13()
{
    std::string source = R"raw(
 int m();
 int init(int* v, int n);

 int foo(int* v, int n)
 {
     init(v, n);
     v[n];
     v[m() + n];
 }
        )raw";

    std::string expected = R"raw(
n:[n,n]
foo:[foo,foo]
v:[v,v]
init:[init,init]
v:[v,v]
m:[m,m]
n:[n,n]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA14()
{
    std::string source = R"raw(
 int Try (int va,int* vb,int* vc,int* vd,int* ve,int* vf) ;

 Doit (int* a, int* b, int* c, int* x)
 {
     int i, q;
     a[9];
     b[17];
     c[15];
     x[9];
     i = 0 - 7;
     while (i <= 16)
       {
       if ((i >= 1) && (i <= 8))
           a[i] = 1;
       if (i >= 2)
           b[i] = 1;
       if (i <= 7)
           c[i + 7] = 1;
       i = i + 1;
       }

     Try (1, &q, b, a, c, x);
     if (!q)

     i;
 }
        )raw";

    std::string expected = R"raw(
a:[-Inf,+Inf]
b:[-Inf,+Inf]
c:[-Inf,+Inf]
i:[-7,17]
x:[x,x]
Doit:[Doit,Doit]
ve:[ve,ve]
vd:[vd,vd]
vc:[vc,vc]
vb:[vb,vb]
vf:[vf,vf]
va:[va,va]
Try:[Try,Try]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA15()
{
    std::string source = R"raw(
 int d();

 int foo(int* v) {
   int k = 4;
   while (k < 5) {
     k = d();
     v[k];
     k = k + 1;
   }
 }
        )raw";

    std::string expected = R"raw(
k:[min((d+1), 4), max((d+1), 4)]
v:[v, v]
foo:[foo, foo]
d:[d, d]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA16()
{
    std::string source = R"raw(
 int foo(int* v, int n) {
   v[42];
   int m = n + 1;
   int k = m + n;
   int tmp = 42 - k;
   if (n < 5)
     m = 2;
   else
     m = 4;
   v[tmp];
 }
        )raw";

    std::string expected = R"raw(
tmp:[(41+(n*-2)),(41+(n*-2))]
m:[2, 4]
n:[n, n]
v:[v, v]
k:[((n*2)+1), ((n*2)+1)]
foo:[foo, foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA17()
{
    std::string source = R"raw(
 int h1(int* a1, int b1, int c1);
 int m1(int n1);
 int d1(int j1);
 int print(int n1);

 int foo(int* v1, int n1)
 {
     int a1[5];
     a1[2] = 1;
     h1(v1, n1, 0);
     v1[n1] = 1;
     int md1 = m1(n1);
     v1[md1];
     int i1;
     int k1 = 4;
     for (i1 = 0; i1 < m1(n1); i1++) {
         k1 = d1(i1) + k1;
         v1[k1 + i1];
         k1 = 1;
     }
     print(n1);
     int z1 = print(n1);
     return z1;
 }
        )raw";

    std::string expected = R"raw(
k1:[1,4]
i1:[0,max(m1,0)]
v1:[1,1]
foo:[foo,foo]
n1:[n1,n1]
print:[print,print]
n1:[n1,n1]
n1:[n1,n1]
m1:[m1,m1]
a1:[1,1]
d1:[d1,d1]
c1:[c1,c1]
j1:[j1,j1]
b1:[b1,b1]
z1:[print,print]
a1:[a1,a1]
md1:[m1,m1]
h1:[h1,h1]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA18()
{
    std::string source = R"raw(
 int foo(int n1)
 {
     int a = n1 << 2;
 }
        )raw";

    std::string expected = R"raw(
foo:[foo,foo]
n1:[n1,n1]
a:[(n1*4),(n1*4)]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA19()
{
    std::string source = R"raw(
 int foo(int n, int m)
 {
     int a = n >> m;
 }
        )raw";

    std::string expected = R"raw(
foo:[foo,foo]
n:[n,n]
m:[m,m]
a:[(n>>m),(n>>m)]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA20()
{
    std::string source = R"raw(
 void ex3(int n) {
   int i = 0, j = 0;

   while(i < n) {
     j = j + n;

     i++;

   }

 }
        )raw";

    std::string expected = R"raw(
j:[-Inf,+Inf]
i:[0,max(n,0)]
n:[n,n]
ex3:[ex3,ex3]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA21()
{
    std::string source = R"raw(
 void foo(int n)
 {
   int i, j, k;
   k = 0;
   while (k < 100) {
     i = 0;
     j = k - 1;
     while (i < j) {
       i = i + 1;
       j = j - 1;
     }
     k = k + 1;
   }
 }
        )raw";

    std::string expected = R"raw(
i:[0,98]
j:[-1,98]
k:[0,100]
n:[n,n]
foo:[foo,foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA22()
{
    std::string source = R"raw(
 void foo (int n) {
   int i = 0, j = 0, k;
   if (1) {
     i = 1;
   } else {
     i = 5;
   }
   if (1) {
     j = 0;
   } else {
     j = 9;
   }
   k = i + j;
 }
        )raw";

    std::string expected = R"raw(
k:[1,14]
foo:[foo,foo]
n:[n,n]
i:[1,5]
j:[0,9]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA23()
{
    std::string source = R"raw(
 void cond(int a) {
   if (a >= 2) {
     a -= 2;
   }
 }
        )raw";

    std::string expected = R"raw(
a:[min(max((-2+a),0),a),max((-2+a),min(1,a))]
cond:[cond,cond]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA24()
{
    std::string source = R"raw(
void foo() {
  int i = 1 ? 1 : 0;
}
        )raw";

    std::string expected = R"raw(
i : [0, 1]
foo : [foo, foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA25()
{
    std::string source = R"raw(
 void foo() {
   int a, b, x;
   if (1) {
     a = 9;
     b = 15;
   } else {
     a = 0;
     b = 5;
   }
   x = a > b ? a + b : a * b;
 }
        )raw";

    std::string expected = R"raw(
x:[0,135]
b:[5,15]
a:[0,9]
foo:[foo,foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA26()
{
    std::string source = R"raw(
 void foo() {
   int a, b, x;
   if (1) {
     a = 9;
     b = 15;
   } else {
     a = 0;
     b = 5;
   }
   x = a > b ? a - b : a + b;
 }
        )raw";

    std::string expected = R"raw(
x:[-2,24]
b:[5,15]
a:[0,9]
foo:[foo,foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA27()
{
    std::string source = R"raw(
 void foo() {
   int a, b, x;
   if (1) {
     a = 9;
     b = 15;
   } else {
     a = 0;
     b = 5;
   }
   x = a > b ? a - b : a / b;
 }
        )raw";

    std::string expected = R"raw(
x:[-2,4]
b:[5,15]
a:[0,9]
foo:[foo,foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA28()
{
    std::string source = R"raw(
 void foo() {
   int a, b, x;
   if (1) {
     a = 9;
     b = 15;
   } else {
     a = 0;
     b = 5;
   }
   x = a > b ? a - b : a - b;
 }
        )raw";

    std::string expected = R"raw(
x:[-15,4]
b:[5,15]
a:[0,9]
foo:[foo,foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA29()
{
    std::string source = R"raw(
 void foo() {
   int a, b, x;
   if (1) {
     a = 9;
     b = 15;
   } else {
     a = 0;
     b = 5;
   }
   x = a > b ? a + b : a + b;
 }
        )raw";

    std::string expected = R"raw(
x:[5,24]
b:[5,15]
a:[0,9]
foo:[foo,foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}

void Tester::testCaseRA30()
{
    std::string source = R"raw(
 void foo() {
   int a, b, x;
   if (1) {
     a = 9;
     b = 15;
   } else {
     a = 0;
     b = 5;
   }
   x = a < b ? a + b : a - b;
 }
        )raw";

    std::string expected = R"raw(
x:[-4,24]
b:[5,15]
a:[0,9]
foo:[foo,foo]
    )raw";

    std:string output = getRanges(source);

    compareText(expected, output);
}
