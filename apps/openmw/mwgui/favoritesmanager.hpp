#ifndef OPENMW_MWGUI_FAVORITESMANAGER_H
#define OPENMW_MWGUI_FAVORITESMANAGER_H

#include <components/esm3/favorites.hpp>
#include <components/esm3/quickkeys.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{
    /// Manages player's favorited items and spells for Quick Equip menu
    class FavoritesManager
    {
    public:
        FavoritesManager() = default;

        /// Toggle favorite status of an item
        /// If asMagicItem is true, save as MagicItem type (for use from magic menu)
        /// If false, save as Item type (for use from inventory)
        void toggleFavorite(const MWWorld::Ptr& item, bool asMagicItem = false);

        /// Toggle favorite status of a spell
        void toggleFavorite(const ESM::RefId& spellId);

        /// Check if an item is favorited
        /// If asMagicItem is true, check as MagicItem type (for use from magic menu)
        /// If false, check as Item type (for use from inventory)
        bool isFavorite(const MWWorld::Ptr& item, bool asMagicItem = false) const;

        /// Check if a spell is favorited
        bool isFavorite(const ESM::RefId& spellId) const;

        /// Get all favorites of a specific type (for display)
        std::vector<ESM::Favorites::Favorite> getFavorites(ESM::QuickKeys::Type type) const;

        /// Get all favorites (for Quick Equip menu)
        const std::vector<ESM::Favorites::Favorite>& getAllFavorites() const { return mFavorites.mFavorites; }

        /// Clear all favorites
        void clear() { mFavorites.mFavorites.clear(); }

        /// Save/load from savegame
        void write(ESM::ESMWriter& writer) const;
        void readRecord(ESM::ESMReader& reader, uint32_t type);
        size_t countSavedGameRecords() const;

    private:
        ESM::Favorites mFavorites;

        /// Helper to add a favorite (checks for duplicates)
        void addFavorite(ESM::QuickKeys::Type type, const ESM::RefId& id);

        /// Helper to remove a favorite
        void removeFavorite(ESM::QuickKeys::Type type, const ESM::RefId& id);

        /// Helper to check if a favorite exists
        bool hasFavorite(ESM::QuickKeys::Type type, const ESM::RefId& id) const;
    };
}

#endif
