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
	print '''ptaskset (python-schedutils)
usage: ptaskset [options] [mask | cpu-list] [pid | cmd [args...]]
set or get the affinity of a process

  -p, --pid                  operate on existing given pid
  -c, --cpu-list             display and specify cpus in list format
  -h, --help                 display this help'''

	return

def hexbitmask(l):
	hexbitmask = []
	bit = 0
	mask = 0
	nr_entries = 1 << max(l)
	for entry in range(nr_entries):
		if entry in l:
			mask |= (1 << bit)
		bit += 1
		if bit == 32:
			bit = 0
			hexbitmask.insert(0, mask)
			mask = 0

	if bit < 32 and mask != 0:
		hexbitmask.insert(0, mask)

	return hexbitmask

def find_last_bit(n, wordsize = 32):
	bit = wordsize - 1
	while bit != 0:
		if n & (1 << bit):
			return bit
		bit -= 1
	return 0

def bitmasklist(line):
	fields = line.strip().split(",")
	bitmasklist = []
	while int(fields[0], 16) == 0:
		fields.pop(0)

	if not fields:
		return []

	nr_entries = (len(fields) - 1) * 32 + find_last_bit(int(fields[0], 16)) + 1

	entry = 0
	for i in range(len(fields) - 1, -1, -1):
		mask = int(fields[i], 16)
		while mask != 0:
			if mask & 1:
				bitmasklist.append(entry)
			mask >>= 1
			entry += 1
			if entry == nr_entries:
				break
		if entry == nr_entries:
			break
	return bitmasklist

def cpustring_to_list(cpustr):
	"""Convert a string of numbers to an integer list.
    
	Given a string of comma-separated numbers and number ranges,
	return a simple sorted list of the integers it represents.

	This function will throw exceptions for badly-formatted strings.
    
	Returns a list of integers."""

	fields = cpustr.strip().split(",")
	cpu_list = []
	for field in fields:
		ends = field.split("-")
		if len(ends) > 2:
			raise "Syntax error"
		if len(ends) == 2:
			cpu_list += range(int(ends[0]), int(ends[1])+1)
		else:
			cpu_list += [int(ends[0])]
	return list(set(cpu_list))

def show_settings(pid, when, cpu_list_mode):
	affinity = schedutils.get_affinity(pid)
	if cpu_list_mode:
		mask = ",".join([str(a) for a in affinity])
	else:
		mask = ",".join(["%x" % a for a in hexbitmask(affinity)])
	print "pid %d's %s affinity mask: %s" % (pid, when, mask)

def change_settings(pid, affinity, cpu_list_mode):
	if cpu_list_mode:
		try:
			affinity = [ int(a) for a in affinity.split(",") ]
		except:
			affinity = cpustring_to_list(affinity)
	else:
		affinity = bitmasklist(affinity)

	try:
		schedutils.set_affinity(pid, affinity)
	except SystemError, err:
		print "sched_setaffinity: %s" % err[1]
		print "failed to set pid %d's affinity" % pid

def main():

	args = sys.argv[1:]
	if not args:
		usage()
		return

	cpu_list_mode = False
	while True:
		o = args.pop(0)

		if o in ("-h", "--help"):
			usage()
			return
		elif o in ("-c", "--cpu-list"):
			cpu_list_mode = True
		elif o in ("-p", "--pid"):
			if len(args) > 1:
				affinity = args.pop(0)
				pid = int(args.pop(0))
				show_settings(pid, "current", cpu_list_mode)
				change_settings(pid, affinity, cpu_list_mode)
				show_settings(pid, "new", cpu_list_mode)
			else:
				pid = int(args.pop(0))
				show_settings(pid, "current", cpu_list_mode)
			return
		else:
			break

	affinity = o
	change_settings(0, affinity, cpu_list_mode)
	os.execvp(args[0], args)

if __name__ == '__main__':
    main()
