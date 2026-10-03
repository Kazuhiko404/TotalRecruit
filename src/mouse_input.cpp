#include "common.h"
#include "mouse_input.h"

#include <mygui/MyGUI_InputManager.h>

namespace totalrecruit
{
void ReadMouseButtons(bool* leftDown, bool* rightDown)
{
    *leftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    *rightDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

bool IsMouseOverMyGui()
{
    TR_TRY
    {
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        return input != NULL && input->getMouseFocusWidget() != NULL;
    }
    TR_CATCH { return false; }
}

bool ReadMousePosition(int* x, int* y)
{
    TR_TRY
    {
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        if (input == NULL)
        {
            return false;
        }
        const MyGUI::IntPoint p = input->getMousePosition();
        *x = p.left;
        *y = p.top;
        return true;
    }
    TR_CATCH { return false; }
}
} // namespace totalrecruit
