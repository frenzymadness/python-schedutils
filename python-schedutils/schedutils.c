#include <Python.h>
#include <sched.h>
#include <errno.h>

#ifndef __unused
#define __unused __attribute__ ((unused))
#endif

static PyObject *get_affinity(PyObject *self __unused, PyObject *args)
{
	PyObject *list;
	cpu_set_t cpus;
	int pid, cpu;

	if (!PyArg_ParseTuple(args, "i", &pid))
		return NULL;

	CPU_ZERO(&cpus);

	if (sched_getaffinity(pid, sizeof(cpus), &cpus) < 0) {
		PyErr_SetFromErrno(PyExc_SystemError);
		return NULL;
	}

	list = PyList_New(0);
	for (cpu = 0; cpu < CPU_SETSIZE; ++cpu)
		if (CPU_ISSET(cpu, &cpus))
			PyList_Append(list, Py_BuildValue("i", cpu));

	return list;
}

static PyObject *set_affinity(PyObject *self __unused, PyObject *args)
{
	int pid, nr_elements, i;
	cpu_set_t cpus;
	PyObject *list;

	if (!PyArg_ParseTuple(args, "iO", &pid, &list))
		return NULL;

	CPU_ZERO(&cpus);

	nr_elements = PyList_Size(list);
	for (i = 0; i < nr_elements; ++i) {
		int cpu = PyInt_AsLong(PyList_GetItem(list, i));

		if (cpu >= CPU_SETSIZE) {
			PyErr_SetString(PyExc_SystemError, "Invalid CPU");
			return NULL;
		}
		CPU_SET(cpu, &cpus);
	}

	if (sched_setaffinity(pid, sizeof(cpus), &cpus) < 0) {
		PyErr_SetFromErrno(PyExc_SystemError);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *get_scheduler(PyObject *self __unused, PyObject *args)
{
	int pid, scheduler;

	if (!PyArg_ParseTuple(args, "i", &pid))
		return NULL;

	scheduler = sched_getscheduler(pid);
	if (scheduler < 0) {
		PyErr_SetFromErrno(PyExc_SystemError);
		return NULL;
	}

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

	if (sched_setscheduler(pid, policy, &param) < 0) {
		PyErr_SetFromErrno(PyExc_SystemError);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *get_priority(PyObject *self __unused, PyObject *args)
{
	int pid;
	struct sched_param param = { .sched_priority = -1, };

	if (!PyArg_ParseTuple(args, "i", &pid))
		return NULL;

	if (sched_getparam(pid, &param) != 0) {
		PyErr_SetFromErrno(PyExc_SystemError);
		return NULL;
	}

	return Py_BuildValue("i", param.sched_priority);
}

static PyObject *schedstr(PyObject *self __unused, PyObject *args)
{
	int scheduler;
	char *s;

	if (!PyArg_ParseTuple(args, "i", &scheduler))
		return NULL;

	switch (scheduler) {
	case SCHED_OTHER: s = "SCHED_OTHER"; break;
	case SCHED_RR:	  s = "SCHED_RR";    break;
	case SCHED_FIFO:  s = "SCHED_FIFO";  break;
	case SCHED_BATCH: s = "SCHED_BATCH"; break;
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
	else {
		PyErr_SetString(PyExc_SystemError, "Unknown scheduler");
		return NULL;
	}

	return Py_BuildValue("i", scheduler);
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
}

