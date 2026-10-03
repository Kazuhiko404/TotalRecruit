#include "common.h"
#include "readout_target.h"
#include "engine_reads.h"
#include "log.h"

#include <kenshi/CameraClass.h>

namespace totalrecruit
{
namespace
{
// The card hangs this many pixels above the projected body centre, about a head
// above. Pixels rather than world units: the unit scale of the world is not
// something this mod has measured.
const int kAnchorLiftPx = 70;

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
} // namespace

Character* SelectedNpc()
{
    Character* selected = NULL;
    if (!SafeSelectedObjectCharacter(&selected))
    {
        return NULL;
    }
    bool mine = false;
    if (!SafeIsPlayerCharacter(selected, &mine) || mine)
    {
        return NULL;
    }
    return selected;
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
} // namespace totalrecruit
