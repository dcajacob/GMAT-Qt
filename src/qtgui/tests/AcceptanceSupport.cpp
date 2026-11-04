// Opt-in instrumentation linked into test executables, never the application.
// Observations are not assertions: programmatic changes are identified as such.
#include "AcceptanceView.hpp"
#include <QApplication>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QImage>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSet>
#include <QScreen>
#include <QPixmap>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <QWindow>
#include <iostream>

namespace {
class AcceptanceObserver final : public QObject
{
public:
   explicit AcceptanceObserver(const QString &directory) : root(directory)
   {
      QDir().mkpath(root+"/screenshots");
      trace.setFileName(root+QString("/trace-%1.jsonl").arg(QCoreApplication::applicationPid()));
      if (!trace.open(QIODevice::WriteOnly|QIODevice::Append)) qFatal("Cannot open GUI acceptance trace");
      visible=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible";
      pause=qBound(0,qEnvironmentVariableIntValue("GMAT_QT_ACCEPTANCE_PAUSE_MS"),5000);
      limit=qBound(0,qEnvironmentVariableIntValue("GMAT_QT_ACCEPTANCE_MAX_CHECKPOINTS"),5000);
      images=qBound(0,qEnvironmentVariableIntValue("GMAT_QT_ACCEPTANCE_MAX_SCREENSHOTS"),1000);
      emitRecord(nullptr,"start");
   }
   ~AcceptanceObserver() override
   {
      emitRecord(nullptr,"summary",{{"observed_objects",observed},{"shown_events",shown},
         {"activation_events",activated},{"change_events",changed},{"screenshots",saved},
         {"captured_frames",captured},{"reused_frames",reused}});
   }
   bool eventFilter(QObject *object,QEvent *event) override
   {
      if (busy) return false;
      if (event->type()==AcceptanceViewBoundary::eventType()) {
         auto *target=static_cast<AcceptanceViewBoundary *>(event)->widget.data();
         if (target) {
            emitRecord(target,"viewing_boundary",{{"visible",target->isVisible()}});
            checkpoint(target,"explicit viewing boundary");
         }
         return false;
      }
      if (auto *widget=qobject_cast<QWidget *>(object)) {
         if (event->type()==QEvent::Polish || event->type()==QEvent::Show) observe(widget);
         if (event->type()==QEvent::Show) {
            ++shown; emitRecord(widget,"shown");
            for (auto *child:widget->findChildren<QWidget *>()) observe(child);
            for (auto *action:widget->findChildren<QAction *>()) observe(action);
            if (widget->isWindow() || widget->inherits("QMdiSubWindow")) checkpoint(widget,"window shown");
         }
         if (event->type()==QEvent::MouseButtonRelease || event->type()==QEvent::KeyRelease)
            emitRecord(widget,"input",{{"spontaneous",event->spontaneous()}});
      } else if (auto *window=qobject_cast<QWindow *>(object);window && event->type()==QEvent::Expose) {
         emitRecord(window,"native_exposure",{{"exposed",window->isExposed()}});
         if (visible && window->isExposed()) {
            for (auto *widget:QApplication::topLevelWidgets()) if (widget->windowHandle()==window) {
               // Let the exposure handler paint before taking a viewing pause.
               // Pausing inside the filter can otherwise hold an unpainted pane.
               QPointer<QWidget> guarded=widget;
               QTimer::singleShot(0,this,[this,guarded] {
                  if (guarded && guarded->isVisible()) checkpoint(guarded,"native window exposed");
               });
               break;
            }
         }
      }
      return false;
   }
private:
   QString root;
   QFile trace;
   QSet<QObject *> connected;
   QImage previousPixels;
   qreal previousScale=1;
   QString previousCapture;
   bool visible=false,busy=false;
   int pause=0,limit=0,images=0,checkpoints=0,saved=0,captured=0,reused=0,observed=0,shown=0,activated=0,changed=0;
   QString key(QObject *object) const
   {
      QStringList parts;
      for (auto *p=object;p && parts.size()<12;p=p->parent()) {
         const auto name=p->objectName();
         parts.prepend(QString::fromLatin1(p->metaObject()->className())+(name.isEmpty() ? QString() : ":"+name));
      }
      return parts.join('/');
   }
   void emitRecord(QObject *object,const char *kind,QJsonObject extra={})
   {
      extra["event"]=QString::fromLatin1(kind);
      extra["utc"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
      extra["test"]=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_TEST");
      if (object) {
         extra["key"]=key(object); extra["name"]=object->objectName();
         extra["class"]=QString::fromLatin1(object->metaObject()->className());
      }
      trace.write(QJsonDocument(extra).toJson(QJsonDocument::Compact)+'\n'); trace.flush();
   }
   void checkpoint(QObject *object,const char *reason)
   {
      if (!visible || busy || checkpoints>=limit) return;
      auto *widget=qobject_cast<QWidget *>(object);
      if (!widget && object) widget=qobject_cast<QWidget *>(object->parent());
      if (!widget || !widget->isVisible()) return;
      QPointer<QWidget> window=widget->window();
      if (!window || window->size().isEmpty()) return;
      bool openGL=window->inherits("QOpenGLWidget");
      for (auto *child:window->findChildren<QWidget *>()) openGL=openGL || child->inherits("QOpenGLWidget");
      const bool exposed=window->windowHandle() && window->windowHandle()->isExposed();
      // QWidget::grab/repaint re-enters OpenGL painting and can disturb the
      // renderer being checked. Read its native surface only after exposure.
      if (openGL && !exposed) return;
      busy=true; ++checkpoints;
      // No nested event loop or timer dispatch: tests own their modal timers.
      if (!openGL) window->repaint();
      QJsonObject extra={{"reason",QString::fromLatin1(reason)},
         {"platform",QGuiApplication::platformName()},
         {"exposed",exposed}};
      if (captured<images) {
         const auto pixels=exposed && window->screen() ? window->screen()->grabWindow(window->winId()) : window->grab();
         const auto rgba=pixels.toImage().convertToFormat(QImage::Format_RGBA8888);
         if (rgba.isNull()) extra["capture_error"]="QWidget grab failed";
         else {
            // Capture every live surface. Exact pixel equality lets adjacent
            // unchanged checkpoints share a PNG without another encoding pass.
            if (!previousCapture.isEmpty() && rgba==previousPixels && pixels.devicePixelRatio()==previousScale) {
               ++captured; ++reused; extra["screenshot"]=previousCapture; extra["screenshot_reused"]=true;
            } else {
               const auto relative=QString("screenshots/%1-%2.png").arg(QCoreApplication::applicationPid()).arg(saved+1,3,10,QChar('0'));
               if (pixels.save(root+'/'+relative)) {
                  ++captured; ++saved; previousPixels=rgba; previousScale=pixels.devicePixelRatio();
                  previousCapture=relative; extra["screenshot"]=relative;
               } else extra["capture_error"]="QWidget grab failed";
            }
         }
      }
      emitRecord(object,"checkpoint",extra);
      std::cout<<"ACCEPTANCE: "<<reason<<" "<<key(object).toStdString()<<std::endl;
      if (pause && exposed) QThread::msleep(static_cast<unsigned long>(pause));
      busy=false;
   }
   void change(QObject *object) { ++changed; emitRecord(object,"changed"); }
   void activate(QObject *object) { ++activated; emitRecord(object,"activated"); checkpoint(object,"control activated"); }
   void observe(QObject *object)
   {
      if (connected.contains(object)) return;
      connected.insert(object); ++observed; emitRecord(object,"observed");
      connect(object,&QObject::destroyed,this,[this,object] { connected.remove(object); });
      if (auto *button=qobject_cast<QAbstractButton *>(object)) {
         connect(button,&QAbstractButton::clicked,this,[this,button] { activate(button); });
         connect(button,&QAbstractButton::toggled,this,[this,button] { change(button); });
      }
      if (auto *action=qobject_cast<QAction *>(object))
         connect(action,&QAction::triggered,this,[this,action] { activate(action); });
      if (auto *edit=qobject_cast<QLineEdit *>(object))
         connect(edit,&QLineEdit::textChanged,this,[this,edit] { change(edit); });
      if (auto *edit=qobject_cast<QPlainTextEdit *>(object))
         connect(edit,&QPlainTextEdit::textChanged,this,[this,edit] { change(edit); });
      if (auto *combo=qobject_cast<QComboBox *>(object))
         connect(combo,&QComboBox::currentTextChanged,this,[this,combo] { change(combo); });
      if (auto *spin=qobject_cast<QSpinBox *>(object))
         connect(spin,&QSpinBox::valueChanged,this,[this,spin] { change(spin); });
      if (auto *spin=qobject_cast<QDoubleSpinBox *>(object))
         connect(spin,&QDoubleSpinBox::valueChanged,this,[this,spin] { change(spin); });
      if (auto *slider=qobject_cast<QAbstractSlider *>(object))
         connect(slider,&QAbstractSlider::valueChanged,this,[this,slider] { change(slider); });
      if (auto *tabs=qobject_cast<QTabWidget *>(object))
         connect(tabs,&QTabWidget::currentChanged,this,[this,tabs] { change(tabs); });
      if (auto *tree=qobject_cast<QTreeWidget *>(object)) {
         connect(tree,&QTreeWidget::itemActivated,this,[this,tree] { activate(tree); });
         connect(tree,&QTreeWidget::itemDoubleClicked,this,[this,tree] { activate(tree); });
      }
   }
};
AcceptanceObserver *observer=nullptr;
void finishAcceptance() { delete observer; observer=nullptr; }
void startAcceptance()
{
   const auto root=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   if (root.isEmpty()) return;
   // These tests automate QFileDialog widgets. Offscreen Qt already uses them;
   // preserve that same driver contract in the visible test desktop.
   if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible")
      QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   observer=new AcceptanceObserver(root);
   QCoreApplication::instance()->installEventFilter(observer);
   qAddPostRoutine(finishAcceptance);
}
}
Q_COREAPP_STARTUP_FUNCTION(startAcceptance)
