-- Lua script to allow installing package groups as defined in comps.xml
-- Licensed under the GPL, by pmatilai@welho.com / 2003
-- This script must be plugged into the APT script slots
-- Scripts::AptGet::Command and Scripts::AptCache::Command
--

helper = confget("Dir::Bin::scripts/f").."/apt-groupinstall.py"

if script_slot == "Scripts::AptCache::Command" then
	if command_args[1] == "showgroups" then
		command_consume = 1
		os.execute(helper.." showgroups")
	elseif command_args[1] == "showgroup" then
		command_consume=1
		group = command_args[2]
		if not group then
			apterror("No groupname given.")
			return
		end
		os.execute(helper.." showgroup "..group)
	end

elseif script_slot == "Scripts::AptGet::Command" then
	if command_args[1] == "groupinstall" then
       		oper = markinstall
		command_consume = 1
	elseif command_args[1] == "groupremove" then
		oper = markremove
		command_consume = 1
	else
		return
	end
		group = command_args[2]
		if not group then
			apterror("No groupname given.")
			return
		end
    	print("Finding packages belonging to group "..group.."...")
    	pkgs = io.popen(helper.." grouppkgs "..group)
    	for name in pkgs:lines() do
		pkg = pkgfind(name)
		if pkg then
			oper(pkg)
		end
	end
end
