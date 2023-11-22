#ifndef SCREEN_H
#define SCREEN_H

#include "utils/rgba.h"
#include <QLabel>
#include <QMouseEvent>
#include <array>

class Screen : public QLabel {
  Q_OBJECT
public:
  int m_width = 0;
  int m_height = 0;

  void init();
  bool saveImageToFile(const QString &file);
  void displayImage();
  void resize(int w, int h);

  // This will be called when the settings have changed
  void settingsChanged();

private:
  std::vector<RGBA> m_data;
};

#endif // SCREEN_H
