#ifndef FILEFERRY_HTML_H
#define FILEFERRY_HTML_H

#include "common.h"


int IsDownloadableURL(const char *URL);
void HTML_ListDir(ListNode *FileList, const char *HTML);

#endif
