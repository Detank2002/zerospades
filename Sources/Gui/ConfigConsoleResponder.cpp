/*
 Copyright (c) 2019 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ConfigConsoleResponder.h"

#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/Strings.h>

#include <Gui/UI/Framework/TextUtils.h>
#include <Gui/Utils/ConsoleCommand.h>

namespace spades {
	namespace gui {
		namespace {
			std::string EscapeQuotes(std::string s) { return Replace(s, "\"", "\\\""); }

			class ConfigNameIterator : public ConsoleCommandCandidateIterator {
				std::vector<std::string> const names;
				std::string query;
				std::size_t i = 0;
				ConsoleCommandCandidate current;

				// Moves to the next name that starts with the query (ignoring case, like the
				// chat does) and is defined by the program. The order of `names` is kept.
				void SkipToMatch() {
					while (i < names.size()) {
						if (ui::StringCommonPrefixLength(query, names[i]) == query.size()) {
							Settings::ItemHandle cvarHandle{names[i], nullptr};
							if (!cvarHandle.IsUnknown())
								break;
						}
						++i;
					}
				}

			public:
				ConfigNameIterator(const std::string& query)
				    : names{Settings::GetInstance()->GetAllItemNames()}, query{query} {
					SkipToMatch();
				}

				const ConsoleCommandCandidate& GetCurrent() override { return current; }

				bool MoveNext() override {
					if (i >= names.size())
						return false;

					// Create `ConsoleCommandCandidate` for the current
					// config variable
					Settings::ItemHandle cvarHandle{names[i], nullptr};
					current.name = names[i];
					current.description = EscapeQuotes(cvarHandle);
					current.isVariable = true;

					i++;
					SkipToMatch();

					return true;
				}
			};

			/**
			 * Returns the name of the config variable that `name` refers to, or an empty string.
			 * An exact match wins, otherwise the case is ignored. Unknown items are skipped, so
			 * no item is created for a name that is not a config variable.
			 */
			std::string ResolveConfigName(const std::string& name) {
				std::string found;
				for (const std::string& v : Settings::GetInstance()->GetAllItemNames()) {
					if (v.size() != name.size() ||
					    ui::StringCommonPrefixLength(v, name) != v.size())
						continue;

					Settings::ItemHandle handle{v, nullptr};
					if (handle.IsUnknown())
						continue;
					if (v == name)
						return v;
					if (found.empty())
						found = v;
				}
				return found;
			}

		} // namespace

		bool ConfigConsoleResponder::ExecCommand(const Handle<ConsoleCommand>& cmd) {
			SPADES_MARK_FUNCTION();

			const std::string name = ResolveConfigName(cmd->GetName());
			if (name.empty())
				return false;

			Settings::ItemHandle cvarHandle{name, nullptr};

			if (cmd->GetNumArguments() == 0) {
				// Display the current value
				SPLog("%s = \"%s\"", name.c_str(), EscapeQuotes(cvarHandle).c_str());
			} else if (cmd->GetNumArguments() == 1) {
				// Set a new value
				const std::string& newValue = cmd->GetArgument(0);
				SPLog("%s = \"%s\" (previously \"%s\")", name.c_str(),
				      EscapeQuotes(newValue).c_str(), EscapeQuotes(cvarHandle).c_str());
				cvarHandle = newValue;
			} else {
				SPLog("Invalid number of arguments (Maybe you meant something "
				      "like 'varname \"value1 value2\"'?)");
			}

			return true;
		}

		Handle<ConsoleCommandCandidateIterator>
		ConfigConsoleResponder::AutocompleteCommandName(const std::string& name) {
			return Handle<ConfigNameIterator>::New(name).Cast<ConsoleCommandCandidateIterator>();
		}
	} // namespace gui
} // namespace spades