/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "glwidget.h"
#include <QMouseEvent>
//#include <cmath>
#include "qwt_math.h"
#include "circleoutline.h"
#include <gl/glu.h>
#include <qsettings.h>
#include <QOpenGLFunctions>
#include <QFont>
#include <QFontMetricsF>
#define STEPS 200
#define SHADE_STEPS 200

GLWidget::GLWidget(QWidget *parent, ContourTools* tools, surfaceAnalysisTools* surfTools )
    : QGLWidget(parent), m_tools(tools),m_surfTools(surfTools),m_red(100),m_green(100),m_blue(100),
      m_edge_mask_offset(0),m_Vscale(200),m_fRangeZ(1000),m_zTrans(-1000),m_zoomFactor(1.),
      m_resolutionPercent(100),
      m_GB_enabled(false),
      m_gbValue(13),
      m_FillMode(GL_LINE),
      m_draw_profiles_on_3d(true),
      m_list_good(false),
      m_dirty_surface(true),
      m_BackWall_Scale(.125),
      m_ortho(false),
      m_flip_y_view(false),
      m_flip_x_view(false)
{
    QSettings set;
    //====== Initial lighting params
    m_LightParam[0] = set.value("xLightParam",-2).toInt();	// X position
    m_LightParam[1] = set.value("yLightParam",200).toInt();	// Y position
    m_LightParam[2] = set.value("zLightParam",-3).toInt();	// Z position
    m_LightParam[3] = set.value("ambientLightParam",18).toInt();	// Ambient light
    m_LightParam[4] = set.value("diffuseLight",7).toInt();	// Diffuse light
    m_LightParam[5] = set.value("specularLight", 100).toInt();	// Specular light
    m_LightParam[6] = set.value("surfaceAmbient",100).toInt();	// Ambient material
    m_LightParam[7] = set.value("surfaceDiffuse", 100).toInt();	// Diffuse material
    m_LightParam[8] = set.value("surfaceSpecular",100).toInt();	// Specular material
    m_LightParam[9] = set.value("surfaceShine",100).toInt();	// Shininess material
    m_LightParam[10] = set.value("surfaceEmission",0).toInt();	// Emission material

    if (set.value("oglFill", true).toBool())
        m_FillMode = GL_FILL;


    m_colorMap = new dftColorMap(set.value("colorMapType",0).toInt(),0,false,.125,.7);
    m_zRangeMode = "Auto";

    xRot = 25. * 16;
    yRot = -45 * 16;
    zRot = 0;
    m_surfProperties = new surfacePropertiesDlg();
    connect(m_surfProperties,SIGNAL(xLightMoved(int)), this, SLOT(xlightChanged(int)));
    connect(m_surfProperties,SIGNAL(yLightMoved(int)), this, SLOT(ylightChanged(int)));
    connect(m_surfProperties,SIGNAL(zLightMoved(int)), this, SLOT(zlightChanged(int)));
    connect(m_surfProperties,SIGNAL(ambientMoved(int)), this, SLOT(ambientChanged(int)));
    connect(m_surfProperties,SIGNAL(diffuseMoved(int)), this, SLOT(diffuseChanged(int)));
    connect(m_surfProperties,SIGNAL(specularMoved(int)), this, SLOT(specularChanged(int)));
    connect(m_surfProperties,SIGNAL(surfaceAmbientMoved(int)), this, SLOT(surfaceAmbient(int)));
    connect(m_surfProperties,SIGNAL(surfaceDiffuseMoved(int)), this, SLOT(surfaceDiffuse(int)));
    connect(m_surfProperties,SIGNAL(surfaceSpecularMoved(int)), this, SLOT(surfaceSpecular(int)));
    connect(m_surfProperties, SIGNAL(surfaceShineMoved(int)), this, SLOT(surfaceShine(int)));
    connect(m_surfProperties,SIGNAL(surfaceEmitionMove(int)), this, SLOT(surfaceEmission(int)));
    connect(m_surfProperties,SIGNAL(red(int)),this, SLOT(setRed(int)));
    connect(m_surfProperties,SIGNAL(green(int)),this,SLOT(setGreen(int)));
    connect(m_surfProperties,SIGNAL(blue(int)),this, SLOT(setBlue(int)));
    connect(m_tools,SIGNAL(ContourMapColorChanged(int)),this, SLOT(colorMapChanged(int)));
    connect(m_tools,SIGNAL(contourWaveRangeChanged(double)), this,SLOT(contourWaveRangeChanged(double)));
    connect(m_tools, SIGNAL(newDisplayErrorRange(double,double)),
            this, SLOT(setMinMaxValues(double,double)));
    connect(tools, SIGNAL(contourColorRangeChanged(QString)), SLOT(contourColorRangeChanged(QString)));
}

GLWidget::~GLWidget()
{
    makeCurrent();
}



void GLWidget::setXRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != xRot) {
        xRot = angle;
        emit xRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setYRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != yRot) {
        yRot = angle;
        emit yRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setZRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != zRot) {
        zRot = angle;
        emit zRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::resolutionChanged(int v) {
    m_resolutionPercent = v;
    m_dirty_surface = true;
    make_surface_from_doubles();
    updateGL();
}


void GLWidget::SetLight()
{

    //====== Both surface sides are considered when calculating
    //====== each pixel color with the lighting formula

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);

    //====== Light source position depends on the object sizes scaled to (0,100)

    float fPos[] =
    {
        //1.,1000., m_fRangeZ,0};
        (float)(.01 * m_fRangeX * (float)m_LightParam[0]),
        (float)(10 * m_fRangeY * (m_LightParam[1])),
         (float)(.1  *m_fRangeZ * m_LightParam[2]),
        1.f
    };

    glLightfv(GL_LIGHT0, GL_POSITION, fPos);

    //====== Ambient light intensity
    float f = m_LightParam[3]/25.f;
    float fAmbient[4] = { f, f, f, 1.f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, fAmbient);

    //====== Diffuse light intensity
    f = m_LightParam[4]/100.f;
    float fDiffuse[4] = { f, f, f, 1.f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, fDiffuse);

    //====== Specular light intensity
    f =   m_LightParam[5]/100.f;
    float fSpecular[4] =  { f * (float)m_red/100.f, f * (float)m_green/100.f, f * (float)m_blue/100.f, 1.f };
    glLightfv(GL_LIGHT0, GL_SPECULAR, fSpecular);

    //====== Surface material reflection properties for each light component
    f = m_LightParam[6]/100.f;
    float fAmbMat[4] = { f, f, f, 1.f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, fAmbMat);

    f = 10 * m_LightParam[7]/100.f;
    float fDifMat[4] = { f, f, f, 1.f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, fDifMat);

    f = m_LightParam[8]/100.f;
    float fSpecMat[4] = { f, f, f, 1.f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, fSpecMat);

    //====== Material shininess
    float fShine =  (float)m_LightParam[9];
    glMaterialf(GL_FRONT, GL_SHININESS, fShine);

    //====== Material light emission property
    f = m_LightParam[10]/100.f;
    float fEmission[4] = { f, f, f, 0.f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, fEmission);

}

void GLWidget::initializeGL()
{
    static const GLfloat lightPos[4] = { 500.0f, .0f, 1.0f, 0.f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);


    glEnable(GL_NORMALIZE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);



}

void GLWidget::paintGL()
{
    /*
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotated(xRot / 16.0, 1.0, 0.0, 0.0);
    glRotated(yRot / 16.0, 0.0, 1.0, 0.0);
    glRotated(zRot / 16.0, 0.0, 0.0, 1.0);

    //drawGear(gear1, -3.0, -2.0, 0.0, gear1Rot / 16.0);
    //drawGear(gear2, +3.1, -2.0, 0.0, -2.0 * (gear1Rot / 16.0) - 9.0);

    //glRotated(+90.0, 1.0, 0.0, 0.0);
    //drawGear(gear3, -3.1, -1.8, -2.2, +2.0 * (gear1Rot / 16.0) - 2.0);

    */

        DrawScene();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(0.f, 0.f, m_zoomFactor * m_zTrans);
    glRotatef (xRot/16.f, 1.0f, 0.0f, 0.0f );
    glRotatef (yRot/16.f, 0.0f, 1.0f, 0.0f );
    glRotatef (zRot/16.f, 0.0f, 0.0f, 1.0f );
    SetLight();
    glCallList(1);

    //glRotatef (35.f, 1.0f, .0f, 0.0f );
    //glRotatef (-45.f, 0.0f, 1.0f, 0.0f );
    glPopMatrix();

}
void perspectiveGL( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
    const GLdouble pi = 3.1415926535897932384626433832795;
    GLdouble fW, fH;

    //fH = tan( (fovY / 2) / 180 * pi ) * zNear;
    fH = tan( fovY / 360 * pi ) * zNear;
    fW = fH * aspect;

    glFrustum( -fW, fW, -fH, fH, zNear, zFar );

}

void GLWidget::perspectiveChanged(bool b){
    m_ortho = b;
    m_list_good = false;
    updateGL();
}

void GLWidget::resizeGL(int width, int height)
{
    /*
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, +1.0, -1.0, 1.0, 5.0, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(0.0, 0.0, -40.0);
    */

    float m_AngleView = 45.f;
    double dAspect = width<=height ? double(height)/width : double(width)/height;
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    if (!m_ortho)
        gluPerspective (m_AngleView, dAspect, 10, 10000.);
        //perspectiveGL (m_AngleView, dAspect, 10, 10000.);
    else
        glOrtho(-dAspect * 500, dAspect * 500, -500, 500,1000,-1000);
    glViewport(0, 0, width,  height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

}
void GLWidget::wheelEvent (QWheelEvent *e)
{
    QString result;
    if (e->delta() == 0)
        return;

    int del = e->delta()/120;
    m_zoomFactor -= .1 * del;
    updateGL();

}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();
    if (event->buttons() & Qt::LeftButton) {
        if(event->modifiers() && Qt::ShiftModifier ){
            //setXTranslation();
            //setYTranslation();
        }
        else if (event->modifiers()&& Qt::ControlModifier ){
            // light movement
        }
        else if (event->modifiers() && Qt::AltModifier ){
        }
        else {
            setXRotation(xRot + 2 * dy);
            setYRotation(yRot + 2 * dx);
        }
    }
    else if (event->buttons() & Qt::RightButton) {
        setXRotation(xRot + 2 * dy);
        setZRotation(zRot + 2 * dx);
    }
    lastPos = event->pos();
}

void GLWidget::fillGridChanged(bool b){
    QSettings set;
    set.setValue("oglFill", b);
    m_FillMode = (b) ? GL_FILL : GL_LINE;

    m_dirty_surface = true;
    make_surface_from_doubles();
    updateGL();
}

void GLWidget::normalizeAngle(int *angle)
{
    while (*angle < 0)
        *angle += 360 * 16;
    while (*angle > 360 * 16)
        *angle -= 360 * 16;
}

QVector3D normal(QVector3D p1, QVector3D p2, QVector3D p3)
{
    double vx = p1.x() - p3.x();
    double vy = p1.y() - p3.y();
    double vz = p1.z() - p3.z();


    double wx = p2.x() - p1.x();
    double wy = p2.y() - p1.y();
    double wz = p2.z() - p1.z();



    //====== Normal vector coordinates
    double
    nx = vy*wz - wy*vz,
    ny = wx*vz-wz*vx,
    nz = vx*wy-wx*vy,

    //====== Normal vector length
    v  = sqrt(nx*nx + ny*ny + nz*nz);
    if (v == 0)
        v = 1.;

    //====== Scale to unity
    nx /= v;
    ny /= v;
    nz /= v;

    //====== Set the normal vector
    return QVector3D(nx,ny,nz);
}

void drawText(QString txt, float x, float y, float z){

    QPainterPath path;
    glDisable(GL_LIGHTING);
    QFont font("Arial", 40);
    path.addText(QPointF(x, y), QFont("Arial", 20),txt);
    QList<QPolygonF> poly = path.toSubpathPolygons();
    for (QList<QPolygonF>::iterator i = poly.begin(); i != poly.end(); i++){
        glBegin(GL_LINE_LOOP);
        for (QPolygonF::iterator p = (*i).begin(); p != i->end(); p++)
            glVertex3f(p->rx(), -p->ry(), z);
        glEnd();
    }

}

void GLWidget::DrawScene()
{

    //====== Create the new list of OpenGL commands
    if (m_list_good)
        return;
    glNewList(1, GL_COMPILE);
    glShadeModel(GL_SMOOTH);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glMatrixMode (GL_PROJECTION);
    //====== Set the polygon filling mode
    if (m_FillMode == GL_LINE)
    {

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, m_FillMode);
    }
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    //====== Turn on the primitive connection mode (not connected)
    glLineWidth(1.0);
    glBegin (GL_QUADS);
    double minr,maxr;
    minr = m_min_y;
    maxr = m_max_y;
    for (UINT n = 0; n < m_quads.size(); ++n)
    {
        //====== Turn on the primitive connection mode (connected)
        //====== The strip of connected quads begins here
        double
            xi = m_quads[n].i.x(),
            zi = m_quads[n].i.y(),
            yi = m_Vscale * m_quads[n].i.z(),

            xj = m_quads[n].j.x(),
            zj = m_quads[n].j.y(),
            yj = m_Vscale * m_quads[n].j.z(),

            xk = m_quads[n].k.x(),
            zk = m_quads[n].k.y(),
            yk = m_Vscale * m_quads[n].k.z(),

            xn = m_quads[n].n.x(),
            zn = m_quads[n].n.y(),
            yn = m_Vscale * m_quads[n].n.z();

            if (!m_flip_y_view)
            {
                zi = -zi;
                zj = -zj;
                zk = - zk;
                zn = -zn;
            }
            if (m_flip_x_view)
            {
                xi = -xi;
                xj = - xj;
                xk = -xk;
                xn = -xn;
            }
            QVector3D norm = normal(QVector3D(xk,yk,zk), QVector3D(xj,yj,zj), QVector3D(xi,yi,zi));


                //====== Set the normal vector
                // y is inverted on windows screens
            glNormal3f (norm.x(), norm.y(), norm.z());

            //====== Vertices are given in counter clockwise direction order
            QColor c = m_colorMap->color(QwtInterval(minr,maxr),yi/m_Vscale);

            glColor3f(c.redF(),c.greenF(),c.blueF());
            glVertex3f (xi, yi, zi);

            //glNormal3f(norm2.x, norm2.y, norm2.z);
            c = m_colorMap->color(QwtInterval(minr,maxr),yj/m_Vscale);
            glColor3f(c.redF(),c.greenF(),c.blueF());
            glVertex3f (xj, yj, zj);

            //glNormal3f(norm3.x, norm3.y, norm3.z);
            c = m_colorMap->color(QwtInterval(minr,maxr),yk/m_Vscale);
            glColor3f(c.redF(),c.greenF(),c.blueF());
            glVertex3f (xk, yk, zk);

            //glNormal3f(norm4.x, norm4.y, norm4.z);
            c = m_colorMap->color(QwtInterval(minr,maxr),yn/m_Vscale);
            glColor3f(c.redF(),c.greenF(),c.blueF());
            glVertex3f (xn, yn, zn);
        }
    glEnd();

    glPushMatrix();


    //float fAmbient[4] = { 100.f, 100.f, 100.f, 1.f };
    //glLightfv(GL_LIGHT0, GL_AMBIENT, fAmbient);

    /* draw axis */

    // 1/4 wave top and bottom from zero
    double top = 50;
    double bottom = -50;

    int left = -1.25 * RADIUS;
    int back = -1.25 * RADIUS;

    glLineWidth(1.0);

    glColor3f(.4f, .4f, .8f);

    glBegin(GL_LINE_STRIP);
    double bot = bottom;
    glVertex3f(left, bot, RADIUS);
    glVertex3f(RADIUS, bot, RADIUS);
    glVertex3f(RADIUS,bot,back);

    glEnd();

    //glDisable(GL_POLYGON_OFFSET_FILL);

    // Filled grid overlay
    glLineWidth(1.0);


    /*
    if ((m_surface_type == S3D) && m_draw_grid_overlay && m_FillMode != GL_LINE)
    {
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1,-1);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBegin (GL_QUADS);
        // make the grid overlay
        glColor3f(.5,.5,.5);
        for (  n = 0; n < m_grids.size(); ++n)
        {
            //====== Turn on the primitive connection mode (connected)
            //====== The strip of connected quads begins here
            double
                xi = m_grids[n].m_p1.x,
                yi = ScaleDlg->m_scale * m_grids[n].m_p1.y,
                zi = m_grids[n].m_p1.z,

                xj = m_grids[n].m_p2.x,
                yj = ScaleDlg->m_scale *m_grids[n].m_p2.y,
                zj = m_grids[n].m_p2.z,

                xk = m_grids[n].m_p3.x,
                yk = ScaleDlg->m_scale *m_grids[n].m_p3.y,
                zk = m_grids[n].m_p3.z,

                xn = m_grids[n].m_p4.x,
                yn = ScaleDlg->m_scale *m_grids[n].m_p4.y,
                zn = m_grids[n].m_p4.z;

                if (m_flip_y_view)
                {
                    zi = -zi;
                    zj = -zj;
                    zk = - zk;
                    zn = -zn;
                }
                if (m_flip_x_view)
                {
                    xi = -xi;
                    xj = - xj;
                    xk = -xk;
                    xn = -xn;
                }


                //====== Vertices are given in counter clockwise direction order

                glVertex3f (xi, yi, zi);
                glVertex3f (xj, yj, zj);
                glVertex3f (xk, yk, zk);
                glVertex3f (xn, yn, zn);
            }

        glEnd();
        glDisable(GL_POLYGON_OFFSET_LINE);

    }
    */

    glColor3f(.4f, .4f, .8f);
    glBegin(GL_LINES);
    glVertex3f(left, bottom, RADIUS);
    glVertex3f(left, bottom, -RADIUS);
    glVertex3f(-RADIUS, bottom, back);
    glVertex3f(RADIUS, bottom, back);
    glEnd();

    drawText(QString().sprintf("% 6.3lf",-m_BackWall_Scale), RADIUS, 2 * top + 10,back);
    drawText(QString().sprintf("% 6.3lf",m_BackWall_Scale), RADIUS, -2 * top, back);
    drawText(" 0", RADIUS, 5, back);

    glEnable(GL_LIGHTING);
    if (m_draw_profiles_on_3d)
    {
        glBegin(GL_LINES);

        glVertex3f(-RADIUS,0, back);
        glVertex3f(RADIUS,0, back);

        glVertex3f(left, 0, RADIUS);
        glVertex3f(left, 0, -RADIUS);

        // top line of v profile
        glVertex3f(left, top, RADIUS);
        glVertex3f(left, top, - RADIUS);

        glVertex3f(left, 2 * top, RADIUS);
        glVertex3f(left, 2 * top, -RADIUS);

        glVertex3f(left, 2 * bottom, RADIUS);
        glVertex3f(left, 2 * bottom, -RADIUS);

        glVertex3f(-RADIUS, top, back);
        glVertex3f(RADIUS, top, back);

        glVertex3f(-RADIUS, 2 * top, back);
        glVertex3f(RADIUS, 2 * top, back);

        glVertex3f(-RADIUS, 2 * bottom, back);
        glVertex3f(RADIUS, 2 * bottom, back);

        glVertex3f(-RADIUS, top, back);
        glVertex3f(RADIUS, top, back);



        glColor3f(.4f, .4f, .8f);
        glVertex3f(left, 2.5 * bottom, 0);
        glVertex3f(left, 2.5 *  top, 0);

        glVertex3f(left, 2 * top, RADIUS/2);
        glVertex3f(left, 2 * bottom, RADIUS/2);

        glVertex3f(left, 2 * top, -RADIUS/2);
        glVertex3f(left, 2 * bottom, -RADIUS/2);

        glVertex3f(left, 2 * bottom, RADIUS);
        glVertex3f(left, 2 * top, RADIUS);

        glVertex3f(left, 2 * bottom, -RADIUS);
        glVertex3f(left, 2 * top, -RADIUS);

        glVertex3f(-RADIUS, 2 * bottom, back);
        glVertex3f(-RADIUS, 2 * top, back);

        glVertex3f(0, 2.5 * bottom, back);
        glVertex3f(0, 2.5 * top, back);

        glVertex3f(RADIUS, 2 * bottom, back);
        glVertex3f(RADIUS, 2 * top, back);

        glVertex3f(RADIUS/2, 2 * bottom, back);
        glVertex3f(RADIUS/2, 2 * top, back);

        glVertex3f(-RADIUS/2, 2 * bottom, back);
        glVertex3f(-RADIUS/2, 2 * top, back);
        glEnd();



        // Draw vert profile
        glLineWidth(1.0);
        glColor3f(1.f,1.f,1.f);
        glBegin(GL_LINE_STRIP);
        double y_Vscale =100./m_BackWall_Scale;
        for (unsigned int n = 0; n < m_vert_profile.size(); ++n)
        {
            QVector3D pt = m_vert_profile[n];
            glVertex3f(pt.x(), y_Vscale * pt.z(), pt.y());
        }
        glEnd();

        glBegin(GL_LINE_STRIP);
        for(unsigned int n = 0; n < m_horz_profile.size(); ++n)
        {
            QVector3D pt = m_horz_profile[n];
            glVertex3f(pt.x(), y_Vscale * pt.z(), pt.y());
        }

        glEnd();
    }

    //filled left back wall
    /*
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor4f(.7f, .9f, .7f,.5f);
    glBegin (GL_QUADS);
    glVertex3f(left-1,2 * bottom, -RADIUS);
    glVertex3f(left-1,2 * top, -RADIUS);
    glVertex3f(left-1,2 * top,RADIUS);
    glVertex3f(left-1,2 * bottom,RADIUS);
    glDisable(GL_BLEND);

    glEnd();*/


    glPopMatrix();
    //====== Close the list of OpenGL commands
    glEndList();

    m_list_good = true;

}
void GLWidget::setSurface(wavefront *wf) {
    m_wf = wf;
    m_dirty_surface = true;

    make_surface_from_doubles();
    updateGL();
}

void GLWidget::make_surface_from_doubles()
{
    if (!m_dirty_surface)
        return;
    cv::Mat rMask;
    cv::Mat resized;
    double fc = (double)m_resolutionPercent/100.;

    cv::resize(m_wf->workData,resized,cv::Size(),fc,fc,cv::INTER_AREA);
    cv::resize(m_wf->workMask,rMask,cv::Size(),fc,fc,cv::INTER_AREA);
    //compute step size based on 200 points across x radius

    int step = (resized.cols-1)/400.;

    if( m_FillMode == GL_LINE)
        step = resized.cols/50;

    if (step < 1)
        step = 1;

    m_quads.clear();

    int h = resized.rows;
    int w = resized.cols;
    double xy_scale = 2.d * RADIUS/h;


    // make the quads
    double dhalf = ((double)h-1)/2.;
    for (int x = 0; x < w-step; x+=step)
    {
        for (int y = 0; y < h-step; y+=step)
        {

            int nx = x  - dhalf;
            int ny = y - dhalf;
            double rho = sqrt(nx * nx + ny * ny)/dhalf;
            // ignore masked values.
            if (rho > 1.0 || rMask.at<uint8_t>(y,x) != 255  ||
                    rMask.at<uint8_t>(y+step,x) != 255 ||
                    rMask.at<uint8_t>(y+step,x+step) != 255 ||
                    rMask.at<uint8_t>(y, x+step) != 255 )

                continue;
            int yy = y;
            QVector3D p1(nx * xy_scale, ny * xy_scale,resized.at<double>(yy,x));
            QVector3D p2(nx * xy_scale, (ny+step) * xy_scale, resized.at<double>(yy+step,x));
            QVector3D p3((nx+step)*xy_scale, (ny+step)*xy_scale, resized.at<double>(yy+step,x+step));
            QVector3D p4((nx+step)*xy_scale, ny * xy_scale, resized.at<double>(yy,x+step));
            m_quads.push_back(CQuad(p1,p2,p3,p4));
        }
    }


    if (m_draw_profiles_on_3d)
    {
        m_vert_profile.clear();
        m_horz_profile.clear();
        int half = w/2;
        double edge = -1.25 * RADIUS;
        for (int y = 0 ; y < h-step; y += step)

        {
            int ny = y - half;

            if (rMask.at<uint8_t>(w/2,y) != 255)
                continue;

            QVector3D p1(edge, ny * xy_scale, resized.at<double>(h - 1 -y,half));

            m_vert_profile.push_back(p1);

            p1.setY(edge);
            p1.setX(ny * xy_scale);
            p1.setZ(resized.at<double>(half,y));
            m_horz_profile.push_back(p1);

        }

    }

    double left = xy_scale * (-resized.cols/2);
    double	right  = -left;
    double	rear  = left;
    double	front = right;
    double	zrange = front - rear;

    setZranges();

    m_fRangeY = m_max_y - m_min_y;
    m_fRangeX = (float)(1.5 * (right - left));
    m_fRangeZ = (float)(1.5 *  zrange);
    m_zTrans = -m_fRangeZ;
    m_xTrans = 0.;
    m_yTrans = 0.f;
    m_list_good = false;
    m_dirty_surface = false;
}


void GLWidget::setZranges(){
    double maxy = 0,miny = 0;
    if (m_zRangeMode == "Fractions of Wave")
    {
        maxy =  m_profile_scale_setting/2.;
        miny = -m_profile_scale_setting/2;
    }
    else if (m_zRangeMode == "Auto")
    {
        maxy = 3 * m_wf->std;
        miny = -3 * m_wf->std;
    }

    else if (m_zRangeMode == "Min/Max"){
        maxy = m_wf->max;
        miny = m_wf->min;
    }

    m_max_y =  maxy;
    m_min_y =  miny;


}

void GLWidget::showContoursChanged(double){

}
void GLWidget::contourZeroOffsetChanged(const QString &){

}

void GLWidget::contourColorRangeChanged(const QString &arg1){
    m_zRangeMode = arg1;
    setZranges();
    m_list_good = false;
    updateGL();
}

void GLWidget::setMinMaxValues(double min, double max){
    m_profile_scale_setting = max - min;
    m_list_good = false;
    updateGL();
}

void GLWidget::contourIntervalChanged(double){

}

void GLWidget::contourWaveRangeChanged(double val){
    m_profile_scale_setting = val;
    setZranges();
    m_list_good = false;
    updateGL();
}


void GLWidget::colorMapChanged(int ndx){
    if (m_colorMap)
        delete m_colorMap;
    m_colorMap = new dftColorMap(ndx,m_wf, false, .125, .7);

    m_list_good = false;
    updateGL();
}
void GLWidget::backWallScale(double v){
    m_BackWall_Scale = v;
    m_list_good = false;
    updateGL();
}

void GLWidget::ogheightMagValue(int val){
    m_Vscale = val;
    m_list_good = false;
    updateGL();
}

/*
void GLWidget::make_surface_fromZerns(void)
{


    int shift = - STEPS/2;
    m_quads.clear();


    int steps = STEPS;
    if (m_FillMode ==GL_LINE)
        steps = 50;

    double ang_step = M_2_PI/steps;

    double obs =  m_wf->m_inside.m_radius * 2;
    ellipse igram_obs;
    if (z_table.size())
    {
        igram_obs = z_table[m_current_zernike_surface].obs;
        if (igram_obs.xrad)
            igram_obs.normalize_to(z_table[m_current_zernike_surface].outside);
    }

    int rho_start = steps * obs;
    int phi_end = steps;
    for (int phi = 0; phi < phi_end; ++phi)
    {
        double ang = MPI + phi * ang_step;
        double ang2 = MPI+( phi + 1) * ang_step;
        double sin_of_phi1 =sin(ang);
        double cos_of_phi1 = cos(ang);
        double sin_of_phi2 = sin(ang2);
        double cos_of_phi2 = cos(ang2);

        for (int rho=rho_start ; rho < steps; ++rho)

        {
            double r1 = (double)rho/(double)steps;
            double r2 = (double)(rho+1)/(double)(steps);
            double x1 = cos_of_phi1 * r1;
            double y1 = sin_of_phi1 * r1;

            double x2 = cos_of_phi1 * r2;
            double y2 = sin_of_phi1 * r2;

            double x3 = cos_of_phi2 * r2;
            double y3 = sin_of_phi2 * r2;

            double x4 = cos_of_phi2 * r1;
            double y4 = sin_of_phi2 * r1;

            if ((igram_obs.xrad > 0.) &&
                 (igram_obs.is_inside(x1,y1,0) ||
                  igram_obs.is_inside(x2,y2,0) ||
                  igram_obs.is_inside(x3,y3,0) ||
                  igram_obs.is_inside(x4,y4,0)) ||
                  r1 > m_edge_mask_percent || r2 > m_edge_mask_percent)
                  continue;

            CPoint3D p1;
            p1.x = x1 * RADIUS;
            p1.z = y1 * RADIUS;
            p1.y = Wavefront(x1, -y1, Z_TERMS);;


            CPoint3D p2;
            p2.x = x2 * RADIUS;
            p2.z = y2 * RADIUS;
            p2.y = Wavefront(x2,-y2, Z_TERMS);;


            CPoint3D p3;
            p3.x = x3 * RADIUS;
            p3.z = y3 * RADIUS;
            p3.y = Wavefront(x3, -y3, Z_TERMS);;


            CPoint3D p4;
            p4.x = x4 * RADIUS;
            p4.z = y4 * RADIUS;
            p4.y = Wavefront(x4, -y4, Z_TERMS);



            m_quads.push_back(CQuad(p4,p3,p2,p1));

        }
    }
    if (m_draw_profiles_on_3d)
    {
        m_vert_profile.clear();
        m_horz_profile.clear();

        rho_start = steps * obs;
        for (int rho = -steps ; rho < steps; ++rho)

        {
            double r1 = (double)rho/(double)steps;
            double x1 = 0;
            double y1 = r1;
            double x2 = r1;
            double y2 = 0;


            if ((igram_obs.xrad > 0.) &&
                 (igram_obs.is_inside(x1,y1,0) ||
                  r1 > m_edge_mask_percent))
              continue;
            CPoint3D p1;
            p1.x = -1.25 * RADIUS;
            p1.z = y1 * RADIUS;
            p1.y = Wavefront(x1, -y1, Z_TERMS) * m_lambda_adjust;
            m_vert_profile.push_back(p1 );


            p1.x = r1 * RADIUS;
            p1.z = -1.25 * RADIUS;
            p1.y = Wavefront(r1, 0, Z_TERMS) * m_lambda_adjust;

            m_horz_profile.push_back(p1);
        }

    }
    if (m_draw_grid_overlay)
    {
        steps = 30;
        m_grids.clear();
        ang_step = M2PI/steps;
        phi_end = 30;

        rho_start = steps * obs;
        for ( phi = 0; phi < phi_end; ++phi)
        {
            double ang = MPI + phi * ang_step;
            double ang2 = MPI+( phi + 1) * ang_step;
            double sin_of_phi1 =sin(ang);
            double cos_of_phi1 = cos(ang);
            double sin_of_phi2 = sin(ang2);
            double cos_of_phi2 = cos(ang2);

            for (int rho=rho_start ; rho < steps; ++rho)

            {
                double r1 = (double)rho/(double)steps;
                double r2 = (double)(rho+1)/(double)(steps);
                double x1 = cos_of_phi1 * r1;
                double y1 = sin_of_phi1 * r1;

                double x2 = cos_of_phi1 * r2;
                double y2 = sin_of_phi1 * r2;

                double x3 = cos_of_phi2 * r2;
                double y3 = sin_of_phi2 * r2;

                double x4 = cos_of_phi2 * r1;
                double y4 = sin_of_phi2 * r1;

                if ((igram_obs.xrad > 0.) &&
                     (igram_obs.is_inside(x1,y1,0) ||
                      igram_obs.is_inside(x2,y2,0) ||
                      igram_obs.is_inside(x3,y3,0) ||
                      igram_obs.is_inside(x4,y4,0)) ||
                      r1 > m_edge_mask_percent || r2 > m_edge_mask_percent)
                      continue;

                CPoint3D p1;
                p1.x = x1 * RADIUS;
                p1.z = y1 * RADIUS;
                p1.y = Wavefront(x1, -y1, Z_TERMS);;


                CPoint3D p2;
                p2.x = x2 * RADIUS;
                p2.z = y2 * RADIUS;
                p2.y = Wavefront(x2,-y2, Z_TERMS);;


                CPoint3D p3;
                p3.x = x3 * RADIUS;
                p3.z = y3 * RADIUS;
                p3.y = Wavefront(x3, -y3, Z_TERMS);;


                CPoint3D p4;
                p4.x = x4 * RADIUS;
                p4.z = y4 * RADIUS;
                p4.y = Wavefront(x4, -y4, Z_TERMS);



                m_grids.push_back(CQuad(p4,p3,p2,p1));

            }
        }
    }




return;


}
*/
void GLWidget::xlightChanged(int value){
    m_LightParam[0] = value;
    QSettings set;
    set.setValue("xLightParam", value);
    SetLight();
    update();
}
void GLWidget::ylightChanged(int value){
    QSettings set;
    set.setValue("yLightParam",value);
    m_LightParam[1] = value;
    SetLight();
    update();
}
void GLWidget::zlightChanged(int value){
    QSettings set;
    set.setValue("zLightParam",value);
    m_LightParam[2] = value;
    SetLight();
    update();
}
void GLWidget::openLightingDlg(){
    m_surfProperties->show();
}
void GLWidget::ambientChanged(int value){
    QSettings set;
    set.setValue("ambientLightParam",value);
    m_LightParam[3] = value;
    SetLight();
    update();
}
void GLWidget::diffuseChanged(int value){
    QSettings set;
    set.setValue("diffuseLightParam",value);
    m_LightParam[4] = value;
    SetLight();
    update();
}
void GLWidget::specularChanged(int value){
    QSettings set;
    set.setValue("specularLightParam",value);
    m_LightParam[5] = value;
    SetLight();
    update();
}
void GLWidget::surfaceAmbient(int value){
    QSettings set;
    set.setValue("surfaceAmbient",value);
    m_LightParam[6] = value;
    SetLight();
    update();
}
void GLWidget::surfaceDiffuse(int value){
    QSettings set;
    set.setValue("surfaceDiffuse",value);
    m_LightParam[7] = value;
    SetLight();
    update();
}
void GLWidget::surfaceSpecular(int value){
    QSettings set;
    set.setValue("surfaceSpecular",value);
    m_LightParam[8] = value;
    SetLight();
    update();
}
void GLWidget::surfaceShine(int value){
    QSettings set;
    set.setValue("surfaceShine",value);
    m_LightParam[9] = value;
    SetLight();
    update();
}
void GLWidget::surfaceEmission(int value){
    QSettings set;
    set.setValue("surfaceEmission",value);
    m_LightParam[10] = value;
    SetLight();
    update();
}
void GLWidget::setRed(int value){
    m_red = value;
    SetLight();
    update();
}
void GLWidget::setGreen(int value){
    m_green = value;
    SetLight();
    update();

}
void GLWidget::setBlue(int value){
    m_blue = value;
    SetLight();
    update();
}