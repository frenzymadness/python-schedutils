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
usage: pchrt [options] [prio] [pid | cmd [args...]]
manipulate real-time attributes of a process
  -b, --batch                        set policy to SCHED_BATCH
  -f, --fifo                         set policy to SCHED_FIFO
  -i, --idle                         set policy to SCHED_IDLE
  -p, --pid                          operate on existing given pid
  -m, --max                          show min and max valid priorities
  -o, --other                        set policy to SCHED_OTHER
  -r, --rr                           set policy to SCHED_RR (default)
  -R, --reset-on-fork                set SCHED_RESET_ON_FORK for FIFO or RR
  -h, --help                         display this help

You must give a priority if changing policy.

Report bugs and send patches to <tuna-devel@lists.fedorahosted.org>'''
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
	reset_on_fork = ""
	if policy & schedutils.SCHED_RESET_ON_FORK:
		reset_on_fork = "|SCHED_RESET_ON_FORK"
	print '''pid %d's current scheduling policy: %s%s
pid %d's current scheduling priority: %d''' % (pid, spolicy, reset_on_fork, pid, rtprio)

def valid_policy_flag(policy, policy_flag):
	if policy_flag == schedutils.SCHED_RESET_ON_FORK and \
	   policy not in (schedutils.SCHED_RR, schedutils.SCHED_FIFO):
		print "SCHED_RESET_ON_FORK flag is supported for SCHED_FIFO and SCHED_RR policies only"
		return False
	return True

def change_settings(pid, policy, policy_flag, rtprio):
	try:
		schedutils.set_scheduler(pid, policy | policy_flag, rtprio)
	except SystemError, err:
		print "sched_setscheduler: %s" % err[1]
		print "failed to set pid %d's policy" % pid

def main():

	args = sys.argv[1:]
	if not args:
		usage()
		return

	policy = schedutils.SCHED_RR
	policy_flag = 0
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
		elif o in ("-i", "--idle"):
			policy = schedutils.SCHED_IDLE
		elif o in ("-m", "--max"):
			show_all_priority_limits()
			return
		elif o in ("-o", "--other"):
			policy = schedutils.SCHED_OTHER
		elif o in ("-r", "--rr"):
			policy = schedutils.SCHED_RR
		elif o in ("-R", "--reset-on-fork"):
			policy_flag |= schedutils.SCHED_RESET_ON_FORK
		elif o in ("-p", "--pid"):
			if len(args) > 1:
				priority = int(args.pop(0))
				pid = int(args.pop(0))
				if not valid_policy_flag(policy, policy_flag):
					return
				change_settings(pid, policy, policy_flag, priority)
			else:
				pid = int(args.pop(0))
				show_settings(pid)
			return
		else:
			usage()
			return

	if not valid_policy_flag(policy, policy_flag):
		return

	schedutils.set_scheduler(0, policy | policy_flag, priority)
	os.execvp(args[0], args)

if __name__ == '__main__':
    main()
