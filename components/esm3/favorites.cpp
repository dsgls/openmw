#include "favorites.hpp"
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

namespace ESM
{
    void Favorites::load(ESMReader& esm)
    {
        mFavorites.clear();

        while (esm.isNextSub("TYPE"))
        {
            Favorite fav;
            esm.getHT(fav.mType);
            fav.mId = esm.getHNRefId("ID__");

            // Skip duplicates
            bool isDuplicate = false;
            for (const auto& existing : mFavorites)
            {
                if (existing == fav)
                {
                    isDuplicate = true;
                    break;
                }
            }

            if (!isDuplicate)
                mFavorites.push_back(fav);
        }
    }

    void Favorites::save(ESMWriter& esm) const
    {
        for (const auto& fav : mFavorites)
        {
            esm.writeHNT("TYPE", fav.mType);
            esm.writeHNRefId("ID__", fav.mId);
        }
    }
}
