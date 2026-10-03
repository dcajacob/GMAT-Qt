#pragma once
#include <QWidget>
class QtPlotReceiver;
class QAction;
class QSlider;
class QTimer;
class QLabel;
class QComboBox;

// One clock for all retained plot histories, including plots closed in the MDI.
class PlotPlaybackControls final : public QWidget
{
public:
   explicit PlotPlaybackControls(QtPlotReceiver &receiver,QWidget *parent=nullptr);
   void syncFromReceiver();
   void stop();
private:
   QtPlotReceiver &receiver;
   QAction *play;
   QAction *slower;
   QAction *faster;
   QSlider *timeline;
   QTimer *timer;
   QLabel *position;
   QComboBox *speed;
   QComboBox *mode;
   double remainder=0;
   void seek(int value);
   void updateSpeedButtons();
};
