#include "screen.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <iostream>

/**
 * @brief Initializes new 500x500 canvas
 */
void Screen::init() {
  setMouseTracking(true);
  m_width = 1024;
  m_height = 768;
}

/**
 * @brief Saves the current canvas image to the specified file path.
 * @param file: file path to save image to
 * @return True if successfully saves image, False otherwise.
 */
bool Screen::saveImageToFile(const QString &file) {
  QImage myImage = QImage(m_width, m_height, QImage::Format_RGBX8888);
  for (int i = 0; i < m_data.size(); i++) {
    myImage.setPixelColor(
        i % m_width, i / m_width,
        QColor(m_data[i].r, m_data[i].g, m_data[i].b, m_data[i].a));
  }
  if (!myImage.save(file)) {
    std::cout << "Failed to save image" << std::endl;
    return false;
  }
  return true;
}

/**
 * @brief Get Screen's image data and display this to the GUI
 */
void Screen::displayImage() {
  QByteArray img(reinterpret_cast<const char *>(m_data.data()),
                 4 * m_data.size());
  QImage now = QImage((const uchar *)img.data(), m_width, m_height,
                      QImage::Format_RGBX8888);
  setPixmap(QPixmap::fromImage(now));
  setFixedSize(m_width, m_height);
  update();
}

/**
 * @brief Resizes canvas to new width and height
 * @param w
 * @param h
 */
void Screen::resize(int w, int h) {
  m_width = w;
  m_height = h;
  m_data.resize(w * h);
  displayImage();
}

/**
 * @brief Called when any of the parameters in the UI are modified.
 */
void Screen::settingsChanged() {}
