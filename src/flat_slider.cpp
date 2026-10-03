#include "common.h"
#include "ui_style.h"
#include "flat_slider.h"

#pragma warning(push)
#pragma warning(disable: 4091)
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_MouseButton.h>
#pragma warning(pop)

// TotalSparring's FlatSlider (src/shared/ui/flat_slider.cpp), on TotalRecruit's
// fills (ui_style MakeFill, tinted tr_white.png) and TR_TRY. Same art, same input
// handling: the surface takes a left press or drag anywhere on the row and the
// thumb's centre jumps under the mouse.

namespace totalrecruit
{
namespace ui
{
namespace
{
const char* const kSurfaceSkin = "Kenshi_TextBoxEmptySkin";
} // namespace

FlatSlider::FlatSlider()
    : m_trackEdge(NULL), m_track(NULL), m_thumbEdge(NULL), m_thumb(NULL), m_surface(NULL),
      m_x(0), m_y(0), m_w(0), m_h(0), m_maxValue(0), m_value(0), m_bound(false),
      m_onChange(NULL)
{
}

void FlatSlider::MakeFills(MyGUI::Widget* parent)
{
    m_trackEdge = MakeFill(parent, kGoldDim);
    m_track = MakeFill(parent, kSumi);
    m_thumbEdge = MakeFill(parent, kLacquer);
    m_thumb = MakeFill(parent, kAmber);
}

void FlatSlider::MakeSurface(MyGUI::Widget* parent, const char* name)
{
    m_surface = parent->createWidget<MyGUI::Button>(
        kSurfaceSkin, MyGUI::IntCoord(0, 0, 10, 10), MyGUI::Align::Default, name);
    if (m_surface == NULL)
    {
        return;
    }
    m_surface->setCaption("");
    m_surface->setNeedMouseFocus(true);
    m_surface->setNeedKeyFocus(false);
    m_surface->setVisible(false);
    m_surface->eventMouseButtonPressed += MyGUI::newDelegate(this, &FlatSlider::OnPressed);
    m_surface->eventMouseDrag += MyGUI::newDelegate(this, &FlatSlider::OnDragged);
    m_bound = true;
}

bool FlatSlider::IsBuilt() const
{
    return m_trackEdge != NULL && m_track != NULL && m_thumbEdge != NULL && m_thumb != NULL &&
           m_surface != NULL;
}

void FlatSlider::Place(int x, int y, int w, int h)
{
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    const int trackY = y + (h - kTrackHeight) / 2;
    ui::Place(m_trackEdge, x, trackY, w, kTrackHeight);
    ui::Place(m_track, x + kTrackBorder, trackY + kTrackBorder,
              w - 2 * kTrackBorder, kTrackHeight - 2 * kTrackBorder);
    ui::Place(m_surface, x, y, w, h);
    RepaintThumb();
}

void FlatSlider::Hide()
{
    ui::Hide(m_trackEdge);
    ui::Hide(m_track);
    ui::Hide(m_thumbEdge);
    ui::Hide(m_thumb);
    ui::Hide(m_surface);
}

void FlatSlider::SetRange(int maxInclusive)
{
    m_maxValue = maxInclusive > 0 ? maxInclusive : 0;
    m_value = ClampSliderValue(m_value, m_maxValue);
}

void FlatSlider::SetValue(int value)
{
    m_value = ClampSliderValue(value, m_maxValue);
}

int FlatSlider::Value() const
{
    return m_value;
}

int FlatSlider::MaxValue() const
{
    return m_maxValue;
}

void FlatSlider::SetOnChange(void (*onChange)(int value))
{
    m_onChange = onChange;
}

void FlatSlider::Forget()
{
    if (m_bound && m_surface != NULL)
    {
        m_surface->eventMouseButtonPressed -= MyGUI::newDelegate(this, &FlatSlider::OnPressed);
        m_surface->eventMouseDrag -= MyGUI::newDelegate(this, &FlatSlider::OnDragged);
    }
    m_trackEdge = NULL;
    m_track = NULL;
    m_thumbEdge = NULL;
    m_thumb = NULL;
    m_surface = NULL;
    m_x = m_y = m_w = m_h = 0;
    m_maxValue = 0;
    m_value = 0;
    m_bound = false;
    m_onChange = NULL;
}

// Unguarded (IntCoord temporaries): Place's caller holds TR_TRY; the input path
// goes through SafeRepaintThumb.
void FlatSlider::RepaintThumb()
{
    const int left = ThumbLeftFor(m_value, m_w, kThumbSize, m_maxValue);
    const int thumbX = m_x + left;
    const int thumbY = m_y + (m_h - kThumbSize) / 2;
    ui::Place(m_thumbEdge, thumbX, thumbY, kThumbSize, kThumbSize);
    ui::Place(m_thumb, thumbX + kThumbBorder, thumbY + kThumbBorder,
              kThumbSize - 2 * kThumbBorder, kThumbSize - 2 * kThumbBorder);
}

void FlatSlider::SafeRepaintThumb()
{
    TR_TRY { RepaintThumb(); }
    TR_CATCH { }
}

// The surface's absolute left, read in a POD-only frame.
static bool SafeAbsoluteLeft(MyGUI::Widget* widget, int* out)
{
    TR_TRY
    {
        *out = widget->getAbsoluteLeft();
        return true;
    }
    TR_CATCH { return false; }
}

void FlatSlider::OnInputAt(int absoluteX)
{
    if (m_maxValue <= 0 || m_surface == NULL)
    {
        return;
    }
    int surfaceLeft = 0;
    if (!SafeAbsoluteLeft(m_surface, &surfaceLeft))
    {
        return;
    }
    const int value = ValueFromX(absoluteX - surfaceLeft, m_w, kThumbSize, m_maxValue);
    if (value == m_value)
    {
        return;
    }
    m_value = value;
    SafeRepaintThumb();
    if (m_onChange != NULL)
    {
        m_onChange(m_value);
    }
}

void FlatSlider::OnPressed(MyGUI::Widget* /*sender*/, int left, int /*top*/, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }
    OnInputAt(left);
}

void FlatSlider::OnDragged(MyGUI::Widget* /*sender*/, int left, int /*top*/, MyGUI::MouseButton id)
{
    if (id != MyGUI::MouseButton::Left)
    {
        return;
    }
    OnInputAt(left);
}
} // namespace ui
} // namespace totalrecruit
