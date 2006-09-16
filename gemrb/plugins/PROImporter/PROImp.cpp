/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/PROImporter/PROImp.cpp,v 1.1 2006/09/16 13:13:19 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/AnimationMgr.h"
#include "../Core/EffectMgr.h"
#include "PROImp.h"

PROImp::PROImp(void)
{
	str = NULL;
	autoFree = false;
}

PROImp::~PROImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	str = NULL;
}

bool PROImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "PRO V1.0", 8 ) == 0) {
		version = 10;
	} else {
		printf( "[PROImporter]: This file is not a valid PRO File\n" );
		return false;
	}

	return true;
}

Projectile* PROImp::GetProjectile(Projectile *s)
{
	if( !s) {
		return NULL;
	}
	str->ReadWord( &s->Type );
	str->ReadWord( &s->Speed );
	str->ReadDword( &s->SFlags ); //spark, ignore center, looping sound etc
	str->ReadResRef( s->SoundRes1 );
	str->ReadResRef( s->SoundRes2 );
	str->ReadResRef( s->SoundRes3 );
	str->ReadDword( &s->SparkColor ); //enabled by PSF_SPARK
	str->Seek(212, GEM_CURRENT_POS); //skipping unused features
	//we should stand at offset 0x100 now
	str->ReadDword( &s->TFlags ); //other projectile flags
	str->ReadResRef( s->BAMRes1 );
	str->ReadResRef( s->BAMRes2 );
	str->Read( &s->Seq1,1 );
	str->Read( &s->Seq2,1 );
	str->ReadWord( &s->LightZ );
	str->ReadWord( &s->LightX );
	str->ReadWord( &s->LightY );
	str->ReadResRef( s->Palette );
	str->Read( s->Gradients, 7);
	str->Read( &s->SmokeSpeed, 1);
	str->Read( s->SmokeGrad, 7);
	str->Read( &s->SmokeAim, 1);
	str->ReadWord( &s->SmokeAnimID);
	str->ReadResRef( s->TrailBAM[0] );
	str->ReadResRef( s->TrailBAM[1] );
	str->ReadResRef( s->TrailBAM[2] );
	str->ReadWord( &s->TrailSpeed[0] );
	str->ReadWord( &s->TrailSpeed[1] );
	str->ReadWord( &s->TrailSpeed[2] );
	str->Seek(172, GEM_CURRENT_POS);
	if (s->Type!=3) {
		return s;
	}
	s->InitExtension();
	GetAreaExtension(s->Extension);
	return s;
}

void PROImp::GetAreaExtension(ProjectileExtension *e)
{
	str->ReadDword( &e->AFlags );
	str->ReadWord( &e->TriggerRadius );
	str->ReadWord( &e->ExplosionRadius );
	str->ReadResRef( e->SoundRes ); //explosion sound
	str->ReadWord( &e->Delay );
	str->ReadWord( &e->FragAnimID );
	str->ReadWord( &e->FragProjIdx );
	str->Read( &e->ExplosionCount,1 );
	str->Read( &e->ExplType,1); //the area puff type (flames, puffs, clouds) fireball.ids
	str->ReadWord( &e->ExplColor);
	str->ReadWord( &e->ExplProjIdx);
	str->ReadResRef( e->VVCRes );
	str->ReadWord( &e->ConeWidth);
	//we skip the rest
	str->Seek(218, GEM_CURRENT_POS);
}
