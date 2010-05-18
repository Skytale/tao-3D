#ifndef MAIN_H
#define MAIN_H
// ****************************************************************************
//  main.h                          (C) 1992-2009 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     The global variables defined in main.cpp
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************

#include "tree.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "syntax.h"
#include "context.h"
#include "compiler.h"
#include "options.h"
#include <map>
#include <time.h>


XL_BEGIN

struct Serializer;
struct Deserializer;


struct SourceFile
// ----------------------------------------------------------------------------
//    A source file and associated data
// ----------------------------------------------------------------------------
{
    SourceFile(text n, Tree *t, Symbols *s, bool readOnly = false);
    SourceFile();
    text        name;
    Tree_p      tree;
    Symbols_p   symbols;
    time_t      modified;
    text        hash;
    bool        changed;
    bool        readOnly;
};
typedef std::map<text, SourceFile> source_files;
typedef std::vector<text> source_names;


struct Main
// ----------------------------------------------------------------------------
//    The main entry point and associated data
// ----------------------------------------------------------------------------
{
    Main(int argc, char **argv, Compiler &comp,
         text syntax = "xl.syntax",
         text style = "xl.stylesheet",
         text builtins = "xl.builtins");
    ~Main();

    int  ParseOptions();
    int  LoadContextFiles(source_names &context_file_names);
    void EvalContextFiles(source_names &context_file_names);
    int  LoadFiles();
    int  LoadFile(text file, bool updateContext = false);
    text SearchFile(text input);
    int  Run();
    int  Diff();

public:
    int          argc;
    char **      argv;

    Positions    positions;
    Errors       errors;
    Syntax       syntax;
    Options      options;
    Compiler    &compiler;
    Context_p    context;
    Renderer     renderer;
    source_files files;
    source_names file_names;
    Deserializer *reader;
    Serializer   *writer;

};

extern Main *MAIN;

XL_END

#endif // MAIN_H
