#ifndef MWGUI_QUICKEQUIPDIALOG_H
#define MWGUI_QUICKEQUIPDIALOG_H

#include "windowbase.hpp"

#include <components/esm3/favorites.hpp>
#include <components/esm3/quickkeys.hpp>

namespace MWWorld
{
    class Ptr;
}

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
            ESM::QuickKeys::Type type;  // Item, Magic, or MagicItem
            ESM::RefId id;              // Item or spell ID
            std::string name;           // Display name
            MyGUI::Button* button;      // Container button
            MyGUI::ImageBox* icon;      // Icon display (unused for now)
            MyGUI::TextBox* label;      // Name text (unused for now)
        };

        std::vector<SlotEntry> mSlots;
        MyGUI::ScrollView* mSlotContainer;
        size_t mControllerFocus = 0;

        void populateSlots();
        void onSlotClicked(MyGUI::Widget* sender);
        void activateSlot(size_t slotIndex);

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
    };
}

#endif
