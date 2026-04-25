#ifndef SITEM_H
#define SITEM_H
#include<string>
struct ScheduleItem
{
    int sID;
    int status;
    int state;
    std::string ocName;
    long long beginTime;
    int stayTime;
    std::string remark;
};
#endif