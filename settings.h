#ifndef FILEFERRY_SETTINGS_H
#define FILEFERRY_SETTINGS_H

#include "common.h"

#define SETTING_VERBOSE 1
#define SETTING_DEBUG 2
#define SETTING_NOERROR 4
#define SETTING_TIMESTAMPS 8
#define SETTING_PROGRESS 16
#define SETTING_BATCH  32
#define SETTING_SIXEL  64
#define SETTING_SYSLOG 128
#define SETTING_INTEGRITY_CHECK 256

//to prevent clashing with anyting else, give this 2^31
#define SETTING_NO_DIR_LIST 2147483648

typedef struct
{
    int Flags;
    char *URL;
    char *User;
    char *Pass;
    char *LogFile;
    char *Encryption;
    char *IdentityFile;
    char *Commands;
    char *ConfigFile;
    char *FileStoresPath;
    char *ProxyChain;
    char *SmtpServer;
    char *EmailForErrors;
    char *EmailSender;
    char *WebhookForErrors;
    char *ImagePreviewSize;
    char *ImageViewers;
    char *Sixelers;
    int ProcessTimeout;
    int CommandTimeout;
    int LogMaxSize;
} TSettings;

extern TSettings *Settings;

int SettingsInit(int argc, const char *argv[]);
int SettingChange(const char *Name, const char *Value);

#endif
