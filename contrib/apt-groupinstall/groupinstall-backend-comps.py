#!/usr/bin/python

# apt-groupinstall v0.2
# groupinstall helper backend for for RHL/RHEL/FC systems 
# by pmatilai@welho.com

import rhpl.comps, sys

def findgroup(comps, grpname):
	if comps.groups.has_key(grpname):
		return comps.groups[grpname]
	for group in comps.groups.values():
		if group.id == grpname:
			return group

def grouppkgs(comps, grpname, recursive = 0, showall = 0):
	group = findgroup(comps, grpname)
	pkgs = []
	if recursive:
		for grp in group.groups:
			pkgs += grouppkgs(comps, grp, recursive, showall)
	if group and group.packages:
		for pkg in group.packages:
			type, name = group.packages[pkg]
			if not showall and type == "optional":
				continue
			pkgs.append(pkg)
	return pkgs

def groupnames(comps, showhidden = 0):
	print "%-40s %s" % ("Group name", "Description")
	print "%-40s %s" % ("----------", "-----------")
	for group in comps.groups.values():
		if group.packages:
			if not showhidden and not group.user_visible:
				continue
			print "%-40s %s" % (group.id, group.name)

def showgroup(comps, grpname, showall = 0):
	group = findgroup(comps, grpname)
	if not group or not group.packages:
		print "No such group: %s" % grpname
		return
	print "Group:\n  %s" % group.id
	print "Description:\n  %s" % group.description
	print "Required groups: "
	for grp in group.groups:
		print "  %s" % grp
	print "Packages: "
	for pkg in grouppkgs(comps, grpname, recursive=0, showall=showall):
		print "  %s" % pkg
		
def usage():
	print "Usage: %s [-p <path>] [-h] showgroups" % sys.argv[0]
	print "       %s [-p <path>] [-a] showgroup <group>" % sys.argv[0]
	print "       %s [-p <path>] [-a] [-r] grouppkgs <group>" % sys.argv[0]
	sys.exit(1)

if __name__ == "__main__":
	import getopt

	recursive = 0
	showhidden = 0
	showall = 0
	comps = None
	compspath = "/usr/share/comps/i386/comps.xml"
	
	try:
		optlist, args = getopt.getopt(sys.argv[1:], 'arhp:')
	except getopt.error:
		usage()

	for opt, arg in optlist:
		if opt == '-r':
			recursive = 1
		if opt == '-h':
			showhidden = 1
		if opt == '-a':
			showall = 1
		if opt == '-p':
			compspath = arg

	if len(args) < 1:
		usage()

	try:
		comps = rhpl.comps.Comps(compspath)
	except:
		print "Unable to open %s!" % compspath
		sys.exit(1)

	if args[0] == "groupnames":
		groupnames(comps, showhidden)
	elif len(args) < 2:
		usage()
	elif args[0] == "grouppkgs":
		for grp in args[1:]:
			for pkg in grouppkgs(comps, grp, recursive, showall):
				print pkg
	elif args[0] == "showgroup":
		for grp in args[1:]:
			showgroup(comps, grp, showall)
	else:
		usage()
	
# vim:ts=4
