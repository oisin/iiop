/*
 * Author:          Iona Technologies Plc.
 *
 * Description:
 *
 *     This file contains #pragma's which we use to
 *     disable warnings gererated by the compiler.
 *
 *     These warnings are caused by the windows files included
 *     in the library.
 *     These warnings occur because these Windows files are
 *     written in a nonstandard extension of C .
 *
 *     Code compiles without including WinWarn.h,
 *     it's just easier to read compiler output without these 
 *     unavoidable warnings.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */


#ifndef WINWARN_H
#define	WINWARN_H


#pragma warning(disable: 4201)  /*: nonstandard extension used:
				    nameless struct/union */
#pragma warning(disable: 4214)  /*: nonstandard extension used: 
				    bit field types other than int */
#pragma warning(disable: 4616)  /*: named type definition in parentheses*/

#pragma warning(disable: 4115)  /*: named type definition in parentheses*/

#pragma warning(disable: 4514)  /*: unreferenced inline function 
				    has been removed*/
#pragma warning(disable: 4245)  /*: conversion from 'const int ' to 
				    'unsigned long ', signed/unsigned mismatch*/
#pragma warning(disable: 4210)  /*: Nonstandard extension used, 
				    Function given File scope*/
#pragma warning(disable: 4505)  /*: Unreferenced local function 
				    has been removed. */
#pragma warning(disable: 4244)  /*: Possible loss of data */

#pragma warning(disable: 4101)  /*: Unreferenced local variable*/

#pragma warning(disable: 4127)  /*: Conditional expression is constant*/

#pragma warning(disable: 4100)  /*: Unreferenced formal parameter*/

#pragma warning(disable: 4273)  /*: Inconsistent DLL Linkage*/


#endif	/* ! WINWARN_H	This MUST be the last line in this file! */





















