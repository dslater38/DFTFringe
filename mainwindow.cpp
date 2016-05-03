#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtWidgets>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <cmath>
#include "zernikedlg.h"
#include <QDateTime>
#include <QtCore>
#include <qtconcurrentrun.h>
#include <qtconcurrentrun.h>
#include "outlinedlg.h"
#include "zernikes.h"
#include "zernikeprocess.h"
#include "simigramdlg.h"
#include "usercolormapdlg.h"
#include <qlayout.h>
#include <opencv/cv.h>
#include "simulationsview.h"

using namespace QtConcurrent;
vector<wavefront*> g_wavefronts;
int g_currentsurface = 0;
QScrollArea *gscrollArea;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),m_showChannels(false), m_showIntensity(false)
{
    ui->setupUi(this);
    const QString toolButtonStyle("QToolButton {"
                                "border-style: outset;"
                                "border-width: 3px;"
                                "border-radius:7px;"
                                "border-color: darkgray;"
                                "font: bold 12px;"
                                "min-width: 10em;"
                                "padding: 6px;}"
                            "QToolButton:hover {background-color: lightblue;"
                                    " }");
    QWidget *rw = ui->toolBar->widgetForAction(ui->actionRead_WaveFront);
    //igramArea = new IgramArea(ui->tabWidget->widget(0));
    rw->setStyleSheet(toolButtonStyle);
    rw = ui->toolBar->widgetForAction(ui->actionSubtract_wave_front);
    rw->setStyleSheet(toolButtonStyle);


    ui->toolBar->setStyleSheet("QToolBar { spacing: 10px;}");
              ui->toolBar->setStyleSheet("QToolButton {border-style: outset;border-width: 3px;"
                                         "border-color: darkgray;border-radius:10px;}"
                                         "QToolButton:hover {background-color: lightblue;}");
    ui->SelectOutSideOutline->setChecked(true);
    setCentralWidget(ui->tabWidget);
    m_loaderThread = new QThread();
    m_waveFrontLoader = new waveFrontLoader();
    m_waveFrontLoader->moveToThread(m_loaderThread);
    m_colorChannels = new ColorChannelDisplay();
    connect(this, SIGNAL(load(QStringList,SurfaceManager*)), m_waveFrontLoader, SLOT(loadx(QStringList,SurfaceManager*)));
    m_intensityPlot = new igramIntensity(0);


    m_loaderThread->start();
    ui->tabWidget->removeTab(0);
    ui->tabWidget->removeTab(0);

    scrollArea = new QScrollArea;
    gscrollArea = scrollArea;
    m_igramArea = new IgramArea(scrollArea, this);
    connect(m_igramArea, SIGNAL(statusBarUpdate(QString)), statusBar(), SLOT(showMessage(QString)));
    connect(m_igramArea, SIGNAL(upateColorChannels(QImage)), this, SLOT(updateChannels(QImage)));
    connect(m_igramArea, SIGNAL(showTab(int)),  ui->tabWidget, SLOT(setCurrentIndex(int)));
    connect(this, SIGNAL(gammaChanged(bool,double)), m_igramArea, SLOT(gammaChanged(bool, double)));
    m_igramArea->setBackgroundRole(QPalette::Base);
    installEventFilter(m_igramArea);
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(m_igramArea);

    scrollAreaDft = new QScrollArea;

    m_dftTools = new DFTTools(this);
    m_vortexDebugTool = new vortexDebug(this);
    //m_vortexTools = new VortexTools(this);
    m_dftArea = new DFTArea(scrollAreaDft, m_igramArea, m_dftTools, m_vortexDebugTool);
    m_contourTools = new ContourTools(this);

    m_surfTools = surfaceAnalysisTools::get_Instance(this);

    //DocWindows
    createDockWindows();

    userMapDlg = new userColorMapDlg();

    m_contourView = new contourView(this, m_contourTools);
    m_ogl = new OGLView(0, m_contourTools, m_surfTools);

    connect(userMapDlg, SIGNAL(colorMapChanged(int)), m_contourView->getPlot(), SLOT(ContourMapColorChanged(int)));
    connect(userMapDlg, SIGNAL(colorMapChanged(int)),m_ogl->m_gl, SLOT(colorMapChanged(int)));
    review = new reviewWindow(this);
    review->s1->addWidget(m_ogl);

    m_profilePlot =  new ProfilePlot(review->s2,m_contourTools);
    m_mirrorDlg = mirrorDlg::get_Instance();
    review->s2->addWidget(m_profilePlot);
    review->s1->addWidget(m_contourView);
    //Surface Manager
    m_surfaceManager = new SurfaceManager(this,m_surfTools, m_profilePlot, m_contourView->getPlot(),
                                          m_ogl->m_gl, metrics);
    connect(m_surfaceManager, SIGNAL(load(QStringList,SurfaceManager*)), m_waveFrontLoader, SLOT(loadx(QStringList,SurfaceManager*)));
    connect(m_surfaceManager, SIGNAL(showMessage(QString)), this, SLOT(showMessage(QString)));
    connect(m_contourView, SIGNAL(showAllContours()), m_surfaceManager, SLOT(showAllContours()));
    connect(m_dftArea, SIGNAL(newWavefront(cv::Mat,CircleOutline,CircleOutline,QString)),
            m_surfaceManager, SLOT(createSurfaceFromPhaseMap(cv::Mat,CircleOutline,CircleOutline,QString)));
    connect(m_surfaceManager, SIGNAL(diameterChanged(double)),this,SLOT(diameterChanged(double)));
    connect(m_surfaceManager, SIGNAL(showTab(int)), ui->tabWidget, SLOT(setCurrentIndex(int)));
    ui->tabWidget->addTab(scrollArea, "igram");
    ui->tabWidget->addTab(m_dftArea, "Analyze");
    ui->tabWidget->addTab(review, "Results");

    ui->tabWidget->addTab(SimulationsView::getInstance(ui->tabWidget), "Star Test, PSF, MTF");
    scrollArea->setWidgetResizable(true);
    createActions();

    //Recent Files list
    separatorAct = ui->menuFiles->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i)
        ui->menuFiles->addAction(recentFileActs[i]);
    updateRecentFileActions();
    qRegisterMetaType<QVector<int> >();

    connect( m_igramArea, SIGNAL(enableShiftButtons(bool)), this,SLOT(enableShiftButtons(bool)));
    connect(m_dftArea, SIGNAL(dftReady(QImage)), m_igramArea,SLOT(dftReady(QImage)));
    connect(m_igramArea, SIGNAL(upateColorChannels(QImage)), m_dftArea, SLOT(newIgram(QImage)));
    enableShiftButtons(true);


    connect(m_dftTools,SIGNAL(doDFT()),m_dftArea,SLOT(doDFT()));
    settingsDlg = Settings2::getInstance();
    connect(settingsDlg->m_igram,SIGNAL(igramLinesChanged(int,int,QColor,QColor,double,int)),
            m_igramArea,SLOT(igramOutlineParmsChanged(int, int, QColor, QColor, double, int)));

    QSettings settings;
    restoreState(settings.value("MainWindow/windowState").toByteArray());
    restoreGeometry(settings.value("geometry").toByteArray());
    connect(m_dftArea,SIGNAL(selectDFTTab()), this, SLOT(selectDftTab()));
    connect(ui->tabWidget,SIGNAL(currentChanged(int)),this, SLOT(mainTabChanged(int)));
    tabifyDockWidget(ui->outlineTools, m_dftTools);
    setTabPosition(Qt::RightDockWidgetArea, QTabWidget::West);
    setTabShape(QTabWidget::Triangular);
    ui->tabWidget->setTabShape(QTabWidget::Triangular);
    tabifyDockWidget(m_dftTools,m_contourTools);
    tabifyDockWidget(m_contourTools,m_surfTools);
    //tabifyDockWidget(m_contourTools,m_vortexTools);
    //tabifyDockWidget(m_vortexDebugTool,m_metrics);
    ui->outlineTools->show();
    ui->outlineTools->raise();

    zernEnables = std::vector<bool>(Z_TERMS, true);

    //disable first 8 enables except for astig
    // enable first 8 nulls except for astig
    for (int i = 0; i < 8; ++ i)
    {
        if (i == 4 || i == 5)
            continue;
        zernEnables[i] = false;
    }
    zernEnableUpdateTime = QDateTime::currentDateTime().toTime_t();

    connect(m_surfaceManager, SIGNAL(rocChanged(double)),this, SLOT(rocChanged(double)));
    connect(m_mirrorDlg, SIGNAL(newPath(QString)),this, SLOT(newMirrorDlgPath(QString)));
    progBar = new QProgressBar(this);
    ui->statusBar->addPermanentWidget(progBar);
//    QMessageBox::about(this, tr("About Image Viewer"),
//            tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
//               "and QScrollArea to display an image. QLabel is typically used "
//               "for displaying a text, but it can also display an image. "
//               "QScrollArea provides a scrolling view around another widget. "
//               "If the child widget exceeds the size of the frame, QScrollArea "
//               "automatically provides scroll bars. </p><p>The example "
//               "demonstrates how QLabel's ability to scale its contents "
//               "(QLabel::scaledContents), and QScrollArea's ability to "
//               "automatically resize its contents "
//               "(QScrollArea::widgetResizable), can be used to implement "
//               "zooming and scaling features. </p><p>In addition the example "
//               "shows how to use QPainter to print an image.</p>"));

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
    QWidget::closeEvent(event);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::mainTabChanged(int ndx){

    switch(ndx){

    case 0:
        ui->outlineTools->show();
        ui->outlineTools->raise();
        break;
    case 1: //dft
        m_dftTools->show();
        m_dftTools->raise();
        break;
    case 2: //surface
        m_surfTools->show();
        m_surfTools->raise();
        break;
    case 3:
        SimulationsView *sv = SimulationsView::getInstance(0);
        if (sv->needs_drawing){

            sv->on_MakePB_clicked();
            QApplication::restoreOverrideCursor();

        }

    }
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        loadFile(action->data().toString());
}


bool MainWindow::loadFile(const QString &fileName)
{
    m_igramArea->openImage(fileName);
    setCurrentFile(fileName);
    return true;
}
void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    setWindowFilePath(curFile);
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("recentFileList", files);
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin)
            mainWin->updateRecentFileActions();
    }
}

void MainWindow::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    separatorAct->setVisible(numRecentFiles > 0);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::on_actionLoad_Interferogram_triggered()
{
    QStringList mimeTypeFilters;
    foreach (const QByteArray &mimeTypeName, QImageReader::supportedMimeTypes())
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    QSettings settings;
    QString lastPath = settings.value("lastPath",".").toString();
    QFileDialog dialog(this, tr("Open File"),lastPath);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}

}

void MainWindow::createActions()
{
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));

    }
}

void MainWindow::on_pushButton_5_clicked()
{
    m_igramArea->crop();
    // make sure current boundary is the outside one.
    ui->SelectOutSideOutline->setChecked(true);
    ui->SelectObsOutline->setChecked(false);
}


void MainWindow::on_pushButton_8_clicked()
{
    m_igramArea->deleteOutline();
}

void MainWindow::on_pushButton_7_clicked()
{
    m_igramArea->readOutlines();
}

void MainWindow::enableShiftButtons(bool enable){
    ui->shiftDown->setEnabled(enable);
    ui->ShiftUp->setEnabled(enable);
    ui->shiftLeft->setEnabled(enable);
    ui->shiftRight->setEnabled(enable);
}
void MainWindow::createDockWindows(){

    m_contourTools->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_dftTools->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    metrics = metricsDisplay::get_instance(this);
    zernTablemodel = metrics->tableModel;
    addDockWidget(Qt::RightDockWidgetArea, m_dftTools);
    addDockWidget(Qt::RightDockWidgetArea, m_contourTools);
    addDockWidget(Qt::RightDockWidgetArea, m_surfTools);
    addDockWidget(Qt::LeftDockWidgetArea,  metrics);
    addDockWidget(Qt::LeftDockWidgetArea, m_vortexDebugTool);
    ui->menuView->addAction(m_dftTools->toggleViewAction());
    ui->menuView->addAction(ui->outlineTools->toggleViewAction());
    ui->menuView->addAction(m_contourTools->toggleViewAction());
    ui->menuView->addAction(m_surfTools->toggleViewAction());
    ui->menuView->addAction(metrics->toggleViewAction());
    ui->menuView->addAction(m_vortexDebugTool->toggleViewAction());
    metrics->hide();
    m_vortexDebugTool->hide();
}


void MainWindow::updateMetrics(wavefront& wf){
    metrics->setName(wf.name);
    metrics->mDiam->setText(QString().sprintf("%6.3lf",wf.diameter));
    metrics->mROC->setText(QString().sprintf("%6.3lf",wf.roc));
    const double  e = 2.7182818285;
    metrics->mRMS->setText(QString().sprintf("<b><FONT FONT SIZE = 12>%6.3lf</b>",wf.std));
    double st =(2. *M_PI * wf.std);
    st *= st;
    double Strehl = pow(e, -st);
    metrics->mStrehl->setText(QString().sprintf("<b><FONT FONT SIZE = 12>%6.3lf</b>",Strehl));

    double roc = m_mirrorDlg->roc;
    double diam = wf.diameter;
    double r3 = roc * roc * roc;
    double d4 = diam * diam * diam * diam;
    double z8 = zernTablemodel->values[8];
    if (m_mirrorDlg->doNull)
        z8 = z8 - m_mirrorDlg->cc * m_mirrorDlg->z8;

    double z1 = z8 * 384. * r3 * m_mirrorDlg->lambda * 1.E-6/(d4);
    BestSC = m_mirrorDlg->cc + z1;
    metrics->setWavePerFringe(m_mirrorDlg->fringeSpacing);
    metrics->mCC->setText(QString().sprintf("<FONT FONT SIZE = 7>%6.3lf",BestSC));
    metrics->show();
}

void MainWindow::on_actionLoad_outline_triggered()
{
    m_igramArea->readOutlines();
}

void MainWindow::on_ShiftUp_clicked()
{
    m_igramArea->shiftUp();
}

void MainWindow::on_shiftLeft_clicked()
{
    m_igramArea->shiftLeft();
}

void MainWindow::on_shiftDown_clicked()
{
    m_igramArea->shiftDown();
}

void MainWindow::on_shiftRight_clicked()
{
    m_igramArea->shiftRight();
}

void MainWindow::selectDftTab(){
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::updateChannels(QImage img){

    m_colorChannels->setImage(img);
    if (m_showChannels)
        m_colorChannels->show();
    m_intensityPlot->setIgram(img);
    if (m_showIntensity)
        m_intensityPlot->show();
}

void MainWindow::on_actionRead_WaveFront_triggered()
{
    QSettings settings;
    QString lastPath = settings.value("lastPath",".").toString();

    QFileDialog dialog(this);
    dialog.setDirectory(lastPath);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(tr("wft (*.wft)"));
    QStringList fileNames;

    if (dialog.exec()) {
        this->setCursor(Qt::WaitCursor);

        QApplication::processEvents();
        fileNames = dialog.selectedFiles();
        QFileInfo info(fileNames[0]);
        lastPath = info.absolutePath();
        QSettings settings;
        settings.setValue("lastPath",lastPath);
        emit load(fileNames, m_surfaceManager);
        this->setCursor(Qt::ArrowCursor);
        ui->tabWidget->setCurrentIndex(2);
    }
}
/*
fields = l.split()
if fields[0] == 'outside':
    el = Bounds.Circle(float(fields[2]), float(fields[3]), float(fields[4]))

elif fields[0] == 'DIAM':
    diam = float(fields[1])
elif fields[0] == 'ROC':
    roc = float(fields[1])
elif fields[0] == 'Lambda':
    iLambda = float(fields[1])
    pass
elif fields[0] == 'obstruction':
    obs = Bounds.Circle(float(fields[2]), float(fields[3]), float(fields[4]))

if not el:
*/

void MainWindow::on_actionPreferences_triggered()
{
    settingsDlg->show();
}



void MainWindow::on_actionMirror_triggered()
{
    m_mirrorDlg->show();
}


wavefront* MainWindow::getCurrentWavefront()
{
    return m_surfaceManager->m_wf;
}
void MainWindow::on_actionSave_outline_triggered()
{
       m_igramArea->saveOutlines();
}
void MainWindow::on_saveOutline_clicked()
{
    m_igramArea->saveOutlines();
}

//void MainWindow::on_actionSimulated_interferogram_triggered()
//{
//    m_igramArea->generateSimIgram();
//}

void MainWindow::on_actionPrevious_Wave_front_triggered()
{
    m_surfaceManager->previous();
}

void MainWindow::on_actionNext_Wave_Front_triggered()
{
    m_surfaceManager->next();
}

void MainWindow::on_actionDelete_wave_front_triggered()
{
    m_surfaceManager->deleteCurrent();
}

void MainWindow::on_actionSave_All_WaveFront_stats_triggered()
{
    m_surfaceManager->saveAllWaveFrontStats();
}

void MainWindow::on_actionWrite_WaveFront_triggered()
{

}


void MainWindow::on_actionSave_Wavefront_triggered()
{
    m_surfaceManager->SaveWavefronts(false);
}
void MainWindow::showMessage(QString msg){

    int ok = QMessageBox(QMessageBox::Information,msg, "",QMessageBox::Yes|QMessageBox::No).exec();
    m_surfaceManager->messageResult = ok;
    m_surfaceManager->pauseCond.wakeAll();


}
void MainWindow::diameterChanged(double v){
    m_mirrorDlg->on_diameter_Changed(v);
}
void MainWindow::rocChanged(double v){
    m_mirrorDlg->on_roc_Changed(v);
}

void MainWindow::on_actionLighting_properties_triggered()
{
    m_ogl->m_gl->openLightingDlg();
}

void MainWindow::on_SelectOutSideOutline_clicked(bool checked)
{
    m_igramArea->SideOutLineActive( checked);
}

void MainWindow::on_SelectObsOutline_clicked(bool checked)
{
        m_igramArea->SideOutLineActive( !checked);
}

void MainWindow::on_pushButton_clicked()
{
    m_igramArea->nextStep();
}

void MainWindow::on_showChannels_clicked(bool checked)
{
    m_showChannels = checked;
    if (checked)
        m_colorChannels->show();
    else
        m_colorChannels->close();
}

void MainWindow::on_showIntensity_clicked(bool checked)
{
    m_showIntensity = checked;
    if (checked)
        m_intensityPlot->show();
    else
        m_intensityPlot->close();
}
void MainWindow::newMirrorDlgPath(QString path){
    QFileInfo info(path);
    QSettings settings;
    settings.setValue("lastPath",info.path());
}
//make a simulated wavefront based on zernike values
#define TSIZE 200
void MainWindow::on_actionWavefront_triggered()
{
    simIgramDlg dlg;
    dlg.setWindowTitle("Wavefront terms");
    if (!dlg.exec())
        return;

    int wx = dlg.size;
    int wy = wx;
    double rad = (double)(wx-1)/2.d;
    double xcen = rad,ycen = rad;
    rad -= 5;

    cv::Mat result = cv::Mat::zeros(wx,wx, CV_64F);

    double rho;


    mirrorDlg *md = m_mirrorDlg;
    for (int i = 4; i <  wx-4; ++i)
    {
        double x1 = (double)(i - (xcen)) / rad;
        for (int j = 4; j < wy-4; ++j)
        {
            double y1 = (double)(j - (ycen )) /rad;
            rho = sqrt(x1 * x1 + y1 * y1);

            if (rho <= 1.)
            {
                double phi = atan2(y1,x1);
                double S1 = md->z8 * dlg.correction * .01d * Zernike(8,x1,y1) +
                        dlg.xtilt  * Zernike(1,x1,y1) +
                        dlg.ytilt * Zernike(2,x1,y1) +
                        dlg.defocus * Zernike(3,x1,y1) +
                        dlg.xastig * Zernike(4, x1, y1)+
                        dlg.yastig * Zernike(5, x1,y1) +
                        1. * dlg.star * cos(10.  *  phi) +
                        1. * dlg.ring * cos(10 * 2. * rho);
                if (dlg.zernNdx > 0)
                    S1 += (dlg.zernValue * Zernike(dlg.zernNdx, x1,y1));
                result.at<double>(j,i) = S1;
            }
            else
            {
                result.at<double>(j,i) = 0 ;
            }
        }
    }




    m_surfaceManager->createSurfaceFromPhaseMap(result,
                                                CircleOutline(QPointF(xcen,ycen),rad),
                                                CircleOutline(QPointF(0,0),0),
                                                QString("Simulated_Wavefront"));
}



void MainWindow::on_actionIgram_triggered()
{
    m_igramArea->generateSimIgram();
}

void MainWindow::on_actionSave_interferogram_triggered()
{
    m_igramArea->save();
}

void MainWindow::on_actionSave_screen_triggered()
{
    QSettings set;
    QString path = set.value("mirrorConfigFile").toString();
    QFile fn(path);
    QFileInfo info(fn.fileName());
    QString dd = info.dir().absolutePath();
    const QList<QByteArray> imageFormats = QImageWriter::supportedImageFormats();
    QStringList filter;
    if ( imageFormats.size() > 0 )
    {
        QString imageFilter( tr( "Images" ) );
        imageFilter += " (";
        for ( int i = 0; i < imageFormats.size(); i++ )
        {
            if ( i > 0 )
                imageFilter += " ";
            imageFilter += "*.";
            imageFilter += imageFormats[i];
        }
        imageFilter += ")";

        filter += imageFilter;
    }
    QString fName = QFileDialog::getSaveFileName(0,
        tr("Save screen"), dd + "//surface.jpg",filter.join( ";;" ));


    if (fName.isEmpty())
        return;

    QImage image( this->size(), QImage::Format_ARGB32 );
    QPoint wr = m_ogl->m_gl->mapTo(this, m_ogl->pos());

    QImage gl = m_ogl->m_gl->grabFrameBuffer();

    QPainter painter( &image );

    this->render( &painter);
    painter.drawImage(wr, gl);
    painter.end();

    image.save( fName, QFileInfo( fName ).suffix().toLatin1() );
}

void MainWindow::on_actionWave_Front_Inspector_triggered()
{
    m_surfaceManager->inspectWavefront();
}

void MainWindow::on_actionUser_Color_Map_triggered()
{
    userMapDlg->show();
}

void MainWindow::on_actionSave_nulled_smoothed_wavefront_triggered()
{
        m_surfaceManager->SaveWavefronts(true);
}

void MainWindow::on_minus_clicked()
{
    m_igramArea->decrease();
}

void MainWindow::on_pluss_clicked()
{
    m_igramArea->increase();
}

void MainWindow::on_actionTest_Stand_Astig_Removal_triggered()
{
    m_surfaceManager->computeTestStandAstig();
}


void MainWindow::on_reverseGammaCB_clicked(bool checked)
{
    emit gammaChanged(checked, ui->gammaValue->text().toDouble());
}


void MainWindow::on_actionSubtract_wave_front_triggered()
{
    m_surfaceManager->subtractWavefronts();
}

void MainWindow::on_actionSave_PDF_report_triggered()
{
    m_surfaceManager->report();
}


void MainWindow::on_actionHelp_triggered()
{
    QMessageBox::information(this, "Help", "Sorry no help other than videos yet.");
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "About", QString().sprintf("DFTFringe version %s",APP_VERSION));
}

void MainWindow::on_actionVideos_triggered()
{
    QString link = "https://www.youtube.com/channel/UCPj57WFSSLpVqPir7Im-kiw";
    QDesktopServices::openUrl(QUrl(link));

}

void MainWindow::on_undo_pressed()
{
    m_igramArea->undo();
}

void MainWindow::on_Redo_pressed()
{
    m_igramArea->redo();
}

void MainWindow::on_pushButton_clicked(bool checked)
{
    m_igramArea->hideOutline(checked);
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    m_igramArea->hideOutline(checked);
}

void MainWindow::on_actionError_Margins_triggered()
{
    QMessageBox::information(0,"info","Sorry not implemented yet.");
}