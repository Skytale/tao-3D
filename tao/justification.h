#ifndef JUSTIFICATION_H
#define JUSTIFICATION_H
// ****************************************************************************
//  justification.h                                                Tao project
// ****************************************************************************
//
//   File Description:
//
//     Low-level handling of justification tasks
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
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "coords3d.h"
#include <vector>
#include <iostream>

TAO_BEGIN

struct Layout;

struct Justification
// ----------------------------------------------------------------------------
//   Describes how elements are supposed to be justified
// ----------------------------------------------------------------------------
//   The same structure is used for horizontal and vertical justification.
//   There are three parameters:
//   - 'amount' indicates how much of the total gap we justify.
//     0.0 means packed (no justification), 1.0 means fully justified
//   - 'partial' indicates how much of the gap we justify for a partially
//     filled line. 0.0 means packed, 1.0 means fully justified.
//   - 'center' indicates how the elements should be centered on page.
//     0.0 means on the left or top, 1.0 on the right or bottm, 0.5 on center
//   - 'spread' indicates how much of the justification is between elements.
//     0.0 means all justification on word boundaries, 1.0 all between letters.
//     Vertically, it's lines and paragraphs.
//   - 'spacing' indicates extra amount of space to add around elements
{
    Justification(float amount = 1.0,
                  float partial = 0.0,
                  float center = 0.0,
                  float spread = 0.0,
                  float spacing = 1.0,
                  float before = 0.0,
                  float after = 0.0)
        : amount(amount), partial(partial), centering(center), spread(spread),
          spacing(spacing), before(before), after(after),
          perSolid(0.0), perBreak(0.0) {}
    float       amount;
    float       partial;
    float       centering;
    float       spread;
    float       spacing;
    float       before;
    float       after;
    float       perSolid;
    float       perBreak;
};


template <class Item>
struct Justifier
// ----------------------------------------------------------------------------
//    Object used to layout lines (vertically) or glyphs (horizontally)
// ----------------------------------------------------------------------------
//    An important invariant for memory management reasons is that items are
//    either in items or in places but not both. Furthermore, if a drawing
//    is broken up (e.g. by LineBreak), both elements are tracked in either
//    items or places.
{
    typedef std::vector<Item>           Items;
    typedef typename Items::iterator    ItemsIterator;

public:
    Justifier(): items(), places() {}
    Justifier(const Justifier &): items(), places() {}
    ~Justifier() { Clear(); }

    // Position items in the layout
    bool        Adjust(coord start, coord end, Justification &j, Layout *l);

    // Build and clear the layout
    void        Add(Item item);
    void        Add(ItemsIterator first, ItemsIterator last);
    void        Clear();

    // Properties of the items in the layout
    Item        Break(Item item, uint &size,
                      bool &hadBreak, bool &hadSep, bool &done);
    scale       Size(Item item, Layout *);
    scale       SpaceSize(Item item, Layout *);
    coord       ItemOffset(Item item, Layout *);

    void        Dump(text msg, Layout *);

    // Structure recording an item after we placed it
    struct Place
    {
        Place(Item item, scale size = 0, coord pos = 0, bool solid=true)
            : size(size), position(pos), item(item), solid(solid) {}
        scale   size;
        coord   position;
        Item    item;
        bool    solid;
    };
    typedef std::vector<Place>          Places;
    typedef typename Places::iterator   PlacesIterator;

public:
    Items         items;        // Items remaining to be placed (e.g. broken)
    Places        places;       // Items placed on the layout
};

TAO_END

#endif // JUSTIFICATION_H
