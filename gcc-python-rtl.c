/*
   Copyright 2011, 2012 David Malcolm <dmalcolm@redhat.com>
   Copyright 2011, 2012 Red Hat, Inc.

   This is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.
*/

#include <Python.h>
#include "gcc-python.h"
#include "gcc-python-wrappers.h"
#include "gcc-python-compat.h"
#include "rtl.h"
#include "tree-flow.h"
#include "tree-flow-inline.h"

PyObject *
gcc_Rtl_get_location(struct PyGccRtl *self, void *closure)
{
    int locator = INSN_LOCATOR (self->insn);
    if (locator && insn_file (self->insn)) {
        return gcc_python_make_wrapper_location(locator_location(locator));
    }

    Py_RETURN_NONE;
}

PyObject *
get_operand_as_object(const_rtx in_rtx, int idx, char fmt)
{
    const char *str;

    /* The operand types are described in gcc/rtl.c */
    switch (fmt) {

    case 'T': /* pointer to a string, with special meaning */
        str = XTMPL (in_rtx, idx);
        goto string;

    case 'S': /* optional pointer to a string */
    case 's': /* a pointer to a string */
        str = XSTR (in_rtx, idx);
    string:
        return gcc_python_string_or_none(str);

    case '0': /* unused, or used in a phase-dependent manner */
        Py_RETURN_NONE; /* for now */

    case 'e': /* pointer to an rtl expression */
        /* Nested expression: */
        return gcc_python_make_wrapper_rtl(XEXP (in_rtx, idx));

    case 'E':
    case 'V':
        /* Nested list of expressions */
        {
            PyObject *list = PyList_New(XVECLEN (in_rtx, idx));
            int j;
            if (!list) {
                return NULL;
            }
            for (j = 0; j < XVECLEN (in_rtx, idx); j++) {
                PyObject *item = gcc_python_make_wrapper_rtl(XVECEXP (in_rtx, idx, j));
                if (!item) {
                    Py_DECREF(list);
                    return NULL;
                }
                if (-1 == PyList_Append(list, item)) {
                    Py_DECREF(item);
                    Py_DECREF(list);
                    return NULL;
                }
                Py_DECREF(item);
            }
            return list;
        }

    case 'w':
        return gcc_python_int_from_long(XWINT (in_rtx, idx));

    case 'i':
        return gcc_python_int_from_long(XINT (in_rtx, idx));

    case 'n':
        /* Return NOTE_INSN names rather than integer codes.  */
        return gcc_python_string_or_none(GET_NOTE_INSN_NAME (XINT (in_rtx, idx)));

    case 'u': /* a pointer to another insn */
        Py_RETURN_NONE; /* for now */

    case 't':
        return gcc_python_make_wrapper_tree(XTREE (in_rtx, idx));

    case '*':
        Py_RETURN_NONE; /* for now */

    case 'B':
        return gcc_python_make_wrapper_basic_block(XBBDEF (in_rtx, idx));

    default:
        gcc_unreachable ();
    }
}

PyObject *
gcc_Rtl_get_operands(struct PyGccRtl *self, void *closure)
{
    const int length = GET_RTX_LENGTH (GET_CODE (self->insn));
    PyObject *result;
    int i;
    const char *format_ptr;

    result = PyTuple_New(length);
    if (!result) {
        return NULL;
    }

    format_ptr = GET_RTX_FORMAT (GET_CODE (self->insn));
    for (i = 0; i < length; i++) {
        PyObject *item = get_operand_as_object(self->insn, i, *format_ptr++);
        if (!item) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, i, item);
    }

    return result;
}

PyObject *
gcc_Rtl_repr(struct PyGccRtl * self)
{
    return gcc_python_string_from_format("%s()",
                                         Py_TYPE(self)->tp_name);
}

PyObject *
gcc_Rtl_str(struct PyGccRtl * self)
{
    /*
      We use print_rtl_single() which takes a FILE*, so
      we need an fmemopen buffer to write to:
    */
    char buf[2048]; /* FIXME */
    FILE *f;

    buf[0] = '\0';
    f = fmemopen(buf, sizeof(buf), "w");
    if (!f) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    print_rtl_single (f, self->insn);

    fclose(f);

    return gcc_python_string_from_string(buf);
}

PyObject*
gcc_python_make_wrapper_rtl(struct rtx_def *insn)
{
    struct PyGccRtl *rtl_obj = NULL;
    PyGccWrapperTypeObject* tp;

    if (!insn) {
        Py_RETURN_NONE;
    }

    tp = gcc_python_autogenerated_rtl_type_for_stmt(insn);
    assert(tp);

    rtl_obj = PyGccWrapper_New(struct PyGccRtl, tp);
    if (!rtl_obj) {
        goto error;
    }
    rtl_obj->insn = insn;

    return (PyObject*)rtl_obj;

error:
    return NULL;
}

void
wrtp_mark_for_PyGccRtl(PyGccRtl *wrapper)
{
    /* Mark the underlying object (recursing into its fields): */
    gt_ggc_mx_rtx_def(wrapper->insn);
}

/*
  PEP-7
Local variables:
c-basic-offset: 4
indent-tabs-mode: nil
End:
*/
