#include "quickequipdialog.hpp"

#include <algorithm>

#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_TextBox.h>

#include <components/esm3/loadspel.hpp>
#include <components/settings/values.hpp>
#include <components/widgets/sharedstatebutton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellutil.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"

#include "favoritesmanager.hpp"
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

        // Reset scroll position to top
        mSlotContainer->setViewOffset(MyGUI::IntPoint(0, 0));

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

        // Get favorites data
        FavoritesManager* favMgr = MWBase::Environment::get().getWindowManager()->getFavoritesManager();
        if (!favMgr)
            return;

        const auto& favorites = favMgr->getAllFavorites();

        // Get player and inventory for validation
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        // Build list of available favorites, preserving user-defined order
        for (size_t i = 0; i < favorites.size(); ++i)
        {
            const auto& fav = favorites[i];
            SlotEntry entry;
            entry.type = fav.mType;
            entry.id = fav.mId;
            entry.icon = nullptr;
            entry.label = nullptr;
            entry.favoritesIndex = i;

            bool isAvailable = false;

            switch (fav.mType)
            {
                case ESM::QuickKeys::Type::Magic:
                {
                    // Check if spell is still known
                    if (player.getClass().getCreatureStats(player).getSpells().hasSpell(fav.mId))
                    {
                        const ESM::Spell* spell
                            = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().search(fav.mId);
                        if (spell)
                        {
                            entry.name = spell->mName;
                            isAvailable = true;
                        }
                    }
                    break;
                }
                case ESM::QuickKeys::Type::Item:
                case ESM::QuickKeys::Type::MagicItem:
                {
                    // Find item in inventory
                    MWWorld::Ptr item = store.findReplacement(fav.mId);
                    if (!item.isEmpty())
                    {
                        entry.name = item.getClass().getName(item);
                        isAvailable = true;
                    }
                    break;
                }
                default:
                    break;
            }

            // Only add if available
            if (isAvailable)
                mSlots.push_back(entry);
        }

        // Create UI widgets
        int yOffset = 0;
        const int slotHeight = Settings::gui().mFontSize + 6;
        const int containerWidth = 314;

        for (size_t i = 0; i < mSlots.size(); ++i)
        {
            auto& entry = mSlots[i];

            // Create button
            entry.button = mSlotContainer->createWidget<Gui::SharedStateButton>("SandTextButton",
                MyGUI::IntCoord(0, yOffset, containerWidth, slotHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            entry.button->setNeedKeyFocus(true);
            entry.button->setUserData(i); // Store slot index
            entry.button->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickEquipDialog::onSlotClicked);
            entry.button->setCaption(entry.name);
            entry.button->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);

            yOffset += slotHeight;
        }

        // Set canvas size
        mSlotContainer->setCanvasSize(containerWidth, std::max(yOffset, 1));

        // Resize dialog
        const int titleHeight = 38;
        const int bottomPadding = 16;
        const int maxEntries = 10;
        const int numEntries = std::min(static_cast<int>(mSlots.size()), maxEntries);
        const int contentHeight = numEntries > 0 ? numEntries * slotHeight : 40;
        const int dialogHeight = titleHeight + contentHeight + bottomPadding;

        mMainWidget->setSize(mMainWidget->getWidth(), dialogHeight);
        mSlotContainer->setSize(mSlotContainer->getWidth(), contentHeight);

        center();
    }

    void QuickEquipDialog::onSlotClicked(MyGUI::Widget* sender)
    {
        size_t index = *sender->getUserData<size_t>();
        activateSlot(index);
    }

    void QuickEquipDialog::activateSlot(size_t slotIndex)
    {
        if (slotIndex >= mSlots.size())
            return;

        const SlotEntry& slot = mSlots[slotIndex];

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        const MWMechanics::CreatureStats& playerStats = player.getClass().getCreatureStats(player);

        // Don't activate if player is paralyzed or dead
        if (playerStats.isParalyzed() || playerStats.isDead())
            return;

        // Handle based on type
        if (slot.type == ESM::QuickKeys::Type::Item || slot.type == ESM::QuickKeys::Type::MagicItem)
        {
            // Find the item in inventory
            MWWorld::ContainerStoreIterator it = store.begin();
            for (; it != store.end(); ++it)
            {
                if (it->getCellRef().getRefId() == slot.id)
                    break;
            }

            // Is the item not in the inventory?
            if (it == store.end())
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sQuickMenu5} " + slot.name);
                return;
            }

            MWWorld::Ptr item = *it;

            if (slot.type == ESM::QuickKeys::Type::Item)
            {
                if (!store.isEquipped(item.getCellRef().getRefId()))
                    MWBase::Environment::get().getWindowManager()->useItem(item);
                MWWorld::ConstContainerStoreIterator rightHand
                    = store.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                // Change draw state only if the item is in player's right hand
                if (rightHand != store.end() && item == *rightHand)
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState::Weapon);
                }
            }
            else if (slot.type == ESM::QuickKeys::Type::MagicItem)
            {
                // Equip if it can be equipped and isn't yet equipped
                if (!item.getClass().getEquipmentSlots(item).first.empty() && !store.isEquipped(item))
                {
                    MWBase::Environment::get().getWindowManager()->useItem(item);

                    // Make sure item was successfully equipped
                    if (!store.isEquipped(item))
                        return;
                }

                store.setSelectedEnchantItem(it);
                MWBase::Environment::get().getWindowManager()->setSelectedEnchantItem(*it);
                MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState::Spell);
            }
        }
        else if (slot.type == ESM::QuickKeys::Type::Magic)
        {
            // Check if player still has the spell
            MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
            MWMechanics::Spells& spells = stats.getSpells();

            if (!spells.hasSpell(slot.id))
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sQuickMenu5} " + slot.name);
                return;
            }

            store.setSelectedEnchantItem(store.end());
            MWBase::Environment::get().getWindowManager()->setSelectedSpell(
                slot.id, int(MWMechanics::getSpellSuccessChance(slot.id, player)));
            MWBase::Environment::get().getWorld()->getPlayer().setDrawState(MWMechanics::DrawState::Spell);
        }

        // Update spell window
        MWBase::Environment::get().getWindowManager()->updateSpellWindow();

        // Close dialog
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_QuickEquipMenu);
    }

    void QuickEquipDialog::moveItemUp()
    {
        if (mSlots.empty() || mControllerFocus >= mSlots.size())
            return;

        // Remember the item we're moving
        ESM::RefId movedItemId = mSlots[mControllerFocus].id;
        ESM::QuickKeys::Type movedItemType = mSlots[mControllerFocus].type;
        size_t favIndex = mSlots[mControllerFocus].favoritesIndex;

        // Try to move it up in the favorites list
        FavoritesManager* favMgr = MWBase::Environment::get().getWindowManager()->getFavoritesManager();
        if (favMgr && favMgr->moveFavoriteUp(favIndex))
        {
            // Refresh the display
            populateSlots();

            // Find the moved item in the new slot list
            for (size_t i = 0; i < mSlots.size(); ++i)
            {
                if (mSlots[i].id == movedItemId && mSlots[i].type == movedItemType)
                {
                    mControllerFocus = i;
                    break;
                }
            }

            // Restore focus highlight and update scroll position
            if (!mSlots.empty() && mControllerFocus < mSlots.size())
            {
                auto* btn = static_cast<Gui::SharedStateButton*>(mSlots[mControllerFocus].button);
                btn->onMouseSetFocus(nullptr);

                // Update scroll position to keep focused item visible
                if (mControllerFocus <= 5)
                    mSlotContainer->setViewOffset(MyGUI::IntPoint(0, 0));
                else
                {
                    const int itemHeight = btn->getHeight();
                    mSlotContainer->setViewOffset(MyGUI::IntPoint(0, -itemHeight * (mControllerFocus - 5)));
                }
            }
        }
    }

    void QuickEquipDialog::moveItemDown()
    {
        if (mSlots.empty() || mControllerFocus >= mSlots.size())
            return;

        // Remember the item we're moving
        ESM::RefId movedItemId = mSlots[mControllerFocus].id;
        ESM::QuickKeys::Type movedItemType = mSlots[mControllerFocus].type;
        size_t favIndex = mSlots[mControllerFocus].favoritesIndex;

        // Try to move it down in the favorites list
        FavoritesManager* favMgr = MWBase::Environment::get().getWindowManager()->getFavoritesManager();
        if (favMgr && favMgr->moveFavoriteDown(favIndex))
        {
            // Refresh the display
            populateSlots();

            // Find the moved item in the new slot list
            for (size_t i = 0; i < mSlots.size(); ++i)
            {
                if (mSlots[i].id == movedItemId && mSlots[i].type == movedItemType)
                {
                    mControllerFocus = i;
                    break;
                }
            }

            // Restore focus highlight and update scroll position
            if (!mSlots.empty() && mControllerFocus < mSlots.size())
            {
                auto* btn = static_cast<Gui::SharedStateButton*>(mSlots[mControllerFocus].button);
                btn->onMouseSetFocus(nullptr);

                // Update scroll position to keep focused item visible
                if (mControllerFocus <= 5)
                    mSlotContainer->setViewOffset(MyGUI::IntPoint(0, 0));
                else
                {
                    const int itemHeight = btn->getHeight();
                    mSlotContainer->setViewOffset(MyGUI::IntPoint(0, -itemHeight * (mControllerFocus - 5)));
                }
            }
        }
    }

    bool QuickEquipDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        // Always handle B button to allow closing the dialog
        if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            exit();
            return true;
        }

        // Other buttons only work when there are slots
        if (mSlots.empty())
            return true;

        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            activateSlot(mControllerFocus);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER && arg.state == SDL_PRESSED)
        {
            moveItemUp();
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER && arg.state == SDL_PRESSED)
        {
            moveItemDown();
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            size_t prevFocus = mControllerFocus;
            mControllerFocus = wrap(mControllerFocus, mSlots.size(), -1);

            auto* prevBtn = static_cast<Gui::SharedStateButton*>(mSlots[prevFocus].button);
            auto* newBtn = static_cast<Gui::SharedStateButton*>(mSlots[mControllerFocus].button);
            prevBtn->onMouseLostFocus(nullptr);
            newBtn->onMouseSetFocus(nullptr);

            // Scroll to keep focused item visible
            if (mControllerFocus <= 5)
                mSlotContainer->setViewOffset(MyGUI::IntPoint(0, 0));
            else
            {
                const int itemHeight = newBtn->getHeight();
                mSlotContainer->setViewOffset(MyGUI::IntPoint(0, -itemHeight * (mControllerFocus - 5)));
            }
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            size_t prevFocus = mControllerFocus;
            mControllerFocus = wrap(mControllerFocus, mSlots.size(), 1);

            auto* prevBtn = static_cast<Gui::SharedStateButton*>(mSlots[prevFocus].button);
            auto* newBtn = static_cast<Gui::SharedStateButton*>(mSlots[mControllerFocus].button);
            prevBtn->onMouseLostFocus(nullptr);
            newBtn->onMouseSetFocus(nullptr);

            // Scroll to keep focused item visible
            if (mControllerFocus <= 5)
                mSlotContainer->setViewOffset(MyGUI::IntPoint(0, 0));
            else
            {
                const int itemHeight = newBtn->getHeight();
                mSlotContainer->setViewOffset(MyGUI::IntPoint(0, -itemHeight * (mControllerFocus - 5)));
            }
        }

        return true;
    }
}
