// ****************************************************************************
//  tree.cpp                        (C) 1992-2003 Christophe de Dinechin (ddd)
//                                                            XL2 project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the parse tree elements
//
//
//
//
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

#include <sstream>
#include <cassert>
#include <iostream>
#include "tree.h"
#include "context.h"
#include "renderer.h"
#include "opcodes.h"
#include "options.h"
#include "sha1.h"
#include "hash.h"

XL_BEGIN

// ============================================================================
//
//    Class Tree
//
// ============================================================================

Tree::~Tree()
// ----------------------------------------------------------------------------
//   Delete the hash if we have one
// ----------------------------------------------------------------------------
{
    Info *next = NULL;
    for (Info *i = info; i; i = next)
    {
        next = i->next;
        delete i;
    }
}


Tree::operator text()
// ----------------------------------------------------------------------------
//   Conversion of a tree to text
// ----------------------------------------------------------------------------
{
    std::ostringstream out;
    out << this;
    return out.str();
}


Name::operator bool()
// ----------------------------------------------------------------------------
//    Conversion of a name to text
// ----------------------------------------------------------------------------
{
    if (this == xl_true)
        return true;
    else if (this == xl_false)
        return false;
    Ooops("Value '$1' is not a boolean value", this);
    return false;
}



// ============================================================================
// 
//   Actions on a tree
// 
// ============================================================================

Tree_p Tree::Do(Action *action)
// ----------------------------------------------------------------------------
//   Perform an action on the tree 
// ----------------------------------------------------------------------------
{
    switch(Kind())
    {
    case INTEGER:       return action->DoInteger((Integer *) this);
    case REAL:          return action->DoReal((Real *) this);
    case TEXT:          return action->DoText((Text *) this);
    case NAME:          return action->DoName((Name *) this);
    case BLOCK:         return action->DoBlock((Block *) this);
    case PREFIX:        return action->DoPrefix((Prefix *) this);
    case POSTFIX:       return action->DoPostfix((Postfix *) this);
    case INFIX:         return action->DoInfix((Infix *) this);
    default:            assert(!"Unexpected tree kind");
    }
    return NULL;
}


Tree_p Action::DoInteger(Integer_p what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree_p Action::DoReal(Real_p what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree_p Action::DoText(Text_p what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree_p Action::DoName(Name_p what)
// ----------------------------------------------------------------------------
//   Default is simply to invoke 'Do'
// ----------------------------------------------------------------------------
{
    return Do(what);
}


Tree_p Action::DoBlock(Block_p what)
// ----------------------------------------------------------------------------
//    Default is to firm perform action on block's child, then on self
// ----------------------------------------------------------------------------
{
    what->child = what->child->Do(this);
    return Do(what);
}


Tree_p Action::DoPrefix(Prefix_p what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the left, then on right
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do(what);
}


Tree_p Action::DoPostfix(Postfix_p what)
// ----------------------------------------------------------------------------
//   Default is to run the action on the right, then on the left
// ----------------------------------------------------------------------------
{
    what->right = what->right->Do(this);
    what->left = what->left->Do(this);
    return Do(what);
}


Tree_p Action::DoInfix(Infix_p what)
// ----------------------------------------------------------------------------
//   Default is to run the action on children first
// ----------------------------------------------------------------------------
{
    what->left = what->left->Do(this);
    what->right = what->right->Do(this);
    return Do(what);
}


text Block::indent   = "I+";
text Block::unindent = "I-";
text Text::textQuote = "\"";
text Text::charQuote = "'";

XL_END

#include "sha1_ostream.h"

XL_BEGIN

text sha1(Tree_p t)
// ----------------------------------------------------------------------------
//    Compute the SHA-1 for a tree and return it
// ----------------------------------------------------------------------------
{
    text result;
    if (t)
    {
        TreeHashAction<> hash;
        t->Do(hash);

        std::ostringstream os;
        os << t->Get< HashInfo<> > ();

        result = os.str();
    }
    return result;
}

XL_END


