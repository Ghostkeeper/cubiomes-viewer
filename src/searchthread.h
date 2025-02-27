#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include <QThread>
#include <QThreadPool>
#include <QMutex>
#include <QVector>
#include <QElapsedTimer>

#include "searchitem.h"


class FormSearchControl;

struct SearchThread : QThread
{
    Q_OBJECT
public:
    struct CheckedSeed
    {
        uint8_t valid;
        int64_t seed;
    };

    SearchThread(FormSearchControl *parent);

    bool set(QObject *mainwin, int type, int threads, Gen48Settings gen48,
             std::vector<int64_t>& slist, int64_t sstart, int mc,
             const QVector<Condition>& cv, int itemsize, int queuesize);

    virtual void run() override;

    void stop() { abort = true; pool.clear(); }
    SearchItem *startNextItem();

signals:
    void progress(uint64_t last, uint64_t end, int64_t seed);
    void searchEnded();     // search thread is exiting (e.g. abort or done)
    void searchFinish();    // search ended and is comlete

public slots:
    void onItemDone(uint64_t itemid, int64_t seed, bool isdone);
    void onItemCanceled(uint64_t itemid);

public:
    FormSearchControl     * parent;

    QVector<Condition>      condvec;
    SearchItemGenerator     itemgen;
    QThreadPool             pool;
    QAtomicInt              activecnt;  // running + queued items
    std::atomic_bool        abort;
    std::atomic_bool        reqstop;

    QVector<CheckedSeed>    recieved;
    uint64_t                lastid;     // last item id
};

#endif // SEARCHTHREAD_H
