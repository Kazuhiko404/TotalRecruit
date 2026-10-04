// Tests for src/menu_anchor.h UnionMenuRect: the rectangle the button block is
// placed beside, from the native menu's one or two columns. With one column it is
// exactly the column the block always took; with two it holds both, so the block
// can never land on the building's column.

#include "test_support.h"
#include "menu_anchor.h"
#include "menu_slot.h"

using namespace totalrecruit;
using namespace totalrecruit::menuanchor;

namespace
{
Column Col(int l, int t, int w, int h)
{
    Column c;
    c.present = true;
    c.visible = true;
    c.rect = menuslot::Rect(l, t, w, h);
    return c;
}

bool Same(const menuslot::Rect& a, const menuslot::Rect& b)
{
    return a.left == b.left && a.top == b.top && a.width == b.width && a.height == b.height;
}

void OneColumnIsTodaysRectangle()
{
    const char* name = "menu anchor: one column gives exactly that column";
    menuslot::Rect out;
    CHECK(UnionMenuRect(Col(470, 300, 122, 100), Column(), &out));
    CHECK(Same(out, menuslot::Rect(470, 300, 122, 100)));
}

// The bug: a prisoner in a cage opens the character's and the cage's columns side
// by side, and a block anchored to the first alone could cover the second.
void TwoColumnsGiveTheRectangleHoldingBoth()
{
    const char* name = "menu anchor: two columns side by side give their union";
    menuslot::Rect out;
    CHECK(UnionMenuRect(Col(470, 300, 122, 100), Col(598, 300, 152, 75), &out));
    CHECK(Same(out, menuslot::Rect(470, 300, 280, 100)));

    // The building's column on the left and lower: still the union.
    CHECK(UnionMenuRect(Col(470, 300, 122, 100), Col(300, 310, 160, 120), &out));
    CHECK(Same(out, menuslot::Rect(300, 300, 292, 130)));
}

void AMissingHiddenOrEmptySecondColumnIsIgnored()
{
    const char* name = "menu anchor: second column null, hidden or zero-width -> first column only";
    const Column first = Col(470, 300, 122, 100);
    menuslot::Rect out;

    Column missing = Col(598, 300, 152, 75);
    missing.present = false;
    CHECK(UnionMenuRect(first, missing, &out));
    CHECK(Same(out, first.rect));

    Column hidden = Col(598, 300, 152, 75);
    hidden.visible = false;
    CHECK(UnionMenuRect(first, hidden, &out));
    CHECK(Same(out, first.rect));

    Column empty = Col(598, 300, 0, 75);
    CHECK(UnionMenuRect(first, empty, &out));
    CHECK(Same(out, first.rect));
}

// Today the first column is taken without asking whether it is visible (the show
// hook may run before the layout is), and a first column without width places no
// block: both stay so.
void TheFirstColumnIsTakenAsBefore()
{
    const char* name = "menu anchor: first column needs presence and width, not visibility";
    menuslot::Rect out;
    Column notYetShown = Col(470, 300, 122, 100);
    notYetShown.visible = false;
    CHECK(UnionMenuRect(notYetShown, Column(), &out));
    CHECK(Same(out, notYetShown.rect));

    CHECK(!UnionMenuRect(Col(470, 300, 0, 100), Col(598, 300, 152, 75), &out));
    CHECK(!UnionMenuRect(Column(), Col(598, 300, 152, 75), &out));
    CHECK(!UnionMenuRect(Col(470, 300, 122, 100), Column(), 0));
}

// The block's slot search over the union never overlaps the second column. The
// case that broke: the building's column opens to the LEFT of the character's, so
// "left of the first column" was on top of it.
void TheBlockBesideTheUnionNeverCoversTheSecondColumn()
{
    const char* name = "menu anchor: the slot resolver over the union keeps off both columns";
    const Column first = Col(470, 300, 122, 100);
    const Column second = Col(346, 300, 122, 100);
    menuslot::Rect u;
    CHECK(UnionMenuRect(first, second, &u));

    menuslot::Request request;
    request.menu = u;
    request.screenWidth = 1920;
    request.screenHeight = 1080;
    request.buttonCount = 3;
    request.buttonWidth = 160;
    request.buttonHeight = 44;
    const menuslot::Placement placement = menuslot::ResolveSlot(request);
    CHECK(placement.rects.size() == 3);
    for (size_t i = 0; i < placement.rects.size(); ++i)
    {
        CHECK(!menuslot::Intersects(placement.rects[i], first.rect));
        CHECK(!menuslot::Intersects(placement.rects[i], second.rect));
    }
}
} // namespace

void RunMenuAnchorTests()
{
    OneColumnIsTodaysRectangle();
    TwoColumnsGiveTheRectangleHoldingBoth();
    AMissingHiddenOrEmptySecondColumnIsIgnored();
    TheFirstColumnIsTakenAsBefore();
    TheBlockBesideTheUnionNeverCoversTheSecondColumn();
}
