#ifndef FORMSEARCHCONTROL_H
#define FORMSEARCHCONTROL_H

#include <QWidget>
#include <QTimer>

#include "searchthread.h"
#include "protobasedialog.h"
#include "settings.h"

namespace Ui {
class FormSearchControl;
}

class MainWindow;


class FormSearchControl : public QWidget
{
    Q_OBJECT

public:
    explicit FormSearchControl(MainWindow *parent);
    ~FormSearchControl();

    QVector<int64_t> getResults();
    SearchConfig getSearchConfig();
    bool setSearchConfig(SearchConfig s, bool quiet);

    bool isbusy();
    bool setList64(QString path, bool quiet);

    void searchLockUi(bool lock);

    void setSearchMode(int mode);

signals:
    void selectedSeedChanged(int64_t seed);
    void searchStatusChanged(bool running);
    void resultsAdded(int cnt);

public slots:
    void on_buttonClear_clicked();
    void on_buttonStart_clicked();
    void on_buttonLoadList_clicked();

    void on_listResults_itemSelectionChanged();
    void on_listResults_customContextMenuRequested(const QPoint& pos);

    void on_buttonSearchHelp_clicked();

    void on_comboSearchType_currentIndexChanged(int index);

    void pasteResults();
    int pasteList(bool dummy);
    int searchResultsAdd(QVector<int64_t> seeds, bool countonly);
    void searchProgressReset();
    void searchProgress(uint64_t last, uint64_t end, int64_t seed);
    void searchFinish();
    void resultTimeout();
    void removeCurrent();
    void copyResults();

private:
    MainWindow *parent;
    Ui::FormSearchControl *ui;
    SearchThread sthread;
    QTimer stimer;

    // the seed list option is not stored in a widget but is loaded with the "..." button
    QString slist64path;
    std::vector<int64_t> slist64;

    // buffer for seed candidates while search is running
    std::vector<int64_t> slist;
};

#endif // FORMSEARCHCONTROL_H
