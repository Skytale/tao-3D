#ifndef RENDERER_H
#define RENDERER_H
// ****************************************************************************
//  renderer.h                      (C) 1992-2009 Christophe de Dinechin (ddd) 
//                                                                 XL2 project 
// ****************************************************************************
// 
//   File Description:
// 
//     Rendering of XL trees
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

#include "base.h"
#include "tree.h"
#include <ostream>

XL_BEGIN

class Syntax;
typedef std::map<text,Tree_p>    formats_table;


struct Renderer
// ----------------------------------------------------------------------------
//   Render a tree to some ostream
// ----------------------------------------------------------------------------
{
    // Construction
    Renderer(std::ostream &out, text styleFile, Syntax &stx);
    Renderer(std::ostream &out, Renderer *from = renderer);

    // Selecting the style sheet file
    void                SelectStyleSheet(text styleFile);

    // Rendering proper
    void                Render (Tree_p what);
    void                RenderOne(Tree_p what);
    void                RenderText(text format);
    void                RenderFormat(Tree_p format);
    void                RenderFormat(text self, text format);
    void                RenderFormat(text self, text format, text generic);
    void                RenderFormat(text self, text f, text g1, text g2);
    Tree_p               ImplicitBlock(Tree_p t);
    bool                IsAmbiguousPrefix(Tree_p test, bool testL, bool testR);
    bool                IsSubFunctionInfix(Tree_p t);
    int                 InfixPriority(Tree_p test);

    std::ostream &      output;
    Syntax &            syntax;
    formats_table       formats;
    uint                indent;
    text                self;
    Tree_p              left;
    Tree_p              right;
    text                current_quote;
    int                 priority;
    bool                had_space;
    bool                had_punctuation;
    bool                need_separator;

    static Renderer *   renderer;
};

std::ostream& operator<< (std::ostream&out, XL::Tree_p t);

XL_END

// For use in a debugger
extern "C" void debug(XL::Tree_p);
extern "C" void debugp(XL::Tree_p);
extern "C" void debugc(XL::Tree_p);

#endif // RENDERER_H
