#include "wftexaminer.h"
#include "ui_wftexaminer.h"
#include <qwt_plot.h>

#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_text.h>
#include <qwt_plot_renderer.h>
#include <qwt_interval_symbol.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_scale_engine.h>
#include "opencv/cv.h"
wftExaminer::wftExaminer( wavefront *wf,QWidget *parent) :
    QDialog(parent),m_wf(wf),
    curve(0),maskCurve(0),ui(new Ui::wftExaminer)
{
    ui->setupUi(this);
    ui->spinBox->setMaximum(wf->data.cols-1);
    m_Pl = new QwtPlot();
    QwtPlotGrid *grid = new QwtPlotGrid();

    grid->attach( m_Pl );
    m_y = wf->data.rows/2;
    ui->spinBox->setValue(m_y);
    m_Pl->resize(1500,400);
    setupPlot();
}

wftExaminer::~wftExaminer()
{
    delete ui;
}

void wftExaminer::setupPlot(){
    //m_Pl->detachItems( QwtPlotItem::Rtti_PlotCurve);
    cv::Mat m = m_wf->workData;
    QPolygonF points;
    QwtScaleEngine *s = m_Pl->axisScaleEngine(QwtPlot::xBottom);
    QwtScaleDiv sd1 = s->divideScale(0,m_wf->data.cols-1, m_wf->data.cols/20,1);
    m_Pl->setAxisScaleDiv(QwtPlot::xBottom, sd1);
    for (int x = 0; x < m.cols; ++x){
        double v = m.at<double>(m_wf->workData.rows - 1 - m_y,x);
        points << QPointF(x,v);
    }
    if (curve)
        delete curve;
    curve = new QwtPlotCurve;
    curve->setPen( Qt::blue, 1 );
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve->setSamples(points);
    curve->attach(m_Pl);

    if (maskCurve)
        delete maskCurve;
    maskCurve = new QwtPlotCurve;
    points.clear();
    for (int x = 0; x < m.cols; ++x){
        double v= -1.* (double)(m_wf->mask.at<uchar>(m_wf->workData.rows - 1 - m_y,x))/255.;
        points<< QPointF(x,v);
    }
    maskCurve->setSamples(points);
    maskCurve->setPen(Qt::red,1);
    maskCurve->attach(m_Pl);

    m_Pl->show();
}

void wftExaminer::on_spinBox_valueChanged(int arg1)
{
    if (arg1 >= m_wf->data.cols)
        return;
    m_y = arg1;
    setupPlot();
    m_Pl->replot();
}