/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "BAMImp.h"
#include "../Core/Interface.h"
#include "../Core/Compressor.h"
#include "../Core/FileStream.h"
#include "../Core/Video.h"
#include "../Core/Palette.h"

BAMImp::BAMImp(void)
{
	str = NULL;
	autoFree = false;
	frames = NULL;
	cycles = NULL;
	palette = NULL;
	FramesCount = 0;
	CyclesCount = 0;
}

BAMImp::~BAMImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	delete[] frames;
	delete[] cycles;
	core->FreePalette(palette);
}

bool BAMImp::Open(DataStream* stream, bool autoFree)
{
	unsigned int i;

	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	delete[] frames;
	delete[] cycles;
	core->FreePalette(palette);

	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "BAMCV1  ", 8 ) == 0) {
		//Check if Decompressed file has already been Cached
		char cpath[_MAX_PATH];
		strcpy( cpath, core->CachePath );
		strcat( cpath, stream->filename );
		FILE* exist_in_cache = fopen( cpath, "rb" );
		if (exist_in_cache) {
			//File was previously cached, using local copy
			if (autoFree) {
				delete( str );
			}
			fclose( exist_in_cache );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		} else {
			//No file found in Cache, Decompressing and storing for further use
			str->Seek( 4, GEM_CURRENT_POS );

			if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID )) {
				printf( "No Compression Manager Available.\nCannot Load Compressed Bam File.\n" );
				return false;
			}
			FILE* newfile = fopen( cpath, "wb" );
			Compressor* comp = ( Compressor* )
				core->GetInterface( IE_COMPRESSION_CLASS_ID );
			comp->Decompress( newfile, str );
			core->FreeInterface( comp );
			fclose( newfile );
			if (autoFree)
				delete( str );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		}
	}
	if (strncmp( Signature, "BAM V1  ", 8 ) != 0) {
		return false;
	}
	str->ReadWord( &FramesCount );
	str->Read( &CyclesCount, 1 );
	str->Read( &CompressedColorIndex, 1 );
	str->ReadDword( &FramesOffset );
	str->ReadDword( &PaletteOffset );
	str->ReadDword( &FLTOffset );
	str->Seek( FramesOffset, GEM_STREAM_START );
	frames = new FrameEntry[FramesCount];
	DataStart = str->Size();
	for (i = 0; i < FramesCount; i++) {
		str->ReadWord( &frames[i].Width );
		str->ReadWord( &frames[i].Height );
		str->ReadWord( &frames[i].XPos );
		str->ReadWord( &frames[i].YPos );
		str->ReadDword( &frames[i].FrameData );
		if ((frames[i].FrameData & 0x7FFFFFFF) < DataStart)
			DataStart = (frames[i].FrameData & 0x7FFFFFFF);
	}
	cycles = new CycleEntry[CyclesCount];
	for (i = 0; i < CyclesCount; i++) {
		str->ReadWord( &cycles[i].FramesCount );
		str->ReadWord( &cycles[i].FirstFrame );
	}
	str->Seek( PaletteOffset, GEM_STREAM_START );
	palette = new Palette();
	//no idea if we have to switch this
	for (i = 0; i < 256; i++) {
		RevColor rc;
		str->Read( &rc, 4 );
		palette->col[i].r = rc.r;
		palette->col[i].g = rc.g;
		palette->col[i].b = rc.b;
		palette->col[i].a = rc.a;
	}

	return true;
}

int BAMImp::GetCycleSize(unsigned char Cycle)
{
	if(Cycle >= CyclesCount ) {
		return -1;
	}
	return cycles[Cycle].FramesCount;
}

Sprite2D* BAMImp::GetFrameFromCycle(unsigned char Cycle, unsigned short frame)
{
	if(Cycle >= CyclesCount ) {
		printf("[BAMImp] Invalid Cycle %d\n", (int) Cycle);
		return NULL;
	}
	if(cycles[Cycle].FramesCount<=frame) {
		printf("[BAMImp] Invalid Frame %d in Cycle %d\n",(int) frame, (int) Cycle);
		return NULL;
	}
	str->Seek( FLTOffset + ( cycles[Cycle].FirstFrame + frame ) * sizeof( ieWord ),
			GEM_STREAM_START );
	ieWord findex;
	str->ReadWord( &findex );
	return GetFrame( findex );
}

Sprite2D* BAMImp::GetFrameInternal(unsigned short findex, unsigned char mode,
								   bool BAMsprite, const unsigned char* data)
{
	Sprite2D* spr = 0;

	if (BAMsprite) {
		bool RLECompressed = (frames[findex].FrameData & 0x80000000) == 0;
		if (data) {
			const unsigned char* framedata = data;
			framedata += (frames[findex].FrameData & 0x7FFFFFFF) - DataStart;
			if (RLECompressed) {
				spr = core->GetVideoDriver()->CreateSpriteBAM8(
					frames[findex].Width, frames[findex].Height,
					true, framedata, 0, palette, CompressedColorIndex);
			} else {
				spr = core->GetVideoDriver()->CreateSpriteBAM8(
					frames[findex].Width, frames[findex].Height, false,
					framedata, 0, palette, CompressedColorIndex );
			}
		} else {
			if (RLECompressed) {
				// FIXME: get the real size of the RLE data somehow, or cache
				// the entire BAM in memory consecutively
				unsigned long RLESize =
				( frames[findex].Width * frames[findex].Height * 3 ) / 2 + 1;
				//without partial reads, we should be careful
				str->Seek( ( frames[findex].FrameData & 0x7FFFFFFF ), GEM_STREAM_START );
				unsigned long remains = str->Remains();
				if (RLESize > remains) {
					RLESize = remains;
				}
				unsigned char* RLEinpix = (unsigned char*)malloc( RLESize );
				if (str->Read( RLEinpix, RLESize ) == GEM_ERROR) {
					free( RLEinpix );
					return NULL;
				}

				spr = core->GetVideoDriver()->CreateSpriteBAM8(
					frames[findex].Width, frames[findex].Height,
					true, RLEinpix, RLESize, palette, CompressedColorIndex);
			} else {
				const unsigned char* pixels;
				pixels = (const unsigned char*)GetFramePixels(findex);
				spr = core->GetVideoDriver()->CreateSpriteBAM8(
					frames[findex].Width, frames[findex].Height, false,
					pixels, frames[findex].Width*frames[findex].Height, 
					palette, CompressedColorIndex );
			}
		}
	} else {
		void* pixels = GetFramePixels(findex);
		spr = core->GetVideoDriver()->CreateSprite8(
			frames[findex].Width, frames[findex].Height, 8,
			pixels, palette->col, true, 0 );
	}

	spr->XPos = (ieWordSigned)frames[findex].XPos;
	spr->YPos = (ieWordSigned)frames[findex].YPos;
	if (mode == IE_SHADED) {
		// CHECKME: is this ever used? Should we modify the sprite's palette
		// without creating a local copy for this sprite?
		Palette* pal = core->GetVideoDriver()->GetPalette(spr);
		pal->CreateShadedAlphaChannel();
		pal->Release();
	}
	return spr;
}

Sprite2D* BAMImp::GetFrame(unsigned short findex, unsigned char mode)
{
	if (findex >= FramesCount) {
		findex = cycles[0].FirstFrame;
	}
	bool videoBAMsupport = core->GetVideoDriver()->SupportsBAMSprites();

	return GetFrameInternal(findex, mode, videoBAMsupport, 0);
}

void* BAMImp::GetFramePixels(unsigned short findex)
{
	if (findex >= FramesCount) {
		findex = cycles[0].FirstFrame;
	}
	str->Seek( ( frames[findex].FrameData & 0x7FFFFFFF ), GEM_STREAM_START );
	unsigned long pixelcount = frames[findex].Height * frames[findex].Width;
	void* pixels = malloc( pixelcount );
	bool RLECompressed = ( ( frames[findex].FrameData & 0x80000000 ) == 0 );
	if (RLECompressed) {
		//if RLE Compressed
		unsigned long RLESize;
		RLESize = ( unsigned long )
			( frames[findex].Width * frames[findex].Height * 3 ) / 2 + 1;
		//without partial reads, we should be careful
		unsigned long remains = str->Remains();
		if (RLESize > remains) {
			RLESize = remains;
		}
		unsigned char* inpix;
		inpix = (unsigned char*)malloc( RLESize );
		if (str->Read( inpix, RLESize ) == GEM_ERROR) {
			free( inpix );
			return NULL;
		}
		unsigned char * p = inpix;
		unsigned char * Buffer = (unsigned char*)pixels;
		unsigned int i = 0;
		while (i < pixelcount) {
			if (*p == CompressedColorIndex) {
				p++;
				// FIXME: Czech HOW has apparently broken frame
				// #141 in REALMS.BAM. Maybe we should put
				// this condition to #ifdef BROKEN_xx ?
				// Or maybe rather put correct REALMS.BAM
				// into override/ dir?
				if (i + ( *p ) + 1 > pixelcount) {
					memset( &Buffer[i], CompressedColorIndex, pixelcount - i );
					printf ("Broken frame %d\n", findex);
				} else {
					memset( &Buffer[i], CompressedColorIndex, ( *p ) + 1 );
				}
				i += *p;
			} else 
				Buffer[i] = *p;
			p++;
			i++;
		}
		free( inpix );
	} else {
		str->Read( pixels, pixelcount );
	}
	return pixels;
}

ieWord * BAMImp::CacheFLT(unsigned int &count)
{
	int i;

	count = 0;
	for (i = 0; i < CyclesCount; i++) {
		unsigned int tmp = cycles[i].FirstFrame + cycles[i].FramesCount;
		if (tmp > count) {
			count = tmp;
		}
	}
	ieWord * FLT = ( ieWord * ) calloc( count, sizeof(ieWord) );
	str->Seek( FLTOffset, GEM_STREAM_START );
	str->Read( FLT, count * sizeof(ieWord) );
	if( DataStream::IsEndianSwitch() ) {
		//msvc likes it as char *
		swab( (char*) FLT, (char*) FLT, count * sizeof(ieWord) );
	}
	return FLT;
}

AnimationFactory* BAMImp::GetAnimationFactory(const char* ResRef, unsigned char mode)
{
	unsigned int i, count;
	AnimationFactory* af = new AnimationFactory( ResRef );
	ieWord *FLT = CacheFLT( count );

	bool videoBAMsupport = core->GetVideoDriver()->SupportsBAMSprites();
	unsigned char* data = NULL;

	if (videoBAMsupport) {
		str->Seek( DataStart, GEM_STREAM_START );
		unsigned long length = str->Remains();
		if (length == 0) return af;
		//data = new unsigned char[length];
		data = (unsigned char *) malloc(length);
		str->Read( data, length );
		af->SetFrameData(data);
	}

	for (i = 0; i < FramesCount; ++i) {
		Sprite2D* frame = GetFrameInternal( i, mode, videoBAMsupport, data );
		assert(frame->BAM);
		af->AddFrame(frame);
	}
	for (i = 0; i < CyclesCount; ++i) {
		af->AddCycle( cycles[i] );
	}
	af->LoadFLT ( FLT, count );
	free (FLT);
	return af;
}

/** This function will load the Animation as a Font */
Font* BAMImp::GetFont()
{
	unsigned int i;

	int w = 0, h = 0;
	unsigned int Count;

	ieWord *FLT = CacheFLT(Count);

	// Numeric fonts have all frames in single cycle
	if (CyclesCount > 1) {
		Count = CyclesCount;
	} else {
		Count = FramesCount;
	}

	for (i = 0; i < Count; i++) {
		unsigned int index;
		if (CyclesCount > 1) {
			index = FLT[cycles[i].FirstFrame];
			if (index >= FramesCount)
				continue;
		} else {
			index = i;
		}

		w = w + frames[index].Width;
		if (frames[index].Height > h)
			h = frames[index].Height;
	}

	Font* fnt = new Font( w, h, palette );
	for (i = 0; i < Count; i++) {
		unsigned int index;
		if (CyclesCount > 1) {
			index = FLT[cycles[i].FirstFrame];
			if (index >= FramesCount) {
				fnt->AddChar( NULL, 0, 0, 0, 0 );
				continue;
			}
		} else {
			index = i;
		}

		unsigned char* pixels = (unsigned char*)GetFramePixels( index );
		if( !pixels) {
			fnt->AddChar( NULL, 0, 0, 0, 0 );
			continue;
		}
		fnt->AddChar( pixels, frames[index].Width,
				frames[index].Height,
				frames[index].XPos,
				frames[index].YPos );
		free( pixels );
	}
	free( FLT );

	fnt->FinalizeSprite( true, 0 );

	return fnt;
}
/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
If the Global Animation Palette is NULL, returns NULL. */
Sprite2D* BAMImp::GetPalette()
{
	unsigned char * pixels = ( unsigned char * ) malloc( 256 );
	unsigned char * p = pixels;
	for (int i = 0; i < 256; i++) {
		*p++ = ( unsigned char ) i;
	}
	return core->GetVideoDriver()->CreateSprite8( 16, 16, 8, pixels, palette->col, false );
}
