#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QWidget>

#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QVector>

#include <atomic>

#include "searchthread.h"
#include "configdialog.h"
#include "formconditions.h"
#include "formgen48.h"
#include "formsearchcontrol.h"

namespace Ui {
class MainWindow;
}

Q_DECLARE_METATYPE(int64_t)
Q_DECLARE_METATYPE(uint64_t)
Q_DECLARE_METATYPE(Pos)
Q_DECLARE_METATYPE(Config)

class MapView;
class ProtoBaseDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QAction *addMapAction(int stype, const char *iconpath, const char *tip);

    bool getSeed(int *mc, int64_t *seed, bool applyrand = true);
    bool setSeed(int mc, int64_t seed);
    MapView *getMapView();

protected:
    void saveSettings();
    void loadSettings();
    bool saveProgress(QString fnam, bool quiet = false);
    bool loadProgress(QString fnam, bool quiet = false);
    void updateMapSeed();

public slots:
    void warning(QString title, QString text);
    void mapGoto(qreal x, qreal z, qreal scale);

    void openProtobaseMsg(QString path);
    void closeProtobaseMsg();

private slots:
    void on_comboBoxMC_currentIndexChanged(int a);
    void on_seedEdit_editingFinished();
    void on_seedEdit_textChanged(const QString &arg1);

    void on_actionSave_triggered();
    void on_actionLoad_triggered();
    void on_actionQuit_triggered();
    void on_actionPreferences_triggered();
    void on_actionGo_to_triggered();
    void on_actionScan_seed_for_Quad_Huts_triggered();
    void on_actionOpen_shadow_seed_triggered();
    void on_actionAbout_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionAddShadow_triggered();

    void on_mapView_customContextMenuRequested(const QPoint &pos);

    void on_cboxArea_toggled(bool checked);
    void on_lineRadius_editingFinished();
    void on_buttonFromVisible_clicked();
    void on_buttonAnalysis_clicked();
    void on_treeAnalysis_itemDoubleClicked(QTreeWidgetItem *item);
    void on_buttonExport_clicked();

    void on_actionSearch_seed_list_triggered();
    void on_actionSearch_full_seed_space_triggered();

    // internal events
    void onAutosaveTimeout();
    void onActionMapToggled(int stype, bool a);
    void onConditionsChanged();
    void onGen48Changed();
    void onSelectedSeedChanged(int64_t seed);
    void onSearchStatusChanged(bool running);
    void copyCoord();


public:
    Ui::MainWindow *ui;
    FormConditions *formCond;
    FormGen48 *formGen48;
    FormSearchControl *formControl;
    Config config;
    QString prevdir;
    QTimer autosaveTimer;

    QVector<QAction*> saction;
    ProtoBaseDialog *protodialog;
};

#endif // MAINWINDOW_H
