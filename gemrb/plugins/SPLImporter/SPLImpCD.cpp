/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include "SPLImpCD.h"
#include "SPLImp.h"

SPLImpCD::SPLImpCD(void)
{
}

SPLImpCD::~SPLImpCD(void)
{
}

void* SPLImpCD::Create(void)
{
	return new SPLImp();
}

const char* SPLImpCD::ClassName(void)
{
	return "SPLImp";
}

SClass_ID SPLImpCD::SuperClassID(void)
{
	return IE_SPL_CLASS_ID;
}

Class_ID SPLImpCD::ClassID(void)
{
	// FIXME?????
	return Class_ID( 0x098a4123, 0xcfc417de );
}
