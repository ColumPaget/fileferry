#include "rss.h"
#include "fileitem.h"

static void RSS_ListDir_Items(ListNode *FileList, ListNode *ItemInfo)
{
    TFileItem *FI;
    const char *ptr;
    ListNode *Curr;

    ptr=ParserGetValue(ItemInfo, "enclosure_url");
    if (! StrValid(ptr)) ptr=ParserGetValue(ItemInfo, "link");
    FI=FileItemCreate(ptr, 0, 0, 0);
    ptr=ParserGetValue(ItemInfo, "enclosure_length");
    if (StrValid(ptr)) FI->size=atoi(ptr);
    ptr=ParserGetValue(ItemInfo, "pubDate");
    if (StrValid(ptr)) FI->mtime=DateStrToSecs("%a, %d %b %Y %H:%M:%S", ptr, NULL);
    FI->title=CopyStr(FI->title, ParserGetValue(ItemInfo, "title"));
    FI->description=CopyStr(FI->description, ParserGetValue(ItemInfo, "description"));

    ListAddNamedItem(FileList, FI->name, FI);
}



void RSS_ListDir(ListNode *FileList, const char *RSS)
{
    ListNode *Parser, *Curr;

    Parser=ParserParseDocument("rss", RSS);
    Curr=ListGetNext(Parser);
    while (Curr)
    {
        if (strncmp(Curr->Tag, "item:", 5)==0)
        {
            RSS_ListDir_Items(FileList, Curr);
        }
        Curr=ListGetNext(Curr);
    }
}

