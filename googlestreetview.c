/*
 GoogleStreetView 1.0
 Copyright (c) 2012 Will Sackfield
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <curl/curl.h>
#include <tinyxml2.h>
#include <jpeglib.h>
#include <jerror.h>
#include "googlestreetview.h"

#define MAX_DOUBLE_CHARACTERS (3 + DBL_MANT_DIG - DBL_MIN_EXP)
#define GSV_DEBUG
#define GSV_WARNINGS

#ifdef GSV_WARNINGS
#define GSV_WARNING(msg,error) if(error!=XML_NO_ERROR)printf("GSV Warning: %s - %s\n",msg,(error==XML_WRONG_ATTRIBUTE_TYPE)?"Wrong Attribute Type":"No Attribute");
#else
#define GSV_WARNING(msg)
#endif

using namespace tinyxml2;

typedef struct CURLBuffer_S {
	void* buffer;
	size_t bufferSize;
} CURLBuffer;

const CURLBuffer CURLBufferDefault = { NULL, 0 };

/*
 * CURL methods
 */

int gsvCURLToBuffer(void* data,size_t size,size_t nmemb,CURLBuffer* buffer)
{
	if(buffer == NULL)
		return 0;
	
	buffer->buffer = realloc(buffer->buffer,buffer->bufferSize+size*nmemb);
	if(buffer->buffer == NULL)
		return 0;
	
	memcpy(((unsigned char*)buffer->buffer)+buffer->bufferSize,data,size*nmemb);
	buffer->bufferSize += size*nmemb;
	
	return size*nmemb;
}

/*
 * libjpeg-turbo
 * http://stackoverflow.com/questions/5280756/libjpeg-ver-6b-jpeg-stdio-src-vs-jpeg-mem-src
 *
 * libjpeg turbo does not have a method to decompress a jpeg in memory, it's pointless to write to the disk and read back just to satisfy an API failure, so this rather ugly bit of code is the result
 */

static void init_source (j_decompress_ptr cinfo) {}
static boolean fill_input_buffer (j_decompress_ptr cinfo)
{
    ERREXIT(cinfo, JERR_INPUT_EMPTY);
return TRUE;
}
static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) cinfo->src;

    if (num_bytes > 0) {
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}
static void term_source (j_decompress_ptr cinfo) {}
static void jpeg_mem_src (j_decompress_ptr cinfo, void* buffer, long nbytes)
{
    struct jpeg_source_mgr* src;

    if (cinfo->src == NULL) {   /* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            sizeof(struct jpeg_source_mgr));
    }

    src = (struct jpeg_source_mgr*) cinfo->src;
    src->init_source = init_source;
    src->fill_input_buffer = fill_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->term_source = term_source;
    src->bytes_in_buffer = nbytes;
    src->next_input_byte = (JOCTET*)buffer;
}

/*
 * Private methods
 */

inline void gsv_copy_string(const char* input,char** output)
{
	if(input != NULL)
	{
		int inputLen = strlen(input)+1;
		*output = (char*) malloc(inputLen);
		memset(*output,'\0',inputLen);
		memcpy(*output,input,inputLen-1);
	}
}

GSV* gsv_parse(char* xmlString)
{
#ifdef GSV_DEBUG
	printf("gsv_parse(%p)\n",xmlString);
	printf("XML = %s\n",xmlString);
#endif
	XMLDocument doc;
	doc.Parse(xmlString);
	
	GSV* gsvHandle = (GSV*) malloc(sizeof(GSV));
	if(gsvHandle == NULL)
		return NULL;
	
	*gsvHandle = GSVDefault;
	
	XMLElement* panoramaElement = doc.FirstChildElement("panorama");
	XMLElement* dataPropertiesElement = panoramaElement->FirstChildElement("data_properties");
	
	int error = dataPropertiesElement->QueryIntAttribute("image_width",&gsvHandle->dataProperties.imageWidth);
	GSV_WARNING("image_width",error);
	error = dataPropertiesElement->QueryIntAttribute("image_height",&gsvHandle->dataProperties.imageHeight);
	GSV_WARNING("image_height",error);
	error = dataPropertiesElement->QueryIntAttribute("tile_width",&gsvHandle->dataProperties.tileWidth);
	GSV_WARNING("tile_width",error);
	error = dataPropertiesElement->QueryIntAttribute("tile_height",&gsvHandle->dataProperties.tileHeight);
	GSV_WARNING("tile_height",error);
	
	char year[5];
	memset(year,'\0',sizeof(year)*sizeof(char));
	char month[3];
	memset(month,'\0',sizeof(month)*sizeof(char));
	const char* imageDate = dataPropertiesElement->Attribute("image_date");
	memcpy(year,imageDate,4*sizeof(char));
	memcpy(month,&imageDate[5],2*sizeof(char));
	struct tm imageDateTm = { 0, 0, 0, 1, atoi(month)-1, atoi(year)-1900, 0, 0, -1 };
	gsvHandle->dataProperties.imageDate = mktime(&imageDateTm);
	
	memcpy(gsvHandle->dataProperties.panoramaId,dataPropertiesElement->Attribute("pano_id"),GSV_PANORAMA_ID_LENGTH);
	error = dataPropertiesElement->QueryIntAttribute("num_zoom_levels",&gsvHandle->dataProperties.numZoomLevels);
	GSV_WARNING("num_zoom_levels",error);
	error = dataPropertiesElement->QueryDoubleAttribute("lat",&gsvHandle->dataProperties.latitude);
	GSV_WARNING("lat",error);
	error = dataPropertiesElement->QueryDoubleAttribute("lng",&gsvHandle->dataProperties.longitude);
	GSV_WARNING("lng",error);
	error = dataPropertiesElement->QueryDoubleAttribute("original_lat",&gsvHandle->dataProperties.longitude);
	GSV_WARNING("original_lat",error);
	error = dataPropertiesElement->QueryDoubleAttribute("original_lng",&gsvHandle->dataProperties.longitude);
	GSV_WARNING("original_lng",error);
	gsv_copy_string(dataPropertiesElement->FirstChildElement("copyright")->GetText(),&gsvHandle->dataProperties.copyright);
	gsv_copy_string(dataPropertiesElement->FirstChildElement("text")->GetText(),&gsvHandle->dataProperties.text);
	gsv_copy_string(dataPropertiesElement->FirstChildElement("street_range")->GetText(),&gsvHandle->dataProperties.streetRange);
	gsv_copy_string(dataPropertiesElement->FirstChildElement("region")->GetText(),&gsvHandle->dataProperties.region);
	gsv_copy_string(dataPropertiesElement->FirstChildElement("country")->GetText(),&gsvHandle->dataProperties.country);
	
	XMLElement* projectionPropertiesElement = panoramaElement->FirstChildElement("projection_properties");
	gsv_copy_string(projectionPropertiesElement->Attribute("projection_type"),&gsvHandle->projectionProperties.projectionType);
	gsvHandle->projectionProperties.panoramaYaw = projectionPropertiesElement->DoubleAttribute("pano_yaw_deg");
	gsvHandle->projectionProperties.tiltYaw = projectionPropertiesElement->DoubleAttribute("tilt_yaw_deg");
	gsvHandle->projectionProperties.tiltYaw = projectionPropertiesElement->DoubleAttribute("tilt_pitch_deg");
	
	XMLElement* annotationProperties = panoramaElement->FirstChildElement("annotation_properties");
	XMLElement* linkElement = annotationProperties->FirstChildElement("link");
	
	while(linkElement != NULL)
	{
		gsvHandle->annotationProperties.links = (gsvLink*) realloc(gsvHandle->annotationProperties.links,(gsvHandle->annotationProperties.numLinks+1)*sizeof(gsvLink));
		gsvLink* link = &gsvHandle->annotationProperties.links[gsvHandle->annotationProperties.numLinks];
		
		error = linkElement->QueryDoubleAttribute("yaw_deg",&link->yaw);
		GSV_WARNING("yaw_deg",error);
		
		memcpy(link->panoramaId,linkElement->Attribute("pano_id"),GSV_PANORAMA_ID_LENGTH);
		
		const char* road_argb = linkElement->Attribute("road_argb");
		*((unsigned int*)&link->roadColour) = (unsigned int)strtoul(&road_argb[2],NULL,16);
		link->scene = linkElement->IntAttribute("scene");
		
		XMLElement* linkTextElement = linkElement->FirstChildElement("link_text");
		gsv_copy_string(linkTextElement->GetText(),&link->text);
		
		gsvHandle->annotationProperties.numLinks++;
		
		XMLNode* siblingNode = linkElement->NextSibling();
		if(siblingNode == NULL)
			break;
		linkElement = siblingNode->ToElement();
	}
	
	return gsvHandle;
}

/*
 * Public methods
 */

GSV* gsv_open(double latitude,double longitude)
{
#ifdef GSV_DEBUG
	printf("gsv_open(%f,%f)\n",latitude,longitude);
#endif
	CURL* curl = curl_easy_init();
	char urlString[43+(MAX_DOUBLE_CHARACTERS*2)];
	
	snprintf(urlString,sizeof(urlString),"http://cbk0.google.com/cbk?output=xml&ll=%f,%f",latitude,longitude);
	
	curl_easy_setopt(curl,CURLOPT_URL,urlString);
	curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,gsvCURLToBuffer);
	
	CURLBuffer buffer = CURLBufferDefault;
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,&buffer);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	
	if(buffer.buffer == NULL)
		return NULL;
	
	GSV* gsvHandle = gsv_parse((char*)buffer.buffer);
	free(buffer.buffer);
	return gsvHandle;
}

GSV* gsv_open(char* panoramaId)
{
	CURL* curl = curl_easy_init();
	char urlString[114+GSV_PANORAMA_ID_LENGTH];
	
	snprintf(urlString,sizeof(urlString),"http://cbk1.google.com/cbk?output=xml&cb_client=maps_sv&hl=en&dm=1&pm=1&ph=1&renderer=cubic,spherical&v=4&panoid=%s",panoramaId);
	
	curl_easy_setopt(curl,CURLOPT_URL,urlString);
	curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,gsvCURLToBuffer);
	
	CURLBuffer buffer = CURLBufferDefault;
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,&buffer);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	
	if(buffer.buffer == NULL)
		return NULL;
	
	GSV* gsvHandle = gsv_parse((char*)buffer.buffer);
	free(buffer.buffer);
	return gsvHandle;
}

IplImage* gsv_tile(GSV* panorama,int zoomLevel,int x,int y)
{
#ifdef GSV_DEBUG
	printf("gsv_tile(%p,%d,%d,%d)\n",panorama,zoomLevel,x,y);
#endif
	CURL* curl = curl_easy_init();
	char urlString[66+GSV_PANORAMA_ID_LENGTH];
	
	snprintf(urlString,sizeof(urlString),"http://cbk0.google.com/cbk?output=tile&panoid=%s&zoom=%d&x=%d&y=%d",panorama->dataProperties.panoramaId,zoomLevel,x,y);
	
	curl_easy_setopt(curl,CURLOPT_URL,urlString);
	curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,gsvCURLToBuffer);
	
	CURLBuffer buffer = CURLBufferDefault;
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,&buffer);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo,(unsigned char*)buffer.buffer,buffer.bufferSize);
	jpeg_read_header(&cinfo,1);
	jpeg_start_decompress(&cinfo);
	
	IplImage* tileImage = cvCreateImage(cvSize(cinfo.output_width,cinfo.output_height),IPL_DEPTH_8U,cinfo.num_components);
	JSAMPROW rowPointer[1] = { (unsigned char*) malloc(sizeof(unsigned char)*cinfo.output_width*cinfo.num_components) };
	
	while(cinfo.output_scanline < cinfo.image_height)
	{
		jpeg_read_scanlines(&cinfo,rowPointer,sizeof(rowPointer)/sizeof(JSAMPROW));
		memcpy(&tileImage->imageData[(cinfo.output_scanline-1)*cinfo.output_width*cinfo.num_components],rowPointer[0],sizeof(char)*cinfo.output_width*cinfo.num_components);
	}
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	free(buffer.buffer);
	buffer.buffer = NULL;
	
	return tileImage;
}

IplImage* gsv_panorama(GSV* panorama,int zoomLevel)
{
#ifdef GSV_DEBUG
	printf("gsv_panorama(%p,%d)\n",panorama,zoomLevel);
#endif
	int maxX = 1;
	int maxY = 1;
	
	switch(zoomLevel)
	{
		default: maxX = 1; maxY = 1; break;
		case 1: maxX = 2; maxY = 1; break;
		case 2: maxX = 4; maxY = 2; break;
		case 3: maxX = 6; maxY = 3; break;
		case 4: maxX = 13; maxY = 7; break;
		case 5: maxX = 26; maxY = 13; break;
	}
	
	IplImage* panoramaImage = cvCreateImage(cvSize(panorama->dataProperties.tileWidth*maxX,panorama->dataProperties.tileHeight*maxY),IPL_DEPTH_8U,3);
	
	for(int x=0;x<maxX;x++)
	{
		for(int y=0;y<maxY;y++)
		{
			IplImage* tileImage = gsv_tile(panorama,zoomLevel,x,y);
			
			cvSetImageROI(panoramaImage,cvRect(x*panorama->dataProperties.tileWidth,y*panorama->dataProperties.tileHeight,tileImage->width,tileImage->height));
			cvCopy(tileImage,panoramaImage);
			cvResetImageROI(panoramaImage);
			cvReleaseImage(&tileImage);
		}
	}
	
	return panoramaImage;
}

void gsv_close(GSV** panorama)
{
	free((*panorama)->dataProperties.copyright);
	free((*panorama)->dataProperties.text);
	free((*panorama)->dataProperties.streetRange);
	free((*panorama)->dataProperties.region);
	free((*panorama)->dataProperties.country);
	free((*panorama)->projectionProperties.projectionType);
	for(int i=0;i<(*panorama)->annotationProperties.numLinks;i++)
		free((*panorama)->annotationProperties.links[i].text);
	free((*panorama)->annotationProperties.links);
}
