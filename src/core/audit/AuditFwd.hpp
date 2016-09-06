/*
    Copyright (C) 2014-2016 Islog

    This file is part of Leosac.

    Leosac is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leosac is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstddef>
#include <flagset.hpp>
#include <memory>

namespace Leosac
{
namespace Audit
{
class AuditEntry;
using AuditEntryPtr = std::shared_ptr<AuditEntry>;

class WSAPICall;
using WSAPICallUPtr = std::unique_ptr<WSAPICall>;

enum class EventType
{
    WSAPI_CALL,
    USER_CREATED,
    USER_DELETED,
    /**
     * A call to "user_get" websocket API has been made.
     */
    USER_GET,
    MEMBERSHIP_CREATED,
    MEMBERSHIP_DELETED,
    LAST__
};

struct EventMask : public FlagSet<EventType>
{
    EventMask() = default;
    EventMask(const std::string bitset_repr)
        : FlagSet<EventType>(bitset_repr)
    {
    }
    static EventMask UserEvent;
};
}
}
