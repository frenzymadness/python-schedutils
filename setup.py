#!/usr/bin/python2

from distutils.core import setup, Extension

schedutils = Extension('schedutils',
		       sources = ['python-schedutils/schedutils.c'])

# don't reformat this line, Makefile parses it
setup(name='schedutils',
      version='0.1',
      description='Python module to interface with the Linux scheduler',
      author='Arnaldo Carvalho de Melo',
      author_email='acme@redhat.com',
      url='http://fedoraproject.org/wiki/python-schedutils',
      ext_modules=[schedutils])
