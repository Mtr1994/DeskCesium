#include "messagewidget.h"

#include <QPainter>
#include <QRect>
#include <QFontMetrics>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPoint>
#include <QtMath>

// test
#include <QDebug>

QList<int> MessageWidget::mListSerialNumber = {};

MessageWidget::MessageWidget(int type, int position, QWidget *parent)
    : QWidget{parent}, mParentWidget(parent), mMessageType(type), mMessagePosition(position)
{
    init();
}

void MessageWidget::showMessage()
{
    if (nullptr == mParentWidget)
    {
        delete this;
        return;
    }

    int number = mListSerialNumber.size();
    if (number >= mMaxMessageNumber) return;

    for (int i = 1; i <= mMaxMessageNumber; i++)
    {
        if (!mListSerialNumber.contains(i))
        {
            mSerialNumber = i;
            mListSerialNumber.append(i);
            break;
        }
    }

    if (mSerialNumber < 0)
    {
        delete mAnimation;
        delete this;
        return;
    }

    // 定义动画
    animateDesign();

    mAnimation->start();

    setVisible(true);
}

void MessageWidget::showMessage(const QString &message)
{
    mMessage = message;
    showMessage();
}

void MessageWidget::init()
{
    QFont font("Microsoft YaHei", 9);
    QFontMetrics metrics(font);
    QRect fontRect = metrics.boundingRect("H");
    mContentMargin = qCeil(fontRect.height() * 0.24);

    setAttribute(Qt::WA_TranslucentBackground);

    // 动画
    mAnimation = new QPropertyAnimation(this, "pos");
    mAnimation->setDuration(240);
    mAnimation->setEasingCurve(QEasingCurve::OutSine);
    connect(mAnimation, &QPropertyAnimation::finished, this, &MessageWidget::slot_position_animation_finish);
}

void MessageWidget::animateDesign()
{
    if (nullptr == mParentWidget) return;
    // 计算合适大小和位置
    int parentWidth = mParentWidget->width();
    int parentHeight = mParentWidget->height();

    QFont font("Microsoft YaHei", 10);
    QFontMetrics metrics(font);

    QRect fontRect = metrics.boundingRect(mMessage);
    int width = fontRect.height() * 3.2 * 1.5 + fontRect.width();
    resize(width, fontRect.height() * 2);

    if (mMessagePosition == P_Top_Center)
    {
        // 在更改父组件大小的时候，动态修改消息控件位置
        setGeometry((parentWidth - width) / 2.0, this->y(), this->width(), this->height());

        mAnimation->setStartValue(QPoint((parentWidth - width) / 2.0, -fontRect.height() * 2.8));
        mAnimation->setEndValue(QPoint((parentWidth - width) / 2.0, fontRect.height() * 4.2 * mSerialNumber));
    }
    else if (mMessagePosition == P_Top_Right)
    {
        // 在更改父组件大小的时候，动态修改消息控件位置
        setGeometry((parentWidth - width - fontRect.height()), this->y(), this->width(), this->height());

        mAnimation->setStartValue(QPoint(parentWidth - width - fontRect.height(), -fontRect.height() * 2.8));
        mAnimation->setEndValue(QPoint(parentWidth - width - fontRect.height(), fontRect.height() * 4.2 * mSerialNumber));
    }
    else if (mMessagePosition == P_Bottom_Center)
    {
        // 在更改父组件大小的时候，动态修改消息控件位置
        setGeometry((parentWidth - width) / 2.0, this->y(), this->width(), this->height());

        mAnimation->setStartValue(QPoint((parentWidth - width) / 2.0, parentHeight + fontRect.height() * 2.8));
        mAnimation->setEndValue(QPoint((parentWidth - width) / 2.0, parentHeight - fontRect.height() * 4.2 * mSerialNumber));
    }
    else if (mMessagePosition == P_Bottom_Right)
    {
        // 在更改父组件大小的时候，动态修改消息控件位置
        setGeometry((parentWidth - width - fontRect.height()), parentHeight - fontRect.height() * 4.2 * mSerialNumber, this->width(), this->height());

        mAnimation->setStartValue(QPoint(parentWidth - width - fontRect.height(), parentHeight + fontRect.height() * 2.8));
        mAnimation->setEndValue(QPoint(parentWidth - width - fontRect.height(), parentHeight - fontRect.height() * 4.2 * mSerialNumber));
    }
}

void MessageWidget::slot_hide_message_widget()
{
    mListSerialNumber.removeOne(mSerialNumber);

    // 在该函数中自删除
    mAnimation->setDuration(120);
    mAnimation->setDirection(QPropertyAnimation::Backward);
    mAnimation->start();
}

void MessageWidget::slot_position_animation_finish()
{
    if (mAnimation->direction() == QPropertyAnimation::Backward)
    {
        setVisible(false);
        delete mAnimation;
        delete this;
    }
    else
    {
        QTimer::singleShot(3200, this, &MessageWidget::slot_hide_message_widget);
    }
}

int MessageWidget::getMessagePosition() const
{
    return mMessagePosition;
}

void MessageWidget::setMessagePosition(int newMessagePosition)
{
    mMessagePosition = newMessagePosition;
}

int MessageWidget::getMessageType() const
{
    return mMessageType;
}

void MessageWidget::setMessageType(int newMessageType)
{
    mMessageType = newMessageType;
}

int MessageWidget::getMaxMessageNumber() const
{
    return mMaxMessageNumber;
}

void MessageWidget::setMaxMessageNumber(int newMaxMessageNumber)
{
    mMaxMessageNumber = newMaxMessageNumber;
}

void MessageWidget::setColor(qreal r, qreal g, qreal b, qreal a)
{
    mColor = QColor(r, g, b, a);
}

void MessageWidget::setBorderColor(qreal r, qreal g, qreal b, qreal a)
{
    mBorderColor = QColor(r, g, b, a);
}

void MessageWidget::setMessage(const QString &message)
{
    mMessage = message;
}

void MessageWidget::paintEvent(QPaintEvent *event)
{
    QPainter *painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(mBorderColor, 2, Qt::SolidLine));
    painter->setBrush(QBrush(mColor));

    int width = this->width();
    int height = this->height();

    // 重新计算动画位置
    animateDesign();

    QRect rect = this->rect();
    painter->drawRoundedRect(rect, mContentMargin, mContentMargin);

    // 绘制图标
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    QString url = (mMessageType == M_Success) ? ":/Resource/image/success.svg" : (mMessageType == M_Error) ? ":/Resource/image/error.svg" : ":/Resource/image/info.svg";
    painter->drawImage(QRect(height * 0.29, height * 0.29, height * 0.42, height * 0.42), QImage(url));

    // 绘制消息
    painter->setPen(QPen(QColor(89, 89, 89, 255), 2, Qt::SolidLine));
    painter->setFont(QFont("Microsoft YaHei", 10));
    QFontMetrics metrics = painter->fontMetrics();
    QRect fontRect = metrics.boundingRect(mMessage);

    painter->drawText(height, ((fontRect.y() < 0) ? fontRect.y() * -1 : 0) + (height - fontRect.height()) / 2.0, mMessage);

    painter->end();

    delete painter;

    QWidget::paintEvent(event);
}
