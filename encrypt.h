#ifndef FILEFERRY2_ENCRYPT_H
#define FILEFERRY2_ENCRYPT_H

#include "common.h"

char *EncryptFile(char *RetStr, const char *Path, const char *Encrypt, int EncryptType);
char *DecryptFile(char *RetStr, const char *Path, const char *Encrypt, int EncryptType);

#endif
