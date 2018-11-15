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



#include "Control.h"
#include "Debug.h"
#include "Literals.h"
#include "Runner.h"
#include "Tester.h"
#include "TranslationUnit.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string.h>

using namespace CPlusPlus;
using namespace psyche;

namespace psyche {

unsigned VisitorDebugger::whiteSpace_ = 0;

extern bool displayStats;

} // namespace psyche

/*!
 * \brief sourceFromFile
 * \param fileName
 * \return
 *
 * Return an string representation for the given file's source.
 */
std::string sourceFromFile(const std::string& fileName)
{
    std::ifstream ifs(fileName);
    if (!ifs.is_open()) {
        std::cerr << "File not found: " << fileName << std::endl;
        return std::string();
    }

    std::stringstream ss;
    ss << ifs.rdbuf();
    ifs.close();
    printDebug("Source-code to be parsed:\n%s\n", ss.str().c_str());
    return ss.str();
}

/*!
 * \brief sourceToFile
 * \param content
 * \param fileName
 *
 * Write the given string as the source of the given file.
 */
void sourceToFile(const std::string& content, const std::string& fileName)
{
    std::ofstream ofs(fileName);
    ofs << content;
    ofs.close();
}

/*!
 * \brief runTests
 *
 * Run the tests we have.
 */
void runTests()
{
    Tester tester;
    tester.testAll();
}

/*!
 * \brief main
 * \return
 */
int main(int argc, char* argv[])
{
    if (argc > 3) {
        std::cout << "Usage:\n"
                  << "   - Run tests, only.\n"
                  << "   $ ./Gen\n\n"
                  << "   - Process a given file\n"
                  << "   $ ./Gen path/to/file.c "
                  << std::endl;
        return 0;
    }

    if (argc == 1) {
        try {
            runTests();
            std::cout << "Tests passed successfully!" << std::endl;
        } catch (...) {
            std::cout << "\nProblem with the tests, take a look at it!"
                      << std::endl;
            return 0;
        }

        return 0;
    }

    const std::string& inFileName = argv[1];
    StringLiteral name(inFileName.c_str(), inFileName.length());
    const std::string& source = sourceFromFile(inFileName);
    Control control;
    AnalysisOptions options;
    options.flag_.handleGNUerrorFunc_ = true;
    analyseProgram(source, control, name, options);

    return 0;
}
