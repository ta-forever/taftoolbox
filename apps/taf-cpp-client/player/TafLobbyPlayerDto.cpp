#include "TafLobbyPlayerDto.h"

TafLobbyPlayerDto::TafLobbyPlayerDto()
{ }

TafLobbyPlayerDto::TafLobbyPlayerDto(const TafLobbyPlayerInfo& playerInfo):
    TafLobbyPlayerInfo(playerInfo)
{ }

QVariant TafLobbyPlayerDto::get(Fields f) const
{
    switch (f)
    {
    case Fields::Dto: return QVariant::fromValue(*this);
    case Fields::IdInt: return QVariant::fromValue(TafLobbyPlayerInfo::id);
    case Fields::LoginStr: return QVariant::fromValue(login);
    case Fields::AliasStr: return QVariant::fromValue(alias);
    case Fields::AvatarUrlStr: return QVariant::fromValue(avatarUrl);
    case Fields::AvatarTooltipStr: return QVariant::fromValue(avatarTooltip);
    case Fields::CountryStr: return QVariant::fromValue(country);
    case Fields::StateStr: return QVariant::fromValue(state);
    case Fields::AfkSeconds: return QVariant::fromValue(afkSeconds);
    case Fields::CurrentGameUidInt: return QVariant::fromValue(currentGameUid);
    default: return QVariant();
    }
}

TafLobbyPlayerDto::IdType TafLobbyPlayerDto::id() const
{
    return TafLobbyPlayerInfo::id;
}
