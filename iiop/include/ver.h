/*
 * Author:          Iona Technologies Plc.
 *
 * Description:
 *
 *	GIOP   Version   settings.  These  macro's  are    the current
 *	majopr/minor version of the engine.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef GIOP_VER_H
#define	GIOP_VER_H

/*
 *	These indicate  the current GIOP  version of the API. Backward
 *	compatability is maintained so that  the API is able to  reply
 *	using  the same version  GIOP  message format as  that of  the
 *	request.
 */
#define GIOP_MAJOR		1
#define GIOP_MINOR		1

/*
 *	The IIOP version is independent of the GIOP version.
 */
#define IIOP_MAJOR		1
#define IIOP_MINOR		1

#endif	/* !GIOP_VER_H	This MUST be the last line in this file! */
