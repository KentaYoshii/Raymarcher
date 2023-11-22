#ifndef ASPECTRATIOWIDGET_HPP
#define ASPECTRATIOWIDGET_HPP

#include <QBoxLayout>
#include <QWidget>

class AspectRatioWidget : public QWidget {
  Q_OBJECT
public:
  AspectRatioWidget(QWidget *parent = 0) : QWidget(parent) {
    m_layout = new QHBoxLayout();
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
  }

  // the widget we want to keep the ratio
  void setAspectWidget(QWidget *widget, const double ratio = 1.0) {
    m_aspect_widget = widget;
    m_layout->addWidget(widget);
    m_ratio = ratio;
  }
  void setRatio(const double ratio) {
    m_ratio = ratio;
    applyAspectRatio();
  }

protected:
  void resizeEvent(QResizeEvent *event) {
    (void)event;
    applyAspectRatio();
  }

public slots:
  void applyAspectRatio() {
    int w = this->width();
    int h = this->height();
    double aspect = static_cast<double>(h) / static_cast<double>(w);

    if (aspect < m_ratio) // parent is too wide
    {
      int target_width = static_cast<int>(static_cast<double>(h) / m_ratio);
      m_aspect_widget->setMaximumWidth(target_width);
      m_aspect_widget->setMaximumHeight(h);

    } else // parent is too high
    {
      int target_heigth = static_cast<int>(static_cast<double>(w) * m_ratio);
      m_aspect_widget->setMaximumHeight(target_heigth);
      m_aspect_widget->setMaximumWidth(w);
    }
  }

private:
  QHBoxLayout *m_layout;

  QWidget *m_aspect_widget;

  double m_ratio;
};

#endif // ASPECTRATIOWINDOW_H
