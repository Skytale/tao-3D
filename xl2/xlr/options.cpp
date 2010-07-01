// ****************************************************************************
//   Christophe de Dinechin                                        XL2 PROJECT
//   XL COMPILER: options.cpp
// ****************************************************************************
//
//   File Description:
//
//     Processing of XL compiler options
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is distributed under the GNU General Public License
// See the enclosed COPYING file or http://www.gnu.org for information
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include "options.h"
#include "errors.h"
#include "renderer.h"

XL_BEGIN

/* ========================================================================= */
/*                                                                           */
/*   The compiler options parsing                                            */
/*                                                                           */
/* ========================================================================= */

Options *Options::options = NULL;

Options::Options(Errors &err, int argc, char **argv):
/*---------------------------------------------------------------------------*/
/*  Set the default values for all options                                   */
/*---------------------------------------------------------------------------*/
#define OPTVAR(name, type, value)       name(value),
#define OPTION(name, descr, code)
#define TRACE(name)
#include "options.tbl"
    arg(0), argc(argc), argv(argv), errors(err)
{}


static void Usage(char **argv)
// ----------------------------------------------------------------------------
//    Display usage information when an invalid name is given
// ----------------------------------------------------------------------------
{
    std::cerr << "Usage:\n";
    std::cerr << argv[0] << " <options> <source_file>\n";

#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
    std::cerr << "\t-" << #name ": " descr "\n";
#if DEBUG
#define TRACE(name)                 std::cerr << "\t-t" #name ": Trace " #name "\n";
#else
#define TRACE(name)
#endif
#include "options.tbl"
}


static bool OptionMatches(kstring &command_line, kstring optdescr)
// ----------------------------------------------------------------------------
//   Check if a given option matches the command line
// ----------------------------------------------------------------------------
// Single character options may accept argument as same or next parameter
{
    size_t len = strlen(optdescr);
    if (strncmp(command_line, optdescr, len) == 0)
    {
        command_line += len;
        return true;
    }
    return false;
}


static kstring OptionString(kstring &command_line, Options &opt)
// ----------------------------------------------------------------------------
//   Check if we find an integer between low and high on the command line
// ----------------------------------------------------------------------------
{
    if (*command_line)
    {
        kstring result = command_line;
        command_line = "";
        return result;
    }
    opt.arg += 1;
    if (opt.arg  < opt.argc)
    {
        command_line = "";
        return opt.argv[opt.arg];
    }
    opt.errors.Error("Option '$1' is not an integer value",
                     opt.arg, command_line);
    return "";
}


static ulong OptionInteger(kstring &command_line, Options &opt,
                           ulong low, ulong high)
// ----------------------------------------------------------------------------
//   Check if we find an integer between low and high on the command line
// ----------------------------------------------------------------------------
{
    uint result = low;
    kstring old = command_line;
    if (*command_line)
    {
        if (isdigit(*command_line))
            result = strtol(command_line, (char**) &command_line, 10);
        else
            opt.errors.Error("Option '$1' is not an integer value",
                             opt.arg, command_line);
    }
    else
    {
        opt.arg += 1;
        if (opt.arg  < opt.argc && isdigit(opt.argv[opt.arg][0]))
            result = strtol(old = opt.argv[opt.arg],
                            (char **) &command_line, 10);
        else
            opt.errors.Error("Option '$1' is not an integer value",
                             opt.arg, command_line);
    }
    if (result < low || result > high)
    {
        char lowstr[15], highstr[15];
        sprintf(lowstr, "%lu", low);
        sprintf(highstr, "%lu", high);
        opt.errors.Error("Option '$1' is out of range $2..$3",
                         opt.arg, old, lowstr, highstr);
        if (result < low)
            result = low;
        else
            result = high;
    }
    return result;
}


text Options::Parse(int argc, char **argv, bool consumeFile)
// ----------------------------------------------------------------------------
//   Start parsing options, return first non-option
// ----------------------------------------------------------------------------
{
    this->arg = 1;
    this->argc = argc;
    this->argv = argv;
    if (cstring envopt = getenv("OPTIONS"))
    {
        this->argv[0] = envopt;
        this->arg = 0;
    }
    return ParseNext(consumeFile);
}


text Options::ParseNext(bool consumeFiles)
// ----------------------------------------------------------------------------
//   Parse the command line, looking for known options, return first unknown
// ----------------------------------------------------------------------------
// Note: What we read here should be compatible with GCC parsing
{
    while (arg < argc)
    {
        if(argv[arg] && argv[arg][0] == '-' && argv[arg][1])
        {
            kstring argval = argv[arg] + 1;

#define OPTVAR(name, type, value)
#define OPTION(name, descr, code)                                       \
            if (OptionMatches(argval, #name))                           \
            {                                                           \
                code;                                                   \
                                                                        \
                if (*argval)                                            \
                    errors.Error("Garbage found after option '$1'",     \
                                 arg, argval);                          \
            }                                                           \
            else
#if XL_DEBUG
#define TRACE(name)                                 \
            if (OptionMatches(argval, "t" #name))   \
                traces.name = true;                 \
            else
#else
#define TRACE(name)
#endif
#define INTEGER(n, m)           OptionInteger(argval, *this, n, m)
#define STRING                  OptionString(argval, *this)
#include "options.tbl"
            {
                // Default: Output usage
                errors.Error("Unknown option '$1' ignored", arg, argval);
                Usage(argv);
            }
            arg++;
        }
        else
        {
            text fileName = text(argv[arg]);
            if (consumeFiles)
                arg++;
            return fileName;
        }
    }
    return text("");
}

XL_END
ulong xl_traces = 0;
// ----------------------------------------------------------------------------
//   Bits for each trace
// ----------------------------------------------------------------------------

