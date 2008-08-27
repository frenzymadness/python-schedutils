#! /usr/bin/python
# -*- python -*-
# -*- coding: utf-8 -*-
#   Copyright (C) 2008 Red Hat Inc.
#
#   Arnaldo Carvalho de Melo <acme@redhat.com>
#
#   This application is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   as published by the Free Software Foundation; version 2.
#
#   This application is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.

import os, schedutils, sys

def usage():
	print '''pchrt (python-schedutils)
usage: chrt [options] [prio] [pid | cmd [args...]]
manipulate real-time attributes of a process
  -b, --batch                        set policy to SCHED_BATCH
  -f, --fifo                         set policy to SCHED_FIFO
  -p, --pid                          operate on existing given pid
  -m, --max                          show min and max valid priorities
  -o, --other                        set policy to SCHED_OTHER
  -r, --rr                           set policy to SCHED_RR (default)
  -h, --help                         display this help

You must give a priority if changing policy.

Report bugs and send patches to <acme@ghostprotocols.net>'''
	return

def show_priority_limits(policy):
	print "%-32.32s: %d/%d" % ("%s min/max priority" % schedutils.schedstr(policy),
				   schedutils.get_priority_min(policy),
				   schedutils.get_priority_max(policy))

def show_all_priority_limits():
	for policy in (schedutils.SCHED_OTHER, schedutils.SCHED_FIFO,
		       schedutils.SCHED_RR, schedutils.SCHED_BATCH):
		show_priority_limits(policy)

def show_settings(pid):
	policy = schedutils.get_scheduler(pid)
	spolicy = schedutils.schedstr(policy)
	rtprio = schedutils.get_priority(pid)
	print '''pid %d's current scheduling policy: %s
pid %d's current scheduling priority: %d''' % (pid, spolicy, pid, rtprio)

def change_settings(pid, policy, rtprio):
	try:
		schedutils.set_scheduler(pid, policy, rtprio)
	except SystemError, err:
		print "sched_setscheduler: %s" % err[1]
		print "failed to set pid %d's policy" % pid

def main():

	args = sys.argv[1:]
	if not args:
		usage()
		return

	policy = schedutils.SCHED_RR
	while True:
		o = args.pop(0)
		try:
			priority = int(o)
			break
		except:
			pass

		if o in ("-h", "--help"):
			usage()
			return
		elif o in ("-b", "--batch"):
			policy = schedutils.SCHED_BATCH
		elif o in ("-f", "--fifo"):
			policy = schedutils.SCHED_FIFO
		elif o in ("-m", "--max"):
			show_all_priority_limits()
			return
		elif o in ("-o", "--other"):
			policy = schedutils.SCHED_OTHER
		elif o in ("-r", "--rr"):
			policy = schedutils.SCHED_RR
		elif o in ("-p", "--pid"):
			if len(args) > 1:
				priority = int(args.pop(0))
				pid = int(args.pop(0))
				change_settings(pid, policy, priority)
			else:
				pid = int(args.pop(0))
				show_settings(pid)
			return
		else:
			usage()
			return

	schedutils.set_scheduler(0, policy, priority)
	os.execvp(args[0], args)

if __name__ == '__main__':
    main()
