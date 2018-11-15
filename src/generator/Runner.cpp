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

#include "Runner.h"
#include "AST.h"
#include "AstFixer.h"
#include "Bind.h"
#include "Control.h"
#include "ConstraintGenerator.h"
#include "ConstraintStreamWriter.h"
#include "Debug.h"
#include "DiagnosticCollector.h"
#include "DependentTypesGenerator.h"
#include "Dumper.h"
#include "Literals.h"
#include "RangeAnalysis.h"
#include "Symbols.h"
#include "TranslationUnit.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include "FunctionGenerator.h"

using namespace CPlusPlus;

namespace psyche {

extern bool debugEnabled;
bool displayStats;

/*!
 * \brief analyseProgram
 * \param source
 * \param control
 * \param name
 *
 * Core function that triggers all the work.
 */
std::unique_ptr<TranslationUnit> analyseProgram(const std::string& source,
                                                Control &control,
                                                StringLiteral &name,
                                                AnalysisOptions &options)
{
    std::unique_ptr<TranslationUnit> program(new TranslationUnit(&control, &name));
    program->setSource(source.c_str(), source.length());

    // Set program features.
    LanguageFeatures features;
    features.qtEnabled = 0;
    features.qtMocRunEnabled = 0;
    features.qtKeywordsEnabled = 0;
    features.cxx11Enabled = 1; // Enable this just to understand NULL as nullptr.
    features.objCEnabled = 0;
    features.c99Enabled = 1;
    features.c11Enabled = 1;
    features.cMode = 1;
    features.nullptrOnNULL = 1;
    program->setLanguageFeatures(features);

    DiagnosticCollector collector;
    control.setDiagnosticClient(&collector); // Collector is alive only within this scope.

    // Check whether the parser finished successfully.
    if (!program->parse()) {
        std::cout << "Parsing failed" << std::endl;
        return nullptr;
    }

    // We only proceed if the source is free from syntax errors.
    if (!collector.isEmpty()) {
        std::cout << "Source has syntax errors" << std::endl;
        return nullptr;
    }

    // If we have no AST, there's nothing to do.
    if (!program->ast() || !program->ast()->asTranslationUnit()) {
        std::cout << "No AST" << std::endl;
        return nullptr;
    }

    TranslationUnitAST* ast = program->ast()->asTranslationUnit();
    Dumper(program.get()).dump(ast, ".ast.dot");

    // Binding phase, this is when we create symbols.
    Namespace* globalNs = control.newNamespace(0, nullptr);
    Bind bind(program.get());
    bind(ast, globalNs);

    // Disambiguate eventual ambiguities.
    AstFixer astFixer(program.get());
    astFixer.fix(ast);
    if (displayStats) {
        std::cout << "Ambiguities stats" << std::endl
                  << astFixer.stats() << std::endl;
    }

    if (isProgramAmbiguous(program.get(), ast)) {
        std::cout << "Code has unresolved ambiguities" << std::endl;
        return nullptr;
    }

    std::ostringstream oss;

    if (options.flag_.writeConstraints_)
        options.constraints_.assign(oss.str());
    ////////////////////////////////////////
    //debugVisit = true;
    debugEnabled = false;
    RangeAnalysis rangeAnalysis(program.get());
    rangeAnalysis.run(ast->asTranslationUnit(), globalNs);
    //std::cout << "Range Analysis... ok!" << std::endl;
    DependentTypesGenerator dependentTypesGenerator(rangeAnalysis);
    dependentTypesGenerator.generate();
    //std::cout << "Dependent Types Generator... ok!" << std::endl;
    FunctionGenerator functionGenerator(program.get(), dependentTypesGenerator);
    functionGenerator.generate(ast->asTranslationUnit(), globalNs);
    //std::cout << "Function Generator... ok!" << std::endl;
    ////////////////////////////////////////

    return program;
}

} // namespace psyche
