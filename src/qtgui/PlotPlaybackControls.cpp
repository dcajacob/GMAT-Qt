#include "PlotPlaybackControls.hpp"
#include "StandardIcons.hpp"
#include "PlotReplaySpeeds.hpp"
#include "PlotModel.hpp"
#include "QtPlotReceiver.hpp"
#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <algorithm>

PlotPlaybackControls::PlotPlaybackControls(QtPlotReceiver &plots,QWidget *parent)
   : QWidget(parent),receiver(plots)
{
   setObjectName("allPlotPlaybackControls");
   setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Preferred);
   auto *layout=new QHBoxLayout(this); layout->setContentsMargins(4,0,4,0); layout->setSpacing(3);
   layout->setAlignment(Qt::AlignLeft);
   auto *scope=new QLabel("All plots",this); scope->setObjectName("allPlotPlaybackScope");
   scope->setToolTip("These controls link playback across all plot windows"); layout->addWidget(scope);
   auto action=[&](const QString &name,const QString &text,QStyle::StandardPixmap icon,const QString &tip) {
      auto *result=new QAction(standardActionIcon(icon,this),text,this); result->setObjectName(name); result->setToolTip(tip);
      auto *button=new QToolButton(this);
      button->setObjectName(name);
      // Derive the accessible name from the action as Play changes to Pause.
      button->setDefaultAction(result);
      layout->addWidget(button);
      return result;
   };
   auto *start=action("allPlotReplayStart","Start",QStyle::SP_MediaSkipBackward,"Pause all plots and return to the start of retained history");
   play=action("allPlotReplayPlay","Play",QStyle::SP_MediaPlay,"Play or pause retained history in all Orbit, Ground Track and XY displays");
   play->setCheckable(true);
   auto *latest=action("allPlotReplayLatest","Latest",QStyle::SP_MediaSkipForward,"Stop animation and follow the latest data in all plots");
   timeline=new QSlider(Qt::Horizontal,this); timeline->setObjectName("allPlotTimeline");
   timeline->setRange(0,1000); timeline->setValue(1000); timeline->setFixedWidth(120);
   timeline->setAccessibleName("All plots playback position");
   timeline->setToolTip("Shared percentage of each display's retained history; this is not a common epoch"); layout->addWidget(timeline);
   position=new QLabel("Latest",this); position->setObjectName("allPlotReplayPosition");
   position->setMinimumWidth(position->fontMetrics().horizontalAdvance("Independent")+4); layout->addWidget(position);
   mode=new QComboBox(this); mode->setObjectName("allPlotReplayMode"); mode->setAccessibleName("All plots playback mode");
   mode->setToolTip("Show retained propagation samples or interpolate at uniform time steps within each segment");
   mode->addItem("Propagation steps",static_cast<int>(PlotReplayMode::PropagationSteps));
   mode->addItem("Interpolated steps",static_cast<int>(PlotReplayMode::InterpolatedSteps));
   mode->setCurrentIndex(1); layout->addWidget(mode);
   slower=action("allPlotReplaySlower","Slower playback",QStyle::SP_MediaSeekBackward,"Decrease playback speed");
   speed=new QComboBox(this); speed->setObjectName("allPlotReplaySpeed"); speed->setAccessibleName("All plots playback speed");
   speed->setToolTip("Animation speed: 1× sweeps retained history in about 3 seconds; this is not a real-time multiplier");
   for (double rate:plotReplaySpeeds) speed->addItem(QString::number(rate)+QString::fromUtf8("×"),rate);
   speed->setCurrentIndex(speed->findData(1.)); layout->addWidget(speed);
   faster=action("allPlotReplayFaster","Faster playback",QStyle::SP_MediaSeekForward,"Increase playback speed");
   timer=new QTimer(this); timer->setObjectName("allPlotReplayTimer"); timer->setInterval(30);
   connect(mode,&QComboBox::currentIndexChanged,this,[this] {
      receiver.setReplayMode(static_cast<PlotReplayMode>(mode->currentData().toInt()));
      syncFromReceiver();
   });
   connect(slower,&QAction::triggered,this,[this] { speed->setCurrentIndex(std::max(0,speed->currentIndex()-1)); });
   connect(faster,&QAction::triggered,this,[this] { speed->setCurrentIndex(std::min(speed->count()-1,speed->currentIndex()+1)); });
   connect(speed,&QComboBox::currentIndexChanged,this,[this] { updateSpeedButtons(); });
   connect(start,&QAction::triggered,this,[this] { stop(); seek(0); });
   connect(latest,&QAction::triggered,this,[this] { stop(); seek(1000); });
   connect(play,&QAction::toggled,this,[this](bool checked) {
      play->setText(checked ? "Pause" : "Play");
      play->setIcon(standardActionIcon(checked ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay,this));
      if (checked) {
         const double current=receiver.replayPosition().value_or(timeline->value());
         seek(current>=1000 ? 0 : current);
         timer->start();
      } else timer->stop();
   });
   connect(timeline,&QSlider::sliderPressed,this,[this] { stop(); });
   connect(timeline,&QSlider::valueChanged,this,[this](int value) { seek(value); });
   connect(timer,&QTimer::timeout,this,[this] {
      const double current=receiver.replayPosition().value_or(timeline->value());
      const double next=std::min(1000.,current+10*speed->currentData().toDouble());
      seek(next);
      if (next>=1000) stop();
   });
   syncFromReceiver();
}
void PlotPlaybackControls::stop()
{
   timer->stop(); play->setChecked(false);
}
void PlotPlaybackControls::seek(double value)
{
   receiver.setReplayPosition(value);
   syncFromReceiver();
}
void PlotPlaybackControls::updateSpeedButtons()
{
   slower->setEnabled(speed->currentIndex()>0);
   faster->setEnabled(speed->currentIndex()<speed->count()-1);
}
void PlotPlaybackControls::syncFromReceiver()
{
   const QSignalBlocker modeBlock(mode);
   mode->setCurrentIndex(mode->findData(static_cast<int>(receiver.replayMode())));
   updateSpeedButtons();
   const bool available=receiver.hasReplayDisplays();
   setEnabled(available);
   const auto shared=receiver.replayPosition();
   if (!available || !shared) stop();
   const QSignalBlocker block(timeline);
   if (!available) timeline->setValue(1000);
   else if (shared) timeline->setValue(qRound(*shared));
   position->setText(!available || (shared && *shared==1000) ? "Latest" : shared ? QString::number(*shared/10.)+"%" : "Independent");
   position->setToolTip(shared ? "All plot windows follow the linked history position" : "Each view controls its own playback. Use these controls to link all plots.");
}
