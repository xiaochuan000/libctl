/* libctl: flexible Guile-based control files for scientific software 
 * Copyright (C) 1998 Steven G. Johnson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * Steven G. Johnson can be contacted at stevenj@alum.mit.edu.
 */

#include <stdlib.h>
#include <string.h>

#include "ctl.h"

/* This file contains glue code that enables us to call libctl from
   Fortran.  We have to take into account several things:

   1) All Fortran parameters are passed by reference.

   2) Fortran compilers are case-insensitive, so they munge identifiers
   in a weird way for the linker.  If we want a Fortran program to
   be able to call us, we have to munge our identifiers in the same
   way.  (We do this with the FORTRANIZE macro--every Fortran compiler
   is different.)

   3) Fortran represents strings in a different way than C.  To handle
   this, we require that Fortran callers pass us the length of a
   string as an explicit parameter.  We also have to include ugly
   hacks to accomodate the fact that Cray Fortran compilers pass
   a data structure instead of a char* for string parameters. 

   4) On some machines, C functions return their results in a way
   that the Fortran compiler can't handle.  To get around this,
   all return results of functions are converted into an extra parameter.

   The name of our Fortran routines is the same as the corresponding
   C routine with the underscores removed.  So, we to construct the
   Fortran call, you do something like:

   C:          foo = bar_baz(x,y,z);
   Fortran:    call barbaz(x,y,z,foo)

   C:          foo = bar_baz(x,string,y);
   Fortran:    call barbaz(x,string,length(string),y,foo)

   (Note that string parameters get converted into two parameters: the
   string and its length.)
*/

/**************************************************************************/

/* Macro to munge identifiers in a system-specific way: */

#if defined(CRAY) || defined(_UNICOS) || defined(_CRAYMPP)
#define FORTRANIZE(x,X) X /* all upper-case on the Cray */

#elif defined(IBM6000) || defined(_AIX)
#define FORTRANIZE(x,X) x  /* all lower-case on RS/6000 */

#elif defined(__hpux)
#define FORTRANIZE(x,X) x  /* all lower-case on HP-UX */

#else
#define FORTRANIZE(x,X) x##_  /* use all lower-case with underscore
                                 by default */
#endif

/**************************************************************************/

/* Convert Fortran string parameters to C char*.  This is required
   in order to accomodate the ugly things that the Cray compilers do. */

#if defined(CRAY) || defined(_UNICOS) || defined(_CRAYMPP)
#include <fortran.h>
typedef _fcd fortran_string;
#define fcp2ccp(fs) _fcdtocp(fs)
#else
typedef char *fortran_string;
#define fcp2ccp(fs) (fs)
#endif

/**************************************************************************/

/* Vector functions:
   (vector3 can be declared as an array of 3 reals in Fortran) */

void FORTRANIZE(vector3scale,VECTOR3SCALE)
     (number *s, vector3 *v, vector3 *vscaled)
{
  *vscaled = vector3_scale(*s,*v);
}

void FORTRANIZE(vector3plus,VECTOR3PLUS)
     (vector3 *v1, vector3 *v2, vector3 *vresult)
{
  *vresult = vector3_plus(*v1,*v2);
}

void FORTRANIZE(vector3minus,VECTOR3MINUS)
     (vector3 *v1, vector3 *v2, vector3 *vresult)
{
  *vresult = vector3_minus(*v1,*v2);
}

void FORTRANIZE(vector3cross,VECTOR3CROSS)
     (vector3 *v1, vector3 *v2, vector3 *vresult)
{
  *vresult = vector3_cross(*v1,*v2);
}

void FORTRANIZE(vector3dot,VECTOR3DOT)
     (vector3 *v1, vector3 *v2, number *result)
{
  *result = vector3_dot(*v1,*v2);
}

void FORTRANIZE(vector3norm,VECTOR3DOT)
     (vector3 *v, number *result)
{
  *result = vector3_norm(*v);
}

/**************************************************************************/

/* variable get/set functions */

/* Note that list and object variables in Fortran should be declared
   as something the same size as the corresponding type in C.  (This
   turns out to be the same size as a long int.) */

/* Getters: */

void FORTRANIZE(ctlgetnumber,CTLGETNUMBER)
     (fortran_string identifier, int *length, number *result)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  *result = ctl_get_number(s);
}

void FORTRANIZE(ctlgetinteger,CTLGETINTEGER)
     (fortran_string identifier, int *length, integer *result)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  *result = ctl_get_integer(s);
}

void FORTRANIZE(ctlgetboolean,CTLGETBOOLEAN)
     (fortran_string identifier, int *length, boolean *result)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  *result = ctl_get_boolean(s);
}

void FORTRANIZE(ctlgetlist,CTLGETLIST)
     (fortran_string identifier, int *length, list *result)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  *result = ctl_get_list(s);
}

void FORTRANIZE(ctlgetobject,CTLGETOBJECT)
     (fortran_string identifier, int *length, object *result)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  *result = ctl_get_object(s);
}

void FORTRANIZE(ctlgetvector3,CTLGETVECTOR3)
     (fortran_string identifier, int *length, vector3 *result)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  *result = ctl_get_vector3(s);
}

/* ctl_get_string doesn't work perfectly--there
   is no portable way to set the length of the Fortran string.
   The length is returned in result_length. */
void FORTRANIZE(ctlgetstring,CTLGETSTRING)
     (fortran_string identifier, int *length,
      fortran_string result, int *result_length)
{
  char *r;
  char *s = fcp2ccp(identifier); s[*length] = 0;
  r = ctl_get_string(s);
  strncpy(fcp2ccp(result), r, *result_length);
  if (*result_length < strlen(r))
    *result_length = strlen(r);
  free(r);
}

/* Setters: */

void FORTRANIZE(ctlsetnumber,CTLSETNUMBER)
     (fortran_string identifier, int *length, number *value)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  ctl_set_number(s, *value);
}

void FORTRANIZE(ctlsetinteger,CTLSETINTEGER)
     (fortran_string identifier, int *length, integer *value)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  ctl_set_integer(s, *value);
}

void FORTRANIZE(ctlsetboolean,CTLSETBOOLEAN)
     (fortran_string identifier, int *length, boolean *value)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  ctl_set_boolean(s, *value);
}

void FORTRANIZE(ctlsetlist,CTLSETLIST)
     (fortran_string identifier, int *length, list *value)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  ctl_set_list(s, *value);
}

void FORTRANIZE(ctlsetobject,CTLSETOBJECT)
     (fortran_string identifier, int *length, object *value)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  ctl_set_object(s, *value);
}

void FORTRANIZE(ctlsetvector3,CTLSETVECTOR3)
     (fortran_string identifier, int *length, vector3 *value)
{
  char *s = fcp2ccp(identifier); s[*length] = 0;
  ctl_set_vector3(s, *value);
}

void FORTRANIZE(ctlsetstring,CTLSETSTRING)
     (fortran_string identifier, int *length,
      fortran_string value, int *value_length)
{
  char *s = fcp2ccp(identifier); 
  char *v = fcp2ccp(value);
  s[*length] = 0;
  v[*value_length] = 0;
  ctl_set_string(s, v);
}

/**************************************************************************/

/* list traversal */

void FORTRANIZE(listlength,LISTLENGTH)(list *l, int *len)
{
  *len = list_length(*l);
}

void FORTRANIZE(numberlistref,NUMBERLISTREF)
     (list *l, int *index, number *value)
{
  *value = number_list_ref(*l, *index);
}

void FORTRANIZE(integerlistref,INTEGERLISTREF)
     (list *l, int *index, integer *value)
{
  *value = integer_list_ref(*l, *index);
}

void FORTRANIZE(booleanlistref,BOOLEANLISTREF)
     (list *l, int *index, boolean *value)
{
  *value = boolean_list_ref(*l, *index);
}

void FORTRANIZE(vector3listref,VECTOR3LISTREF)
     (list *l, int *index, vector3 *value)
{
  *value = vector3_list_ref(*l, *index);
}

void FORTRANIZE(listlistref,LISTLISTREF)
     (list *l, int *index, list *value)
{
  *value = list_list_ref(*l, *index);
}

void FORTRANIZE(objectlistref,OBJECTLISTREF)
     (list *l, int *index, object *value)
{
  *value = object_list_ref(*l, *index);
}

void FORTRANIZE(stringlistref,STRINGLISTREF)
     (list *l, int *index, fortran_string value, int *value_length)
{
  char *v;
  v = string_list_ref(*l, *index);
  strncpy(fcp2ccp(value), v, *value_length);
  if (*value_length < strlen(v))
    *value_length = strlen(v);
  free(v);
}

/**************************************************************************/

/* list creation */

void FORTRANIZE(makenumberlist,MAKENUMBERLIST)
     (int *num_items, number *items, list *result)
{
  *result = make_number_list(*num_items, items);
}

void FORTRANIZE(makeintegerlist,MAKEINTEGERLIST)
     (int *num_items, integer *items, list *result)
{
  *result = make_integer_list(*num_items, items);
}

void FORTRANIZE(makebooleanlist,MAKEBOOLEANLIST)
     (int *num_items, boolean *items, list *result)
{
  *result = make_boolean_list(*num_items, items);
}

void FORTRANIZE(makevector3list,MAKEVECTOR3LIST)
     (int *num_items, vector3 *items, list *result)
{
  *result = make_vector3_list(*num_items, items);
}

void FORTRANIZE(makelistlist,MAKELISTLIST)
     (int *num_items, list *items, list *result)
{
  *result = make_list_list(*num_items, items);
}

void FORTRANIZE(makeobjectlist,MAKEOBJECTLIST)
     (int *num_items, object *items, list *result)
{
  *result = make_object_list(*num_items, items);
}

/* make_string_list is not supported.  Strings in Fortran suck. */

/**************************************************************************/

/* object properties */

void FORTRANIZE(objectismember,OBJECTISMEMBER)
     (fortran_string type_name, int *length, object *o, boolean *result)
{
  char *s = fcp2ccp(type_name); s[*length] = 0; 
  *result = object_is_member(s,*o);
}

void FORTRANIZE(numberobjectproperty,NUMBEROBJECTPROPERTY)
     (object *o, fortran_string property_name, int *length, number *result)
{
  char *s = fcp2ccp(property_name); s[*length] = 0;
  *result = number_object_property(*o,s);
}

void FORTRANIZE(integerobjectproperty,INTEGEROBJECTPROPERTY)
     (object *o, fortran_string property_name, int *length, integer *result)
{
  char *s = fcp2ccp(property_name); s[*length] = 0;
  *result = integer_object_property(*o,s);
}

void FORTRANIZE(booleanobjectproperty,BOOLEANOBJECTPROPERTY)
     (object *o, fortran_string property_name, int *length, boolean *result)
{
  char *s = fcp2ccp(property_name); s[*length] = 0;
  *result = boolean_object_property(*o,s);
}

void FORTRANIZE(vector3objectproperty,VECTOR3OBJECTPROPERTY)
     (object *o, fortran_string property_name, int *length, vector3 *result)
{
  char *s = fcp2ccp(property_name); s[*length] = 0;
  *result = vector3_object_property(*o,s);
}

void FORTRANIZE(listobjectproperty,LISTOBJECTPROPERTY)
     (object *o, fortran_string property_name, int *length, list *result)
{
  char *s = fcp2ccp(property_name); s[*length] = 0;
  *result = list_object_property(*o,s);
}

void FORTRANIZE(objectobjectproperty,OBJECTOBJECTPROPERTY)
     (object *o, fortran_string property_name, int *length, object *result)
{
  char *s = fcp2ccp(property_name); s[*length] = 0;
  *result = object_object_property(*o,s);
}

void FORTRANIZE(stringobjectproperty,STRINGOBJECTPROPERTY)
     (object *o, fortran_string property_name, int *length, 
      fortran_string result, int *result_length)
{
  char *r;
  char *s = fcp2ccp(property_name); s[*length] = 0;
  r = string_object_property(*o,s);
  strncpy(fcp2ccp(result), r, *result_length);
  if (*result_length < strlen(r))
    *result_length = strlen(r);
  free(r);
}

/**************************************************************************/

