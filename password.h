#ifndef FILEFERRY_PASSWORD_H
#define FILEFERRY_PASSWORD_H

#include "common.h"
#include "filestore.h"

int PasswordGet(TFileStore *FS, int Try, const char **Pass);

#endif
