#include <Python.h>
#include <sched.h>
#include <errno.h>
#include <syscall.h>

#ifndef __unused
#define __unused __attribute__ ((unused))
#endif

#ifndef CPU_ALLOC

/* From glibc-headers-2.12-1.7.el6.x86_64 */

/* Basic access functions.  */
#define __CPUELT(cpu)  ((cpu) / __NCPUBITS)
#define __CPUMASK(cpu) ((__cpu_mask) 1 << ((cpu) % __NCPUBITS))

#define CPU_ALLOC_SIZE(ncpus) \
	((((ncpus) + __NCPUBITS - 1) / __NCPUBITS) * sizeof (__cpu_mask))

/* Access functions for CPU masks.  */
# if __GNUC_PREREQ (2, 91)
#  define __CPU_ZERO_S(setsize, cpusetp) \
  do __builtin_memset (cpusetp, '\0', setsize); while (0)
# else
#  define __CPU_ZERO_S(setsize, cpusetp) \
  do {                                                                        \
    size_t __i;                                                               \
    size_t __imax = (setsize) / sizeof (__cpu_mask);                          \
    __cpu_mask *__bits = (cpusetp)->__bits;                                   \
    for (__i = 0; __i < __imax; ++__i)                                        \
      __bits[__i] = 0;                                                        \
  } while (0)
# endif

#define CPU_FREE(cpus)  free(cpus)
#define CPU_ALLOC(ncpus) malloc(CPU_ALLOC_SIZE(ncpus))

# define __CPU_SET_S(cpu, setsize, cpusetp) \
  (__extension__                                                              \
   ({ size_t __cpu = (cpu);                                                   \
      __cpu < 8 * (setsize)                                                   \
      ? (((__cpu_mask *) ((cpusetp)->__bits))[__CPUELT (__cpu)]               \
         |= __CPUMASK (__cpu))                                                \
      : 0; }))

# define __CPU_ISSET_S(cpu, setsize, cpusetp) \
  (__extension__                                                              \
   ({ size_t __cpu = (cpu);                                                   \
      __cpu < 8 * (setsize)                                                   \
      ? ((((__const __cpu_mask *) ((cpusetp)->__bits))[__CPUELT (__cpu)]      \
          & __CPUMASK (__cpu))) != 0                                          \
      : 0; }))

#define CPU_SET_S(cpu, setsize, cpusetp) __CPU_SET_S (cpu, setsize, cpusetp)
#define CPU_ISSET_S(cpu, setsize, cpusetp) __CPU_ISSET_S (cpu, setsize, cpusetp)
#define CPU_ZERO_S(setsize, cpusetp) __CPU_ZERO_S (setsize, cpusetp)

#endif /* CPU_ALLOC */


#ifndef SCHED_RESET_ON_FORK
#define SCHED_RESET_ON_FORK 0x40000000
#endif

/*
 * The following bitmask declarations, bitmask_*() routines, and associated
 * _setbit() and _getbit() routines are:
 * Copyright (c) 2004 Silicon Graphics, Inc. (SGI) All rights reserved.
 * SGI publishes it under the terms of the GNU General Public License, v2,
 * as published by the Free Software Foundation.
 */
#define howmany(x,y) (((x)+((y)-1))/(y))
#define bitsperlong (8 * sizeof(unsigned long))
#define longsperbits(n) howmany(n, bitsperlong)
#define bytesperbits(x) ((x+7)/8)

/*
 * Number of bits in a CPU bitmask on current system
 */
static int __get_max_number_of_cpus(void)
{
	int n;
	int cpus = 2048;

	for (;;) {
		unsigned long buffer[longsperbits(cpus)];
		memset(buffer, 0, sizeof(buffer));
		/* the library version does not return size of cpumask_t */
		n = syscall(SYS_sched_getaffinity, 0, bytesperbits(cpus), &buffer);
		if (n < 0 && errno == EINVAL && cpus < 1024 * 1024) {
			cpus *= 2;
			continue;
		}
		return n * 8;
	}
	return -1;
}

static PyObject *get_max_number_of_cpus(PyObject *self __unused, PyObject *args __unused)
{
	int ret = __get_max_number_of_cpus();

	if (ret < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	return Py_BuildValue("i", ret);
}

static PyObject *get_affinity(PyObject *self __unused, PyObject *args)
{
	PyObject *list = NULL;
	cpu_set_t *cpus;
	int pid, cpu;
	size_t cpusetsize;
	int max_cpus;

	if (!PyArg_ParseTuple(args, "i", &pid))
		goto out_error;

	max_cpus = __get_max_number_of_cpus();
	if (max_cpus < 0)
		goto out_error;

	cpus = CPU_ALLOC(max_cpus);
	if (cpus == NULL)
		return PyErr_NoMemory();

	cpusetsize = CPU_ALLOC_SIZE(max_cpus);
	CPU_ZERO_S(cpusetsize, cpus);

	if (sched_getaffinity(pid, cpusetsize, cpus) < 0)
		goto out_free;

	list = PyList_New(0);
	for (cpu = 0; cpu < max_cpus; ++cpu)
		if (CPU_ISSET_S(cpu, cpusetsize, cpus))
			PyList_Append(list, Py_BuildValue("i", cpu));

	CPU_FREE(cpus);

	return list;
out_free:
	CPU_FREE(cpus);
out_error:
	return PyErr_SetFromErrno(PyExc_OSError);
}

static PyObject *set_affinity(PyObject *self __unused, PyObject *args)
{
	int pid, nr_elements, i, max_cpus;
	cpu_set_t *cpus;
	PyObject *list;
	size_t cpusetsize;

	if (!PyArg_ParseTuple(args, "iO", &pid, &list))
		goto out_error;

	max_cpus = __get_max_number_of_cpus();
	if (max_cpus < 0)
		goto out_error;

	cpus = CPU_ALLOC(max_cpus);
	if (cpus == NULL)
		return PyErr_NoMemory();

	cpusetsize = CPU_ALLOC_SIZE(max_cpus);
	CPU_ZERO_S(cpusetsize, cpus);

	nr_elements = PyList_Size(list);
	for (i = 0; i < nr_elements; ++i) {
		int cpu = PyInt_AsLong(PyList_GetItem(list, i));

		if (cpu >= max_cpus) {
			PyErr_SetString(PyExc_OSError, "Invalid CPU");
			CPU_FREE(cpus);
			return NULL;
		}
		CPU_SET_S(cpu, cpusetsize, cpus);
	}

	i = sched_setaffinity(pid, cpusetsize, cpus);
	CPU_FREE(cpus);

	if (i < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_INCREF(Py_None);
	return Py_None;
out_error:
	return PyErr_SetFromErrno(PyExc_OSError);
}

static PyObject *get_scheduler(PyObject *self __unused, PyObject *args)
{
	int pid, scheduler;

	if (!PyArg_ParseTuple(args, "i", &pid))
		return NULL;

	scheduler = sched_getscheduler(pid);
	if (scheduler < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	return Py_BuildValue("i", scheduler);
}

static PyObject *set_scheduler(PyObject *self __unused, PyObject *args)
{
	int pid, policy, priority;
	struct sched_param param;

	if (!PyArg_ParseTuple(args, "iii", &pid, &policy, &priority))
		return NULL;

	memset(&param, 0, sizeof(param));
	param.sched_priority = priority;

	if (sched_setscheduler(pid, policy, &param) < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *get_priority(PyObject *self __unused, PyObject *args)
{
	int pid;
	struct sched_param param = { .sched_priority = -1, };

	if (!PyArg_ParseTuple(args, "i", &pid))
		return NULL;

	if (sched_getparam(pid, &param) != 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	return Py_BuildValue("i", param.sched_priority);
}

#ifndef SCHED_BATCH
#define SCHED_BATCH             3
#endif
#ifndef SCHED_IDLE
#define SCHED_IDLE              5
#endif
#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE          6
#endif


static PyObject *schedstr(PyObject *self __unused, PyObject *args)
{
	int scheduler;
	char *s;

	if (!PyArg_ParseTuple(args, "i", &scheduler))
		return NULL;

	switch (scheduler & ~SCHED_RESET_ON_FORK) {
	case SCHED_OTHER: s = "SCHED_OTHER"; break;
	case SCHED_RR:	  s = "SCHED_RR";    break;
	case SCHED_FIFO:  s = "SCHED_FIFO";  break;
	case SCHED_BATCH: s = "SCHED_BATCH"; break;
	case SCHED_IDLE:  s = "SCHED_IDLE";  break;
        case SCHED_DEADLINE:  s = "SCHED_DEADLINE";  break;
	default:	  s = "UNKNOWN";     break;
	}

	return PyString_FromString(s);
}

static PyObject *schedfromstr(PyObject *self __unused, PyObject *args)
{
	int scheduler;
	char *s;

	if (!PyArg_ParseTuple(args, "s", &s))
		return NULL;

	if (strcmp(s, "SCHED_OTHER") == 0)
		scheduler = SCHED_OTHER;
	else if (strcmp(s, "SCHED_RR") == 0)
		scheduler = SCHED_RR;
	else if (strcmp(s, "SCHED_FIFO") == 0)
		scheduler = SCHED_FIFO;
	else if (strcmp(s, "SCHED_BATCH") == 0)
		scheduler = SCHED_BATCH;
	else if (strcmp(s, "SCHED_IDLE") == 0)
		scheduler = SCHED_IDLE;
        else if (strcmp(s, "SCHED_DEADLINE") == 0)
                scheduler = SCHED_DEADLINE;
	else {
		PyErr_SetString(PyExc_OSError, "Unknown scheduler");
		return NULL;
	}

	return Py_BuildValue("i", scheduler);
}

static PyObject *get_priority_min(PyObject *self __unused, PyObject *args)
{
	int policy, min;

	if (!PyArg_ParseTuple(args, "i", &policy))
		return NULL;

	min = sched_get_priority_min(policy);
	if (min < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	return Py_BuildValue("i", min);
}

static PyObject *get_priority_max(PyObject *self __unused, PyObject *args)
{
	int policy, max;

	if (!PyArg_ParseTuple(args, "i", &policy))
		return NULL;

	max = sched_get_priority_max(policy);
	if (max < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	return Py_BuildValue("i", max);
}

static struct PyMethodDef PySchedutilsModuleMethods[] = {
	{
		.ml_name = "get_affinity",
		.ml_meth = (PyCFunction)get_affinity,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_affinity",
		.ml_meth = (PyCFunction)set_affinity,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "get_scheduler",
		.ml_meth = (PyCFunction)get_scheduler,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_scheduler",
		.ml_meth = (PyCFunction)set_scheduler,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "get_priority",
		.ml_meth = (PyCFunction)get_priority,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "schedstr",
		.ml_meth = (PyCFunction)schedstr,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "schedfromstr",
		.ml_meth = (PyCFunction)schedfromstr,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "get_priority_min",
		.ml_meth = (PyCFunction)get_priority_min,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "get_priority_max",
		.ml_meth = (PyCFunction)get_priority_max,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "get_max_number_of_cpus",
		.ml_meth = (PyCFunction)get_max_number_of_cpus,
	},
	{	.ml_name = NULL, },
};

PyMODINIT_FUNC initschedutils(void)
{
	PyObject *m = Py_InitModule("schedutils", PySchedutilsModuleMethods);
	if (m == NULL)
		return;

	PyModule_AddIntConstant(m, "SCHED_OTHER", SCHED_OTHER);
	PyModule_AddIntConstant(m, "SCHED_FIFO", SCHED_FIFO);
	PyModule_AddIntConstant(m, "SCHED_RR", SCHED_RR);
	PyModule_AddIntConstant(m, "SCHED_BATCH", SCHED_BATCH);
	PyModule_AddIntConstant(m, "SCHED_IDLE", SCHED_IDLE);
        PyModule_AddIntConstant(m, "SCHED_DEADLINE", SCHED_DEADLINE);
	PyModule_AddIntConstant(m, "SCHED_RESET_ON_FORK", SCHED_RESET_ON_FORK);
}

