#pragma once

#include <QThread>

#include "Arguments.h"

/* Base class for all the flasher threads */
class FlasherThread : public QThread
{
    Q_OBJECT

public:
    FlasherThread(CommonArgs commonArgs);

signals:
    void progressChanged(const int cur);
    void progressRangeChanged(const int min, const int max);
    void progressSetText(const QString& text);
    void statusTextAdded(const QString& text);
    void RxSpeedUpdate(const int value);
    void TxSpeedUpdate(const int value);


public slots:
    virtual void onCanceled();

protected:
    void run();

    bool canceled;

    CommonArgs commonArgs;

private:
    virtual void Operation() = 0;
};
