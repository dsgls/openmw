#ifndef MWGUI_QUICKEQUIPDIALOG_H
#define MWGUI_QUICKEQUIPDIALOG_H

#include "windowbase.hpp"

namespace MWGui
{
    class QuickEquipDialog : public WindowModal
    {
    public:
        QuickEquipDialog();

        void onOpen() override;
        void onClose() override;
        bool exit() override;

    private:
        struct SlotEntry
        {
            int index;                  // Quickkey slot number (1-10)
            MyGUI::Button* button;      // Container button
            MyGUI::ImageBox* icon;      // Icon display
            MyGUI::TextBox* label;      // Name text
        };

        std::vector<SlotEntry> mSlots;
        MyGUI::ScrollView* mSlotContainer;
        size_t mControllerFocus = 0;

        void populateSlots();
        void onSlotClicked(MyGUI::Widget* sender);
        void activateSlot(int index);

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
    };
}

#endif
