#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "gotodialog.h"
#include "quadlistdialog.h"
#include "aboutdialog.h"
#include "protobasedialog.h"
#include "filterdialog.h"

#include "quad.h"
#include "cutil.h"

#include <QIntValidator>
#include <QMetaType>
#include <QMessageBox>
#include <QtDebug>
#include <QDataStream>
#include <QMenu>
#include <QClipboard>
#include <QFont>
#include <QFileDialog>
#include <QTextStream>
#include <QSettings>
#include <QTreeWidget>
#include <QDateTime>
#include <QStandardPaths>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , prevdir(".")
    , protodialog()
{
    ui->setupUi(this);

    formCond = new FormConditions(this);
    ui->collapseConstraints->init("Conditions", formCond, false);
    connect(formCond, &FormConditions::changed, this, &MainWindow::onConditionsChanged);
    ui->collapseConstraints->setInfo(
        "Help: Search conditions",
        "The search conditions define the properties by which potential seeds are filtered."
        "\n\n"
        "Conditions can reference each other to produce relative positionial "
        "dependencies (indicated with the ID in square brackets [XY]). "
        "The conditions will be checked in the same order they are listed, "
        "so make sure that references are not broken. Conditions can be reordered "
        "by dragging the list items."
    );

    formGen48 = new FormGen48(this);
    ui->collapseGen48->init("Seed generator (48-bit)", formGen48, true);
    connect(formGen48, &FormGen48::changed, this, &MainWindow::onGen48Changed);
    ui->collapseGen48->setInfo(
        "Help: Seed generator",
        "For some searches, the 48-bit structure seed candidates can be generated without searching, "
        "which can vastly reduce the search space that has to be checked. The generator mode \"Auto\" "
        "is recommended for general use, which automatically selects suitable options based on the "
        "conditions list."
    );

    formControl = new FormSearchControl(this);
    ui->collapseControl->init("Matching seeds", formControl, false);
    connect(formControl, &FormSearchControl::selectedSeedChanged, this, &MainWindow::onSelectedSeedChanged);
    connect(formControl, &FormSearchControl::searchStatusChanged, this, &MainWindow::onSearchStatusChanged);


    this->update();

    //ui->frameMap->layout()->addWidget(ui->toolBar);
    //ui->toolBar->setContentsMargins(0, 0, 0, 0);

    QAction *toorigin = new QAction(QIcon(":/icons/origin.png"), "Goto origin", this);
    toorigin->connect(toorigin, &QAction::triggered, [=](){ this->mapGoto(0,0,16); });
    ui->toolBar->addAction(toorigin);
    ui->toolBar->addSeparator();

    saction.resize(STRUCT_NUM);
    addMapAction(D_GRID, "grid", "Show grid");
    addMapAction(D_SLIME, "slime", "Show slime chunks");
    ui->toolBar->addSeparator();
    addMapAction(D_DESERT, "desert", "Show desert pyramid");
    addMapAction(D_JUNGLE, "jungle", "Show jungle temples");
    addMapAction(D_IGLOO, "igloo", "Show igloos");
    addMapAction(D_HUT, "hut", "Show swamp huts");
    addMapAction(D_VILLAGE, "village", "Show villages");
    addMapAction(D_MANSION, "mansion", "Show woodland mansions");
    addMapAction(D_MONUMENT, "monument", "Show ocean monuments");
    addMapAction(D_RUINS, "ruins", "Show ocean ruins");
    addMapAction(D_SHIPWRECK, "shipwreck", "Show shipwrecks");
    addMapAction(D_TREASURE, "treasure", "Show buried treasures");
    addMapAction(D_OUTPOST, "outpost", "Show illager outposts");
    addMapAction(D_PORTAL, "portal", "Show ruined portals");
    addMapAction(D_SPAWN, "spawn", "Show world spawn");
    addMapAction(D_STRONGHOLD, "stronghold", "Show strongholds");

    saction[D_GRID]->setChecked(true);

    protodialog = new ProtoBaseDialog(this);

    ui->splitterMap->setSizes(QList<int>({6000, 10000}));
    ui->splitterSearch->setSizes(QList<int>({1000, 1000, 2000}));

    qRegisterMetaType< int64_t >("int64_t");
    qRegisterMetaType< uint64_t >("uint64_t");
    qRegisterMetaType< QVector<int64_t> >("QVector<int64_t>");
    qRegisterMetaType< Config >("Config");

    QIntValidator *intval = new QIntValidator(this);
    ui->lineRadius->setValidator(intval);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);
    on_cboxArea_toggled(false);

    formCond->updateSensitivity();

    connect(&autosaveTimer, &QTimer::timeout, this, &MainWindow::onAutosaveTimeout);

    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

QAction *MainWindow::addMapAction(int stype, const char *iconpath, const char *tip)
{
    QIcon icon;
    QString inam = QString(":icons/") + iconpath;
    icon.addPixmap(QPixmap(inam + ".png"), QIcon::Normal, QIcon::On);
    icon.addPixmap(QPixmap(inam + "_d.png"), QIcon::Normal, QIcon::Off);
    QAction *action = new QAction(icon, tip, this);
    action->setCheckable(true);
    action->connect(action, &QAction::toggled, [=](bool state){ this->onActionMapToggled(stype, state); });
    ui->toolBar->addAction(action);
    saction[stype] = action;
    return action;
}


MapView* MainWindow::getMapView()
{
    return ui->mapView;
}

bool MainWindow::getSeed(int *mc, int64_t *seed, bool applyrand)
{
    bool ok = true;
    if (mc)
    {
        const std::string& mcs = ui->comboBoxMC->currentText().toStdString();
        *mc = str2mc(mcs.c_str());
        if (*mc < 0)
        {
            *mc = MC_1_16;
            qDebug() << "Unknown MC version: " << *mc;
            ok = false;
        }
    }

    if (seed)
    {
        int v = str2seed(ui->seedEdit->text(), seed);
        if (applyrand && v == S_RANDOM)
            ui->seedEdit->setText(QString::asprintf("%" PRId64, *seed));
    }

    return ok;
}

bool MainWindow::setSeed(int mc, int64_t seed)
{
    const char *mcstr = mc2str(mc);
    if (!mcstr)
    {
        qDebug() << "Unknown MC version: " << mc;
        return false;
    }

    ui->comboBoxMC->setCurrentText(mcstr);
    ui->seedEdit->setText(QString::asprintf("%" PRId64, seed));
    ui->mapView->setSeed(mc, seed);
    return true;
}

void MainWindow::saveSettings()
{
    QSettings settings("cubiomes-viewer", "cubiomes-viewer");
    settings.setValue("mainwindow/size", size());
    settings.setValue("mainwindow/pos", pos());
    settings.setValue("mainwindow/prevdir", prevdir);
    settings.setValue("config/restoreSession", config.restoreSession);
    settings.setValue("config/autosaveCycle", config.autosaveCycle);
    settings.setValue("config/smoothMotion", config.smoothMotion);
    settings.setValue("config/seedsPerItem", config.seedsPerItem);
    settings.setValue("config/queueSize", config.queueSize);
    settings.setValue("config/maxMatching", config.maxMatching);

    int mc = MC_1_16;
    int64_t seed = 0;
    getSeed(&mc, &seed, false);
    settings.setValue("map/mc", mc);
    settings.setValue("map/seed", (qlonglong)seed);
    settings.setValue("map/x", ui->mapView->getX());
    settings.setValue("map/z", ui->mapView->getZ());
    settings.setValue("map/scale", ui->mapView->getScale());
    for (int stype = 0; stype < STRUCT_NUM; stype++)
    {
        QString s = QString("map/show_") + mapopt2str(stype);
        settings.setValue(s, ui->mapView->getShow(stype));
    }
    if (config.restoreSession)
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        saveProgress(path + "/session.save", true);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("cubiomes-viewer", "cubiomes-viewer");
    resize(settings.value("mainwindow/size", size()).toSize());
    move(settings.value("mainwindow/pos", pos()).toPoint());
    prevdir = settings.value("mainwindow/prevdir", pos()).toString();
    config.smoothMotion = settings.value("config/smoothMotion", config.smoothMotion).toBool();
    config.restoreSession = settings.value("config/restoreSession", config.restoreSession).toBool();
    config.autosaveCycle = settings.value("config/autosaveCycle", config.autosaveCycle).toInt();
    config.seedsPerItem = settings.value("config/seedsPerItem", config.seedsPerItem).toInt();
    config.queueSize = settings.value("config/queueSize", config.queueSize).toInt();
    config.maxMatching = settings.value("config/maxMatching", config.maxMatching).toInt();

    ui->mapView->setSmoothMotion(config.smoothMotion);

    int mc = MC_1_16;
    int64_t seed = 0;
    getSeed(&mc, &seed, true);
    mc = settings.value("map/mc", mc).toInt();
    seed = settings.value("map/seed", QVariant::fromValue(seed)).toLongLong();
    setSeed(mc, seed);

    qreal x = ui->mapView->getX();
    qreal z = ui->mapView->getZ();
    qreal scale = ui->mapView->getScale();

    x = settings.value("map/x", x).toDouble();
    z = settings.value("map/z", z).toDouble();
    scale = settings.value("map/scale", scale).toDouble();

    for (int stype = 0; stype < STRUCT_NUM; stype++)
    {
        bool show = ui->mapView->getShow(stype);
        QString sopt = QString("map/show_") + mapopt2str(stype);
        show = settings.value(sopt, show).toBool();
        saction[stype]->setChecked(show);
        ui->mapView->setShow(stype, show);
    }
    mapGoto(x, z, scale);

    if (config.restoreSession)
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        loadProgress(path + "/session.save", true);
    }

    if (config.autosaveCycle > 0)
    {
        autosaveTimer.setInterval(config.autosaveCycle * 60000);
        autosaveTimer.start();
    }
    else
    {
        autosaveTimer.stop();
    }
}

bool MainWindow::saveProgress(QString fnam, bool quiet)
{
    QFile file(fnam);

    if (!file.open(QIODevice::WriteOnly))
    {
        if (!quiet)
            warning("Warning", "Failed to open file.");
        return false;
    }

    SearchConfig searchconf = formControl->getSearchConfig();
    Gen48Settings gen48 = formGen48->getSettings(false);
    QVector<Condition> condvec = formCond->getConditions();
    QVector<int64_t> results = formControl->getResults();

    int mc = MC_1_16;
    getSeed(&mc, 0);

    QTextStream stream(&file);
    stream << "#Version:  " << VERS_MAJOR << "." << VERS_MINOR << "." << VERS_PATCH << "\n";
    stream << "#Time:     " << QDateTime::currentDateTime().toString() << "\n";
    // MC version of the session should take priority over the one in the settings
    stream << "#MC:       " << mc2str(mc) << "\n";

    stream << "#Search:   " << searchconf.searchmode << "\n";
    if (!searchconf.slist64path.isEmpty())
        stream << "#List64:   " << searchconf.slist64path.replace("\n", "") << "\n";
    stream << "#Progress: " << searchconf.startseed << "\n";
    stream << "#Threads:  " << searchconf.threads << "\n";
    stream << "#ResStop:  " << (int)searchconf.stoponres << "\n";

    stream << "#Mode48:   " << gen48.mode << "\n";
    if (!gen48.slist48path.isEmpty())
        stream << "#List48:   " << gen48.slist48path.replace("\n", "") << "\n";
    stream << "#HutQual:  " << gen48.qual << "\n";
    stream << "#MonArea:  " << gen48.qmarea << "\n";
    if (gen48.salt != 0)
        stream << "#Salt:     " << gen48.salt << "\n";
    if (gen48.manualarea)
    {
        stream << "#Gen48X1:  " << gen48.x1 << "\n";
        stream << "#Gen48Z1:  " << gen48.z1 << "\n";
        stream << "#Gen48X2:  " << gen48.x2 << "\n";
        stream << "#Gen48Z2:  " << gen48.z2 << "\n";
    }

    for (Condition &c : condvec)
        stream << "#Cond: " << QByteArray((const char*) &c, sizeof(Condition)).toHex() << "\n";

    for (int64_t s : results)
        stream << QString::asprintf("%" PRId64 "\n", s);

    return true;
}

bool MainWindow::loadProgress(QString fnam, bool quiet)
{
    QFile file(fnam);

    if (!file.open(QIODevice::ReadOnly))
    {
        if (!quiet)
            warning("Warning", "Failed to open file.");
        return false;
    }

    int major = 0, minor = 0, patch = 0;
    SearchConfig searchconf = formControl->getSearchConfig();
    Gen48Settings gen48 = formGen48->getSettings(false);
    QVector<Condition> condvec;
    QVector<int64_t> seeds;

    char buf[4096];
    int tmp;
    int mc = MC_1_16;
    int64_t seed;
    getSeed(&mc, &seed, true);

    QTextStream stream(&file);
    QString line;
    line = stream.readLine();
    if (sscanf(line.toLatin1().data(), "#Version: %d.%d.%d", &major, &minor, &patch) != 3)
        return false;
    if (cmpVers(major, minor, patch) > 0 && !quiet)
        warning("Warning", "Progress file was created with a newer version.");

    while (stream.status() == QTextStream::Ok)
    {
        line = stream.readLine();
        QByteArray ba = line.toLatin1();
        const char *p = ba.data();

        if (line.isEmpty())
            break;

        if (line.startsWith("#Time:")) continue;
        else if (sscanf(p, "#MC:       %8[^\n]", buf) == 1)                     { mc = str2mc(buf); if (mc < 0) return false; }
        // SearchConfig
        else if (sscanf(p, "#Search:   %d", &searchconf.searchmode) == 1)       {}
        else if (sscanf(p, "#Progress: %" PRId64, &searchconf.startseed) == 1)  {}
        else if (sscanf(p, "#Threads:  %d", &searchconf.threads) == 1)          {}
        else if (sscanf(p, "#ResStop:  %d", &tmp) == 1)                         { searchconf.stoponres = tmp; }
        else if (line.startsWith("#List64:   "))                                { searchconf.slist64path = line.mid(11).trimmed(); }
        // Gen48Settings
        else if (sscanf(p, "#Mode48:   %d", &gen48.mode) == 1)                  {}
        else if (sscanf(p, "#HutQual:  %d", &gen48.qual) == 1)                  {}
        else if (sscanf(p, "#MonArea:  %d", &gen48.qmarea) == 1)                {}
        else if (sscanf(p, "#Salt:     %" PRId64, &gen48.salt) == 1)            {}
        else if (sscanf(p, "#Gen48X1:  %d", &gen48.x1) == 1)                    { gen48.manualarea = true; }
        else if (sscanf(p, "#Gen48Z1:  %d", &gen48.z1) == 1)                    { gen48.manualarea = true; }
        else if (sscanf(p, "#Gen48X2:  %d", &gen48.x2) == 1)                    { gen48.manualarea = true; }
        else if (sscanf(p, "#Gen48Z2:  %d", &gen48.z2) == 1)                    { gen48.manualarea = true; }
        else if (line.startsWith("#List48:   "))                                { gen48.slist48path = line.mid(11).trimmed(); }
        // Conditions
        else if (line.startsWith("#Cond:"))
        {
            QString hex = line.mid(6).trimmed();
            QByteArray ba = QByteArray::fromHex(QByteArray(hex.toLatin1().data()));
            if (ba.size() == sizeof(Condition))
            {
                Condition c = *(Condition*) ba.data();
                condvec.push_back(c);
            }
            else return false;
        }
        else
        {
            int64_t s;
            if (sscanf(line.toLatin1().data(), "%" PRId64, &s) == 1)
                seeds.push_back(s);
            else return false;
        }
    }

    setSeed(mc, seed);

    formControl->on_buttonClear_clicked();
    formControl->searchResultsAdd(seeds, false);
    formControl->setSearchConfig(searchconf, quiet);

    formGen48->setSettings(gen48, quiet);

    formCond->on_buttonRemoveAll_clicked();
    for (Condition &c : condvec)
    {
        QListWidgetItem *item = new QListWidgetItem();
        formCond->addItemCondition(item, c);
    }

    return true;
}


void MainWindow::updateMapSeed()
{
    int mc;
    int64_t seed;
    if (getSeed(&mc, &seed))
        ui->mapView->setSeed(mc, seed);
}


void MainWindow::warning(QString title, QString text)
{
    QMessageBox::warning(this, title, text, QMessageBox::Ok);
}

void MainWindow::mapGoto(qreal x, qreal z, qreal scale)
{
    ui->mapView->setView(x, z, scale);
}

void MainWindow::openProtobaseMsg(QString path)
{
    protodialog->setPath(path);
    protodialog->show();
}

void MainWindow::closeProtobaseMsg()
{
    if (protodialog->closeOnDone())
        protodialog->close();
}

void MainWindow::on_comboBoxMC_currentIndexChanged(int)
{
    updateMapSeed();
    update();
}

void MainWindow::on_seedEdit_editingFinished()
{
    updateMapSeed();
    update();
}

void MainWindow::on_seedEdit_textChanged(const QString &a)
{
    int64_t s;
    int v = str2seed(a, &s);
    switch (v)
    {
        case 0: ui->labelSeedType->setText("(text)"); break;
        case 1: ui->labelSeedType->setText("(numeric)"); break;
        case 2: ui->labelSeedType->setText("(random)"); break;
    }
}

void MainWindow::on_actionSave_triggered()
{
    QString fnam = QFileDialog::getSaveFileName(this, "Save progress", prevdir, "Text files (*.txt);;Any files (*)");
    if (!fnam.isEmpty())
    {
        QFileInfo finfo(fnam);
        prevdir = finfo.absolutePath();
        saveProgress(fnam);
    }
}

void MainWindow::on_actionLoad_triggered()
{
    if (formControl->isbusy())
    {
        warning("Warning", "Cannot load progress: search is still active.");
        return;
    }
    QString fnam = QFileDialog::getOpenFileName(this, "Load progress", prevdir, "Text files (*.txt);;Any files (*)");
    if (!fnam.isEmpty())
    {
        QFileInfo finfo(fnam);
        prevdir = finfo.absolutePath();
        if (!loadProgress(fnam))
            warning("Warning", "Failed to parse progress file.");
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionPreferences_triggered()
{
    ConfigDialog *dialog = new ConfigDialog(this, &config);
    int status = dialog->exec();
    if (status == QDialog::Accepted)
    {
        config = dialog->getSettings();
        ui->mapView->setSmoothMotion(config.smoothMotion);

        if (config.autosaveCycle)
        {
            autosaveTimer.setInterval(config.autosaveCycle * 60000);
            autosaveTimer.start();
        }
        else
        {
            autosaveTimer.stop();
        }
    }
}

void MainWindow::on_actionGo_to_triggered()
{
    GotoDialog *dialog = new GotoDialog(this, ui->mapView->getX(), ui->mapView->getZ(), ui->mapView->getScale());
    dialog->show();
}

void MainWindow::on_actionScan_seed_for_Quad_Huts_triggered()
{
    QuadListDialog *dialog = new QuadListDialog(this);
    dialog->show();
}

void MainWindow::on_actionOpen_shadow_seed_triggered()
{
    int mc;
    int64_t seed;
    if (getSeed(&mc, &seed))
    {
        setSeed(mc, getShadow(seed));
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog *dialog = new AboutDialog(this);
    dialog->show();
}

void MainWindow::on_actionCopy_triggered()
{
    formControl->copyResults();
}

void MainWindow::on_actionPaste_triggered()
{
    formControl->pasteResults();
}

void MainWindow::on_actionAddShadow_triggered()
{
    QVector<int64_t> results = formControl->getResults();
    QVector<int64_t> shadows;
    shadows.reserve(results.size());
    for (int64_t s : results)
        shadows.push_back( getShadow(s) );
    formControl->searchResultsAdd(shadows, false);
}

void MainWindow::on_mapView_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction("Copy coordinates", this, &MainWindow::copyCoord);
    menu.addAction("Go to coordinates...", this, &MainWindow::on_actionGo_to_triggered);
    menu.exec(ui->mapView->mapToGlobal(pos));
}

void MainWindow::on_cboxArea_toggled(bool checked)
{
    ui->labelSquareArea->setEnabled(!checked);
    ui->lineRadius->setEnabled(!checked);
    ui->labelX1->setEnabled(checked);
    ui->labelZ1->setEnabled(checked);
    ui->labelX2->setEnabled(checked);
    ui->labelZ2->setEnabled(checked);
    ui->lineEditX1->setEnabled(checked);
    ui->lineEditZ1->setEnabled(checked);
    ui->lineEditX2->setEnabled(checked);
    ui->lineEditZ2->setEnabled(checked);
}

void MainWindow::on_lineRadius_editingFinished()
{
    on_buttonAnalysis_clicked();
}

void MainWindow::on_buttonFromVisible_clicked()
{
    qreal uiw = ui->mapView->width() * ui->mapView->getScale();
    qreal uih = ui->mapView->height() * ui->mapView->getScale();
    int bx0 = (int) floor(ui->mapView->getX() - uiw/2);
    int bz0 = (int) floor(ui->mapView->getZ() - uih/2);
    int bx1 = (int) ceil(ui->mapView->getX() + uiw/2);
    int bz1 = (int) ceil(ui->mapView->getZ() + uih/2);

    ui->cboxArea->setChecked(true);
    ui->lineEditX1->setText( QString::number(bx0) );
    ui->lineEditZ1->setText( QString::number(bz0) );
    ui->lineEditX2->setText( QString::number(bx1) );
    ui->lineEditZ2->setText( QString::number(bz1) );
}

void MainWindow::on_buttonAnalysis_clicked()
{
    int x1, z1, x2, z2;

    if (ui->lineRadius->isEnabled())
    {
        int d = ui->lineRadius->text().toInt();
        x1 = (-d) >> 1;
        z1 = (-d) >> 1;
        x2 = (d) >> 1;
        z2 = (d) >> 1;
    }
    else
    {
        x1 = ui->lineEditX1->text().toInt();
        z1 = ui->lineEditZ1->text().toInt();
        x2 = ui->lineEditX2->text().toInt();
        z2 = ui->lineEditZ2->text().toInt();
    }
    if (x2 < x1 || z2 < z1)
    {
        warning("Warning", "Invalid area for analysis");
        return;
    }

    int mc;
    int64_t seed;
    if (!getSeed(&mc, &seed))
        return;

    const int step = 512;

    LayerStack g;
    setupGenerator(&g, mc);
    applySeed(&g, seed);
    int *ids = allocCache(g.entry_1, step, step);

    long idcnt[256] = {0};
    for (int x = x1; x <= x2; x += step)
    {
        for (int z = z1; z <= z2; z += step)
        {
            int w = x2-x+1 < step ? x2-x+1 : step;
            int h = z2-z+1 < step ? z2-z+1 : step;
            genArea(g.entry_1, ids, x, z, w, h);

            for (int i = 0; i < w*h; i++)
                idcnt[ ids[i] & 0xff ]++;
        }
    }

    int bcnt = 0;
    for (int i = 0; i < 256; i++)
        bcnt += !!idcnt[i];

    free(ids);
    ids = NULL;

    QTreeWidget *tree = ui->treeAnalysis;
    while (tree->topLevelItemCount() > 0)
        delete tree->takeTopLevelItem(0);

    QTreeWidgetItem* item_cat;
    item_cat = new QTreeWidgetItem(tree);
    item_cat->setText(0, "Biomes");
    item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(bcnt));

    for (int id = 0; id < 256; id++)
    {
        long cnt = idcnt[id];
        if (cnt <= 0)
            continue;
        const char *s;
        if (!(s = biome2str(id)))
            continue;
        QTreeWidgetItem* item = new QTreeWidgetItem(item_cat, QTreeWidgetItem::UserType + id);
        item->setText(0, s);
        item->setData(1, Qt::DisplayRole, QVariant::fromValue(cnt));
    }

    tree->insertTopLevelItem(0, item_cat);

    std::vector<VarPos> st;
    for (int stype = Desert_Pyramid; stype <= Treasure; stype++)
    {
        st.clear();
        StructureConfig sconf;
        if (!getConfig(stype, mc, &sconf))
            continue;
        getStructs(&st, sconf, &g, mc, seed, x1, z1, x2, z2);
        if (st.empty())
            continue;

        item_cat = new QTreeWidgetItem(tree);
        const char *s = struct2str(stype);
        item_cat->setText(0, s);
        item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(st.size()));

        for (size_t i = 0; i < st.size(); i++)
        {
            VarPos vp = st[i];
            QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
            item->setData(0, Qt::UserRole, QVariant::fromValue(vp.p));
            item->setText(0, QString::asprintf("%d,\t%d", vp.p.x, vp.p.z));
            if (vp.variant)
            {
                if (stype == Village)
                    item->setText(1, "Abandoned");
            }
        }
        tree->insertTopLevelItem(stype, item_cat);
    }

    Pos pos = getSpawn(mc, &g, NULL, seed);
    if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
    {
        item_cat = new QTreeWidgetItem(tree);
        item_cat->setText(0, "Spawn");
        item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(1));
        QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
        item->setData(0, Qt::UserRole, QVariant::fromValue(pos));
        item->setText(0, QString::asprintf("%d,\t%d", pos.x, pos.z));
    }

    StrongholdIter sh;
    initFirstStronghold(&sh, mc, seed);
    std::vector<Pos> shp;
    while (nextStronghold(&sh, &g, NULL) > 0)
    {
        pos = sh.pos;
        if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
            shp.push_back(pos);
    }

    if (!shp.empty())
    {
        item_cat = new QTreeWidgetItem(tree);
        item_cat->setText(0, "Stronghold");
        item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(shp.size()));
        for (Pos pos : shp)
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
            item->setData(0, Qt::UserRole, QVariant::fromValue(pos));
            item->setText(0, QString::asprintf("%d,\t%d", pos.x, pos.z));
        }
    }

    ui->buttonExport->setEnabled(true);
}

void MainWindow::on_buttonExport_clicked()
{
    QString fnam = QFileDialog::getSaveFileName(this, "Export analysis", prevdir,
        "Text files (*.txt *csv);;Any files (*)");
    if (fnam.isEmpty())
        return;

    QFileInfo finfo(fnam);
    QFile file(fnam);
    prevdir = finfo.absolutePath();

    if (!file.open(QIODevice::WriteOnly))
    {
        warning("Warning", "Failed to open file.");
        return;
    }

    QTextStream stream(&file);

    QTreeWidgetItemIterator it(ui->treeAnalysis);
    for (; *it; ++it)
    {
        QTreeWidgetItem *item = *it;
        if (item->type() >= QTreeWidgetItem::UserType)
            stream << QString::number(item->type() - QTreeWidgetItem::UserType) << ", ";
        stream << item->text(0).replace('\t', ' ');
        if (!item->text(1).isEmpty())
            stream << ", " << item->text(1);
        stream << "\n";
    }
}

void MainWindow::on_treeAnalysis_itemDoubleClicked(QTreeWidgetItem *item)
{
    QVariant dat = item->data(0, Qt::UserRole);
    if (dat.isValid())
    {
        Pos p = qvariant_cast<Pos>(dat);
        ui->mapView->setView(p.x+0.5, p.z+0.5);
    }
}

void MainWindow::on_actionSearch_seed_list_triggered()
{
    formControl->setSearchMode(SEARCH_LIST);
}

void MainWindow::on_actionSearch_full_seed_space_triggered()
{
    formControl->setSearchMode(SEARCH_BLOCKS);
}

void MainWindow::onAutosaveTimeout()
{
    if (config.autosaveCycle)
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        saveProgress(path + "/session.save", true);
    }
}

void MainWindow::onActionMapToggled(int stype, bool show)
{
    ui->mapView->setShow(stype, show);
}

void MainWindow::onConditionsChanged()
{
    QVector<Condition> conds = formCond->getConditions();
    formGen48->updateAutoConditions(conds);
}

void MainWindow::onGen48Changed()
{
    formGen48->updateCount();
    formControl->searchProgressReset();
}

void MainWindow::onSelectedSeedChanged(int64_t seed)
{
    ui->seedEdit->setText(QString::asprintf("%" PRId64, seed));
    on_seedEdit_editingFinished();
}

void MainWindow::onSearchStatusChanged(bool running)
{
    formGen48->setEnabled(!running);
}

void MainWindow::copyCoord()
{
    Pos p = ui->mapView->getActivePos();
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(QString::asprintf("%d, %d", p.x, p.z));
}

