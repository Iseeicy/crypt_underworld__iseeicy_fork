/*
 * This file is part of Crypt Underworld.
 *
 * Crypt Underworld is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later
 * version.
 *
 * Crypt Underworld is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Crypt Underworld. If not, see
 * <https://www.gnu.org/licenses/>.
 *
 * Copyright (c) 2021 Zoë Sparks <zoe@milky.flowers>
 */

#include "log.hpp"

#include <iostream>
#include <chrono>
#include <sstream>

namespace cu {

// global log
Log log;

std::string LoggableObjMember::str(std::string::size_type extra_spaces)
{
    std::string opening = name + spacer;
    const auto opening_len = opening.length();
    std::string padding = "";
    if (extra_spaces > opening_len) {
        std::string::size_type space_cnt = extra_spaces - opening_len;
        for (std::string::size_type i = 0; i < space_cnt; ++i) {
            padding.push_back(' ');
        }
     }

    return name + spacer + padding + value;
}

std::string LoggableObj::str()
{
    if (members.size() == 0) {
        return "";
    }

    const std::string opening = name + " { ";

    std::string::size_type longest_name_len = 0;
    for (auto& m : members) {
        auto name_len = m.name.length();
        if (name_len > longest_name_len) {
            longest_name_len = name_len;
        }
    }
    std::string::size_type member_indent_spaces =
        longest_name_len + LoggableObjMember::spacer.length();

    std::stringstream ss;
    ss << opening << members.at(0).str(member_indent_spaces);
    if (members.size() == 1) {
        ss << " }\n";
    } else {
        std::string indent = "";
        for (std::string::size_type i = 0; i < opening.length(); ++i) {
            indent.push_back(' ');
        }

        ss << ",\n";

        for (std::vector<LoggableObjMember>::size_type i = 1;
             i < members.size() - 1;
             ++i) {
            ss << indent << members.at(i).str(member_indent_spaces) << ",\n";
        }

        ss << indent << members.back().str(member_indent_spaces) << " }\n";
    }

    return ss.str();
}

Log::Log()
{}

Log::~Log() noexcept
{
    stopped = true;
}

void Log::turn_on() noexcept
{
    on = true;
}

void Log::turn_off() noexcept
{
    on = false;
}

void Log::async_on()
{
    if (!async) {
        emptier = guarded_thread {std::thread{&Log::wait_to_empty, this}};
        async = true;
    }
}

void Log::async_off()
{
    if (async) {
        auto stopped_memo = stopped;

        // TODO: this would be better handled in a destructor
        // (i.e. an Emptier class)
        stopped = true;
        emptier.join();

        async = false;
        stopped = stopped_memo;
    }
}

void safe_err(const char* oper) noexcept
{
    fprintf(stderr, "*** could not %s! discarding entry...\n", oper);
}

bool Log::append_newline(std::string& entry, bool newline) noexcept
{
    try {
        if (newline) {
            entry += "\n";
        }
    } catch(...) {
        safe_err("append newline to entry");
        return false;
    }

    return true;
}

bool Log::indent_entry(std::string& entry) noexcept
{
    try {
        if (indent_amt > 0) {
            bool insert = true;
            for (std::string::size_type i = 0; i < entry.size(); ++i) {
                if (insert) {
                    for (std::string::size_type j = 0;
                         j < indent_amt;
                         ++j) {
                        entry.insert(i, indentation);
                    }
                    insert = false;
                }

                if (entry.at(i) == '\n' && i < entry.size() - 1) {
                    insert = true;
                }
            }
        }
    } catch(...) {
        safe_err("prepend indentation to entry");
        return false;
    }

    return true;
}

bool Log::format_entry(std::string& entry, bool newline) noexcept
{
    if (!indent_entry(entry) || !append_newline(entry, newline)) {
        return false;
    }

     return true;
}

void Log::enter_async(std::string entry, bool newline) noexcept
{
    try {
        msgs_mutex.lock();
    } catch(...) {
        safe_err("lock msgs mutex for log entry");
        return;
    }

    if (!format_entry(entry, newline)) {
        msgs_mutex.unlock();
        return;
    }

    try {
        msgs.push_back(entry);
    } catch(...) {
        msgs_mutex.unlock();
        safe_err("push log entry onto queue");
        return;
    }

    msgs_mutex.unlock();
}

void write_entry(std::string entry) noexcept
{
    try {
        std::cout << entry;
    } catch(...) {
        safe_err("write log message due to stream state");
    }
}

void Log::enter_sync(std::string entry, bool newline) noexcept
{
    if (!format_entry(entry, newline)) {
        return;
    }

    write_entry(entry);
}

void Log::enter(std::string entry, bool newline) noexcept
{
    if (on && !entry.empty()) {
        if (async) {
            enter_async(entry, newline);
        } else {
            enter_sync(entry, newline);
        }
    }
}

void Log::enter(std::string name, std::vector<const char*>& entries) noexcept
{
    if (on) {
        try {
            std::string entry = name + ": ";
            if (entries.size() == 0) {
                entry += "<none>\n";
            } else if (entries.size() == 1) {
                entry += std::string{entries.front()} + "\n";
            } else {
                for (std::string::size_type i = 0;
                     i < entries.size() - 1;
                     ++i) {
                    entry += std::string{entries.at(i)} + ", ";
                }
                entry += std::string{entries.back()} + "\n";
            }
            enter(entry, false);
        } catch(...) {
            safe_err("construct log entry from vector");
        }
    }
}

void Log::enter_obj(LoggableObj&& obj) noexcept
{
    if (on) {
        try {
            enter(obj.str(), false);
        } catch(...) {
            safe_err("construct log entry from object");
        }
    }
}

void Log::attempt(std::string entry) noexcept
{
    enter(entry + "...", false);
}

void Log::indent() noexcept
{
    indent_amt = 1;
}

void Log::indent(std::string::size_type amt) noexcept
{
    indent_amt = amt;
}

void Log::finish() noexcept
{
    enter("OK");
}

void Log::brk() noexcept
{
    indent_amt = 0;
    enter("\n", false);
}

void Log::wait_to_empty() noexcept
{
    while (!stopped) {
        empty_queue();
    }
    empty_queue();
}

void Log::empty_queue() noexcept
{
    try {
        msgs_mutex.lock();
    } catch(...) {
        fprintf(stderr, "*** could not lock msgs mutex for log output!");
        return;
    }

    while (!msgs.empty()) {
        write_entry(msgs.front());
        msgs.pop_front();
    }

    msgs_mutex.unlock();
}

} // namespace cu
