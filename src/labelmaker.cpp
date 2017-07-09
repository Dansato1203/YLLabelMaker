#include "labelmaker.h"
#include <ui_labelmaker.h>
#include <ui_dirdialog.h>
using namespace std;

LabelMaker::LabelMaker(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LabelMaker),
    d_ui(new Ui::DirDialog),
    img_index(0),
    viewoffset(50),
    key(QDir::homePath()+"/.labelmaker.ini",QSettings::IniFormat)
{
    ui->setupUi(this);
    d_ui->setupUi(&dialog);
    dialog.setWindowFlags(Qt::WindowStaysOnTopHint);
    connectSignals();
    ui->graphicsView->setScene(&scene);
    readKey();
	dialog.show();
	installEventFilter(this);
}

void LabelMaker::readKey()
{
    int index = key.value("INDEX").toInt();
    QString imagedir = key.value("IMAGESDIR").toString();
    QString savedir = key.value("SAVEDIR").toString();
    if(index)
    {
        img_index = index;
    }
    if(imagedir.size() > 0)
    {
        d_ui->lineImageDir->setText(imagedir);
    }
    if(savedir.size() > 0)
    {
        d_ui->lineSaveTo->setText(savedir);
    }
}

void LabelMaker::saveKey()
{
    key.setValue("IMAGESDIR",d_ui->lineImageDir->text());
    key.setValue("SAVEDIR",d_ui->lineSaveTo->text());
    key.setValue("INDEX",img_index);
}

LabelMaker::~LabelMaker()
{
    writeText();
    saveKey();
    delete ui;
    delete d_ui;
}

void LabelMaker::onMouseMovedGraphicsView(int x, int y, Qt::MouseButton b)
{
    correctCoordiante(x,y);
    c_view.x = x;
    c_view.y = y;
    c_view.b = b;
    updateView();
}

void LabelMaker::onMousePressedGraphicsView(int mx, int my, Qt::MouseButton b)
{
    if(b == Qt::LeftButton)
    {
        correctCoordiante(mx,my);
        c_rect.x = mx;
        c_rect.y = my;
        c_rect.b = b;
        c_rect.flag = 1;
        updateView();
    }
    if(b == Qt::RightButton)
    {
        if(bboxes.size() > 0)
        {
            bboxes.pop_back();
        }
    }
}

void LabelMaker::onMouseReleasedGraphicsView(int mx, int my, Qt::MouseButton b)
{
    if(b == Qt::LeftButton)
    {
		if(img_list.size() != 0)
		{
			appendBbox(ui->spinLabelNumber->value(),c_rect.x,c_rect.y,c_view.x,c_view.y);
		}
		c_rect.flag = 0;
		updateView();

    }
    else if(b == Qt::RightButton)
    {
        updateView();
    }
}

void LabelMaker::resizeGraphicsView()
{
    updateView();
}

void LabelMaker::onPushNext()
{
    changeIndex(1);
}

void LabelMaker::onPushBack()
{
    changeIndex(-1);
}

void LabelMaker::onPushChooseDirectory()
{
    dialog.show();
}

void LabelMaker::onPushChooseImagesDir()
{
    img_index = 0;
    QDir dir = myq.selectDir(QDir(d_ui->lineImageDir->text()));
    d_ui->lineImageDir->setText(dir.path());
    d_ui->lineSaveTo->setText(dir.path()+"_labels");
}

void LabelMaker::onPushChooseSaveTo()
{
    QDir dir = myq.selectDir(QDir(d_ui->lineImageDir->text()));
    d_ui->lineSaveTo->setText(dir.path());
}

void LabelMaker::destroyDirDialog()
{
    img_list.clear();
    img_list = makeImageList(d_ui->lineImageDir->text());
	if(img_list.size() != 0)
	{
		loadImage();
		readText();
		updateView();
	}
}

void LabelMaker::onPushPlus()
{
    ui->spinLabelNumber->setValue(ui->spinLabelNumber->value()+1);
}

void LabelMaker::onPushMinus()
{
    ui->spinLabelNumber->setValue(ui->spinLabelNumber->value()-1);
}

void LabelMaker::textChangedLinePage()
{
	bool ok;
	int page = ui->linePage->text().toInt(&ok, 10) -1;
	if(ok)
	{
		if(0 <= page && page < img_list.size())
		{
			changeIndex(page - img_index);
		}
	}
}

void LabelMaker::connectSignals()
{
    bool ret;
    ret = connect(ui->graphicsView,SIGNAL(mouseMoved(int,int,Qt::MouseButton)),this,SLOT(onMouseMovedGraphicsView(int,int,Qt::MouseButton)));
    assert(ret);
    ret = connect(ui->graphicsView,SIGNAL(mousePressed(int,int,Qt::MouseButton)),this,SLOT(onMousePressedGraphicsView(int,int,Qt::MouseButton)));
    assert(ret);
    ret = connect(ui->graphicsView,SIGNAL(mouseReleased(int,int,Qt::MouseButton)),this,SLOT(onMouseReleasedGraphicsView(int,int,Qt::MouseButton)));
    assert(ret);
    ret = connect(ui->graphicsView,SIGNAL(resized()),this, SLOT(resizeGraphicsView()));
    assert(ret);
    ret = connect(ui->pushNext,SIGNAL(clicked()),this,SLOT(onPushNext()));
    assert(ret);
    ret = connect(ui->pushBack,SIGNAL(clicked()),this,SLOT(onPushBack()));
    assert(ret);
    ret = connect(ui->pushChooseDirectory,SIGNAL(clicked()),this,SLOT(onPushChooseDirectory()));
    assert(ret);
    ret = connect(d_ui->pushChooseImageDir,SIGNAL(clicked()),this,SLOT(onPushChooseImagesDir()));
    assert(ret);
    ret = connect(d_ui->pushChooseSaveto,SIGNAL(clicked()),this,SLOT(onPushChooseSaveTo()));
    assert(ret);
    ret = connect(d_ui->pushOK,SIGNAL(clicked()),&dialog,SLOT(close()));
    assert(ret);
    ret = connect(&dialog,SIGNAL(finished(int)),this,SLOT(destroyDirDialog()));
    assert(ret);
    ret = connect(ui->pushPlus,SIGNAL(clicked()),this,SLOT(onPushPlus()));
    assert(ret);
    ret = connect(ui->pushMinus,SIGNAL(clicked()),this,SLOT(onPushMinus()));
    assert(ret);
	ret = connect(ui->linePage,SIGNAL(editingFinished()),this,SLOT(textChangedLinePage()));
    assert(ret);

}

int LabelMaker::updateView()
{
    scene.clear();
	scene.setSceneRect(0,0,ui->graphicsView->width(), ui->graphicsView->height());
    if(img_list.size() == 0)
    {
        return -1;
    }
    if(currentimg.empty())
    {
        qDebug() << "img empty.";
        return -1;
    }
    setImage(currentimg);
    drawCursur();
    drawBbox();
    if(c_rect.flag == 1)
    {
        drawRect();
    }
    ui->labelFilename->setText(img_list[img_index].fileName());
	ui->linePage->setText(QString("%1").arg(img_index+1));
    ui->linePageNum->setText(QString("%1").arg(img_list.size()));
    ui->labelDebug->clear();
	ui->labelDebug->setText(
            QString("NUM_BBOX[%1]").arg(bboxes.size())
            + QString(" x:%1 y:%2").arg(c_view.x).arg(c_view.y) 
            + QString(" (size %1 x %2 )").arg(scene_img_w).arg(scene_img_h)
            );
    return 0;
}

int LabelMaker::setImage(cv::Mat img)
{
    int offset = viewoffset;
    scene_img_w = ui->graphicsView->width()-offset*2;
    scene_img_h = ui->graphicsView->height()-offset*2;
    QPixmap pix;
    if(scene_img_w<=0 || scene_img_h<=0)
    {
        return -1;
    }
    cv::resize(img,img,cv::Size(scene_img_w,scene_img_h));
    pix = myq.MatBGR2pixmap(img);
    QGraphicsPixmapItem *p = scene.addPixmap(pix);
    p->setPos(offset,offset);
    return 0;
}

int LabelMaker::drawCursur()
{
	int r = 4;
	int offset = viewoffset;
    int w = scene_img_w;
    int h = scene_img_h;
	QPen p = QPen(myq.retColor(ui->spinLabelNumber->value())),QBrush(myq.retColor(ui->spinLabelNumber->value()));
    scene.addEllipse(c_view.x-(r/2),c_view.y-(r/2),r,r,p);
	if(ui->checkCrossLine->checkState() == Qt::Checked)
	{
		scene.addLine(c_view.x   ,offset+1          ,c_view.x   ,h-1+offset   ,p);
		scene.addLine(offset+1   ,c_view.y   ,w-1+offset ,c_view.y   ,p);
	}
    return 0;
}

int LabelMaker::drawRect()
{
    scene.addRect(QRect(QPoint(c_rect.x,c_rect.y),QPoint(c_view.x,c_view.y)),QPen(myq.retColor(ui->spinLabelNumber->value()),3));
    return 0;
}

int LabelMaker::drawBbox()
{
	int offset=viewoffset;
    int w = scene_img_w;
    int h = scene_img_h;
    for(int i=0;i<bboxes.size();i++)
    {
        int x1 = w * (bboxes[i].x - (bboxes[i].w/2));
        int y1 = h * (bboxes[i].y - (bboxes[i].h/2));
        int x2 = w * (bboxes[i].x + (bboxes[i].w/2));
        int y2 = h * (bboxes[i].y + (bboxes[i].h/2));
		x1 += offset;
		y1 += offset;
		x2 += offset;
		y2 += offset;
        scene.addRect(QRect(QPoint(x1,y1),QPoint(x2,y2)),QPen(QBrush(myq.retColor(bboxes[i].label)),3));
    }
    return 0;
}

void LabelMaker::loadImage()
{
	currentimg = cv::imread(img_list[img_index].filePath().toStdString());
}

void LabelMaker::correctCoordiante(int &x, int &y)
{
	int offset = viewoffset;
    int w = scene_img_w;
    int h = scene_img_h;
    x = (x > offset) ?x:offset;
    y = (y > offset) ?y:offset;
    x = (x < w+offset-1) ?x:w+offset-1;
    y = (y < h+offset-1) ?y:h+offset-1;
}

QFileInfoList LabelMaker::makeImageList(QString path)
{
    QFileInfoList list,l_png,l_jpg;
    l_png = myq.scanFiles(path,"*png");
    l_jpg = myq.scanFiles(path,"*jpg");
    for(int i=0;i<l_png.size();i++)
    {
        list.append(l_png[i]);
    }
    for(int i=0;i<l_jpg.size();i++)
    {
        list.append(l_jpg[i]);
    }
    return list;
}

void LabelMaker::writeText()
{
    if(bboxes.size() > 0)
    {
        QString save_dir = d_ui->lineSaveTo->text();
        QDir dir(save_dir);
        QString fn = QString(dir.path()+"/"+img_list[img_index].fileName());
        fn.chop(4);
        fn = fn + ".txt";
        if (!dir.exists()){
            dir.mkpath(save_dir);
        }
        ofstream ofs;
        ofs.open(fn.toLocal8Bit());
        for(int i=0;i<bboxes.size();i++)
        {
            QString line = QString("%1 %2 %3 %4 %5").arg(bboxes[i].label).arg(bboxes[i].x).arg(bboxes[i].y).arg(bboxes[i].w).arg(bboxes[i].h);
            ofs << line.toStdString() << std::endl;
        }
        ofs.close();
    }
    bboxes.clear();
}

void LabelMaker::readText()
{
	bboxes.clear();
    vector<string> lines;
    QString save_dir = d_ui->lineSaveTo->text();
    QDir dir(save_dir);
    QString fn = QString(dir.path()+"/"+img_list[img_index].fileName());
    fn.chop(4);
    fn = fn + ".txt";
    QFileInfo finfo(fn);
    if(finfo.exists())
    {
        ifstream ifs;
        ifs.open(fn.toLocal8Bit());
        string l;
        while(getline(ifs,l))lines.push_back(l);
    }
    for(int i=0;i<lines.size();i++)
    {
        QString qstr = QString::fromStdString(lines[i]);
        QStringList ql = qstr.split(" ");
        Bbox box;
        box.label = ql[0].toInt();
        box.x = ql[1].toFloat();
        box.y = ql[2].toFloat();
        box.w = ql[3].toFloat();
        box.h = ql[4].toFloat();
        bboxes.push_back(box);
    }
}

void LabelMaker::appendBbox(int label, int x1, int y1, int x2, int y2)
{
	int offset = viewoffset;
	x1 -= offset;
	y1 -= offset;
	x2 -= offset;
	y2 -= offset;
    int width = scene_img_w;
    int height = scene_img_h;
    Bbox bbox;
    int x = (x1+x2)/2;
    int y = (y1+y2)/2;
    int w = abs(x1 - x2);
    int h = abs(y1 - y2);
    bbox.label = label;
    bbox.x = (float)x/width;
    bbox.y = (float)y/height;
    bbox.w = (float)w/width;
    bbox.h = (float)h/height;
    //qDebug() << QString("x:%1 ,y:%2 ,w:%3 ,h:%4").a(abs(c_rect.y - c_view.y))rg(bbox.x).arg(bbox.y).arg(bbox.w).arg(bbox.h);
    if((x&&y) && (w&&h))
    {
        bboxes.push_back(bbox);
    }
}

void LabelMaker::changeIndex(int num)
{
	writeText();
    img_index+=num;
	if( img_index < 0)
    {
		img_index = 0;
    }
	if(img_list.size()-1 < img_index)
	{
		img_index = img_list.size()-1;
	}
	readText();
    loadImage();
    updateView();
}

bool LabelMaker::eventFilter(QObject *obj, QEvent *eve)
{
	QKeyEvent *key;
	if(eve->type() == QEvent::KeyRelease)
	{
		key = static_cast<QKeyEvent*>(eve);
		if(key->key()==0x1000014 || key->text()=="d")
		{
			onPushNext();
		}
		if(key->key()==0x1000012 || key->text()=="a")
		{
			onPushBack();
		}
		return true;
	}
}
