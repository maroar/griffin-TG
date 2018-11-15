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

#include <iostream>
#include <string>

namespace psyche {

extern bool debugEnabled;
extern bool debugVisit;
extern bool runingTests;

extern bool generateCSV;

#define PSYCHE_COMPONENT "psyche"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*!
 * \brief printDebug
 *
 * Print debug info (if debug is enabled) with variable args.
 */
template<typename... Args>
void printDebug(const char* message, Args... args)
{
    if (!debugEnabled)
        return;

    std::cout << "[" << PSYCHE_COMPONENT << "] ";
    std::printf(message, args...);
}

inline void printDebug(const char* message)
{
    if (!debugEnabled)
        return;

    std::cout << "[" << PSYCHE_COMPONENT << "] " << message;
}

inline void y(std::string txt) {
    std::cout << ANSI_COLOR_YELLOW << txt
         << ANSI_COLOR_RESET << std::endl;
}

inline void r(std::string txt) {
    std::cout << ANSI_COLOR_RED << txt
         << ANSI_COLOR_RESET << std::endl;
}

inline void b(std::string txt) {
    std::cout << ANSI_COLOR_BLUE << txt
         << ANSI_COLOR_RESET << std::endl;
}

inline void g(std::string txt) {
    std::cout << ANSI_COLOR_GREEN << txt
         << ANSI_COLOR_RESET << std::endl;
}

inline void d(std::string txt) {
    if (debugEnabled)
        std::cout << ANSI_COLOR_GREEN << txt
            << ANSI_COLOR_RESET << std::endl;
}

inline void dg(std::string txt) {
    if (debugEnabled)
        std::cout << ANSI_COLOR_GREEN << txt
            << ANSI_COLOR_RESET << std::endl;
}

inline void db(std::string txt) {
    if (debugEnabled)
        std::cout << ANSI_COLOR_BLUE << txt
            << ANSI_COLOR_RESET << std::endl;
}

inline void dy(std::string txt) {
    if (debugEnabled)
        std::cout << ANSI_COLOR_YELLOW << txt
            << ANSI_COLOR_RESET << std::endl;
}

inline void dr(std::string txt) {
    if (debugEnabled)
        std::cout << ANSI_COLOR_RED << txt
            << ANSI_COLOR_RESET << std::endl;
}

/*!
 * \brief The VisitorDebugger struct
 *
 * Helper RAII class to debug visitor's visit methods.
 */
class VisitorDebugger
{
    static unsigned whiteSpace_;

public:
    VisitorDebugger(const std::string& visit)
        : visit_(visit)
    {
        if (debugVisit) {
            whiteSpace_++;
            int i = 0;
            printf(ANSI_COLOR_GREEN);
            while (i < whiteSpace_) {
                printf(".");
                i++;
            }
            printf("begin - %s" ANSI_COLOR_RESET "\n", visit_.c_str());
        }
    }

    ~VisitorDebugger()
    {
        if (debugVisit) {
            int i = 0;
            printf(ANSI_COLOR_RED);
            while (i < whiteSpace_) {
                printf(".");
                i++;
            }
            printf("end   - %s" ANSI_COLOR_RESET "\n", visit_.c_str());
            whiteSpace_--;
        }

    }

private:
    std::string visit_;
};

} // namespace psyche

#define DEBUG_VISIT(METHOD) VisitorDebugger x(VISITOR_NAME"["#METHOD"]")
