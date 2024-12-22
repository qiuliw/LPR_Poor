#include "widget.h"
#include "ui_widget.h"
#include <QFile>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>

Widget::Widget(QWidget *parent):
    QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::mouseDoubleClickEvent(QMouseEvent *event)  //鼠标双击事件
{
    if(!img.empty()||onWork==true)
        return;
    if (event->button() == Qt::LeftButton)
    {
        QString Qpath = QFileDialog::getOpenFileName(
            nullptr, "Open File", QDir::homePath(),
            "All Files (*);;Text Files (*.txt)");
        if (!Qpath.isEmpty())
        {
            String path = String((const char *)Qpath.toLocal8Bit());
            Mat mat = imread(path, 1);//一定要使用绝对路径，其他可以回报错
            if (!mat.empty())
            {
                src=mat.clone();
                img=src.clone();
                showImage(img, ui->lb_main);
                ui->lb_output->setText("点击右下方蓝色按钮进行识别");
            }
            else
            {
                QMessageBox::warning(this,"文件错误","不支持该文件类型");
                qWarning() << "Unsupported File type";
            }
        }
    }
}

void Widget::dragEnterEvent(QDragEnterEvent *event)  //将文件拖拽进来的事件
{
    if(event->mimeData()->hasUrls()&&onWork==false)
    {
        event->acceptProposedAction();
    }
    else
        event->ignore();//不接受鼠标事件
}

void Widget::dropEvent(QDropEvent *event)  //拖拽释放事件
{
    if(event->mimeData()->hasUrls()&&onWork==false)
    {
        const QMimeData *qm = event->mimeData();//获取MIMEData
        String path = String((const char *)qm->urls()[0].toLocalFile().toLocal8Bit());
        src = imread(path, 1);//一定要使用绝对路径，其他容易报错
        if (!src.empty())
        {
            img=src.clone();
            showImage(img, ui->lb_main);
            ui->lb_output->setText("点击右下方蓝色按钮进行识别");
        }
        else
        {
            event->ignore();//不接受鼠标事件
            QMessageBox::warning(this,"图片打开错误","无法解析该图片格式");
            qWarning() << "图片打开错误";
        }
    }
    else
    {
        event->ignore();//不接受鼠标事件
        QMessageBox::warning(this,"文件路径错误","找不到文件所在路径");
        qWarning() << "文件路径错误";
    }
}

void Widget::resizeEvent(QResizeEvent *event)  //窗口大小改变事件
{
    if(!img.empty())
        showImage(img,ui->lb_main);
}

void Widget::on_btn1_recognize_clicked()  //点击识别按钮
{
    if(img.empty()||onWork==true)
        return;
    onWork=true;
    ui->lb_output->setText("正在识别中，请耐心等候");
    string str_lic=recognize(src);
    if(str_lic!="")
        ui->lb_output->setText(QString::fromLocal8Bit(str_lic.data()));
    else
        ui->lb_output->setText("识别失败");
    img=src.clone();
    showImage(img,ui->lb_main);
    onWork=false;
}

void Widget::on_btn2_clear_clicked()  //点击清空按钮
{
    if(!img.empty()&&onWork==false)
    {
        ui->lb_main->clear();
        ui->lb_main->setText("拖 动 图 片 到 此 处\n或 双 击 此 处");
        ui->lb_output->clear();
        ui->lb_output->setText("请打开待识别图片");
        src=Mat();
        img=src.clone();
    }
}

string Widget::recognize(Mat input)  //车牌识别主函数
{
    img=lic_locate(input);
    if(img.empty())
    {
        qWarning() << "车牌定位失败";
        return "";
    }
    showImage(img,ui->lb_main);

    img=char_extract(img);
    showImage(img,ui->lb_main);

    vector<Mat> licChar=char_segment(img);
    if(licChar.size()<7)
    {
        qWarning() << "字符分割失败";
        return "";
    }

    string str_lic="";
    Mat mat = licChar[0].clone();
    str_lic+=match_province(mat);
    for (size_t i = 1; i < licChar.size(); i++)
        str_lic+=match_NumLet(licChar[i]);
    return str_lic;
}


// 显示分割后的字符
void Widget::displaySegmentedChars(const std::vector<cv::Mat>& segmentedChars)
{
    // 创建一个新的窗口
    QWidget* window = new QWidget;
    window->setWindowTitle("3.分割后的字符");

    // 创建一个横向布局
    QHBoxLayout* layout = new QHBoxLayout(window);

    // 设置布局中的间距（例如，设置为10像素）
    layout->setSpacing(10);

    // 遍历分割后的字符图像，并添加到布局中
    for (const auto& mat : segmentedChars)
    {
        // 将OpenCV的Mat转换为QImage
        // 注意：这里假设mat是单通道的灰度图像
        QImage qimg(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);

        QImage imgCopy = qimg.copy();

        // 创建一个QLabel并设置图像
        QLabel* label = new QLabel;
        label->setPixmap(QPixmap::fromImage(imgCopy));
        label->setAlignment(Qt::AlignCenter);


        // 将QLabel添加到布局中
        layout->addWidget(label);
    }


    // 设置窗口的布局
    window->setLayout(layout);

    // 显示窗口
    window->show();
}



// 显示处理后的图像的函数
void Widget::showImage(const cv::Mat &image, const QString &windowTitle)
{
    // 将OpenCV的Mat对象转换为QImage对象
    // 注意：这里假设图像是8位单通道（灰度图）或8位三通道（彩色图）
    QImage qimg;
    if (image.type() == CV_8UC1) {
        // 灰度图
        qimg = QImage((const unsigned char*)(image.data), image.cols, image.rows, static_cast<int>(image.step), QImage::Format_Grayscale8);
    } else if (image.type() == CV_8UC3) {
        // 彩色图
        qimg = QImage((const unsigned char*)(image.data), image.cols, image.rows, static_cast<int>(image.step), QImage::Format_BGR888);
    } else {
        qWarning() << "Unsupported image format!";
        return;
    }

    // 转换为QPixmap以便在QLabel中使用
    QPixmap pixmap = QPixmap::fromImage(qimg);

    // 创建一个新的QWidget作为窗口（或者可以使用现有的QWidget）
    QWidget *window = new QWidget;
    window->setWindowTitle(windowTitle); // 设置窗口标题

    // 创建一个QLabel来显示图像
    QLabel *label = new QLabel(window);
    label->setPixmap(pixmap.scaled(400, pixmap.height() * 400 / pixmap.width(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    // 注意：这里我假设您想要固定窗口宽度为400px，并相应地缩放高度。
    // 如果您想要窗口大小与图像大小一致，可以直接使用 pixmap.size() 作为 scaled 的参数。

    // 创建一个垂直布局，并将标签添加到布局中
    QVBoxLayout *layout = new QVBoxLayout(window);
    layout->addWidget(label);
    layout->setContentsMargins(0, 0, 0, 0); // 去除布局边距

    // 设置窗口的布局
    window->setLayout(layout);

    // 显示窗口
    window->show();
}
