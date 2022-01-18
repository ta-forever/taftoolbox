#pragma once

#include "tafclient/TafLobbyClient.h"
#include "DtoTableModel.h"

class TafLobbyPlayerDto : public TafLobbyPlayerInfo
{
public:
    enum class Fields
    {
        Dto = 0,
        IdInt,
        LoginStr,
        AliasStr,
        AvatarUrlStr,
        AvatarTooltipStr,
        CountryStr,
        StateStr,
        AfkSeconds,
        CurrentGameUidInt,
        _COLUMN_COUNT
    };

    typedef int IdType;

    TafLobbyPlayerDto();
    TafLobbyPlayerDto(const TafLobbyPlayerInfo& playerInfo);
    QVariant get(Fields f) const;
    IdType id() const;
};

Q_DECLARE_METATYPE(TafLobbyPlayerDto);
Q_DECLARE_METATYPE(QSharedPointer<DtoTableModel<TafLobbyPlayerDto> >);