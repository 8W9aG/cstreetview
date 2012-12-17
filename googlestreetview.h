/*
 GoogleStreetView 1.0
 Copyright (c) 2012 Will Sackfield
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef GOOGLESTREETVIEW_H
#define GOOGLESTREETVIEW_H

#include <opencv2/opencv.hpp>

#define GSV_PANORAMA_ID_LENGTH 23

typedef struct gsvDataProperties_S {
	int imageWidth;
	int imageHeight;
	int tileWidth;
	int tileHeight;
	time_t imageDate;
	char panoramaId[GSV_PANORAMA_ID_LENGTH];
	// This is a lie, often the max is 5, but Google says 3
	int numZoomLevels;
	double latitude;
	double longitude;
	double originalLatitude;
	double originalLongitude;
	char* copyright;
	char* text;
	char* streetRange;
	char* region;
	char* country;
} gsvDataProperties;

const gsvDataProperties gsvDataPropertiesDefault = { 0, 0, 0, 0, NULL, "\0", 0, 0.0, 0.0, 0.0, 0.0, NULL, NULL, NULL, NULL, NULL };

typedef struct gsvProjectionProperties_S {
	// Should be an enum but I can't account for any types other than spherical
	char* projectionType;
	// In Degrees
	double panoramaYaw;
	double tiltYaw;
	double tiltPitch;
} gsvProjectionProperties;

const gsvProjectionProperties gsvProjectionPropertiesDefault = { NULL, 0.0, 0.0, 0.0 };

typedef struct gsvLink_S {
	// In Degrees
	double yaw;
	char panoramaId[GSV_PANORAMA_ID_LENGTH];
	// ARGB
	unsigned char roadColour[4];
	int scene;
	char* text;
} gsvLink;

typedef struct gsvAnnotationProperties_S {
	gsvLink* links;
	int numLinks;
} gsvAnnotationProperties;

const gsvAnnotationProperties gsvAnnotationPropertiesDefault = { NULL, 0 };

typedef struct GSV_S {
	gsvDataProperties dataProperties;
	gsvProjectionProperties projectionProperties;
	gsvAnnotationProperties annotationProperties;
} GSV;

const GSV GSVDefault = { gsvDataPropertiesDefault, gsvProjectionPropertiesDefault, gsvAnnotationPropertiesDefault };

GSV* gsv_open(double latitude,double longitude);
GSV* gsv_open(char* panoramaId);
IplImage* gsv_tile(GSV* panorama,int zoomLevel,int x,int y);
IplImage* gsv_panorama(GSV* panorama,int zoomLevel);
void gsv_close(GSV** gsvHandle);

#endif
