CStreetView 1.0.1
=================

CStreetView is a third party C API for interfacing to the web service Google uses for its Street View application.

Distributed under the [MIT license](http://opensource.org/licenses/mit-license.php)

Dependencies
------------

- [opencv](http://opencv.willowgarage.com/wiki/ "OpenCV")
- [curl](http://curl.haxx.se "cURL")
- [tinyxml2](http://www.grinninglizard.com/tinyxml2/index.html "TinyXML")
- [libjpeg-turbo](http://libjpeg-turbo.virtualgl.org "libjpeg-turbo")

Changelog
---------

1.0.1:
- Changed project name to CStreetView
- Fixed bug in gsv_close that attempted to free unallocated memory
- Added null terminators to the XML downloads
- Fixed buggy handling of broken XML files from the web service
- Fixed the blue channel being represented as the red channel
- Fixed bug in gsv_close that did not adequately free the allocated memory
