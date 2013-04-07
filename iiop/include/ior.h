/*
 * Author:          Iona Technologies Plc.
 *
 * Description:
 *	Interoperable Object Reference interface. 
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *  
 */
#ifndef IOR_H
#define	IOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "typ.h"
#include "ver.h"

#ifndef IOR_EXTERN
#define	IOR_EXTERN	extern
#endif


/* 
 *	Return Codes from IOR API calls 
 */
#define IOR_OK			0

#define IOR_ENULL_IOR		1	/* null IOR param */
#define IOR_ENULL_PROFILE	2	/* null profile param */
#define IOR_ENULL_TYPEID	3	/* expects type id and null passed */
#define IOR_EZERO_PROFILE	4	/* no profiles in IOR */
#define IOR_EINV_TAG		5	/* unknown tag type */
#define IOR_ENULL_DATA		6	/* null data specified */
#define IOR_ENOSPACE		7	/* could not allocate memory */
#define IOR_ENULL_CODER		8	/* null coder specified */
#define IOR_ENULL_RETURN	9	/* null return param specified  */
#define IOR_EINV_IORSTR		10	/* IOR string must start with 'IOR:' */


/* IOR return status type */

typedef unsigned long	IORStatusT;


/* Stringified IOR prefix */

#define IOR_PREFIX		"IOR:"
#define IOR_PREFIX_LEN		4

/*
 * 	IOR Tagged Profile (content) tag values.
 */
#define TAG_INTERNET_IOP 		0
#define TAG_MULTIPLE_COMPONENTS  	1


/* Maximum tag value. NOTE: update this when new IOR tags are added */

#define TAG_MAXIMUM			TAG_MULTIPLE_COMPONENTS

/*
 *	IOR component tag types 
 */
#define TAG_ORB_TYPE			0
#define TAG_CODE_SETS			1
#define TAG_SEC_NAME			2
#define TAG_ASSOCIATION_OPTION		3
#define TAG_GENERIC_SEC_MECH		4

/*
 * Internal tag types (not made public outside the Engine)
 */
#define TAG_IIOP_CTRL_BLK		0


/*
 *	Action to take when  multiple   profiles are provided in   the
 *	IOR.
 *
 *	The first  option is that  the first profile  to be used which
 *	results  in   a  successful connection  is  the  only protocol
 *	instance associated with this object reference.
 *
 *	The   alternative  is to attempt   to   re-establish a dropped
 *	connection by using any of   the profiles available,  possibly
 *	including the profile that was in use when the connection went
 *	down.  This   functionality  would   be  transparent   to  the
 *	application and act as a primitive fault tolerance mechanism.
 */
#define IOR_MP_TRYONE	      	0
#define IOR_MP_AUTORETRY	1


/* tag types */

typedef unsigned long  	IORProfileIdT;
typedef unsigned long  	IORComponentIdT;
typedef unsigned long	IORServiceIdT;


/*
 *	Tagged Profile
 */
typedef struct IORTaggedProfileS
{
	IORProfileIdT	tp_tag;

	unsigned long	tp_profile_max;
	unsigned long	tp_profile_len;
	octet		*tp_profile_data_p;
}
        IORTaggedProfileT;

/*
 *	Tagged Component
 */
typedef struct IORTaggedComponentS
{
	IORComponentIdT	tc_tag;

	unsigned long	tc_component_max;
	unsigned long	tc_component_len;
	octet		*tc_component_data_p;
}
        IORTaggedComponentT;

typedef struct IORMultipleComponentProfileS
{
	unsigned long	mcp_components_max;
	unsigned long	mcp_components_len;
	IORTaggedComponentT
		        *mcp_components_p;
}
        IORMultipleComponentProfileT;


/*
 *	Global IOR structure.  This contains a  type id and ("sequence
 *	of") tagged profiles.
 */
typedef struct IORS
{
	char			*ior_type_id_p;

	unsigned long		ior_profiles_max;
	unsigned long		ior_profiles_len;
	IORTaggedProfileT	*ior_profiles_p;
}
        IORT;

/*
 *	Service Contexts. These are encapsulated into a Tagged Profile.
 */
typedef struct IORServiceContextS
{
	IORServiceIdT	sc_context_id;

	unsigned long	sc_context_max;
	unsigned long	sc_context_len;
	octet		*sc_context_data_p;
}
        IORServiceContextT;

/* Service Context List */

typedef struct IORServiceContextListS
{
	unsigned long		scl_contexts_max;
	unsigned long		scl_contexts_len;
	IORServiceContextT	*scl_contexts_p;
}
	IORServiceContextListT;


/*
 *	IIOP Profile Body (revisions 1.0 and 1.1)
 */
typedef struct IIOPBody_1_0S
{
	octet			ib_major;
	unsigned char		ib_minor;
	char 			*ib_host_p;
	unsigned short		ib_port;
	unsigned long		ib_objkey_len;
	octet			*ib_objkey_p;
} 
        IIOPBody_1_0T;

typedef struct IIOPBody_1_1S
{
	unsigned char		ib_major;
	unsigned char		ib_minor;
	char 			*ib_host_p;
	unsigned short		ib_port;
	unsigned long		ib_objkey_len;
	octet			*ib_objkey_p;

	unsigned long		ib_components_max;
	unsigned long		ib_components_len;
	IORTaggedComponentT 	*ib_components_p;
} 
        IIOPBody_1_1T;


/*----------------------------------------------------------------------
 * Prototypes for module: IOR
 */

DLL_LINKAGE IORStatusT 
IORCreateIor(
	IORT 		*ior_p, 
	char 		*typid_p, 
	unsigned long 	max_profiles, 
	GIOPAllocFpT 	getmem
);


DLL_LINKAGE IORStatusT 
IORAddTaggedProfile(
	IORT 		*ior_p,
	IORProfileIdT 	tag,
	unsigned long 	data_len,
	octet 		*data_p,
	GIOPAllocFpT 	getmem
);

/* Convert between actual and stringified IORs */

DLL_LINKAGE IORStatusT
IORFromString(
	IORT 		*ior_p, 
	unsigned char 	*iorstr_p, 
	GIOPAllocFpT 	getmem,
	GIOPDeallocFpT 	delmem
);

DLL_LINKAGE IORStatusT 
IORToString(IORT *ior_p, unsigned char **iorstr_pp, GIOPAllocFpT getmem);


DLL_LINKAGE void
IORFree(IORT *ior_p, GIOPDeallocFpT delmem);

/*
 * End of Prototypes.
 */
#undef	IOR_EXTERN

#ifdef __cplusplus
}
#endif

#endif	/* !IOR_H	This MUST be the last line in this file! */
