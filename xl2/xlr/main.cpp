// ****************************************************************************
//  main.cpp                                                        XLR project
// ****************************************************************************
//
//   File Description:
//
//    Main entry point of the XL runtime and compiler
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
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Jerome Forissier <jerome@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "configuration.h"
#include <unistd.h>
#include <stdlib.h>
#include <map>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include "main.h"
#include "scanner.h"
#include "parser.h"
#include "renderer.h"
#include "errors.h"
#include "tree.h"
#include "context.h"
#include "compiler.h"
#include "options.h"
#include "basics.h"
#include "serializer.h"
#include "diff.h"
#include "bfs.h"
#include "gv.h"
#include "runtime.h"

XL_BEGIN

Main *MAIN = NULL;

SourceFile::SourceFile(text n, Tree *t, Symbols *s)
// ----------------------------------------------------------------------------
//   Construct a source file given a name
// ----------------------------------------------------------------------------
    : name(n), tree(t), symbols(s),
      modified(0), changed(false)
{
    struct stat st;
    stat (n.c_str(), &st);
    modified = st.st_mtime;
}


SourceFile::SourceFile()
// ----------------------------------------------------------------------------
//   Default constructor
// ----------------------------------------------------------------------------
    : name(""), tree(NULL), symbols(NULL),
      modified(0), changed(false)
{}


Main::Main(int inArgc, char **inArgv, Compiler &comp)
// ----------------------------------------------------------------------------
//   Initialization of the globals
// ----------------------------------------------------------------------------
    : argc(inArgc), argv(inArgv),
      positions(),
      errors(&positions),
      syntax("xl.syntax"),
      builtins(""),
      options(errors),
      compiler(comp),
      context(new Context(errors, &compiler)),
      renderer(std::cout, "xl.stylesheet", syntax),
      reader(NULL), writer(NULL)
{
    Options::options = &options;
    Context::context = context;
    Symbols::symbols = context;
    Renderer::renderer = &renderer;
    Syntax::syntax = &syntax;
}


Main::~Main()
// ----------------------------------------------------------------------------
//   Destructor
// ----------------------------------------------------------------------------
{
    delete reader;
    delete writer;
}


int Main::ParseOptions()
// ----------------------------------------------------------------------------
//   Load all files given on the command line and compile them
// ----------------------------------------------------------------------------
{
    text                         cmd, end = "";
    int                          filenum = 0;

    // Make sure debug function is linked in...
    if (getenv("SHOW_INITIAL_DEBUG"))
        debug(NULL);

    // Initialize the locale
    if (!setlocale(LC_CTYPE, ""))
        std::cerr << "WARNING: Cannot set locale.\n"
                  << "         Check LANG, LC_CTYPE, LC_ALL.\n";

    // Initialize basics
    EnterBasics(context);

    // Scan options and build list of files we need to process
    cmd = options.Parse(argc, argv);
    if (options.doDiff)
        options.parseOnly = true;
    if (options.builtins)
    {
        if (builtins.empty() || options.builtinsOverride)
            builtins = options.builtinsFile;
    }
    else
    {
        builtins = "";
    }

    for (; cmd != end; cmd = options.ParseNext())
    {
        if (options.doDiff && ++filenum > 2)
        {
          std::cerr << "Error: -diff option needs exactly 2 files" << std::endl;
          return true;
        }
        file_names.push_back(cmd);
    }
    return false;
}


int Main::LoadFiles()
// ----------------------------------------------------------------------------
//   Load all files given on the command line and compile them
// ----------------------------------------------------------------------------
{
    std::vector<text>::iterator  file;
    bool                         hadError = false;
    // Loop over files we will process
    for (file = file_names.begin(); file != file_names.end(); file++)
        hadError |= LoadFile(*file);

    return hadError;
}


int Main::LoadContextFiles( source_names context_file_names)
// ----------------------------------------------------------------------------
//   Load all files given on the command line and compile them
// ----------------------------------------------------------------------------
{
    std::vector<text>::iterator  file;
    bool                         hadError = false;

    // clear previous context

    // load builtins
    if (!builtins.empty() )
        hadError |= LoadFile(builtins, true);

    // Loop over files we will process
    for (file = context_file_names.begin();
         file != context_file_names.end(); file++)
    {
        hadError |= LoadFile(*file, true);
    }
    return hadError;
}


void Main::EvalContextFiles(source_names context_file_names)
// ----------------------------------------------------------------------------
//   Evaluate the context files
// ----------------------------------------------------------------------------
{
    std::vector<text>::iterator  file;

    // Execute xl.builtins file first
    if (!builtins.empty())
        if (Tree *builtins_file = files[builtins].tree)
            xl_evaluate(builtins_file);

    // Execute other context files (user.xl, theme.xl)
    for (file = context_file_names.begin();
         file != context_file_names.end();
         file++)
        if (Tree *context_file = files[*file].tree)
            xl_evaluate(context_file);
}


int Main::LoadFile(text file, bool updateContext)
// ----------------------------------------------------------------------------
//   Load an individual file
// ----------------------------------------------------------------------------
{
    Tree_p tree = NULL;
    bool hadError = false;
    FILE *f;

    if ((f = fopen(file.c_str(), "r")) == NULL)
    {
        perror(std::string(argv[0]).append(": ").append(file).c_str());
        return true;
    }
    fclose(f);

    // Parse program - Local parser to delete scanner and close file
    // This ensures positions are updated even if there is a 'load'
    // being called during execution.
    if (options.readSerialized)
    {
        if (!reader)
            reader = new Deserializer(std::cin);
        tree = reader->ReadTree();
        if (!reader->IsValid())
        {
            std::cerr << "Error in input stream '" << file << "'\n";
            hadError = true;
            return hadError;
        }
    }
    else
    {
        std::string nt = "";
        std::ifstream ifs(file.c_str(), std::ifstream::in);
        Deserializer ds(ifs);
        tree = ds.ReadTree();
        if (!ds.IsValid())
        {
            // File is not in serialized format, try to parse it as XL source
            nt = "not ";
            Parser parser (file.c_str(), syntax, positions, errors);
            tree = parser.Parse();
        }
        if (options.verbose)
            std::cout << "Info: file " << file << " is "
                      << nt << "in serialized format" << '\n';
    }

    if (options.writeSerialized)
    {
        if (!writer)
            writer = new Serializer(std::cout);
        if (tree)
            tree->Do(writer);
    }

    if (!tree)
    {
        if (options.doDiff)
        {
            files[file] = SourceFile (file, NULL, NULL);
            hadError = false;
        }
        else
        {
            hadError = true;
        }
        return hadError;
    }

    Symbols *syms = Symbols::symbols;
    Symbols *savedSyms = syms;
    if (file == builtins)
    {
        syms = context;
    }
    else
    {
        syms = new Symbols(syms);
    }
    Symbols::symbols = syms;
    tree->SetSymbols(syms);

    if (options.fileLoad)
        std::cout << "Loading: " << file << "\n";

    files[file] = SourceFile (file, tree, syms);

    if (options.showGV)
    {
        SetNodeIdAction sni;
        BreadthFirstSearch bfs(sni);
        tree->Do(bfs);
        GvOutput gvout(std::cout);
        tree->Do(gvout);
    }

    if (options.showSource)
        std::cout << tree << "\n";

    if (!options.parseOnly)
    {
        if (options.optimize_level)
        {
            tree = syms->CompileAll(tree);
        }
        if (!tree)
            hadError = true;
        else
            files[file].tree = tree;
    }

    if (options.verbose)
        debugp(tree);

    // Decide if we update symbols for next run
    Symbols::symbols = updateContext ? syms : savedSyms;

    return hadError;
}


int Main::Run()
// ----------------------------------------------------------------------------
//   Run all files given on the command line
// ----------------------------------------------------------------------------
{
    text cmd, end = "";
    source_names::iterator file;
    bool hadError = false;

    // If we only parse or compile, return
    if (options.parseOnly || options.compileOnly || options.doDiff)
        return -1;

    // Loop over files we will process
    for (file = file_names.begin(); file != file_names.end(); file++)
    {
        SourceFile &sf = files[*file];
        Symbols::symbols = sf.symbols;

        // Evaluate the given tree
        Tree_p result = sf.tree;
        try
        {
            result = sf.symbols->Run(sf.tree);
        }
        catch (XL::Error &e)
        {
            e.Display();
            result = NULL;
        }
        catch (...)
        {
            std::cerr << "Got unknown exception.\n";
            result = NULL;
        }

        if (!result)
        {
            hadError = true;
        }
        else
        {
#ifdef TAO
            if (options.verbose)
                std::cout << "RESULT of " << sf.name << "\n" << result << "\n";
#else // XLR without TAO
            std::cout << result << "\n";
#endif // TAO
        }

        Symbols::symbols = Context::context;
    }

    return hadError;
}

int Main::Diff()
// ----------------------------------------------------------------------------
//   Perform a tree diff between the two loaded files
// ----------------------------------------------------------------------------
{
    source_names::iterator file;

    file = file_names.begin();
    SourceFile &sf1 = files[*file];
    file++;
    SourceFile &sf2 = files[*file];

    Tree_p t1 = sf1.tree;
    Tree_p t2 = sf2.tree;

    TreeDiff d(t1, t2);
    return d.Diff(std::cout);
}


XL_END


#ifndef TAO
int main(int argc, char **argv)
// ----------------------------------------------------------------------------
//   Parse the command line and run the compiler phases
// ----------------------------------------------------------------------------
{
#if CONFIG_USE_SBRK
    char *low_water = (char *) sbrk(0);
#endif

    using namespace XL;
    source_names noSpecificContext;
    Compiler compiler("xl_tao");
    MAIN = new Main(argc, argv, compiler);
    int rc = 0;
    if ( !(rc=MAIN->ParseOptions()) ||
         !(rc=MAIN->LoadContextFiles(noSpecificContext)))
    {
        delete MAIN;
        return rc;
    }
    rc = MAIN->LoadFiles();
    if (!rc && Options::options->doDiff)
        rc = MAIN->Diff();
    else
    if (!rc && !Options::options->parseOnly)
        rc = MAIN->Run();
    delete MAIN;

#if CONFIG_USE_SBRK
    IFTRACE(memory)
        fprintf(stderr, "Total memory usage: %ldK\n",
                long ((char *) malloc(1) - low_water) / 1024);
#endif

    return rc;
}

#endif // TAO
