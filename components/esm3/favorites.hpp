#ifndef OPENMW_ESM_FAVORITES_H
#define OPENMW_ESM_FAVORITES_H

#include <vector>
#include <components/esm/refid.hpp>
#include <components/esm3/quickkeys.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    /// Player's favorited items and spells for Quick Equip menu
    struct Favorites
    {
        struct Favorite
        {
            QuickKeys::Type mType;
            RefId mId;

            bool operator==(const Favorite& other) const
            {
                return mType == other.mType && mId == other.mId;
            }
        };

        std::vector<Favorite> mFavorites;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
