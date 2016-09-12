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

#include "WebSockFwd.hpp"
#include "core/audit/AuditFwd.hpp"
#include "tools/db/db_fwd.hpp"

namespace Leosac
{
namespace Module
{
namespace WebSockAPI
{
/**
 * Holds valuable pointer to provide context to a request.
 */
struct RequestContext
{
    APIPtr session;
    DBServicePtr dbsrv;
    WSServer &server;

    /**
     * The initial audit trail for the request.
     * It is garanteed that this audit object is:
     *     + Non null
     *     + Already persisted
     */
    Audit::IAuditEntryPtr audit;
};
}
}
}
