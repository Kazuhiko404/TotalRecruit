#include "common.h"
#include "readout_target.h"
#include "engine_reads.h"
#include "log.h"

#include <kenshi/CameraClass.h>
#include <ogre/OgreCamera.h>
#include <ogre/OgreSceneManager.h>

namespace totalrecruit
{
namespace
{
// How far over the character's position (their feet) the card hangs, in world
// units. Kenshi's units are about 10 cm (TotalSparring challenge/world_scale.h:
// shoulder to shoulder reads 7.5), so 20 is just over a standing head. World units
// rather than pixels: the gap then shrinks and grows with the zoom like the body.
const float kHeadHeightUnits = 20.0f;
// A few pixels more between the head and the card's arrow gap.
const int kAnchorLiftPx = 6;

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

void CopyMatrix(const Ogre::Matrix4& from, Mat4* to)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            to->m[r][c] = from[r][c];
        }
    }
}

// The player camera's matrices and the scene's relative origin, the three things
// the engine's own world-to-screen reads. getViewMatrix(true): the camera's own
// frustum, never a culling stand-in.
bool SafeCameraState(Mat4* view, Mat4* proj, Vec3* origin)
{
    TR_TRY
    {
        if (ou == NULL || ou->player == NULL || ou->player->camera == NULL)
        {
            return false;
        }
        Ogre::Camera* camera = ou->player->camera->camera;
        if (camera == NULL)
        {
            return false;
        }
        Ogre::SceneManager* scene = camera->getSceneManager();
        if (scene == NULL)
        {
            return false;
        }
        const Ogre::Vector3 o = scene->getRelativeOrigin();
        origin->x = o.x;
        origin->y = o.y;
        origin->z = o.z;
        CopyMatrix(camera->getViewMatrix(true), view);
        CopyMatrix(camera->getProjectionMatrix(), proj);
        return true;
    }
    TR_CATCH { return false; }
}

// Game space, the same space the AI and the physics use. `lift` is added to the
// height: 0 for the feet, kHeadHeightUnits for the point over the head.
bool SafeLiftedPoint(Character* character, float lift, Vec3* out)
{
    TR_TRY
    {
        const Ogre::Vector3 p = character->getPosition();
        out->x = p.x;
        out->y = p.y + lift;
        out->z = p.z;
        return true;
    }
    TR_CATCH { return false; }
}

bool SafeHeadPoint(Character* character, Vec3* out)
{
    return SafeLiftedPoint(character, kHeadHeightUnits, out);
}

// A projection whose pixel can be used: on the screen, or just off it.
bool PixelUsable(ProjectionResult result)
{
    return result == Projection_Ok || result == Projection_OffScreen;
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

AnchorReading ReadoutAnchorFor(Character* character)
{
    AnchorReading a;
    Mat4 view;
    Mat4 proj;
    if (character == NULL || !SafeViewSize(&a.viewW, &a.viewH) ||
        !SafeCameraState(&view, &proj, &a.origin))
    {
        a.result = Anchor_NoCamera;
        return a;
    }
    if (!SafeHeadPoint(character, &a.head))
    {
        a.result = Anchor_NoPosition;
        return a;
    }

    switch (ProjectToScreen(view, proj, a.head, a.origin, a.viewW, a.viewH, &a.x, &a.y))
    {
    case Projection_Ok:
        a.result = Anchor_Ok;
        a.y -= kAnchorLiftPx;
        break;
    case Projection_Behind:
        a.result = Anchor_Behind;
        break;
    case Projection_OffScreen:
        a.result = Anchor_OffScreen;
        break;
    case Projection_NoView:
    default:
        a.result = Anchor_NoCamera;
        break;
    }
    return a;
}

bool ReadoutBodySpanFor(Character* character, BodySpan* out)
{
    int viewW = 0;
    int viewH = 0;
    Mat4 view;
    Mat4 proj;
    Vec3 origin;
    Vec3 feet;
    Vec3 head;
    if (character == NULL || !SafeViewSize(&viewW, &viewH) ||
        !SafeCameraState(&view, &proj, &origin) ||
        !SafeLiftedPoint(character, 0.0f, &feet) || !SafeHeadPoint(character, &head))
    {
        return false;
    }
    return PixelUsable(ProjectToScreen(view, proj, feet, origin, viewW, viewH, &out->footX,
                                       &out->footY)) &&
           PixelUsable(ProjectToScreen(view, proj, head, origin, viewW, viewH, &out->headX,
                                       &out->headY));
}

const char* AnchorResultWord(AnchorResult result)
{
    switch (result)
    {
    case Anchor_Ok:         return "ok";
    case Anchor_NoPosition: return "no position";
    case Anchor_Behind:     return "behind the camera";
    case Anchor_OffScreen:  return "off the screen";
    case Anchor_NoCamera:
    default:                return "no camera";
    }
}
} // namespace totalrecruit
