#ifndef CONTEXT_H
#define CONTEXT_H
// ****************************************************************************
//  context.h                       (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                                 XL2 project
// ****************************************************************************
//
//   File Description:
//
//     The execution environment for XL
//
//     This defines both the compile-time environment (Context), where we
//     keep symbolic information, e.g. how to rewrite trees, and the
//     runtime environment (Runtime), which we use while executing trees
//
//
//
//
// ****************************************************************************
// This program is released under the GNU General Public License.
// See http://www.gnu.org/copyleft/gpl.html for details
// ****************************************************************************
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************
/*
  COMPILATION STRATEGY:
  
  The version of XL implemented here is a very simple language based
  on tree rewrites, designed to serve as a dynamic document description
  language (DDD), as well as a tool to implement the "larger" XL in a more
  dynamic way. Both usage models imply that the language is compiled on the
  fly, not statically.

  Also, because the language is designed to manipulate program trees, which
  server as the primary data structure, this implies that the program trees
  will exist at run-time as well. There needs to be a garbage collection
  phase. The chosen garbage collection technique is mark and sweep, so that we
  can deal with cyclic data structures. This allows us to replace a name with
  what it references, even in cases such as X->1,X, an infinite
  comma-separated tree of 1s.

  The chosen approach is to add an evaluation function pointer to each tree,
  the field being called 'code' in struct Tree. This function pointer is
  filled in by the compiler as a result of compilation. This means that it is
  possible to render the source tree as well as to execute optimized code
  when we evaluate it.

  In some cases, the compiler may want to choose a more efficient data
  structure layout for the tree being represented. For example, we may decide
  to use an memory-contiguous array to represent [ 1, 2, 3, 4 ], even if the
  native tree representation not memory contiguous nor easy to access. The
  above technique makes this possible too. For such cases, we generate the
  data, and prefix it with a very small code thunk that simply returns the
  result of a call to xl_data(). xl_data() can get the address of the data
  from its return address. See xl_data() for details.

  
  PREDEFINED FORMS:

  XL is really built on a very small number of predefined forms recognized by
  the compilation phase.

    "A->B" defines a rewrite rule, rewriting A as B. The form A can be
           "guarded" by a "when" clause, e.g. "N! when N>0 -> N * (N-1)!"
    "A:B" is a type annotation, indicating that the type of A is B
    "(A)" is the same as A, allowing to override default precedence,
          and the same holds for A in an indentation (indent block)
    "A;B" is a sequence, evaluating A, then B, the value being B.
          The newline infix operator plays the same role.
    "data A" declares A as a form that cannot be reduced further

  The XL type system is itself based on tree shapes. For example, "integer" is
  a type that covers all Integer trees. "integer+integer" is a type that
  covers all Infix trees with "+" as the name and an Integer node in the left
  and right nodes. "integer+0" covers "+" infix trees with an Integer on the
  left and the exact value 0 on the right.

  The compiler is allowed to special-case any such form. For example, if it
  determines that the type of a tree is "integer", it may represent it using a
  machine integer. It must however convert to/from tree when connecting to
  code that deals with less specialized trees.

  The compiler is also primed with a number of declarations such as:
     x:integer+y:integer -> native addint:integer
  This tells the compiler to lookup a native compilation function called
  "xl_addint" and invoke that.

  
  RUNTIME EXECUTION:

  The evaluation functions pointed to by 'code' in the Tree structure are
  designed to be invoked by normal C code using the normal C stack. This
  facilitate the interaction with other code.

  At top-level, the compiler generates only functions with the same
  prototype as eval_fn, i.e. Tree_p (Tree_p). The code is being
  generated on invokation of a form, and helps rewriting it, although
  attempts are made to leverage existing rewrites. This is implemented
  in Context::CompileAll.

  Compiling such top-level forms invokes a number of rewrites. A
  specific rewrite can invoke multiple candidates. For example,
  consider the following factorial declaration:
     0! -> 1
     N! when N>0 -> N * (N-1)!

  In that case, code invoking N! will generate a test to check if N is 0, and
  if so invoke the first rule, otherwise invoke the second rule. The same
  principle applies when there are guarded rules, i.e. the "when N>0" clause
  will cause a test N>0 to be added guarding the second rule.

  If all these invokations fail, then the input tree is in "reduced form",
  i.e. it cannot be reduced further. If it is a non-leaf tree, then an attempt
  is made to evaluate its children.

  If no evaluation can be found for a given form that doesn't match a 'data'
  clause, the compiler will emit a diagnostic. This is not a fatal condition,
  however, and code will be generated that simply leaves the tree as is when
  that happens.
 */

#include <map>
#include <set>
#include <vector>
#include "base.h"
#include "tree.h"

XL_BEGIN

// ============================================================================
// 
//    Forward type declarations
// 
// ============================================================================

struct Tree;                                    // Abstract syntax tree
struct Name;                                    // Name node, e.g. ABC or +
struct Action;                                  // Action on trees
struct TreeRoot;                                // Prevent GC from killing tree
struct Context;                                 // Compile-time context
struct Rewrite;                                 // Tree rewrite data
struct Runtime;                                 // Runtime context
struct Errors;                                  // Error handlers
struct Compiler;                                // JIT compiler
struct CompiledUnit;                            // Compilation unit
struct GCAction;                                // Garbage collection action

typedef std::map<text, Tree_p>     symbol_table; // Symbol table in context
typedef std::set<Tree_p>           active_set;   // Not to be garbage collected
typedef std::set<TreeRoot *>       root_set;     // Set of tree roots
typedef std::set<Symbols *>        symbols_set;  // Set of symbol tables
typedef std::vector<Symbols *>     symbols_list; // List of symbols table
typedef std::map<ulong, Rewrite*>  rewrite_table;// Hashing of rewrites
typedef symbol_table::iterator     symbol_iter;  // Iterator over sym table
typedef std::map<Tree_p, Symbols*> capture_table;// Symbol capture table
typedef std::map<Tree_p, Tree_p>   value_table;  // Used for value caching
typedef value_table::iterator      value_iter;   // Used to iterate over values
typedef Tree_p (*typecheck_fn) (Tree_p src, Tree_p value);



// ============================================================================
// 
//    Compile-time symbols and rewrites management
// 
// ============================================================================

struct Symbols
// ----------------------------------------------------------------------------
//   Holds the symbols in a given context
// ----------------------------------------------------------------------------
{
    Symbols(Symbols *s);
    ~Symbols();

    // Symbols properties
    Symbols *           Parent()                { return parent; }
    ulong               Depth();
    void                Import (Symbols *other) { imported.insert(other); }

    // Symbol management
    Tree_p               Named (text name, bool deep = true);
    Rewrite *           Rewrites()              { return rewrites; }

    // Entering symbols in the symbol table
    void                EnterName (text name, Tree_p value);
    Rewrite *           EnterRewrite(Rewrite *r);
    Rewrite *           EnterRewrite(Tree_p from, Tree_p to);
    Name_p               Allocate(Name_p varName);

    // Clearing symbol tables
    void                Clear();

    // Compiling and evaluating a tree in scope defined by these symbols
    Tree_p               Compile(Tree_p s, CompiledUnit &,
                                bool nullIfBad = false,
                                bool keepOtherConstants = false);
    Tree_p               CompileAll(Tree_p s,
                                   bool nullIfBad = false,
                                   bool keepOtherConstants = false);
    Tree_p               CompileCall(text callee, TreeList &args,
                                    bool nullIfBad=false, bool cached = true);
    Infix_p              CompileTypeTest(Tree_p type);
    Tree_p               Run(Tree_p t);

    // Error handling
    Tree_p               Error (text message,
                               Tree_p a1=NULL, Tree_p a2=NULL, Tree_p a3=NULL);

    // Garbage collection
    bool                Mark(GCAction &gc);

public:
    Symbols *           parent;
    symbol_table        names;
    Rewrite *           rewrites;
    symbol_table        calls;
    value_table         type_tests;
    symbols_set         imported;
    Tree_p              error_handler;
    bool                has_rewrites_for_constants;

    static Symbols *    symbols;
};


struct Context : Symbols
// ----------------------------------------------------------------------------
//   The compile-time context in which we are currently evaluating
// ----------------------------------------------------------------------------
{
    // Constructors and destructors
    Context(Errors &err, Compiler *comp):
        Symbols(NULL),
        errors(err),                            // Global error list
        compiler(comp),                         // Tree compilation
        active(), active_symbols(), roots(),    // Garbage collection
        gc_threshold(200) {}                    // When do we collect?
    ~Context();

    // Garbage collection
    void                Mark(Tree_p t)           { active.insert(t); }
    void                CollectGarbage();

    // Helpers for compilation of trees
    Tree_p *             AddGlobal(Tree_p value);

public:
    static ulong        gc_increment;
    static ulong        gc_growth_percent;
    static Context *    context;

    Errors &            errors;
    Compiler *          compiler;
    active_set          active;
    symbols_set         active_symbols;
    root_set            roots;
    ulong               gc_threshold;
};


struct Rewrite
// ----------------------------------------------------------------------------
//   Information about a rewrite, e.g fact N -> N * fact(N-1)
// ----------------------------------------------------------------------------
//   Note that a rewrite with 'to' = NULL is used for 'data' statements
{
    Rewrite (Symbols *s, Tree_p f, Tree_p t):
        symbols(s), from(f), to(t), hash(), parameters() {}
    ~Rewrite();

    Rewrite *           Add (Rewrite *rewrite);
    Tree_p              Do(Action &a);
    Tree_p              Compile(void);

public:
    Symbols *           symbols;
    Tree_p              from;
    Tree_p              to;
    rewrite_table       hash;
    TreeList            parameters;
};



// ============================================================================
// 
//    Symbol information associated with a tree
// 
// ============================================================================

struct SymbolsInfo : Info
// ----------------------------------------------------------------------------
//    Record the symbol associated with a tree
// ----------------------------------------------------------------------------
{
    SymbolsInfo(Symbols *syms) : symbols(syms) {}
    typedef Symbols *   data_t;
    operator            data_t()  { return symbols; }
    SymbolsInfo *       Copy();
    Symbols *           symbols;
};



// ============================================================================
// 
//    Actions used in interpretation
// 
// ============================================================================

struct InterpretedArgumentMatch : Action
// ----------------------------------------------------------------------------
//   Check if a tree matches the form of the left of a rewrite
// ----------------------------------------------------------------------------
{
    InterpretedArgumentMatch (Tree_p t,
                              Symbols *s, Symbols *l, Symbols *r) :
        symbols(s), locals(l), rewrite(r),
        test(t), defined(NULL) {}

    // Action callbacks
    virtual Tree_p Do(Tree_p what);
    virtual Tree_p DoInteger(Integer_p what);
    virtual Tree_p DoReal(Real_p what);
    virtual Tree_p DoText(Text_p what);
    virtual Tree_p DoName(Name_p what);
    virtual Tree_p DoPrefix(Prefix_p what);
    virtual Tree_p DoPostfix(Postfix_p what);
    virtual Tree_p DoInfix(Infix_p what);
    virtual Tree_p DoBlock(Block_p what);

public:
    Symbols *     symbols;      // Context in which we evaluate values
    Symbols *     locals;       // Symbols where we declare arguments
    Symbols *     rewrite;      // Symbols in which the rewrite was declared
    Tree_p         test;         // Tree we test
    Tree_p         defined;      // Tree beind defined, e.g. 'sin' in 'sin X'
};



// ============================================================================
// 
//    Compilation actions
// 
// ============================================================================

struct DeclarationAction : Action
// ----------------------------------------------------------------------------
//   Record data and rewrite declarations in the input tree
// ----------------------------------------------------------------------------
{
    DeclarationAction (Symbols *c): symbols(c) {}

    virtual Tree_p Do(Tree_p what);
    virtual Tree_p DoInteger(Integer_p what);
    virtual Tree_p DoReal(Real_p what);
    virtual Tree_p DoText(Text_p what);
    virtual Tree_p DoName(Name_p what);
    virtual Tree_p DoPrefix(Prefix_p what);
    virtual Tree_p DoPostfix(Postfix_p what);
    virtual Tree_p DoInfix(Infix_p what);
    virtual Tree_p DoBlock(Block_p what);

    void        EnterRewrite(Tree_p defined, Tree_p definition);

    Symbols *symbols;
};


struct CompileAction : Action
// ----------------------------------------------------------------------------
//   Compute the input tree in the given compiled unit
// ----------------------------------------------------------------------------
{
    CompileAction (Symbols *s, CompiledUnit &, bool nullIfBad, bool keepAlt);

    virtual Tree_p Do(Tree_p what);
    virtual Tree_p DoInteger(Integer_p what);
    virtual Tree_p DoReal(Real_p what);
    virtual Tree_p DoText(Text_p what);
    virtual Tree_p DoName(Name_p what);
    virtual Tree_p DoPrefix(Prefix_p what);
    virtual Tree_p DoPostfix(Postfix_p what);
    virtual Tree_p DoInfix(Infix_p what);
    virtual Tree_p DoBlock(Block_p what);

    // Build code selecting among rewrites in current context
    Tree_p         Rewrites(Tree_p what);

    Symbols *     symbols;
    CompiledUnit &unit;
    bool          nullIfBad;
    bool          keepAlternatives;
};


struct ParameterMatch : Action
// ----------------------------------------------------------------------------
//   Collect parameters on the left of a rewrite
// ----------------------------------------------------------------------------
{
    ParameterMatch (Symbols *s)
        : symbols(s), defined(NULL) {}

    virtual Tree_p Do(Tree_p what);
    virtual Tree_p DoInteger(Integer_p what);
    virtual Tree_p DoReal(Real_p what);
    virtual Tree_p DoText(Text_p what);
    virtual Tree_p DoName(Name_p what);
    virtual Tree_p DoPrefix(Prefix_p what);
    virtual Tree_p DoPostfix(Postfix_p what);
    virtual Tree_p DoInfix(Infix_p what);
    virtual Tree_p DoBlock(Block_p what);

    Symbols * symbols;          // Symbols in which we test
    Tree_p    defined;          // Tree beind defined, e.g. 'sin' in 'sin X'
    TreeList  order;            // Record order of parameters
};


struct ArgumentMatch : Action
// ----------------------------------------------------------------------------
//   Check if a tree matches the form of the left of a rewrite
// ----------------------------------------------------------------------------
{
    ArgumentMatch (Tree_p t,
                   Symbols *s, Symbols *l, Symbols *r,
                   CompileAction *comp):
        symbols(s), locals(l), rewrite(r),
        test(t), defined(NULL), compile(comp), unit(comp->unit) {}

    // Action callbacks
    virtual Tree_p Do(Tree_p what);
    virtual Tree_p DoInteger(Integer_p what);
    virtual Tree_p DoReal(Real_p what);
    virtual Tree_p DoText(Text_p what);
    virtual Tree_p DoName(Name_p what);
    virtual Tree_p DoPrefix(Prefix_p what);
    virtual Tree_p DoPostfix(Postfix_p what);
    virtual Tree_p DoInfix(Infix_p what);
    virtual Tree_p DoBlock(Block_p what);

    // Compile a tree
    Tree_p         Compile(Tree_p source);
    Tree_p         CompileValue(Tree_p source);
    Tree_p         CompileClosure(Tree_p source);

public:
    Symbols *      symbols;     // Context in which we evaluate values
    Symbols *      locals;      // Symbols where we declare arguments
    Symbols *      rewrite;     // Symbols in which the rewrite was declared
    Tree_p          test;        // Tree we test
    Tree_p          defined;     // Tree beind defined, e.g. 'sin' in 'sin X'
    CompileAction *compile;     // Action in which we are compiling
    CompiledUnit  &unit;        // JIT compiler compilation unit
};


struct EnvironmentScan : Action
// ----------------------------------------------------------------------------
//   Collect variables in the tree that are imported from environment
// ----------------------------------------------------------------------------
{
    EnvironmentScan (Symbols *s): symbols(s) {}

    virtual Tree_p Do(Tree_p what);
    virtual Tree_p DoInteger(Integer_p what);
    virtual Tree_p DoReal(Real_p what);
    virtual Tree_p DoText(Text_p what);
    virtual Tree_p DoName(Name_p what);
    virtual Tree_p DoPrefix(Prefix_p what);
    virtual Tree_p DoPostfix(Postfix_p what);
    virtual Tree_p DoInfix(Infix_p what);
    virtual Tree_p DoBlock(Block_p what);

public:
    Symbols *           symbols;        // Symbols in which we test
    capture_table       captured;       // Captured symbols
};


struct BuildChildren : Action
// ----------------------------------------------------------------------------
//   Build a clone of a tree, evaluating its children
// ----------------------------------------------------------------------------
{
    BuildChildren(CompileAction *comp);
    ~BuildChildren();

    virtual Tree_p Do(Tree_p what)                { return what; }
    virtual Tree_p DoInteger(Integer_p what)      { return what; }
    virtual Tree_p DoReal(Real_p what)            { return what; }
    virtual Tree_p DoText(Text_p what)            { return what; }
    virtual Tree_p DoName(Name_p what)            { return what; }
    virtual Tree_p DoPrefix(Prefix_p what);
    virtual Tree_p DoPostfix(Postfix_p what);
    virtual Tree_p DoInfix(Infix_p what);
    virtual Tree_p DoBlock(Block_p what);
 
public:
    CompileAction *compile;             // Compilation in progress
    CompiledUnit & unit;                // JIT compiler compilation unit
    bool           saveNullIfBad;       // Unit original "nib" settings
};



// ============================================================================
// 
//   Garbage collection of trees - Mark trees that are alive from root
// 
// ============================================================================

struct GCAction : Action
// ----------------------------------------------------------------------------
//   Mark trees for garbage collection and compute active set
// ----------------------------------------------------------------------------
{
    GCAction (): alive(), alive_symbols() {}
    ~GCAction () {}

    bool Mark(Tree_p what)
    {
        typedef std::pair<active_set::iterator, bool> inserted;
        inserted ins = alive.insert(what);
        if (ins.second)
        {
            if (Symbols *syms = what->Get<SymbolsInfo> ())
                syms->Mark(*this);
            if (what->source && what->source != what)
                what->source->Do(this);
        }
        return ins.second;
    }
    Tree_p Do(Tree_p what)
    {
        Mark(what);
        return what;
    }
    Tree_p DoBlock(Block_p what)
    {
        if (Mark(what))
            Action::DoBlock(what);              // Do child
        return what;
    }
    Tree_p DoInfix(Infix_p what)
    {
        if (Mark(what))
            Action::DoInfix(what);              // Do children
        return what;
    }
    Tree_p DoPrefix(Prefix_p what)
    {
        if (Mark(what))
            Action::DoPrefix(what);             // Do children
        return what;
    }
    Tree_p DoPostfix(Postfix_p what)
    {
        if (Mark(what))
            Action::DoPostfix(what);            // Do children
        return what;
    }
    active_set  alive;
    symbols_set alive_symbols;
};


// ============================================================================
// 
//   LocalSave helper class
// 
// ============================================================================

template <class T>
struct LocalSave
// ----------------------------------------------------------------------------
//    Save a variable locally
// ----------------------------------------------------------------------------
{
    LocalSave (T &source, T value): reference(source), saved(source)
    {
        reference = value;
    }
    LocalSave(const LocalSave &o): reference(o.reference), saved(o.saved) {}
    LocalSave (T &source): reference(source), saved(source)
    {
    }
    ~LocalSave()
    {
        reference = saved;
    }
    operator T()        { return saved; }

    T&  reference;
    T   saved;
};



// ============================================================================
// 
//    Global error handler
// 
// ============================================================================

inline Tree_p Ooops (text msg, Tree_p a1=NULL, Tree_p a2=NULL, Tree_p a3=NULL)
// ----------------------------------------------------------------------------
//   Error using the global context
// ----------------------------------------------------------------------------
{
    Symbols *syms = Symbols::symbols;
    if (!syms)
        syms = Context::context;
    return syms->Error(msg, a1, a2, a3);
}



// ============================================================================
// 
//   Inline functions
// 
// ============================================================================

inline Symbols::Symbols(Symbols *s)
// ----------------------------------------------------------------------------
//   Create a "child" symbol table
// ----------------------------------------------------------------------------
    : parent(s), rewrites(NULL), error_handler(NULL),
      has_rewrites_for_constants(false)
{}


inline Symbols::~Symbols()
// ----------------------------------------------------------------------------
//   Delete all included rewrites if necessary and unlink from context
// ----------------------------------------------------------------------------
{
    if (rewrites)
        delete rewrites;
 }


inline ulong Symbols::Depth()
// ----------------------------------------------------------------------------
//    Return the depth for the current symbol table
// ----------------------------------------------------------------------------
{
    ulong depth = 0;
    for (Symbols *s = this; s; s = s->parent)
        depth++;
    return depth;
}

XL_END

extern "C"
{
    void debugs(XL::Symbols *s);
    void debugsc(XL::Symbols *s);
}

#endif // CONTEXT_H
