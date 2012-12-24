/*
 Copyright (c) 2012 Will Sackfield
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cstreetview.h"

void breadthFirstSearch(double latitude,double longitude,const char* city,const char* country,int maxCount)
{
	GSV* panorama = gsv_open(latitude,longitude);
	if(panorama == NULL)
		return;
	
	char completedPanoramaIds[maxCount][GSV_PANORAMA_ID_LENGTH];
	memset(completedPanoramaIds,'\0',sizeof(char)*maxCount*GSV_PANORAMA_ID_LENGTH);
	int numberCompleted = 0;
	
	char** panoramaIds = (char**) malloc(sizeof(char*));
	panoramaIds[0] = (char*) malloc(sizeof(char)*GSV_PANORAMA_ID_LENGTH);
	memcpy(panoramaIds[0],panorama->dataProperties.panoramaId,sizeof(char)*GSV_PANORAMA_ID_LENGTH);
	int numPanoramaIds = 1;
	
	gsv_close(&panorama);
	
	while(maxCount > 0)
	{
		char** tmpPanoramaIds = panoramaIds;
		int tmpNumPanoramaIds = numPanoramaIds;
		
		panoramaIds = NULL;
		numPanoramaIds = 0;
		
		for(int i=0;i<tmpNumPanoramaIds;i++)
		{
			panorama = gsv_open(tmpPanoramaIds[i]);
			free(tmpPanoramaIds[i]);
			
			char panoramaFileName[GSV_PANORAMA_ID_LENGTH+1+2+1+64+4+1+3+1+18];
			IplImage* panoramaImage = gsv_panorama(panorama,5);
			snprintf(panoramaFileName,sizeof(panoramaFileName),"example_panoramas/%s-%s-%d-%s.jpg",country,city,100-maxCount,panorama->dataProperties.panoramaId);
			cvSaveImage(panoramaFileName,panoramaImage);
			cvReleaseImage(&panoramaImage);
			memcpy(completedPanoramaIds[numberCompleted],panorama->dataProperties.panoramaId,sizeof(char)*GSV_PANORAMA_ID_LENGTH);
			numberCompleted++;
			
			for(int i=0;i<panorama->annotationProperties.numLinks;i++)
			{
				char* linkPanoramaId = panorama->annotationProperties.links[i].panoramaId;
				int found=0;
				for(int j=0;j<numberCompleted;j++)
				{
					if(memcmp(completedPanoramaIds[j],linkPanoramaId,GSV_PANORAMA_ID_LENGTH) == 0)
					{
						found = 1;
						break;
					}
				}
				if(found == 0)
				{
					numPanoramaIds++;
					char* panoramaId = (char*) malloc(sizeof(char)*GSV_PANORAMA_ID_LENGTH);
					memcpy(panoramaId,panorama->annotationProperties.links[i].panoramaId,sizeof(char)*GSV_PANORAMA_ID_LENGTH);
					panoramaIds = (char**) realloc(panoramaIds,sizeof(char*)*numPanoramaIds);
					panoramaIds[numPanoramaIds-1] = panoramaId;
				}
			}
			
			gsv_close(&panorama);
			
			if(--maxCount == 0)
				break;
		}
		
		free(tmpPanoramaIds);
	}
}

int main(int argc,char* argv[])
{
	if(argc != 5)
	{
		printf("Invalid arguments: example [latitude] [longitude] [city] [country]\n");
		return EXIT_FAILURE;
	}
	
	breadthFirstSearch(atof(argv[1]),atof(argv[2]),argv[3],argv[4],100);
	
	return EXIT_SUCCESS;
}
