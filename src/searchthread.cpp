#include "searchthread.h"
#include "formsearchcontrol.h"
#include "cutil.h"
#include "mainwindow.h" // TODO: remove

#include <QMessageBox>
#include <QEventLoop>

#include <x86intrin.h>

#define ITEM_SIZE 1024


extern MainWindow *gMainWindowInstance;


SearchThread::SearchThread(FormSearchControl *parent)
    : QThread()
    , parent(parent)
    , condvec()
    , itemgen()
    , pool()
    , activecnt()
    , abort()
    , reqstop()
    , recieved()
    , lastid()
{
    itemgen.abort = &abort;
}

bool SearchThread::set(QObject *mainwin, int type, int threads, Gen48Settings gen48,
                       std::vector<int64_t>& slist, int64_t sstart, int mc,
                       const QVector<Condition>& cv, int itemsize, int queuesize)
{
    char refbuf[100] = {};

    for (const Condition& c : cv)
    {
        if (c.save < 1 || c.save > 99)
        {
            QMessageBox::warning(NULL, "Warning", QString::asprintf("Condition with invalid ID [%02d].", c.save));
            return false;
        }
        if (c.relative && refbuf[c.relative] == 0)
        {
            QMessageBox::warning(NULL, "Warning", QString::asprintf(
                    "Condition with ID [%02d] has a broken reference position:\n"
                    "condition missing or out of order.", c.save));
            return false;
        }
        if (++refbuf[c.save] > 1)
        {
            QMessageBox::warning(NULL, "Warning", QString::asprintf("More than one condition with ID [%02d].", c.save));
            return false;
        }
        if (c.type < 0 || c.type >= FILTER_MAX)
        {
            QMessageBox::warning(NULL, "Error", QString::asprintf("Encountered invalid filter type %d in condition ID [%02d].", c.type, c.save));
            return false;
        }
        if (mc < g_filterinfo.list[c.type].mcmin)
        {
            const char *mcs = mc2str(g_filterinfo.list[c.type].mcmin);
            QString s = QString::asprintf("Condition [%02d] requires a minimum Minecraft version of %s.", c.save, mcs);
            QMessageBox::warning(NULL, "Warning", s);
            return false;
        }
        if (c.type >= F_BIOME && c.type <= F_BIOME_256_OTEMP)
        {
            if ((c.exclb & (c.bfilter.riverToFind | c.bfilter.oceanToFind)) ||
                (c.exclm & c.bfilter.riverToFindM))
            {
                QMessageBox::warning(NULL, "Warning", QString::asprintf("Biome filter condition with ID [%02d] has contradicting flags for include and exclude.", c.save));
                return false;
            }
            // TODO: compare mc version and available biomes
            if (c.count == 0)
            {
                QMessageBox::information(NULL, "Info", QString::asprintf("Biome filter condition with ID [%02d] specifies no biomes.", c.save));
            }
        }
        if (c.type == F_TEMPS)
        {
            int w = c.x2 - c.x1 + 1;
            int h = c.z2 - c.z1 + 1;
            if (c.count > w * h)
            {
                QMessageBox::warning(NULL, "Warning", QString::asprintf(
                        "Temperature category condition with ID [%02d] has too many restrictions (%d) for the area (%d x %d).",
                        c.save, c.count, w, h));
                return false;
            }
        }
    }

    condvec = cv;
    itemgen.init(mainwin, mc, condvec.data(), condvec.size(), gen48, slist, itemsize, type, sstart);
    pool.setMaxThreadCount(threads);
    recieved.resize(queuesize);
    lastid = itemgen.itemid;
    reqstop = false;
    abort = false;
    return true;
}


void SearchThread::run()
{
    itemgen.presearch();
    pool.waitForDone();

    uint64_t prog, end;
    itemgen.getProgress(&prog, &end);
    emit progress(prog, end, itemgen.seed);

    for (int idx = 0; idx < recieved.size(); idx++)
    {
        recieved[idx].valid = false;
        startNextItem();
    }

    emit searchEnded();
}


SearchItem *SearchThread::startNextItem()
{
    SearchItem *item = itemgen.requestItem();
    if (!item)
        return NULL;
    // call back here when done
    QObject::connect(item, &SearchItem::itemDone, this, &SearchThread::onItemDone, Qt::BlockingQueuedConnection);
    QObject::connect(item, &SearchItem::canceled, this, &SearchThread::onItemCanceled, Qt::QueuedConnection);
    // redirect results to mainwindow
    QObject::connect(item, &SearchItem::results, parent, &FormSearchControl::searchResultsAdd, Qt::BlockingQueuedConnection);
    ++activecnt;
    pool.start(item);
    return item;
}


void SearchThread::onItemDone(uint64_t itemid, int64_t seed, bool isdone)
{
    --activecnt;

    itemgen.isdone |= isdone;
    if (!itemgen.isdone && !reqstop && !abort)
    {
        if (itemid == lastid)
        {
            int64_t len = recieved.size();
            int idx;
            for (idx = 1; idx < len; idx++)
            {
                if (!recieved[idx].valid)
                    break;
            }

            lastid += idx;

            for (int i = idx; i < len; i++)
                recieved[i-idx] = recieved[i];
            for (int i = len-idx; i < len; i++)
                recieved[i].valid = false;

            for (int i = 0; i < idx; i++)
                startNextItem();

            uint64_t prog, end;
            itemgen.getProgress(&prog, &end);
            emit progress(prog, end, seed);
        }
        else
        {
            int idx = itemid - lastid;
            recieved[idx].valid = true;
            recieved[idx].seed = seed;
        }
    }

    if (activecnt == 0)
        emit searchFinish();
}

void SearchThread::onItemCanceled(uint64_t itemid)
{
    (void) itemid;
    --activecnt;
    if (activecnt == 0)
        emit searchFinish();
}

