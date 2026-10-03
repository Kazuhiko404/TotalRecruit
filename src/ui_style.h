#pragma once

// The mockup's look as MyGUI widgets, shared by the readout card (readout_ui.cpp)
// and the recruit popup (offer_popup.cpp).
//
// Palette = ui-mockup/css/base.css (TotalSparring ui_palette.h). The foreground
// tokens stay bright on purpose: a MyGUI tint multiplies against the skin
// underneath, and a skin mod (DarkUI, PowerModUI) turns a dark-on-dark palette
// black. Fills are the shipped gui/images/tr_white.png tinted; without it the
// stock WhiteSkin is used.
//
// Render order: a textured widget is drawn in the order it was textured, so
// every fill is created (and textured) before anything that sits on top of it,
// and nothing is re-textured later -- only recoloured, moved, shown or hidden.
//
// The Make* builders create C++ temporaries (IntCoord, UString) and are NOT
// guarded: call them from a function whose caller holds TR_TRY (C2712). The
// Safe* helpers are guarded and can be called from anywhere.

#include <string>

#pragma warning(push)
#pragma warning(disable: 4091)
#include <mygui/MyGUI_Colour.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_EditBox.h>
#pragma warning(pop)

namespace totalrecruit
{
namespace ui
{
// ------------------------------------------------------------------ Palette
extern const MyGUI::Colour kLacquer;
extern const MyGUI::Colour kBgDark;
extern const MyGUI::Colour kSumi;
extern const MyGUI::Colour kLine;
extern const MyGUI::Colour kText;
extern const MyGUI::Colour kBrass;
extern const MyGUI::Colour kGoldDim;
extern const MyGUI::Colour kMuted;
extern const MyGUI::Colour kAmber;
extern const MyGUI::Colour kDanger;
extern const MyGUI::Colour kReady;
extern const MyGUI::Colour kIndigo;
// Red text that stays readable on lacquer (#f08a7c).
extern const MyGUI::Colour kSoftDanger;

// --------------------------------------------------------------------- Fonts
extern const char* const kBodyFont;    // Kenshi_StandardFont_MediumSmall
extern const char* const kMediumFont;  // Kenshi_StandardFont_Medium
extern const char* const kSmallFont;   // Kenshi_StandardFont_Small
extern const char* const kNameFont;    // Kenshi_PaintedTextFont_Small

// Registers <mod dir>\gui\images with Ogre's GUI group once. False when the PNG
// is missing or could not be registered (fills then use WhiteSkin).
bool EnsureResources();

// A tinted 1x1 fill, hidden until placed.
MyGUI::Widget* MakeFill(MyGUI::Widget* parent, const MyGUI::Colour& colour);

// A one-line label (TextBox does not wrap), hidden until placed, never focused.
MyGUI::TextBox* MakeLabel(MyGUI::Widget* parent, const char* font, const MyGUI::Colour& colour);

// A word-wrapping, read-only label: an EditBox, because only EditBox wraps.
// Black shadow, as TotalSparring's reason line. Hidden until placed.
MyGUI::EditBox* MakeWrapText(MyGUI::Widget* parent, const char* name, const char* font,
                             const MyGUI::Colour& colour);

// How tall `text` is once wrapped at `width`, asked of the font itself.
int MeasureWrapText(MyGUI::EditBox* box, const std::string& text, int width);

// Shows `w` at the rectangle.
void Place(MyGUI::Widget* w, int x, int y, int width, int height);
void Hide(MyGUI::Widget* w);
// A 1px frame as four fills.
void PlaceFrame(MyGUI::Widget* const parts[4], int x, int y, int w, int h);

// ------------------------------------------------------------- Flat button
// A coloured plate (and an optional 1px edge under it) with a skinless
// Kenshi_TextBoxEmptySkin button carrying the caption on top (TotalSparring
// duel_popup FlatButton).
struct FlatButton
{
    MyGUI::Widget* edge;    // NULL when the button has none
    MyGUI::Widget* plate;
    MyGUI::Button* button;
};

// Step 1, with the other fills: edge then plate. Pass edge = NULL for none.
void MakeFlatButtonPlates(MyGUI::Widget* parent, const MyGUI::Colour& plate,
                          const MyGUI::Colour* edge, FlatButton* out);
// Step 2, after every fill: the clickable caption over the plates.
void MakeFlatButtonCaption(MyGUI::Widget* parent, const char* name, const char* font,
                           const MyGUI::Colour& ink, FlatButton* out);

void PlaceButton(const FlatButton& b, int x, int y, int w, int h);
void ShowButton(const FlatButton& b, bool visible);
void SetButtonCaption(const FlatButton& b, const std::string& caption);

// Guarded: safe from any frame.
void SafeSetVisible(MyGUI::Widget* widget, bool visible);
} // namespace ui
} // namespace totalrecruit
