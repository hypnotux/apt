#!/usr/bin/python

# apt-groupinstall v0.1
# groupinstall helper script for apt
# by pmatilai@welho.com

import rhpl.comps, sys, string

comps = rhpl.comps.Comps("/usr/share/comps/i386/comps.xml")

for group in comps.groups.values():
	if group.id == sys.argv[1]:
		for pkg in comps.groups[group.name].packages:
			print("%s" % pkg)
