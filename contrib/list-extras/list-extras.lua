-- This script will list all installed packages which are not
-- availabe in any online repository. It must be plugged
-- in the slot Scripts::AptCache::Command
--
-- Author: Gustavo Niemeyer <niemeyer@conectiva.com>

if command_args[1] ~= "list-extras" then
	return
end
command_consume = 1

for i, pkg in pairs(pkglist()) do
	ver = pkgvercur(pkg)
	verlist = pkgverlist(pkg)
	if ver and not verisonline(ver)
	   and table.getn(verlist) == 1 then
		print(pkgname(pkg) .. "-" .. verstr(ver))
	end
end
