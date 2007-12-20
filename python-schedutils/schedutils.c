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
		PyErr_SetString(PyExc_OSError, strerror(errno));
		return NULL;
	}

	list = PyList_New(0);
	for (cpu = 0; cpu < CPU_SETSIZE; ++cpu)
		if (CPU_ISSET(cpu, &cpus))
			PyList_Append(list, Py_BuildValue("i", cpu));

	return list;
}

static PyObject *get_scheduler(PyObject *self __unused, PyObject *args)
{
	int pid, scheduler;

	if (!PyArg_ParseTuple(args, "i", &pid))
		return NULL;

	scheduler = sched_getscheduler(pid);
	if (scheduler < 0) {
		PyErr_SetString(PyExc_OSError, strerror(errno));
		return NULL;
	}

	return Py_BuildValue("i", scheduler);
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

static struct PyMethodDef PySchedutilsModuleMethods[] = {
	{
		.ml_name = "get_affinity",
		.ml_meth = (PyCFunction)get_affinity,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "get_scheduler",
		.ml_meth = (PyCFunction)get_scheduler,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "schedstr",
		.ml_meth = (PyCFunction)schedstr,
		.ml_flags = METH_VARARGS,
	},
	{	.ml_name = NULL, },
};

PyMODINIT_FUNC initschedutils(void)
{
	Py_InitModule("schedutils", PySchedutilsModuleMethods);
}

