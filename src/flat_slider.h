#pragma once

// A flat horizontal slider in the mockup's style, ported from TotalSparring
// (src/shared/ui/flat_slider.{h,cpp} and flat_slider_math.h, the wager slider of
// its duel popup): a thin golddim-edged sumi track with a lacquer-edged amber
// square thumb, drawn from ui_style fills rather than any Kenshi skin -- no stock
// skin has that look (Kenshi_BehaviorSlider is vertical, Kenshi_SliderHBig is a
// thicker HScroll), and the popup has to match the mockup (ui-mockup/js/popup.js
// ppSlider).
//
// Pieces, created in this order so the track paints under the thumb (ui_style.h:
// a widget draws in the order it was textured):
//   1. track edge   (kGoldDim, the full track rect, kTrackHeight tall)
//   2. track        (kSumi, inset 1px)
//   3. thumb edge   (kLacquer, kThumbSize square)
//   4. thumb        (kAmber, inset 2px)
//   5. input surface: a transparent Kenshi_TextBoxEmptySkin button over the whole
//      row, created with the other clickable widgets (after every fill), which
//      turns a press or a drag anywhere on the row into a value.
//
// The value is a slider POSITION, 0..maxValue; what a position means (an offer in
// cats, for the haggle) is the caller's business (haggle_rules.h OfferAtStep).
//
// The pure position <-> pixel math is the inline functions at the bottom of this
// header: no MyGUI there, so tests/rules_tests includes this header without the
// engine (the MyGUI types below are only forward-declared).

namespace MyGUI
{
class Widget;
class Button;
struct MouseButton;
}

namespace totalrecruit
{
namespace ui
{
class FlatSlider
{
public:
    // The mockup's thumb and track (SLIDER_ART in ui-mockup/js/haggle.js).
    static const int kThumbSize = 22;
    static const int kThumbBorder = 2;
    static const int kTrackHeight = 10;
    static const int kTrackBorder = 1;

    FlatSlider();

    // Step 1, with the other fills: the four fills, hidden until placed. Unguarded
    // like ui_style's Make* (the caller holds TR_TRY).
    void MakeFills(MyGUI::Widget* parent);
    // Step 2, after every fill: the input surface, hidden until placed. Unguarded.
    // `name` must be unique among the parent's widgets.
    void MakeSurface(MyGUI::Widget* parent, const char* name);
    bool IsBuilt() const;

    // Shows the slider in the row at (x, y, w, h), the thumb at the current value.
    // Unguarded (IntCoord temporaries): call it from a TR_TRY caller.
    void Place(int x, int y, int w, int h);
    // Hides every piece. Unguarded.
    void Hide();

    // maxInclusive <= 0 is a single position. The value is clamped into it. Neither
    // fires the change callback (as MyGUI's setScrollPosition never fires
    // eventScrollChangePosition); only a press or a drag does.
    void SetRange(int maxInclusive);
    void SetValue(int value);
    int Value() const;
    int MaxValue() const;

    // Called with the new position whenever a press or a drag moves the thumb.
    void SetOnChange(void (*onChange)(int value));

    // Unbinds the surface's events and forgets every widget pointer. The widgets
    // themselves die with their parent (the caller destroys the window).
    void Forget();

private:
    MyGUI::Widget* m_trackEdge;
    MyGUI::Widget* m_track;
    MyGUI::Widget* m_thumbEdge;
    MyGUI::Widget* m_thumb;
    MyGUI::Button* m_surface;

    int m_x;
    int m_y;
    int m_w;
    int m_h;
    int m_maxValue;
    int m_value;
    bool m_bound;

    void (*m_onChange)(int value);

    void RepaintThumb();
    void SafeRepaintThumb();
    void OnPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
    void OnDragged(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
    void OnInputAt(int absoluteX);
};

// ------------------------------------------------------------ Pure math
// TotalSparring flat_slider_math.h, unchanged. The track spans [0, trackWidth) in
// pixels from its own left edge; the thumb is thumbW wide and these give its LEFT
// edge. Values are inclusive [0, maxValue]; maxValue <= 0 clamps everything to 0.

inline int ClampSliderValue(int v, int maxValue)
{
    if (maxValue <= 0) return 0;
    if (v < 0) return 0;
    if (v > maxValue) return maxValue;
    return v;
}

// A mouse x, in pixels from the track's left edge, as a value. The thumb's centre
// (not its left edge) follows the mouse, so the travel is trackWidth - thumbW, and
// the value is rounded so both ends are reachable from a symmetric span of pixels.
inline int ValueFromX(int localX, int trackWidth, int thumbW, int maxValue)
{
    if (maxValue <= 0) return 0;
    const int travel = trackWidth - thumbW;
    if (travel <= 0) return 0;

    int left = localX - thumbW / 2;
    if (left < 0) left = 0;
    if (left > travel) left = travel;

    const long numerator = static_cast<long>(left) * maxValue * 2 + travel;
    const int value = static_cast<int>(numerator / (static_cast<long>(travel) * 2));
    return ClampSliderValue(value, maxValue);
}

// The thumb's left edge (from the track's left edge) for a value; the inverse of
// ValueFromX.
inline int ThumbLeftFor(int value, int trackWidth, int thumbW, int maxValue)
{
    const int travel = trackWidth - thumbW;
    if (travel <= 0 || maxValue <= 0) return 0;

    const int v = ClampSliderValue(value, maxValue);
    int left = static_cast<int>(static_cast<long>(v) * travel / maxValue);
    if (left < 0) left = 0;
    if (left > travel) left = travel;
    return left;
}
} // namespace ui
} // namespace totalrecruit
