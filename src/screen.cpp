#include "screen.h"
#include "settings.h"
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
  clearScreen();
}

/**
 * @brief Canvas2D::clearCanvas sets all canvas pixels to blank white
 */
void Screen::clearScreen() {
  m_data.assign(m_width * m_height, RGBA{255, 255, 255, 255});
  settings.imagePath = "";
  displayImage();
}

/**
 * @brief Stores the image specified from the input file in this class's
 * `std::vector<RGBA> m_image`.
 * Also saves the image width and height to canvas width and height
 * respectively.
 * @param file: file path to an image
 * @return True if successfully loads image, False otherwise.
 */
bool Screen::loadImageFromFile(const QString &file) {
  QImage myImage;
  if (!myImage.load(file)) {
    std::cout << "Failed to load in image" << std::endl;
    return false;
  }
  myImage = myImage.convertToFormat(QImage::Format_RGBX8888);
  m_width = myImage.width();
  m_height = myImage.height();
  QByteArray arr = QByteArray::fromRawData((const char *)myImage.bits(),
                                           myImage.sizeInBytes());

  m_data.clear();
  m_data.reserve(m_width * m_height);
  for (int i = 0; i < arr.size() / 4.f; i++) {
    m_data.push_back(
        RGBA{(std::uint8_t)arr[4 * i], (std::uint8_t)arr[4 * i + 1],
             (std::uint8_t)arr[4 * i + 2], (std::uint8_t)arr[4 * i + 3]});
  }
  displayImage();
  return true;
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
 * @brief Get Canvas2D's image data and display this to the GUI
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
 * @brief Canvas2D::resize resizes canvas to new width and height
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
