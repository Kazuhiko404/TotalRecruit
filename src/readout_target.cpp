#include "common.h"
#include "readout_target.h"
#include "engine_reads.h"
#include "recruit_overlay.h"
#include "log.h"

#include <kenshi/CameraClass.h>

#include <vector>

namespace totalrecruit
{
namespace
{
// How far from the camera centre characters are looked for, in world units. The
// hail sweep uses 900 around the player for "close enough to hear"; what is on
// screen at the far zoom is wider than that.
const float kHoverRadius = 2500.0f;
// The pick: a character counts as under the cursor within this many pixels of their
// projected body centre.
const int kPickRadiusPx = 48;
// The card hangs this many pixels above the projected body centre, about a head
// above. Pixels rather than world units: the unit scale of the world is not
// something this mod has measured.
const int kAnchorLiftPx = 70;
// A widget bigger than this share of the screen is a HUD layer, not a window.
const int kHudAreaPercent = 80;

bool SafeSelectedObjectCharacter(Character** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL)
        {
            return false;
        }
        *out = ou->player->selectedObject.getCharacter();
        return *out != NULL;
    }
    TR_CATCH { return false; }
}

bool SafeCamera(CameraClass** out)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL || ou->player->camera == NULL)
        {
            return false;
        }
        *out = ou->player->camera;
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeCameraCentre(Ogre::Vector3* out)
{
    TR_TRY
    {
        CameraClass* camera = NULL;
        if (!SafeCamera(&camera))
        {
            return false;
        }
        *out = camera->getCenter();
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeViewSize(int* w, int* h)
{
    TR_TRY
    {
        MyGUI::RenderManager* render = MyGUI::RenderManager::getInstancePtr();
        if (render == NULL)
        {
            return false;
        }
        *w = render->getViewSize().width;
        *h = render->getViewSize().height;
        return *w > 0 && *h > 0;
    }
    TR_CATCH { return false; }
}

// The mouse, and whether it is over a window of the game (or of another mod): the
// card follows the world, so it steps aside while the player works in a window.
bool SafeMouse(int viewW, int viewH, int* x, int* y, bool* overWindow)
{
    TR_TRY
    {
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        if (input == NULL)
        {
            return false;
        }
        *x = input->getMousePosition().left;
        *y = input->getMousePosition().top;

        MyGUI::Widget* focus = input->getMouseFocusWidget();
        *overWindow = false;
        if (focus != NULL)
        {
            const int area = focus->getAbsoluteCoord().width * focus->getAbsoluteCoord().height;
            *overWindow = area * 100 < viewW * viewH * kHudAreaPercent;
        }
        return true;
    }
    TR_CATCH { return false; }
}

// World point to screen pixels through the camera, with no engine-side helper: the
// plain view-projection transform. False behind the camera or well off the screen.
bool SafeProject(Character* character, int viewW, int viewH, int* x, int* y)
{
    TR_TRY
    {
        CameraClass* cam = NULL;
        if (!SafeCamera(&cam) || cam->camera == NULL)
        {
            return false;
        }

        const Ogre::Vector3 p = character->getPositionBip01();
        const Ogre::Matrix4 vp = cam->camera->getProjectionMatrix() * cam->camera->getViewMatrix();

        const float w = vp[3][0] * p.x + vp[3][1] * p.y + vp[3][2] * p.z + vp[3][3];
        if (w <= 0.0001f)
        {
            return false;
        }
        const float nx = (vp[0][0] * p.x + vp[0][1] * p.y + vp[0][2] * p.z + vp[0][3]) / w;
        const float ny = (vp[1][0] * p.x + vp[1][1] * p.y + vp[1][2] * p.z + vp[1][3]) / w;
        if (nx < -1.2f || nx > 1.2f || ny < -1.2f || ny > 1.2f)
        {
            return false;
        }
        *x = (int)((nx * 0.5f + 0.5f) * (float)viewW);
        *y = (int)((1.0f - (ny * 0.5f + 0.5f)) * (float)viewH);
        return true;
    }
    TR_CATCH { return false; }
}

// The nearest character to the mouse within the pick radius, never one of the
// player's own.
Character* PickUnderMouse(int mouseX, int mouseY, int viewW, int viewH)
{
    Ogre::Vector3 centre;
    if (!SafeCameraCentre(&centre))
    {
        return NULL;
    }

    std::vector<Character*> nearby;
    if (!CharactersNear(centre, kHoverRadius, &nearby))
    {
        return NULL;
    }

    Character* best = NULL;
    long bestDistance = (long)kPickRadiusPx * (long)kPickRadiusPx;
    for (size_t i = 0; i < nearby.size(); ++i)
    {
        bool mine = false;
        if (SafeIsPlayerCharacter(nearby[i], &mine) && mine)
        {
            continue;
        }
        int sx = 0, sy = 0;
        if (!SafeProject(nearby[i], viewW, viewH, &sx, &sy))
        {
            continue;
        }
        const long dx = sx - mouseX;
        const long dy = sy - mouseY;
        const long d = dx * dx + dy * dy;
        if (d < bestDistance)
        {
            bestDistance = d;
            best = nearby[i];
        }
    }
    return best;
}
} // namespace

bool FindReadoutTarget(bool hoverAllowed, ReadoutTarget* out)
{
    int viewW = 0, viewH = 0;
    SafeViewSize(&viewW, &viewH);

    int mx = 0, my = 0;
    bool overWindow = false;
    const bool haveMouse = viewW > 0 && SafeMouse(viewW, viewH, &mx, &my, &overWindow);
    out->mouseX = mx;
    out->mouseY = my;

    // A right-click menu is about someone on purpose; it wins over everything.
    Character* menu = OverlayMenuTarget();
    if (menu != NULL)
    {
        out->character = menu;
        out->source = ReadoutSource_Menu;
        return true;
    }

    Character* selected = NULL;
    if (SafeSelectedObjectCharacter(&selected))
    {
        bool mine = false;
        if (!(SafeIsPlayerCharacter(selected, &mine) && mine))
        {
            out->character = selected;
            out->source = ReadoutSource_Selection;
            return true;
        }
    }

    if (hoverAllowed && haveMouse && !overWindow)
    {
        Character* hovered = PickUnderMouse(mx, my, viewW, viewH);
        if (hovered != NULL)
        {
            out->character = hovered;
            out->source = ReadoutSource_Hover;
            return true;
        }
    }
    return false;
}

bool ReadoutAnchorFor(Character* character, int* x, int* y)
{
    int viewW = 0, viewH = 0;
    if (character == NULL || !SafeViewSize(&viewW, &viewH))
    {
        return false;
    }
    if (!SafeProject(character, viewW, viewH, x, y))
    {
        return false;
    }
    *y -= kAnchorLiftPx;
    return true;
}

const char* ReadoutSourceName(ReadoutSource source)
{
    switch (source)
    {
    case ReadoutSource_Menu:      return "menu";
    case ReadoutSource_Selection: return "selection";
    case ReadoutSource_Hover:     return "hover";
    default:                      return "?";
    }
}
} // namespace totalrecruit
