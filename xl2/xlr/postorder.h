#ifndef POSTORDER_H
#define POSTORDER_H
// ****************************************************************************
//  postorder.h                                                    XLR project
// ****************************************************************************
//
//   File Description:
//
//     Implementation of the post-order traversal algorithm on a tree.
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
// * File       : $RCSFile$
// * Revision   : $Revision$
// * Date       : $Date$
// ****************************************************************************


#include "tree.h"

XL_BEGIN

struct PostOrderTraversal : Action
// ----------------------------------------------------------------------------
//   Execute action on a tree (whole or part), following the post-order algo
// ----------------------------------------------------------------------------
{
    PostOrderTraversal (Action &action, bool fullScan = true):
        action(action), fullScan(fullScan) {}
    Tree *DoBlock(Block *what)
    {
        Tree *ret = NULL;
        if (what->child)
            ret = Do(what->child);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }
    Tree *DoInfix(Infix *what)
    {
        Tree *  ret;
        ret = Do(what->left);
        if (!fullScan && ret)
            return ret;
        ret = Do(what->right);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }
    Tree *DoPrefix(Prefix *what)
    {
        Tree *  ret;
        ret = Do(what->left);
        if (!fullScan && ret)
            return ret;
        ret = Do(what->right);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }
    Tree *DoPostfix(Postfix *what)
    {
        Tree *  ret;
        ret = Do(what->left);
        if (!fullScan && ret)
            return ret;
        ret = Do(what->right);
        if (!fullScan && ret)
            return ret;
        return what->Do(action);
    }
    Tree *Do(Tree *what)
    {
        switch(what->Kind())
        {
        case INTEGER:
        case REAL:
        case TEXT:
        case NAME:          return what->Do(action);
        case BLOCK:         return DoBlock(what->AsBlock());
        case PREFIX:        return DoPrefix(what->AsPrefix());
        case POSTFIX:       return DoPostfix(what->AsPostfix());
        case INFIX:         return DoInfix(what->AsInfix());
        default:            assert(!"Unexpected tree kind");
        }
        return NULL;
    }

    Action & action;
    bool fullScan;
};

XL_END

#endif // POSTORDER_H
