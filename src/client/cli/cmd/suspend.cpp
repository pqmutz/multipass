/*
 * Copyright (C) 2019-2020 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "suspend.h"
#include "common_cli.h"

#include "animated_spinner.h"

#include <multipass/cli/argparser.h>
#include <multipass/constants.h>
#include <multipass/settings.h>

namespace mp = multipass;
namespace cmd = multipass::cmd;
using RpcMethod = mp::Rpc::Stub;

mp::ReturnCode cmd::Suspend::run(mp::ArgParser* parser)
{
    auto ret = parse_args(parser);
    if (ret != ParseCode::Ok)
    {
        return parser->returnCodeFrom(ret);
    }

    auto on_success = [](mp::SuspendReply& reply) { return ReturnCode::Ok; };

    AnimatedSpinner spinner{cout};
    auto on_failure = [this, &spinner](grpc::Status& status) {
        spinner.stop();
        return standard_failure_handler_for(name(), cerr, status);
    };

    spinner.start(instance_action_message_for(request.instance_names(), "Suspending "));
    request.set_verbosity_level(parser->verbosityLevel());
    return dispatch(&RpcMethod::suspend, request, on_success, on_failure);
}

std::string cmd::Suspend::name() const
{
    return "suspend";
}

QString cmd::Suspend::short_help() const
{
    return QStringLiteral("Suspend running instances");
}

QString cmd::Suspend::description() const
{
    return QStringLiteral("Suspend the named instances, if running. Exits with\n"
                          "return code 0 if successful.\n"
                          "If primary instances are disabled and no instance\n"
                          "name is provided, an error code will be returned.");
}

mp::ParseCode cmd::Suspend::parse_args(mp::ArgParser* parser)
{
    const auto petenv_name = MP_SETTINGS.get(petenv_key);

    parser->addPositionalArgument(
        "name",
        QString{"Names of instances to suspend. If omitted, and without the --all option, '%1' will be assumed."}.arg(
            petenv_name),
        "[<name> ...]");

    QCommandLineOption all_option("all", "Suspend all instances");
    parser->addOptions({all_option});

    auto status = parser->commandParse(this);
    if (status != ParseCode::Ok)
        return status;

    auto parse_code = check_for_name_and_all_option_conflict(parser, cerr, /*allow_empty=*/!petenv_name.isEmpty());
    if (parse_code != ParseCode::Ok)
    {
        cerr << "error: primary instance disabled, need an instance name or --all\n";
        return parse_code;
    }

    request.mutable_instance_names()->CopyFrom(add_instance_names(parser, /*default_name=*/petenv_name.toStdString()));

    return status;
}
