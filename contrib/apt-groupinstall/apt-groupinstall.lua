-- Lua script to allow installing package groups as defined in comps.xml
-- Licensed under the GPL, by pmatilai@welho.com / 2003
-- This script must be plugged into the APT script slot
-- Scripts::AptGet::Install::TranslateArg
--

if string.sub(argument, 1, 6) == "group-" then
    group = string.sub(argument, 7)
    print("Finding packages belonging to "..argument.."...")
    pkgs = io.popen(confget("Dir::Bin::scripts/f").."/apt-groupinstall.py "..group)
    for line in pkgs:lines() do
        table.insert(translated, line)
    end
end
