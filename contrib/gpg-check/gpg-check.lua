if confget("RPM::GPG-Check/b", "true") == "false" then
	return
end

if table.getn(files_install) < 1 then
	return
end

print(_("Checking GPG signatures..."))
good = 1
unknown = 0
illegal = 0
unsigned = 0

for i, file in ipairs(files_install) do
    inp = io.popen("LANG=C /bin/rpm --checksig  "..file.." 2>&1")
 
    for line in inp.lines(inp) do
        if string.find(line, "gpg") then
            break
        elseif string.find(line, "GPG") then
            print(_("Unknown signature "..line))
			unknown = unknown + 1
            good = nil
        elseif string.find(line, "rpmReadSignature") then
            print(_("Illegal signature "..line))
			illegal = illegal + 1
            good = nil
        else
            print(_("Unsigned "..line))
			unsigned = unsigned + 1
			good = nil
        end
    end
    io.close(inp)
end

if not good then
	apterror(_("Error(s) while checking package signatures:\n"..unsigned.." unsigned package(s)\n"..unknown.." package(s) with unknown signatures\n"..illegal.." package(s) with illegal/corrupted signatures"))
end

-- vim::ts=4
