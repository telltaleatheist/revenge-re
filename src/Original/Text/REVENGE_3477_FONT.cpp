// Reconstructed original font resource loader and metrics (segment 3477).

#include "REVENGE.h"
#include "revenge/game_file.h"
#include "revenge/original_data.h"

#include <string.h>

// 3477:000B: unsigned char *LoadFontResource(
//     const char *fileName)
byte *LoadFontResource(const char *fileName)
{
	word16 totalFileSize = 0x00;
	byte dimensions[0x02];
	byte *fontResource = NULL;
	int16 fileHandle = GameFile_Open(fileName, GAME_FILE_READ); /* 8001h */

	if (fileHandle != -0x01)
	{
		word16 headerBytesRead = GameFile_Read(
			fileHandle,
			(byte *) &totalFileSize,
			0x02);
		totalFileSize = (word16) (totalFileSize + 0x02);
		if (headerBytesRead == 0x02 && totalFileSize > 0x0104)
		{
			int32 physicalFileLength = GameFile_Length(fileHandle);
			if (HIGH_WORD(physicalFileLength) == 0x00 &&
				LOW_WORD(physicalFileLength) == totalFileSize)
			{
				int32 seekPosition = GameFile_Seek(
					fileHandle, 0x0102L, GAME_FILE_FROM_START);
				if (seekPosition == 0x0102L)
				{
					word16 dimensionsRead = GameFile_Read(
						fileHandle,
						dimensions,
						0x02);
					if (dimensionsRead == 0x02)
					{
						int16 glyphHeight = (int8) dimensions[0x00];
						int16 bytesPerGlyphRow =
							((int8) dimensions[0x01] + 0x07) / 0x08;
						word16 glyphDataSize = (word16)
							((word16) glyphHeight *
							(word16) bytesPerGlyphRow << 0x07);
						word16 allocationSize =
							(word16) (glyphDataSize + 0x0104);

						if (allocationSize == totalFileSize)
						{
							fontResource = (byte *)
								Heap_Allocate(allocationSize, "3477:00FF");
							if (fontResource != NULL)
							{
								if (GameFile_Seek(
									fileHandle,
									0x00L,
									GAME_FILE_FROM_START) == 0x00L)
								{
									word16 fontBytesRead =
										GameFile_Read(
											fileHandle,
											fontResource,
											totalFileSize);
									if (fontBytesRead != totalFileSize)
									{
										Heap_Free(fontResource);
										fontResource = NULL;
									}
								}
								else
								{
									Heap_Free(fontResource);
									fontResource = NULL;
								}
							}
						}
					}
				}
			}
		}
		GameFile_Close(fileHandle);
	}
	return fontResource;
}

// 3477:018F: void SetActiveFontResource(byte *fontResource)
void SetActiveFontResource(byte *fontResource)
{
	ActiveFontResource = fontResource;
	ActiveFontGlyphAdvanceX = (int8)fontResource[0x0103];
	ActiveFontLineHeight = (int8)fontResource[0x0102];
}

// 3477:01B5: uint16 MeasureTextWidth(const char *text)
uint16 MeasureTextWidth(const char *text)
{
	const byte *activeFont = ActiveFontResource;
	int16 glyphAdvance = (int8) activeFont[0x0103];

	return (word16) ((word16) strlen(text) * glyphAdvance);
}
