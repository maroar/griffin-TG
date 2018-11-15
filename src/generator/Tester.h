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

#ifndef PSYCHE_TESTER_H__
#define PSYCHE_TESTER_H__

#include "CPlusPlusForwardDeclarations.h"
#include "Range.h"
#include "Runner.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// Use the macros below to control the testing worflow.

#define PSYCHE_TEST(F) TestData { &Tester::F, #F }

#define PSYCHE_TEST_FAIL(MSG) \
    do { \
        std::cout << "[psyche] Test " << currentTest_ << " -> FAILED\n" \
                  << "         Reason: " << MSG << "\n" \
                  << "         " << __FILE__ << ":" << __LINE__ << std::endl; \
        throw TestFailed(); \
    } while (0)

#define PSYCHE_EXPECT_EQ(EXPECTED, ACTUAL, EQ) \
    do { \
        if (!(EQ(EXPECTED, ACTUAL))) { \
            std::cout << "[psyche] Test " << ANSI_COLOR_RED << currentTest_ << ANSI_COLOR_RESET << " -> FAILED\n" \
                      << "         Expected: " << EXPECTED << "\n" \
                      << "         Actual  : " << ACTUAL << "\n" \
                      << "         " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw TestFailed(); \
        } \
    } while (0)

#define PSYCHE_EXPECT(EXPR, BOOLEAN) \
    do { \
        if (bool(EXPR) != BOOLEAN) { \
            std::cout << "[psyche] Test " << currentTest_ << " -> FAILED\n" \
                      << "        Expression is NOT " << #BOOLEAN << "\n" \
                      << "        " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw TestFailed(); \
        } \
    } while (0)

#define PSYCHE_EQ_OPR(A, B) A == B
#define PSYCHE_STD_EQUAL(A, B) std::equal(A.begin(), A.end(), B.begin())
#define PSYCHE_EXPECT_PTR_EQ(EXPECTED, ACTUAL) PSYCHE_EXPECT_EQ(EXPECTED, ACTUAL, PSYCHE_EQ_OPR)
#define PSYCHE_EXPECT_STR_EQ(EXPECTED, ACTUAL) PSYCHE_EXPECT_EQ(EXPECTED, ACTUAL, PSYCHE_EQ_OPR)
#define PSYCHE_EXPECT_INT_EQ(EXPECTED, ACTUAL) PSYCHE_EXPECT_EQ(EXPECTED, ACTUAL, PSYCHE_EQ_OPR)
#define PSYCHE_EXPECT_CONTAINER_EQ(EXPECTED, ACTUAL) PSYCHE_EXPECT_EQ(EXPECTED, ACTUAL, PSYCHE_STD_EQUAL)
#define PSYCHE_EXPECT_TRUE(EXPR) PSYCHE_EXPECT(EXPR, true)
#define PSYCHE_EXPECT_FALSE(EXPR) PSYCHE_EXPECT(EXPR, false)

namespace psyche {

/*!
 * \brief The Tester class
 *
 * A very minimal and simple testing infrastructure.
 */
class Tester final
{
public:
    Simbol* i_;
    Simbol* j_;
    Simbol* k_;
    Simbol* m_;
    Simbol* n_;
    Simbol* x_;
    Simbol* y_;
    Simbol* z_;
    /*!
     * \brief testAll
     *
     * Test everything.
     */
    void testAll();

    // Tests for Symbol
    //  +
    void testCaseSymbol0(); // n + i
    void testCaseSymbol1(); // i + s
    void testCaseSymbol2(); // s + i
    void testCaseSymbol3(); // s + s
    void testCaseSymbol4(); // I + s
    void testCaseSymbol5(); // s + I
    void testCaseSymbol6(); // n + s
    void testCaseSymbol7(); // s + n
    void testCaseSymbol8(); // n + n
    void testCaseSymbol9(); // n1 + n2
    void testCaseSymbol10(); // i + n
    //  *
    void testCaseSymbol11(); // i * s
    void testCaseSymbol12(); // s * i
    void testCaseSymbol13(); // s * s
    void testCaseSymbol14(); // I * s
    void testCaseSymbol15(); // s * I
    void testCaseSymbol16(); // n * s
    void testCaseSymbol17(); // s * n
    void testCaseSymbol18(); // n * n
    void testCaseSymbol19(); // n1 * n2
    void testCaseSymbol20(); // n * i
    void testCaseSymbol20_1(); // (n1/n2) * n3
    void testCaseSymbol20_2(); // n3 * (n1/n2)
    //  /
    void testCaseSymbol21(); // i / s
    void testCaseSymbol22(); // s / i
    void testCaseSymbol23_0(); // s / s
    void testCaseSymbol23_1(); // s1 / s2
    void testCaseSymbol23_2(); // s1 / s2*s1
    void testCaseSymbol23_3(); // s1*s2 / s2
    void testCaseSymbol23_4(); // s1*s2*s3 / s4*s1*s2
    void testCaseSymbol23_5(); // i1 / i2 // no mdc
    void testCaseSymbol23_6(); // n1 / n2*n2
    void testCaseSymbol23_7(); // n1*n2 / n2
    void testCaseSymbol23_8(); // n1*n2*n3 / n4*n1*n2
    void testCaseSymbol23_9(); // i*n1 / i*n2
    void testCaseSymbol23_10(); // i1*n1 / i2*n2
    void testCaseSymbol23_11(); // i1 / i2 // with mdc
    void testCaseSymbol24(); // I / s
    void testCaseSymbol25(); // s / I
    void testCaseSymbol26_1(); // s1*s2*s3 / s2
    void testCaseSymbol26_2(); // (s1+s2) / s2
    void testCaseSymbol26_3(); // s1*s2 / s2
    void testCaseSymbol27_1(); // s2 / s1*s2*s3
    void testCaseSymbol27_2(); // s1 / (s1+s2)
    void testCaseSymbol27_3(); // s4 / s1*s2*s3
    void testCaseSymbol28(); // n / n
    void testCaseSymbol29(); // n1 / n2
    void testCaseSymbol30(); // (i1*i2*n) / i1*i3 = i2*n / i3
    void testCaseSymbol31(); // i1*i2 / (n*i1*i3) = i2 / i3*n
    void testCaseSymbol32(); // (n+i2) / i2 = (n+i2) / i2
    void testCaseSymbol33(); // i2 / (n+i2) = i2 / (n+i2)
    void testCaseSymbol34(); // n / (-1*i1)
    void testCaseSymbol35(); // s / (-1*i1)
    void testCaseSymbol36(); // i1 / (-1*i2)
    void testCaseSymbol37(); // n1 / (-1*n2)
    void testCaseSymbol38(); // n1*n2 / n1
    void testCaseSymbol49(); // (s1+s2) / (s1+1)*(s1+s2)
    void testCaseSymbol50(); // min(n1, n2) / s3*min(n2, n1)*(s1+s2)
    void testCaseSymbol51(); // max(n1, n2) / max(n2, n1)*i1
    void testCaseSymbol52(); // (s1+1)*(s1+s2) / (s1+s2)
    void testCaseSymbol53(); // s3*min(n2, n1)*(s1+s2) / min(n1, n2)
    void testCaseSymbol54(); // max(n2, n1)*i1 / max(n1, n2)
    void testCaseSymbol55(); // (i1/i2) / i3
    void testCaseSymbol56(); // (s1/s2) / s3
    void testCaseSymbol57(); // (n1/n2) / n3
    void testCaseSymbol58(); // (n1/n2) / (n3/n4)
    void testCaseSymbol59(); // i3 / (i1/i2)
    void testCaseSymbol60(); // s3 / (s1/s2)
    void testCaseSymbol61(); // s1 / (n1/n2)
    void testCaseSymbol62(); // s1 / (n1/n2)
    void testCaseSymbol63(); // n3 / (n1/n2)
    void testCaseSymbol64(); // n3 / (n1/n2)
    void testCaseSymbol65(); // n3 / (n1/n2)

    void testCaseSymbol66(); // n1/i3 + n2/i3
    void testCaseSymbol66_1(); // n1/s3 + n2/s3
    void testCaseSymbol67(); // i1/i3 + i2/i3
    void testCaseSymbol68(); // s1/s3 + s2/s3
    void testCaseSymbol69(); // n1/n3 + n2/n3
    void testCaseSymbol70(); // i1/n3 + i2/n3
    void testCaseSymbol71(); // s1/n3 + s2/n3
    void testCaseSymbol72(); // n1/n3 + n2/n3

    //  =
    void testCaseSymbol39(); // s1 == s2
    void testCaseSymbol40(); // s1 == s1
    void testCaseSymbol41(); // s1 == 1*s2
    void testCaseSymbol42(); // s1 == 1*s1
    void testCaseSymbol43(); // s1 == 1*s2 + 0
    void testCaseSymbol44(); // s1 == 1*s1 + 0
    void testCaseSymbol45(); // 5*s2 == s1
    void testCaseSymbol46(); // 5*s1 == s1
    void testCaseSymbol47(); // 1*s1 + 2 == s1
    void testCaseSymbol48(); // 2*s1 + 0 == s1

    // Symbolic arithmetics
    void testCase1();
    void testCase2();
    void testCase3();
    void testCase4();
    void testCase5();
    void testCase6();
    void testCase7();
    void testCase8();
    void testCase9();
    void testCase10();
    void testCase11();
    void testCase12();
    void testCase13();
    void testCase14();
    void testCase15();
    void testCase16();
    void testCase17();
    void testCase18();
    void testCase19();
    void testCase20();
    void testCase21();
    void testCase22();
    void testCase23();
    void testCase24();
    void testCase25();
    void testCase26();
    void testCase27();
    void testCase28();
    void testCase29();
    void testCase30();
    void testCase31();
    void testCase32();
    void testCase33();
    void testCase34();
    void testCase35();
    void testCase36();
    void testCase37();
    void testCase38();
    void testCase39();
    void testCase40();
    void testCase41();
    void testCase42();
    void testCase43();
    void testCase44();
    void testCase45();
    void testCase46();
    void testCase47();
    void testCase48();
    void testCase49();
    void testCase50();
    void testCase51();
    void testCase52();
    void testCase53();
    void testCase54();
    void testCase55();
    void testCase56_1();
    void testCase56_2();
    void testCase57();

    //  - relational operators
    void testCaseR01(); // i1 > i2
    void testCaseR01_1(); // i1 > (i2/i3) // when true
    void testCaseR01_2(); // i1 > (i2/i3) // when false
    void testCaseR01_3(); // (i2/i3) > i1 // when true
    void testCaseR01_4(); // (i2/i3) > i1 // when false
    void testCaseR02(); // i2 < i1
    void testCaseR02_1(); // i1 < (i2/i3) // when true
    void testCaseR02_2(); // i1 < (i2/i3) // when false
    void testCaseR02_3(); // (i2/i3) < i1 // when true
    void testCaseR02_4(); // (i2/i3) < i1 // when false
    void testCaseR03(); // s1 > s1
    void testCaseR04(); // s1 < s1
    void testCaseR05(); // s1 < s2
    void testCaseR06(); // s1 > s2
    void testCaseR07(); // (s1 + 1) > s1
    void testCaseR07_1(); // (n1 - 1) > n1
    void testCaseR08(); // s1 > (s1 + 1)
    void testCaseR08_1(); // n1 > (n1 - 1)
    void testCaseR09(); // (s1 + 1) < s1
    void testCaseR09_1(); // (s1 - 1) < s1
    void testCaseR10(); // s1 < (s1 + 1)
    void testCaseR10_1(); // s1 < (s1 - 1)
    void testCaseR11(); // (s1 + 2) < (s1 + 1)
    void testCaseR11_1(); // (s1 - 2) < (s1 - 1)
    void testCaseR12(); // (s1 + 2) > (s1 + 1)
    void testCaseR12_1(); // (s1 - 2) > (s1 + 1)
    void testCaseR13(); // (s1 + s2 + 2) > (s1 + 1 + s2)
    void testCaseR14(); // (s1 + s2 + 2) < (s1 + 1 + s2)
    void testCaseR15(); // (s1 - s2 + 2) > (s1 - 1 + s2)
    void testCaseR16(); // (s1 - s2 + 2) < (s1 - 1 + s2)
    void testCaseR17(); // (s1 + s2 + s3) > (s1 + s2 + s4)
    void testCaseR18(); // (s1 + s2 + s3) < (s1 + s2 + s4)

    // tests for min/max operators (basics)
    void testCaseM01(); // min(i1, i2)
    void testCaseM02(); // max(i1, i2)
    void testCaseM03(); // min(i, i) = i
    void testCaseM04(); // max(i, i) = i
    void testCaseM05(); // max(s, i)
    void testCaseM06(); // min(i, s)
    void testCaseM07(); // max(n, s)
    void testCaseM08(); // min(s, n)
    void testCaseM09(); // max(+Inf, n)
    void testCaseM10(); // max(n, -Inf)
    void testCaseM11(); // min(+Inf, n)
    void testCaseM12(); // min(n, -Inf)
    void testCaseM13(); // min(s1+1, s1)
    void testCaseM14(); // max(s1+1, s1)
    void testCaseM15(); // min(s1-1, s1)
    void testCaseM16(); // max(s1-1, s1)

    // tests for min/max operators (advanced)
    void testCaseMA01(); // max(min(a, b), a) = a
    void testCaseMA02(); // max(a, min(a, b)) = a
    void testCaseMA03(); // min(a, max(a, b)) = a
    void testCaseMA04(); // min(max(a, b), a) = a
    void testCaseMA05(); // max(a, a, b, a) = max(a, b)
    void testCaseMA06(); // min(a, a, b, a) = min(a, b)
    void testCaseMA07(); // min(a, min(min(c, d), e)) = min(a, b, c, d, e)
    void testCaseMA08(); // max(a, max(max(c, d), e)) = max(a, b, c, d, e)
    void testCaseMA09(); // max(a, b, a-1, a+1, a+2, b+3) = min(a-1, b)
    void testCaseMA10(); // max(a, b, a+1, a+2, b+3) = max(a+2, b+3)
    void testCaseMA11(); // max(a, b) + max(a, b)
    void testCaseMA12(); // min(a, b) + min(a, b)
    void testCaseMA13(); // max(a, b) * max(a, b)
    void testCaseMA14(); // min(a, b) * min(a, b)
    void testCaseMA15(); // max(a, b) - max(a, b)
    void testCaseMA16(); // min(a, b) - min(a, b)
    void testCaseMA17(); // max(a, b) / max(a, b)
    void testCaseMA17_1(); // max(a, b) / max(a, c)
    void testCaseMA18(); // min(a, b) / min(a, b)
    void testCaseMA18_1(); // min(a, b) / min(a, c)
    void testCaseMA19(); // max(a, b) / min(a, b)
    void testCaseMA20(); // min(a, b) / max(a, b)
    void testCaseMA21(); // min(a, b) / max(c, d)
    void testCaseMA22(); // max(a, b) / min(c, d)
    void testCaseMA23(); // max(s, i) + 2
    void testCaseMA24(); // min(i, s) + 2
    void testCaseMA25(); // max(i, s) - 2
    void testCaseMA26(); // min(s, i) - 2
    void testCaseMA27(); // max(s1, i) + s2
    void testCaseMA28(); // min(i, s1) + s2
    void testCaseMA29(); // max(i, s1) - s2
    void testCaseMA30(); // min(s1, i) - s2
    void testCaseMA31(); // max(s1, i) + s1
    void testCaseMA32(); // min(i, s1) + s1
    void testCaseMA33(); // max(i, s1) - s1
    void testCaseMA34(); // min(s1, i) - s1
    void testCaseMA35(); // max(n1, i) + n2
    void testCaseMA36(); // min(i, n1) + n2
    void testCaseMA37(); // max(i, n1) - n2
    void testCaseMA38(); // min(n1, i) - n2
    void testCaseMA39(); // max(n1, n2) + n1
    void testCaseMA40(); // min(n1, n2) - n2
    void testCaseMA41(); // max(n2, n1) - n1
    void testCaseMA42(); // min(n1, n2) - n1
    void testCaseMA43(); // 2 + max(s, i)
    void testCaseMA44(); // 2 + min(i, s)
    void testCaseMA45(); // 2 - max(i, s)
    void testCaseMA46(); // 2 - min(s, i)
    void testCaseMA47(); // s2 + max(s1, i)
    void testCaseMA48(); // s2 + min(i, s1)
    void testCaseMA49(); // s2 - max(i, s1)
    void testCaseMA50(); // s2 - min(s1, i)
    void testCaseMA51(); // s1 + max(s1, i)
    void testCaseMA52(); // s1 + min(i, s1)
    void testCaseMA53(); // s1 - max(i, s1)
    void testCaseMA54(); // s1 - min(s1, i)
    void testCaseMA55(); // n2 + max(n1, i)
    void testCaseMA56(); // n2 + min(i, n1)
    void testCaseMA57(); // n2 - max(i, n1)
    void testCaseMA58(); // n2 - min(n1, i)
    void testCaseMA59(); // n1 + max(n1, i)
    void testCaseMA60(); // n1 + min(i, n1)
    void testCaseMA61(); // n1 - max(i, n1)
    void testCaseMA62(); // n1 - min(n1, i)
    void testCaseMA63(); // min(s1, n1) + (s2 - min(s1, s2-1))/2

    // Undefined Value
    void testCaseU01(); // U + i
    void testCaseU02(); // U - s
    void testCaseU03(); // U * Inf
    void testCaseU04(); // U / i
    void testCaseU05(); // U + n
    void testCaseU06(); // U - n
    void testCaseU07(); // U * n
    void testCaseU08(); // U / n
    void testCaseU09(); // min(U, n)
    void testCaseU10(); // max(U, n)
    void testCaseU11(); // min(n, U)
    void testCaseU12(); // max(n, U)
    void testCaseU13(); // U >> s
    void testCaseU14(); // U << i
    void testCaseU15(); // U > -Inf
    void testCaseU16(); // U < +Inf
    void testCaseU17(); // s + U
    void testCaseU18(); // i - U
    void testCaseU19(); // Inf * U
    void testCaseU20(); // i / U
    void testCaseU21(); // n + U
    void testCaseU22(); // n - U
    void testCaseU23(); // n * U
    void testCaseU24(); // n / U
    void testCaseU25(); // i >> U
    void testCaseU26(); // s << U
    void testCaseU27(); // s - U
    void testCaseU28(); // i * U
    void testCaseU29(); // s * U
    void testCaseU30(); // s / U
    void testCaseU31(); // U + s
    void testCaseU32(); // U * s
    void testCaseU33(); // U / s
    void testCaseU34(); // n >> U
    void testCaseU35(); // U >> n
    void testCaseU36(); // n << U
    void testCaseU37(); // U << n
    void testCaseU38(); // +Inf > U
    void testCaseU39(); // -Inf < U
    void testCaseU40(); // i >> A(U)
    void testCaseU41(); // i >> Inf
    void testCaseU42(); // i << A(U)
    void testCaseU43(); // i << Inf
    void testCaseU44(); // i / A(U)
    void testCaseU45(); // i < A(U)
    void testCaseU46(); // i > A(U)
    void testCaseU47(); // s >> A(U)
    void testCaseU48(); // s >> Inf
    void testCaseU49(); // s << A(U)
    void testCaseU50(); // s << Inf
    void testCaseU51(); // Inf >> A
    void testCaseU52(); // Inf >> i
    void testCaseU53(); // Inf >> s
    void testCaseU54(); // Inf >> Inf
    void testCaseU55(); // Inf >> n
    void testCaseU56(); // Inf << A
    void testCaseU57(); // Inf << i
    void testCaseU58(); // Inf << s
    void testCaseU59(); // Inf << Inf
    void testCaseU60(); // Inf << n
    void testCaseU61(); // Inf << U
    void testCaseU62(); // Inf >> U
    void testCaseU63(); // Inf / s
    void testCaseU64_0(); // Inf * Inf
    void testCaseU64_1(); // -Inf * Inf
    void testCaseU64_2(); // -Inf * -Inf
    void testCaseU65(); // Inf / A
    void testCaseU66(); // Inf * s
    void testCaseU67(); // Inf / Inf
    void testCaseU68(); // +Inf < A(U)
    void testCaseU69(); // -Inf < A(U)
    void testCaseU70(); // +Inf > A(U)
    void testCaseU71(); // -Inf > A(U)
    void testCaseU72(); // Inf == A(U)
    void testCaseU73(); // n * Inf
    void testCaseU74(); // n / U
    void testCaseU75(); // n >> U
    void testCaseU76(); // n >> Inf
    void testCaseU77(); // n << U
    void testCaseU78(); // n << Inf
    void testCaseU79(); // n < U
    void testCaseU80(); // n > U
    void testCaseU81(); // s * inf
    void testCaseU82(); // inf * A // A == i
    void testCaseU83(); // A * inf // A == i

    // test some simplifications
    void testCaseS01(); // s1 * (i1 + s2)
    void testCaseS02(); // (s1 + i1) * s2
    void testCaseS03(); // s3*s4*i1 * (s1 + s2 + i1)
    void testCaseS04(); // (s1 + s2 + i1) * s3*s4*i1
    void testCaseS05(); // (s1*s2 + s3 + i1) * (s4 + i2 + s2*s5)
    void testCaseS06(); // (s1 + s2 + i1) * (s1 + s2 + i2)
    void testCaseS07(); // s1>>s2 * (s1>>s2 + s3 + s2>>s1)
    void testCaseS08(); // (s1>>s2 + s3 + s2>>s1) * s1>>s2
    void testCaseS09(); // min(s1, s2) * (s1 + i1 + s1*s2)
    void testCaseS10(); // (s1 + i1 + s1/s2) * max(s1, s2)
    void testCaseS11(); // s1*s2*i1 * s1*s3*s4
    void testCaseS12(); // s1*s2*i1 * s1*s2*s4*i2
    void testCaseS13(); // s1 * s1*s2*s4*i2
    void testCaseS14(); // s1 * s2
    void testCaseS15(); // s2*s4*i2 * s1

    // square root
    void testCaseSQ01(); // sqrt(i)
    void testCaseSQ02(); // sqrt(i')
    void testCaseSQ03(); // sqrt(s)
    void testCaseSQ04(); // sqrt(+Inf)
    void testCaseSQ04_1(); // sqrt(-Inf)
    void testCaseSQ05(); // sqrt(U)
    void testCaseSQ06(); // sqrt(n1 + n2)
    void testCaseSQ07(); // sqrt(n1 * n2)
    void testCaseSQ08(); // sqrt(n1 / n2)
    void testCaseSQ09(); // sqrt(n1 - n2)
    void testCaseSQ10(); // sqrt(min(n1, n2))
    void testCaseSQ11(); // sqrt(max(n1, n2))
    void testCaseSQ12(); // sqrt(n >> s)
    void testCaseSQ13(); // sqrt(n1 << n2)
    // unary
    void testCaseSQ14(); // Un + i
    void testCaseSQ15(); // i + Un
    void testCaseSQ16(); // s + Un
    void testCaseSQ17(); // Un + s
    void testCaseSQ18(); // Un + n1
    void testCaseSQ19(); // n1 + Un
    void testCaseSQ19_1(); // +Inf + Un
    void testCaseSQ19_2(); // -Inf + Un
    void testCaseSQ19_3(); // Un + +Inf
    void testCaseSQ19_4(); // Un + -Inf
    void testCaseSQ20(); // Un * i
    void testCaseSQ21(); // i * Un
    void testCaseSQ22(); // Un * s
    void testCaseSQ23(); // s * Un
    void testCaseSQ24(); // Un * n
    void testCaseSQ25(); // n * Un
    void testCaseSQ26(); // Un / i
    void testCaseSQ27(); // i / Un
    void testCaseSQ28(); // Un / s
    void testCaseSQ29(); // s / Un
    void testCaseSQ30(); // Un / n
    void testCaseSQ31(); // n / Un
    void testCaseSQ32(); // min(i, Un)
    void testCaseSQ33(); // min(s, Un)
    void testCaseSQ34(); // min(n, Un)
    void testCaseSQ35(); // max(i, Un)
    void testCaseSQ36(); // max(s, Un)
    void testCaseSQ37(); // max(n, Un)
    void testCaseSQ38(); // Un >> i
    void testCaseSQ39(); // i >> Un
    void testCaseSQ40(); // Un >> s
    void testCaseSQ41(); // s >> Un
    void testCaseSQ42(); // Un >> n
    void testCaseSQ43(); // n >> Un
    void testCaseSQ44(); // Un << i
    void testCaseSQ45(); // i << Un
    void testCaseSQ46(); // Un << s
    void testCaseSQ47(); // s << Un
    void testCaseSQ48(); // Un << n
    void testCaseSQ49(); // n << Un
    void testCaseSQ50(); // Un + U
    void testCaseSQ51(); // Un - U
    void testCaseSQ52(); // Un * U
    void testCaseSQ53(); // Un / U
    void testCaseSQ54(); // Un min U
    void testCaseSQ55(); // Un max U
    void testCaseSQ56(); // Un >> U
    void testCaseSQ57(); // Un << U
    void testCaseSQ58(); // Un op U
    void testCaseSQ59(); // U op Un
    void testCaseSQ60(); // Un == Un
    void testCaseSQ61(); // Un == Un'
    void testCaseSQ62(); // Un == i
    void testCaseSQ63(); // Un == s
    void testCaseSQ64(); // Un == n
    void testCaseSQ65(); // Un == U
    void testCaseSQ66(); // Un == Inf
    void testCaseSQ67(); // i == Un
    void testCaseSQ68(); // s == Un
    void testCaseSQ69(); // n == Un
    void testCaseSQ70(); // U == Un
    void testCaseSQ71(); // Inf == Un
    void testCaseSQ72(); // Un > i
    void testCaseSQ73(); // Un > s
    void testCaseSQ74(); // Un > n
    void testCaseSQ75(); // Un > U
    void testCaseSQ76(); // Un > Inf
    void testCaseSQ77(); // Un < i
    void testCaseSQ78(); // Un < s
    void testCaseSQ79(); // Un < n
    void testCaseSQ80(); // Un < U
    void testCaseSQ81(); // Un < Inf
    void testCaseSQ82(); // i < Un
    void testCaseSQ83(); // s < Un
    void testCaseSQ84(); // n < Un
    void testCaseSQ85(); // U < Un
    void testCaseSQ86(); // Inf < Un
    void testCaseSQ87(); // i > Un
    void testCaseSQ88(); // s > Un
    void testCaseSQ89(); // n > Un
    void testCaseSQ90(); // U > Un
    void testCaseSQ91(); // Inf > Un
    void testCaseSQ92(); // Un + Un
    void testCaseSQ93(); // Un - Un
    void testCaseSQ94(); // Un / Un
    void testCaseSQ95(); // Un * Un
    void testCaseSQ96(); // Un 'min' Un
    void testCaseSQ97(); // Un 'max' Un
    void testCaseSQ98(); // Un(Un)
    void testCaseSQ99(); // Un >> Un
    void testCaseSQ100(); // Un << Un
    void testCaseSQ101(); // Un > Un
    void testCaseSQ102(); // Un < Un

    // Ragne Analysis
    void testCaseRA1();
    void testCaseRA2();
    void testCaseRA3();
    void testCaseRA4();
    void testCaseRA5();
    void testCaseRA6();
    void testCaseRA7();
    void testCaseRA8();
    void testCaseRA9();
    void testCaseRA10();
    void testCaseRA11();
    void testCaseRA12();
    void testCaseRA13();
    void testCaseRA14();
    void testCaseRA15();
    void testCaseRA16();
    void testCaseRA17();
    void testCaseRA18();
    void testCaseRA19();
    void testCaseRA20();
    void testCaseRA21();
    void testCaseRA22();
    void testCaseRA23();
    void testCaseRA24();
    void testCaseRA25();
    void testCaseRA26();
    void testCaseRA27();
    void testCaseRA28();
    void testCaseRA29();
    void testCaseRA30();
private:
    using TestData = std::pair<std::function<void(Tester*)>, const char*>;

    void checkAst(const std::string& source, std::string expected);
    void checkConstraints(const std::string& source, std::string expected);

    void compareText(std::string expected, std::string actual) const;

    void reset();

    /*
     * Add the name of all test functions to the vector below. Use the macro.
     */
    std::vector<TestData> tests_
    {
        // Basics on Symbols
        PSYCHE_TEST(testCaseSymbol0),
        PSYCHE_TEST(testCaseSymbol1),
        PSYCHE_TEST(testCaseSymbol2),
        PSYCHE_TEST(testCaseSymbol3),
        PSYCHE_TEST(testCaseSymbol4),
        PSYCHE_TEST(testCaseSymbol5),
        PSYCHE_TEST(testCaseSymbol6),
        PSYCHE_TEST(testCaseSymbol7),
        PSYCHE_TEST(testCaseSymbol8),
        PSYCHE_TEST(testCaseSymbol9),
        PSYCHE_TEST(testCaseSymbol10),
        PSYCHE_TEST(testCaseSymbol11),
        PSYCHE_TEST(testCaseSymbol12),
        PSYCHE_TEST(testCaseSymbol13),
        PSYCHE_TEST(testCaseSymbol14),
        PSYCHE_TEST(testCaseSymbol15),
        PSYCHE_TEST(testCaseSymbol16),
        PSYCHE_TEST(testCaseSymbol17),
        PSYCHE_TEST(testCaseSymbol18),
        PSYCHE_TEST(testCaseSymbol19),
        PSYCHE_TEST(testCaseSymbol20),
        PSYCHE_TEST(testCaseSymbol20_1),
        PSYCHE_TEST(testCaseSymbol20_2),
        PSYCHE_TEST(testCaseSymbol21),
        PSYCHE_TEST(testCaseSymbol22),
        PSYCHE_TEST(testCaseSymbol23_0),
        PSYCHE_TEST(testCaseSymbol23_1),
        PSYCHE_TEST(testCaseSymbol23_2),
        PSYCHE_TEST(testCaseSymbol23_3),
        PSYCHE_TEST(testCaseSymbol23_4),
        PSYCHE_TEST(testCaseSymbol23_5),
        PSYCHE_TEST(testCaseSymbol23_6),
        PSYCHE_TEST(testCaseSymbol23_7),
        PSYCHE_TEST(testCaseSymbol23_8),
        PSYCHE_TEST(testCaseSymbol23_9),
        PSYCHE_TEST(testCaseSymbol23_10),
        PSYCHE_TEST(testCaseSymbol23_11),
        PSYCHE_TEST(testCaseSymbol24),
        PSYCHE_TEST(testCaseSymbol25),
        PSYCHE_TEST(testCaseSymbol26_1),
        PSYCHE_TEST(testCaseSymbol26_2),
        PSYCHE_TEST(testCaseSymbol26_3),
        PSYCHE_TEST(testCaseSymbol27_1),
        PSYCHE_TEST(testCaseSymbol27_2),
        PSYCHE_TEST(testCaseSymbol27_3),
        PSYCHE_TEST(testCaseSymbol28),
        PSYCHE_TEST(testCaseSymbol29),
        PSYCHE_TEST(testCaseSymbol30),
        PSYCHE_TEST(testCaseSymbol31),
        PSYCHE_TEST(testCaseSymbol32),
        PSYCHE_TEST(testCaseSymbol33),
        PSYCHE_TEST(testCaseSymbol34),
        PSYCHE_TEST(testCaseSymbol35),
        PSYCHE_TEST(testCaseSymbol36),
        PSYCHE_TEST(testCaseSymbol37),
        PSYCHE_TEST(testCaseSymbol38),
        PSYCHE_TEST(testCaseSymbol39),
        PSYCHE_TEST(testCaseSymbol40),
        PSYCHE_TEST(testCaseSymbol41),
        PSYCHE_TEST(testCaseSymbol42),
        PSYCHE_TEST(testCaseSymbol43),
        PSYCHE_TEST(testCaseSymbol44),
        PSYCHE_TEST(testCaseSymbol45),
        PSYCHE_TEST(testCaseSymbol46),
        PSYCHE_TEST(testCaseSymbol47),
        PSYCHE_TEST(testCaseSymbol48),
        PSYCHE_TEST(testCaseSymbol49),
        PSYCHE_TEST(testCaseSymbol50),
        PSYCHE_TEST(testCaseSymbol51),
        PSYCHE_TEST(testCaseSymbol52),
        PSYCHE_TEST(testCaseSymbol53),
        PSYCHE_TEST(testCaseSymbol54),
        PSYCHE_TEST(testCaseSymbol55),
        PSYCHE_TEST(testCaseSymbol56),
        PSYCHE_TEST(testCaseSymbol57),
        PSYCHE_TEST(testCaseSymbol58),
        PSYCHE_TEST(testCaseSymbol59),
        PSYCHE_TEST(testCaseSymbol60),
        PSYCHE_TEST(testCaseSymbol61),
        PSYCHE_TEST(testCaseSymbol62),
        PSYCHE_TEST(testCaseSymbol63),
        PSYCHE_TEST(testCaseSymbol64),
        PSYCHE_TEST(testCaseSymbol65),
        PSYCHE_TEST(testCaseSymbol66),
        PSYCHE_TEST(testCaseSymbol66_1),
        PSYCHE_TEST(testCaseSymbol67),
        PSYCHE_TEST(testCaseSymbol68),
        PSYCHE_TEST(testCaseSymbol69),
        PSYCHE_TEST(testCaseSymbol70),
        PSYCHE_TEST(testCaseSymbol71),
        PSYCHE_TEST(testCaseSymbol72),

        //  - relational operators (basics)
        PSYCHE_TEST(testCaseR01),
        PSYCHE_TEST(testCaseR01_1),
        PSYCHE_TEST(testCaseR01_2),
        PSYCHE_TEST(testCaseR01_3),
        PSYCHE_TEST(testCaseR01_4),
        PSYCHE_TEST(testCaseR02),
        PSYCHE_TEST(testCaseR02_1),
        PSYCHE_TEST(testCaseR02_2),
        PSYCHE_TEST(testCaseR02_3),
        PSYCHE_TEST(testCaseR02_4),
        PSYCHE_TEST(testCaseR03),
        PSYCHE_TEST(testCaseR04),
        PSYCHE_TEST(testCaseR05),
        PSYCHE_TEST(testCaseR06),
        PSYCHE_TEST(testCaseR07),
        PSYCHE_TEST(testCaseR07_1),
        PSYCHE_TEST(testCaseR08),
        PSYCHE_TEST(testCaseR08_1),
        PSYCHE_TEST(testCaseR09),
        PSYCHE_TEST(testCaseR09_1),
        PSYCHE_TEST(testCaseR10),
        PSYCHE_TEST(testCaseR10_1),
        PSYCHE_TEST(testCaseR11),
        PSYCHE_TEST(testCaseR11_1),
        PSYCHE_TEST(testCaseR12),
        PSYCHE_TEST(testCaseR12_1),
        PSYCHE_TEST(testCaseR13),
        PSYCHE_TEST(testCaseR14),
        PSYCHE_TEST(testCaseR15),
        PSYCHE_TEST(testCaseR16),
        PSYCHE_TEST(testCaseR17),
        PSYCHE_TEST(testCaseR18),

        // min/max operators
        PSYCHE_TEST(testCaseM01),
        PSYCHE_TEST(testCaseM02),
        PSYCHE_TEST(testCaseM03),
        PSYCHE_TEST(testCaseM04),
        PSYCHE_TEST(testCaseM05),
        PSYCHE_TEST(testCaseM06),
        PSYCHE_TEST(testCaseM07),
        PSYCHE_TEST(testCaseM08),
        PSYCHE_TEST(testCaseM09),
        PSYCHE_TEST(testCaseM10),
        PSYCHE_TEST(testCaseM11),
        PSYCHE_TEST(testCaseM12),
        PSYCHE_TEST(testCaseM13),
        PSYCHE_TEST(testCaseM14),
        PSYCHE_TEST(testCaseM15),
        PSYCHE_TEST(testCaseM16),

        // tests for min/max operators (advanced)
        PSYCHE_TEST(testCaseMA01),
        PSYCHE_TEST(testCaseMA02),
        PSYCHE_TEST(testCaseMA03),
        PSYCHE_TEST(testCaseMA04),
        PSYCHE_TEST(testCaseMA05),
        PSYCHE_TEST(testCaseMA06),
        PSYCHE_TEST(testCaseMA07),
        PSYCHE_TEST(testCaseMA08),
        PSYCHE_TEST(testCaseMA09),
        PSYCHE_TEST(testCaseMA10),
        PSYCHE_TEST(testCaseMA11),
        PSYCHE_TEST(testCaseMA12),
        PSYCHE_TEST(testCaseMA13),
        PSYCHE_TEST(testCaseMA14),
        PSYCHE_TEST(testCaseMA15),
        PSYCHE_TEST(testCaseMA16),
        PSYCHE_TEST(testCaseMA17),
        PSYCHE_TEST(testCaseMA17_1),
        PSYCHE_TEST(testCaseMA18),
        PSYCHE_TEST(testCaseMA18_1),
        PSYCHE_TEST(testCaseMA19),
        PSYCHE_TEST(testCaseMA20),
        PSYCHE_TEST(testCaseMA21),
        PSYCHE_TEST(testCaseMA22),
        PSYCHE_TEST(testCaseMA23),
        PSYCHE_TEST(testCaseMA24),
        PSYCHE_TEST(testCaseMA25),
        PSYCHE_TEST(testCaseMA26),
        PSYCHE_TEST(testCaseMA27),
        PSYCHE_TEST(testCaseMA28),
        PSYCHE_TEST(testCaseMA29),
        PSYCHE_TEST(testCaseMA30),
        PSYCHE_TEST(testCaseMA31),
        PSYCHE_TEST(testCaseMA32),
        PSYCHE_TEST(testCaseMA33),
        PSYCHE_TEST(testCaseMA34),
        PSYCHE_TEST(testCaseMA35),
        PSYCHE_TEST(testCaseMA36),
        PSYCHE_TEST(testCaseMA37),
        PSYCHE_TEST(testCaseMA38),
        PSYCHE_TEST(testCaseMA39),
        PSYCHE_TEST(testCaseMA40),
        PSYCHE_TEST(testCaseMA41),
        PSYCHE_TEST(testCaseMA42),
        PSYCHE_TEST(testCaseMA43),
        PSYCHE_TEST(testCaseMA44),
        PSYCHE_TEST(testCaseMA45),
        PSYCHE_TEST(testCaseMA46),
        PSYCHE_TEST(testCaseMA47),
        PSYCHE_TEST(testCaseMA48),
        PSYCHE_TEST(testCaseMA49),
        PSYCHE_TEST(testCaseMA50),
        PSYCHE_TEST(testCaseMA51),
        PSYCHE_TEST(testCaseMA52),
        PSYCHE_TEST(testCaseMA53),
        PSYCHE_TEST(testCaseMA54),
        PSYCHE_TEST(testCaseMA55),
        PSYCHE_TEST(testCaseMA56),
        PSYCHE_TEST(testCaseMA57),
        PSYCHE_TEST(testCaseMA58),
        PSYCHE_TEST(testCaseMA59),
        PSYCHE_TEST(testCaseMA60),
        PSYCHE_TEST(testCaseMA61),
        PSYCHE_TEST(testCaseMA62),
        PSYCHE_TEST(testCaseMA63),

        // Undefined operations
        PSYCHE_TEST(testCaseU01),
        PSYCHE_TEST(testCaseU02),
        PSYCHE_TEST(testCaseU03),
        PSYCHE_TEST(testCaseU04),
        PSYCHE_TEST(testCaseU05),
        PSYCHE_TEST(testCaseU06),
        PSYCHE_TEST(testCaseU07),
        PSYCHE_TEST(testCaseU08),
        PSYCHE_TEST(testCaseU09),
        PSYCHE_TEST(testCaseU10),
        PSYCHE_TEST(testCaseU11),
        PSYCHE_TEST(testCaseU12),
        PSYCHE_TEST(testCaseU13),
        PSYCHE_TEST(testCaseU14),
        PSYCHE_TEST(testCaseU15),
        PSYCHE_TEST(testCaseU16),
        PSYCHE_TEST(testCaseU17),
        PSYCHE_TEST(testCaseU18),
        PSYCHE_TEST(testCaseU19),
        PSYCHE_TEST(testCaseU20),
        PSYCHE_TEST(testCaseU21),
        PSYCHE_TEST(testCaseU22),
        PSYCHE_TEST(testCaseU23),
        PSYCHE_TEST(testCaseU24),
        PSYCHE_TEST(testCaseU25),
        PSYCHE_TEST(testCaseU26),
        PSYCHE_TEST(testCaseU27),
        PSYCHE_TEST(testCaseU28),
        PSYCHE_TEST(testCaseU29),
        PSYCHE_TEST(testCaseU30),
        PSYCHE_TEST(testCaseU31),
        PSYCHE_TEST(testCaseU32),
        PSYCHE_TEST(testCaseU33),
        PSYCHE_TEST(testCaseU34),
        PSYCHE_TEST(testCaseU35),
        PSYCHE_TEST(testCaseU36),
        PSYCHE_TEST(testCaseU37),
        PSYCHE_TEST(testCaseU38),
        PSYCHE_TEST(testCaseU39),
        PSYCHE_TEST(testCaseU40),
        PSYCHE_TEST(testCaseU41),
        PSYCHE_TEST(testCaseU42),
        PSYCHE_TEST(testCaseU43),
        PSYCHE_TEST(testCaseU44),
        PSYCHE_TEST(testCaseU45),
        PSYCHE_TEST(testCaseU46),
        PSYCHE_TEST(testCaseU47),
        PSYCHE_TEST(testCaseU48),
        PSYCHE_TEST(testCaseU49),
        PSYCHE_TEST(testCaseU50),
        PSYCHE_TEST(testCaseU51),
        PSYCHE_TEST(testCaseU52),
        PSYCHE_TEST(testCaseU53),
        PSYCHE_TEST(testCaseU54),
        PSYCHE_TEST(testCaseU55),
        PSYCHE_TEST(testCaseU56),
        PSYCHE_TEST(testCaseU57),
        PSYCHE_TEST(testCaseU58),
        PSYCHE_TEST(testCaseU59),
        PSYCHE_TEST(testCaseU60),
        PSYCHE_TEST(testCaseU61),
        PSYCHE_TEST(testCaseU62),
        PSYCHE_TEST(testCaseU63),
        PSYCHE_TEST(testCaseU64_0),
        PSYCHE_TEST(testCaseU64_1),
        PSYCHE_TEST(testCaseU64_2),
        PSYCHE_TEST(testCaseU65),
        PSYCHE_TEST(testCaseU66),
        PSYCHE_TEST(testCaseU67),
        PSYCHE_TEST(testCaseU68),
        PSYCHE_TEST(testCaseU69),
        PSYCHE_TEST(testCaseU70),
        PSYCHE_TEST(testCaseU71),
        PSYCHE_TEST(testCaseU72),
        PSYCHE_TEST(testCaseU73),
        PSYCHE_TEST(testCaseU74),
        PSYCHE_TEST(testCaseU75),
        PSYCHE_TEST(testCaseU76),
        PSYCHE_TEST(testCaseU77),
        PSYCHE_TEST(testCaseU78),
        PSYCHE_TEST(testCaseU79),
        PSYCHE_TEST(testCaseU80),
        PSYCHE_TEST(testCaseU81),
        PSYCHE_TEST(testCaseU82),
        PSYCHE_TEST(testCaseU83),

        // sqrt
        PSYCHE_TEST(testCaseSQ01),
        PSYCHE_TEST(testCaseSQ02),
        PSYCHE_TEST(testCaseSQ03),
        PSYCHE_TEST(testCaseSQ04),
        PSYCHE_TEST(testCaseSQ04_1),
        PSYCHE_TEST(testCaseSQ05),
        PSYCHE_TEST(testCaseSQ06),
        PSYCHE_TEST(testCaseSQ07),
        PSYCHE_TEST(testCaseSQ08),
        PSYCHE_TEST(testCaseSQ09),
        PSYCHE_TEST(testCaseSQ10),
        PSYCHE_TEST(testCaseSQ11),
        PSYCHE_TEST(testCaseSQ12),
        PSYCHE_TEST(testCaseSQ13),
        // unique
        PSYCHE_TEST(testCaseSQ14),
        PSYCHE_TEST(testCaseSQ15),
        PSYCHE_TEST(testCaseSQ16),
        PSYCHE_TEST(testCaseSQ17),
        PSYCHE_TEST(testCaseSQ18),
        PSYCHE_TEST(testCaseSQ19),
        PSYCHE_TEST(testCaseSQ19_1),
        PSYCHE_TEST(testCaseSQ19_2),
        PSYCHE_TEST(testCaseSQ19_3),
        PSYCHE_TEST(testCaseSQ19_4),
        PSYCHE_TEST(testCaseSQ20),
        PSYCHE_TEST(testCaseSQ21),
        PSYCHE_TEST(testCaseSQ22),
        PSYCHE_TEST(testCaseSQ23),
        PSYCHE_TEST(testCaseSQ24),
        PSYCHE_TEST(testCaseSQ25),
        PSYCHE_TEST(testCaseSQ26),
        PSYCHE_TEST(testCaseSQ27),
        PSYCHE_TEST(testCaseSQ28),
        PSYCHE_TEST(testCaseSQ29),
        PSYCHE_TEST(testCaseSQ30),
        PSYCHE_TEST(testCaseSQ31),
        PSYCHE_TEST(testCaseSQ32),
        PSYCHE_TEST(testCaseSQ33),
        PSYCHE_TEST(testCaseSQ34),
        PSYCHE_TEST(testCaseSQ35),
        PSYCHE_TEST(testCaseSQ36),
        PSYCHE_TEST(testCaseSQ37),
        PSYCHE_TEST(testCaseSQ38),
        PSYCHE_TEST(testCaseSQ39),
        PSYCHE_TEST(testCaseSQ40),
        PSYCHE_TEST(testCaseSQ41),
        PSYCHE_TEST(testCaseSQ42),
        PSYCHE_TEST(testCaseSQ43),
        PSYCHE_TEST(testCaseSQ44),
        PSYCHE_TEST(testCaseSQ45),
        PSYCHE_TEST(testCaseSQ46),
        PSYCHE_TEST(testCaseSQ47),
        PSYCHE_TEST(testCaseSQ48),
        PSYCHE_TEST(testCaseSQ49),
        PSYCHE_TEST(testCaseSQ50),
        PSYCHE_TEST(testCaseSQ51),
        PSYCHE_TEST(testCaseSQ52),
        PSYCHE_TEST(testCaseSQ53),
        PSYCHE_TEST(testCaseSQ54),
        PSYCHE_TEST(testCaseSQ55),
        PSYCHE_TEST(testCaseSQ56),
        PSYCHE_TEST(testCaseSQ57),
        PSYCHE_TEST(testCaseSQ58),
        PSYCHE_TEST(testCaseSQ59),
        PSYCHE_TEST(testCaseSQ60),
        PSYCHE_TEST(testCaseSQ61),
        PSYCHE_TEST(testCaseSQ62),
        PSYCHE_TEST(testCaseSQ63),
        PSYCHE_TEST(testCaseSQ64),
        PSYCHE_TEST(testCaseSQ65),
        PSYCHE_TEST(testCaseSQ66),
        PSYCHE_TEST(testCaseSQ67),
        PSYCHE_TEST(testCaseSQ68),
        PSYCHE_TEST(testCaseSQ69),
        PSYCHE_TEST(testCaseSQ70),
        PSYCHE_TEST(testCaseSQ71),
        PSYCHE_TEST(testCaseSQ72),
        PSYCHE_TEST(testCaseSQ73),
        PSYCHE_TEST(testCaseSQ74),
        PSYCHE_TEST(testCaseSQ75),
        PSYCHE_TEST(testCaseSQ76),
        PSYCHE_TEST(testCaseSQ77),
        PSYCHE_TEST(testCaseSQ78),
        PSYCHE_TEST(testCaseSQ79),
        PSYCHE_TEST(testCaseSQ80),
        PSYCHE_TEST(testCaseSQ81),
        PSYCHE_TEST(testCaseSQ82),
        PSYCHE_TEST(testCaseSQ83),
        PSYCHE_TEST(testCaseSQ84),
        PSYCHE_TEST(testCaseSQ85),
        PSYCHE_TEST(testCaseSQ86),
        PSYCHE_TEST(testCaseSQ87),
        PSYCHE_TEST(testCaseSQ88),
        PSYCHE_TEST(testCaseSQ89),
        PSYCHE_TEST(testCaseSQ90),
        PSYCHE_TEST(testCaseSQ91),
        PSYCHE_TEST(testCaseSQ92),
        PSYCHE_TEST(testCaseSQ93),
        PSYCHE_TEST(testCaseSQ94),
        PSYCHE_TEST(testCaseSQ95),
        PSYCHE_TEST(testCaseSQ96),
        PSYCHE_TEST(testCaseSQ97),
        PSYCHE_TEST(testCaseSQ98),
        PSYCHE_TEST(testCaseSQ99),
        PSYCHE_TEST(testCaseSQ100),
        PSYCHE_TEST(testCaseSQ101),
        PSYCHE_TEST(testCaseSQ102),

        // the others about the range analysis
        PSYCHE_TEST(testCaseRA1),
        PSYCHE_TEST(testCaseRA2),
        PSYCHE_TEST(testCaseRA3),
        PSYCHE_TEST(testCaseRA4),
        PSYCHE_TEST(testCaseRA5),
        PSYCHE_TEST(testCaseRA6),
        PSYCHE_TEST(testCaseRA7),
        PSYCHE_TEST(testCaseRA8),
        PSYCHE_TEST(testCaseRA9),
        PSYCHE_TEST(testCaseRA10),
        PSYCHE_TEST(testCaseRA11),
        PSYCHE_TEST(testCaseRA12),
        PSYCHE_TEST(testCaseRA13),
        PSYCHE_TEST(testCaseRA14),
        PSYCHE_TEST(testCaseRA15),
        PSYCHE_TEST(testCaseRA16),
        PSYCHE_TEST(testCaseRA17),
        PSYCHE_TEST(testCaseRA18),
        PSYCHE_TEST(testCaseRA19),
        PSYCHE_TEST(testCaseRA20),
        PSYCHE_TEST(testCaseRA21),
        PSYCHE_TEST(testCaseRA22),
        PSYCHE_TEST(testCaseRA23),
        PSYCHE_TEST(testCaseRA24),
        PSYCHE_TEST(testCaseRA25),
        PSYCHE_TEST(testCaseRA26),
        PSYCHE_TEST(testCaseRA27),
        PSYCHE_TEST(testCaseRA28),
        PSYCHE_TEST(testCaseRA29),
        PSYCHE_TEST(testCaseRA30),

        // test some simplifications
        // multiplication (here some cases using the distribution)
        PSYCHE_TEST(testCaseS01),
        PSYCHE_TEST(testCaseS02),
        PSYCHE_TEST(testCaseS03),
        PSYCHE_TEST(testCaseS04),
        PSYCHE_TEST(testCaseS05),
        PSYCHE_TEST(testCaseS06),
        PSYCHE_TEST(testCaseS07),
        PSYCHE_TEST(testCaseS08),
        PSYCHE_TEST(testCaseS09),
        PSYCHE_TEST(testCaseS10),
        PSYCHE_TEST(testCaseS11),
        PSYCHE_TEST(testCaseS12),
        PSYCHE_TEST(testCaseS13),
        PSYCHE_TEST(testCaseS14),
        PSYCHE_TEST(testCaseS15),

        // the ones about symbolic arithmetic
        PSYCHE_TEST(testCase1),
        PSYCHE_TEST(testCase2),
        PSYCHE_TEST(testCase3),
        PSYCHE_TEST(testCase4),
        PSYCHE_TEST(testCase5),
        PSYCHE_TEST(testCase6),
        PSYCHE_TEST(testCase7),
        PSYCHE_TEST(testCase8),
        PSYCHE_TEST(testCase9),
        PSYCHE_TEST(testCase10),
        PSYCHE_TEST(testCase11),
        PSYCHE_TEST(testCase12),
        PSYCHE_TEST(testCase13),
        PSYCHE_TEST(testCase14),
        PSYCHE_TEST(testCase15),
        PSYCHE_TEST(testCase16),
        PSYCHE_TEST(testCase17),
        PSYCHE_TEST(testCase18),
        PSYCHE_TEST(testCase19),
        PSYCHE_TEST(testCase20),
        PSYCHE_TEST(testCase21),
        PSYCHE_TEST(testCase22),
        PSYCHE_TEST(testCase23),
        PSYCHE_TEST(testCase24),
        PSYCHE_TEST(testCase25),
        PSYCHE_TEST(testCase26),
        PSYCHE_TEST(testCase27),
        PSYCHE_TEST(testCase28),
        PSYCHE_TEST(testCase29),
        PSYCHE_TEST(testCase30),
        PSYCHE_TEST(testCase31),
        PSYCHE_TEST(testCase32),
        PSYCHE_TEST(testCase33),
        PSYCHE_TEST(testCase34),
        PSYCHE_TEST(testCase35),
        PSYCHE_TEST(testCase36),
        PSYCHE_TEST(testCase37),
        PSYCHE_TEST(testCase38),
        PSYCHE_TEST(testCase39),
        PSYCHE_TEST(testCase40),
        PSYCHE_TEST(testCase41),
        PSYCHE_TEST(testCase42),
        PSYCHE_TEST(testCase43),
        PSYCHE_TEST(testCase44),
        PSYCHE_TEST(testCase45),
        PSYCHE_TEST(testCase46),
        PSYCHE_TEST(testCase47),
        PSYCHE_TEST(testCase48),
        PSYCHE_TEST(testCase49),
        PSYCHE_TEST(testCase50),
        PSYCHE_TEST(testCase51),
        PSYCHE_TEST(testCase52),
        PSYCHE_TEST(testCase53),
        PSYCHE_TEST(testCase54),
        PSYCHE_TEST(testCase55),
        PSYCHE_TEST(testCase56_1),
        PSYCHE_TEST(testCase56_2),
        PSYCHE_TEST(testCase57)
    };

    std::string currentTest_;
    AnalysisOptions options_;
};

struct TestFailed {};

} // namespace psyche

#endif
