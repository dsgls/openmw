#include "quickequipdialog.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_TextBox.h>

#include <components/settings/values.hpp>
#include <components/widgets/sharedstatebutton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "itemwidget.hpp"
#include "quickkeysmenu.hpp"
#include "windowmanagerimp.hpp"
#include "widgets.hpp"

namespace MWGui
{
    QuickEquipDialog::QuickEquipDialog()
        : WindowModal("openmw_quickequip_dialog.layout")
    {
        getWidget(mSlotContainer, "SlotContainer");

        if (Settings::gui().mControllerMenus)
        {
            mDisableGamepadCursor = true;
            mControllerButtons.mA = "#{Interface:Activate}";
            mControllerButtons.mB = "#{Interface:Cancel}";
        }

        center();
    }

    void QuickEquipDialog::onOpen()
    {
        WindowModal::onOpen();
        populateSlots();

        if (Settings::gui().mControllerMenus && !mSlots.empty())
        {
            mControllerFocus = 0;
            auto* btn = static_cast<Gui::SharedStateButton*>(mSlots[0].button);
            btn->onMouseSetFocus(nullptr);
        }
    }

    void QuickEquipDialog::onClose()
    {
        WindowModal::onClose();

        // Clear widgets
        for (auto& slot : mSlots)
        {
            MyGUI::Gui::getInstance().destroyWidget(slot.button);
        }
        mSlots.clear();
        mControllerFocus = 0;
    }

    bool QuickEquipDialog::exit()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_QuickEquipMenu);
        return true;
    }

    void QuickEquipDialog::populateSlots()
    {
        // Clear previous widgets
        for (auto& slot : mSlots)
        {
            MyGUI::Gui::getInstance().destroyWidget(slot.button);
        }
        mSlots.clear();

        // Get QuickKeysMenu data
        auto* winMgr = static_cast<WindowManager*>(&*MWBase::Environment::get().getWindowManager());
        QuickKeysMenu* qkm = winMgr->getQuickKeysMenu();
        if (!qkm)
            return;

        const auto& keys = qkm->getKeys();

        int yOffset = 0;
        const int slotHeight = Settings::gui().mFontSize + 2;
        const int containerWidth = 314;

        for (const auto& key : keys)
        {
            // Skip unassigned slots (Hand-to-Hand has its own type, not Unassigned)
            if (key.type == ESM::QuickKeys::Type::Unassigned)
                continue;

            // Create slot entry
            SlotEntry entry;
            entry.index = key.index;

            // Create button using SharedStateButton for proper selection highlighting
            entry.button = mSlotContainer->createWidget<Gui::SharedStateButton>(
                "SpellTextUnequippedController", MyGUI::IntCoord(0, yOffset, containerWidth, slotHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            entry.button->setNeedKeyFocus(true);
            entry.button->setUserData(entry.index);
            entry.button->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickEquipDialog::onSlotClicked);

            // Set caption - use name if available, otherwise "Slot N"
            std::string caption = key.name;
            if (caption.empty())
                caption = "Slot " + std::to_string(key.index);
            entry.button->setCaption(caption);
            entry.button->setTextAlign(MyGUI::Align::Left);

            // Store as MyGUI::Widget* for compatibility
            entry.label = nullptr;
            entry.icon = nullptr;

            mSlots.push_back(entry);
            yOffset += slotHeight;
        }

        // Set the canvas size to fit all slots
        mSlotContainer->setCanvasSize(containerWidth, std::max(yOffset, 1));

        // If no slots, show message
        if (mSlots.empty())
        {
            MyGUI::TextBox* emptyMsg = mSlotContainer->createWidget<MyGUI::TextBox>(
                "SandText", MyGUI::IntCoord(8, 8, containerWidth - 16, 24), MyGUI::Align::Center);
            emptyMsg->setCaption("No quick keys assigned");
            emptyMsg->setTextAlign(MyGUI::Align::Center);
        }

        // Resize dialog to fit content (cap at 10 entries max)
        const int titleHeight = 38; // Space for title text
        const int bottomPadding = 16; // Bottom margin
        const int maxEntries = 10;
        const int numEntries = std::min(static_cast<int>(mSlots.size()), maxEntries);
        const int contentHeight = numEntries > 0 ? numEntries * slotHeight : 40; // Minimum height if empty
        const int dialogHeight = titleHeight + contentHeight + bottomPadding;

        mMainWidget->setSize(mMainWidget->getWidth(), dialogHeight);
        mSlotContainer->setSize(mSlotContainer->getWidth(), contentHeight);

        center();
    }

    void QuickEquipDialog::onSlotClicked(MyGUI::Widget* sender)
    {
        int index = *sender->getUserData<int>();
        activateSlot(index);
    }

    void QuickEquipDialog::activateSlot(int index)
    {
        // Reuse existing activation logic
        auto* winMgr = static_cast<WindowManager*>(&*MWBase::Environment::get().getWindowManager());
        QuickKeysMenu* qkm = winMgr->getQuickKeysMenu();
        if (qkm)
        {
            qkm->activateQuickKey(index);
        }

        // Close dialog
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_QuickEquipMenu);
    }

    bool QuickEquipDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (mSlots.empty())
            return true;

        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            activateSlot(mSlots[mControllerFocus].index);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            exit();
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            size_t prevFocus = mControllerFocus;
            mControllerFocus = wrap(mControllerFocus, mSlots.size(), -1);

            auto* prevBtn = static_cast<Gui::SharedStateButton*>(mSlots[prevFocus].button);
            auto* newBtn = static_cast<Gui::SharedStateButton*>(mSlots[mControllerFocus].button);
            prevBtn->onMouseLostFocus(nullptr);
            newBtn->onMouseSetFocus(nullptr);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            size_t prevFocus = mControllerFocus;
            mControllerFocus = wrap(mControllerFocus, mSlots.size(), 1);

            auto* prevBtn = static_cast<Gui::SharedStateButton*>(mSlots[prevFocus].button);
            auto* newBtn = static_cast<Gui::SharedStateButton*>(mSlots[mControllerFocus].button);
            prevBtn->onMouseLostFocus(nullptr);
            newBtn->onMouseSetFocus(nullptr);
        }

        return true;
    }
}
