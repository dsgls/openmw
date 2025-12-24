#include "favoritesmanager.hpp"

#include <algorithm>

#include <components/esm/defs.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    void FavoritesManager::toggleFavorite(const MWWorld::Ptr& item, bool asMagicItem)
    {
        if (item.isEmpty())
            return;

        const ESM::RefId id = item.getCellRef().getRefId();

        // Determine type based on parameter
        // From inventory: always Item (even if enchanted)
        // From magic menu: MagicItem
        ESM::QuickKeys::Type type = asMagicItem ? ESM::QuickKeys::Type::MagicItem : ESM::QuickKeys::Type::Item;

        if (hasFavorite(type, id))
            removeFavorite(type, id);
        else
            addFavorite(type, id);
    }

    void FavoritesManager::toggleFavorite(const ESM::RefId& spellId)
    {
        if (hasFavorite(ESM::QuickKeys::Type::Magic, spellId))
            removeFavorite(ESM::QuickKeys::Type::Magic, spellId);
        else
            addFavorite(ESM::QuickKeys::Type::Magic, spellId);
    }

    bool FavoritesManager::isFavorite(const MWWorld::Ptr& item, bool asMagicItem) const
    {
        if (item.isEmpty())
            return false;

        const ESM::RefId id = item.getCellRef().getRefId();

        // Check specific type based on context
        ESM::QuickKeys::Type type = asMagicItem ? ESM::QuickKeys::Type::MagicItem : ESM::QuickKeys::Type::Item;
        return hasFavorite(type, id);
    }

    bool FavoritesManager::isFavorite(const ESM::RefId& spellId) const
    {
        return hasFavorite(ESM::QuickKeys::Type::Magic, spellId);
    }

    std::vector<ESM::Favorites::Favorite> FavoritesManager::getFavorites(ESM::QuickKeys::Type type) const
    {
        std::vector<ESM::Favorites::Favorite> result;
        for (const auto& fav : mFavorites.mFavorites)
        {
            if (fav.mType == type)
                result.push_back(fav);
        }
        return result;
    }

    void FavoritesManager::write(ESM::ESMWriter& writer) const
    {
        if (mFavorites.mFavorites.empty())
            return;

        writer.startRecord(ESM::REC_FAVS);
        mFavorites.save(writer);
        writer.endRecord(ESM::REC_FAVS);
    }

    void FavoritesManager::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_FAVS)
        {
            mFavorites.load(reader);
        }
    }

    size_t FavoritesManager::countSavedGameRecords() const
    {
        return mFavorites.mFavorites.empty() ? 0 : 1;
    }

    // Private helpers

    void FavoritesManager::addFavorite(ESM::QuickKeys::Type type, const ESM::RefId& id)
    {
        if (hasFavorite(type, id))
            return;

        ESM::Favorites::Favorite fav;
        fav.mType = type;
        fav.mId = id;
        mFavorites.mFavorites.push_back(fav);
    }

    void FavoritesManager::removeFavorite(ESM::QuickKeys::Type type, const ESM::RefId& id)
    {
        auto& favs = mFavorites.mFavorites;
        favs.erase(
            std::remove_if(favs.begin(), favs.end(),
                [&](const ESM::Favorites::Favorite& fav) { return fav.mType == type && fav.mId == id; }),
            favs.end());
    }

    bool FavoritesManager::hasFavorite(ESM::QuickKeys::Type type, const ESM::RefId& id) const
    {
        for (const auto& fav : mFavorites.mFavorites)
        {
            if (fav.mType == type && fav.mId == id)
                return true;
        }
        return false;
    }
}
