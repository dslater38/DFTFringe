#ifndef SURFACEMANAGER_H
#define SURFACEMANAGER_H
#include <QTextEdit>
#include <QWidget>
#include <qobject.h>
#include "wavefront.h"
#include "contourplot.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "glwidget.h"
#include "dftarea.h"
#include "igramarea.h"
#include "profileplot.h"
#include "surfaceanalysistools.h"
#include <QObject>
#include "zernikedlg.h"
#include "mirrordlg.h"
#include "metricsdisplay.h"
#include "statsdlg.h"
#include <QTimer>
#include <QProgressDialog>
#include "wftstats.h"
#include "circleoutline.h"
#include "simulationsview.h"
#include "standastigwizard.h"


struct textres {
    QTextEdit *Edit;
    QList<QString> res;
};
class SurfaceManager : public QObject
{
    Q_OBJECT
public:

    explicit SurfaceManager(QObject *parent=0, surfaceAnalysisTools *tools = 0, ProfilePlot *profilePlot =0,
                   ContourPlot *contourPlo = 0, GLWidget *glPlot = 0, metricsDisplay *mets = 0);
    ~SurfaceManager();
    bool loadWavefront(const QString &fileName);
    void sendSurface(wavefront* wf);
    void computeMetrics(wavefront *wf);
    void makeMask(int waveNdx);
    void previous();
    void next();
    void deleteCurrent();
    void processSmoothing();
    void saveAllWaveFrontStats();
    void SaveWavefronts(bool saveNulled);
    void writeWavefront(QString fname, wavefront *wf, bool saveNulled);
    void useDemoWaveFront();
    cv::Mat computeWaveFrontFromZernikes(int wx, std::vector<double> &zerns, QVector<int> zernsToUse);
    void report();
    void computeTestStandAstig();
    QVector<wavefront*> m_wavefronts;
    surfaceAnalysisTools *m_surfaceTools;
    ProfilePlot *m_profilePlot;
    ContourPlot* m_contourPlot;
    SimulationsView *m_simView;
    GLWidget *m_oglPlot;
    QImage m_allContours;
    metricsDisplay *m_metrics;
    int m_gbValue;
    bool m_GB_enabled;
    int m_currentNdx;
    wavefront* m_wf;
    mirrorDlg *m_mirrorDlg;
    int insideOffset;
    int outsideOffset;
    StatsDlg * m_statsDlg;
    QWaitCondition pauseCond;

    QMutex sync;
    int messageResult;
    void inspectWavefront();
    bool useZernikeBase;
    void average(QList<wavefront *> wfList);
    void subtractWavefronts();
    bool m_askAboutReverse;
private:
    QProgressDialog *pd;
    QThread *m_generatorThread;
    QTimer *m_waveFrontTimer;
    QTimer *m_toolsEnableTimer;
    int workToDo;
    bool m_surface_finished;
    int workProgress;
    void subtract(wavefront *wf1, wavefront *wf2);
    wftStats *m_wftStats;
    textres Phase2(QList<rotationDef *> list, QList<int> inputs, int avgNdx);

signals:
    void currentNdxChanged(int);
    void waveFrontClicked(int);
    void generateSurfacefromWavefront(int ndx, SurfaceManager * sm);
    void deleteWavefront(int);
    void rotateTheseSig(int, QList<int>);
    void progress(int);
    void showMessage(QString);
    void diameterChanged(double);
    void rocChanged(double);
    void nameChanged(QString, QString);
    void showTab(int);
    void load(QStringList, SurfaceManager *);
    void enableControls(bool);
private slots:
    void waveFrontClickedSlot(int ndx);
    void wavefrontDClicked(const QString & name);
    void centerMaskValue(int val);
    void outsideMaskValue(int val);
    void surfaceSmoothGBEnabled(bool b);
    void surfaceSmoothGBValue(int value);
    void computeZerns();
    void surfaceGenFinished(int ndx);
    void backGroundUpdate();
    void deleteWaveFronts(QList<int> list);
    void average(QList<int> list);
    void transfrom(QList<int> list);
    void saveAllContours();
    void enableTools();
public slots:
    void rotateThese(double angle, QList<int> list);
    void createSurfaceFromPhaseMap(cv::Mat phase, CircleOutline outside,
                                   CircleOutline center, QString name);
    void surfaceBaseChanged(bool b);
    void invert(QList<int> list);
    void wftNameChanged(int, QString);
    void showAllContours();
    void computeStandAstig(QList<rotationDef *>);
    void ObstructionChanged();
};


class surfaceGenerator : public QObject {
    Q_OBJECT

public:
    surfaceGenerator(SurfaceManager *sm);
    ~surfaceGenerator();

public slots:
    void process(int wavefront_index, SurfaceManager *sm);

signals:
    void finished(int wavefront_index);
    void error(QString err);
    void showMessage(QString);

private:
    // add your variables here
    SurfaceManager* m_sm;
    QMutex sync;
    zern_generator * m_zg;
};

#endif // SURFACEMANAGER_H