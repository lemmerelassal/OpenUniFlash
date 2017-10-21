#include "NorCommonThread.h"

NorCommonThread::NorCommonThread(CommonArgs _commonArgs, NorArgs _norArgs) :
    FlasherThread(_commonArgs), norArgs(_norArgs)
{
}
