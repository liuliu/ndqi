/********************************************************
 * Metadata API of NDQI
 * Copyright (C) 2009 Liu Liu
 ********************************************************/

#include "nqmeta.h"
#include <cctype>
#include <string.h>
#include <libexif/exif-data.h>

static void strtolower(char* buf)
{
	int i, n = strlen(buf);
	for (i = 0; i < n; i++, buf++)
		*buf = tolower(*buf);
}

TCMAP* nqmetanew(const char* file)
{
	TCMAP* cols = tcmapnew();
	ExifData* exif = exif_data_new_from_file(file);
	ExifEntry* exif_entry;
	ExifByteOrder o;
	ExifRational v_rat;
	double d;
	short int v_short;
	char buf[1024];
	o = exif_data_get_byte_order(exif);
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_0], EXIF_TAG_MAKE))
	{
		exif_entry_get_value(exif_entry, buf, sizeof(buf));
		strtolower(buf);
		tcmapput(cols, "make", 4, buf, strlen(buf));
	}
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_0], EXIF_TAG_MODEL))
	{
		exif_entry_get_value(exif_entry, buf, sizeof(buf));
		strtolower(buf);
		tcmapput(cols, "model", 5, buf, strlen(buf));
	}
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_0], EXIF_TAG_DATE_TIME))
	{
		exif_entry_get_value(exif_entry, buf, sizeof(buf));
		int year, month, day, hour, minute, second;
		sscanf(buf, "%d:%d:%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
		snprintf(buf, 1024, "%i", year);
		tcmapput(cols, "year", 4, buf, strlen(buf));
		snprintf(buf, 1024, "%i", month);
		tcmapput(cols, "month", 5, buf, strlen(buf));
		snprintf(buf, 1024, "%i", day);
		tcmapput(cols, "day", 3, buf, strlen(buf));
		snprintf(buf, 1024, "%i", hour);
		tcmapput(cols, "hour", 4, buf, strlen(buf));
		snprintf(buf, 1024, "%i", minute);
		tcmapput(cols, "minute", 6, buf, strlen(buf));
		snprintf(buf, 1024, "%i", second);
		tcmapput(cols, "second", 6, buf, strlen(buf));
	}
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_EXIF], EXIF_TAG_EXPOSURE_TIME))
	{
		v_rat = exif_get_rational(exif_entry->data, o);
		d = (double)v_rat.numerator / (double)v_rat.denominator;
		snprintf(buf, 1024, "%lf", d);
		tcmapput(cols, "exposure", 8, buf, strlen(buf));
	}
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_EXIF], EXIF_TAG_FNUMBER))
	{
		v_rat = exif_get_rational(exif_entry->data, o);
		d = (double)v_rat.numerator / (double)v_rat.denominator;
		snprintf(buf, 1024, "%lf", d);
		tcmapput(cols, "fnumber", 7, buf, strlen(buf));
	}
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_EXIF], EXIF_TAG_FOCAL_LENGTH))
	{
		v_rat = exif_get_rational(exif_entry->data, o);
		d = (double)v_rat.numerator / (double)v_rat.denominator;
		snprintf(buf, 1024, "%lf", d);
		tcmapput(cols, "focal", 5, buf, strlen(buf));
	}
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_EXIF], EXIF_TAG_FLASH))
	{
		v_short = exif_get_short(exif_entry->data, o);
		snprintf(buf, 1024, "%i", v_short & 0x1);
		tcmapput(cols, "flash", 5, buf, strlen(buf));
	}
	if (exif_entry = exif_content_get_entry(exif->ifd[EXIF_IFD_EXIF], EXIF_TAG_ISO_SPEED_RATINGS))
	{
		v_short = exif_get_long(exif_entry->data, o);
		snprintf(buf, 1024, "%i", v_short);
		tcmapput(cols, "iso", 3, buf, strlen(buf));
	}
	exif_data_unref(exif);
	return cols;
}

bool nqmetasetindex(TCTDB* tdb)
{
	return tctdbsetindex(tdb, "make", TDBITLEXICAL) &&
		   tctdbsetindex(tdb, "model", TDBITLEXICAL) &&
		   tctdbsetindex(tdb, "year", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "month", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "day", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "hour", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "minute", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "second", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "exposure", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "fnumber", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "focal", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "flash", TDBITDECIMAL) &&
		   tctdbsetindex(tdb, "iso", TDBITDECIMAL);
}
