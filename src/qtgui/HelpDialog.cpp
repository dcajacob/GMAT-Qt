#include "HelpDialog.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QStringDecoder>
#include <QTextBrowser>
#include <QVBoxLayout>

// Keep script attachments in the browser's URL history while rendering their
// literal contents rather than interpreting them as HTML.
class HelpBrowser final : public QTextBrowser
{
public:
   using QTextBrowser::QTextBrowser;
   QVariant loadResource(int type,const QUrl &url) override
   {
      const auto suffix=QFileInfo(url.path()).suffix().toLower();
      if (type==QTextDocument::HtmlResource && url.isLocalFile() && (suffix=="script" || suffix=="gmf")) {
         QFile file(url.toLocalFile());
         if (!file.open(QIODevice::ReadOnly)) return {};
         const auto bytes=file.readAll();
         QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);
         const QString contents=decoder(bytes);
         if (file.error()!=QFileDevice::NoError || decoder.hasError()) return {};
         // A span prevents Qt from discarding a boundary newline inside <pre>.
         return QString("<html><head><meta charset=\"utf-8\"></head><body><pre><span>"+
                        contents.toHtmlEscaped()+"</span></pre></body></html>");
      }
      return QTextBrowser::loadResource(type,url);
   }
};

static QUrl helpUrl(const QString &topic)
{
   QSettings settings;
   const auto override=settings.value("Help/"+topic).toString();
   if (!override.isEmpty()) {
      const QUrl url(override);
      if (!url.scheme().isEmpty()) return url;
      return QUrl::fromLocalFile(QFileInfo(override).absoluteFilePath());
   }
   // Topics are engine type names, never paths supplied by the editor.
   if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(topic).hasMatch()) return {};
   try {
      const auto path=QString::fromStdString(FileManager::Instance()->GetFullPathname("HELP_PATH"));
      static const QMap<QString,QString> aliases={{"ChemicalTank","FuelTank"},{"ChemicalThruster","Thruster"},
         {"PropSetup","Propagator"},{"ODEModel","ForceModel"},{"Equation","Assignment"},{"GMAT","Assignment"},
         {"Planet","CelestialBody"},{"Moon","CelestialBody"},{"Asteroid","CelestialBody"},{"Comet","CelestialBody"},{"Star","CelestialBody"},
         {"CustomFOV","FieldOfView"},{"ConicalFOV","FieldOfView"},{"RectangularFOV","FieldOfView"}};
      return QUrl::fromLocalFile(QDir(path).filePath("html/"+aliases.value(topic,topic)+".html"));
   } catch (BaseException &) { return {}; }
}

HelpDialog::HelpDialog(const QString &topic,QWidget *parent) : QDialog(parent)
{
   setObjectName("helpDialog"); resize(920,700);
   auto *layout=new QVBoxLayout(this);
   auto *navigation=new QHBoxLayout;
   auto button=[&](const QString &text,const QString &name,auto callback) {
      auto *value=new QPushButton(text,this); value->setObjectName(name);
      navigation->addWidget(value); connect(value,&QPushButton::clicked,this,callback); return value;
   };
   auto *back=button("Back","helpBack",[this] { browser->backward(); });
   auto *forward=button("Forward","helpForward",[this] { browser->forward(); });
   button("Topic","helpHome",[this] { load(home); });
   button("Contents","helpContents",[this] { load(helpUrl("index")); });
   button("Retry","helpRetry",[this] { load(requested); });
   button("Open in browser","helpOpenBrowser",[this] {
      if (!requested.isEmpty() && !QDesktopServices::openUrl(requested)) status->setText("Cannot open this help page in the browser.");
   });
   navigation->addStretch(); layout->addLayout(navigation);
   browser=new HelpBrowser(this); browser->setObjectName("helpBrowser");
   browser->setOpenLinks(false); browser->setOpenExternalLinks(false); layout->addWidget(browser,1);
   connect(browser,&QTextBrowser::backwardAvailable,back,&QPushButton::setEnabled);
   connect(browser,&QTextBrowser::forwardAvailable,forward,&QPushButton::setEnabled);
   back->setEnabled(false); forward->setEnabled(false);
   connect(browser,&QTextBrowser::anchorClicked,this,[this](const QUrl &url) { load(browser->source().resolved(url)); });
   status=new QLabel(this); status->setObjectName("helpStatus"); status->setWordWrap(true); layout->addWidget(status);
   connect(browser,&QTextBrowser::sourceChanged,this,[this](const QUrl &url) { requested=url; status->clear(); });
   auto *findRow=new QHBoxLayout;
   findRow->addWidget(new QLabel("Find",this)); search=new QLineEdit(this); search->setObjectName("helpFindText"); findRow->addWidget(search,1);
   auto *previous=new QPushButton("Previous",this),*next=new QPushButton("Next",this);
   previous->setObjectName("helpFindPrevious"); next->setObjectName("helpFindNext"); findRow->addWidget(previous); findRow->addWidget(next); layout->addLayout(findRow);
   connect(search,&QLineEdit::returnPressed,this,[this] { find(); });
   connect(previous,&QPushButton::clicked,this,[this] { find(true); }); connect(next,&QPushButton::clicked,this,[this] { find(); });
   auto shortcut=[this](const QKeySequence &key,auto callback) { auto *value=new QShortcut(key,this); connect(value,&QShortcut::activated,this,callback); };
   shortcut(QKeySequence::Find,[this] { search->setFocus(); search->selectAll(); });
   shortcut(QKeySequence::FindNext,[this] { find(); }); shortcut(QKeySequence::FindPrevious,[this] { find(true); });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   // Return in Find must not also activate a navigation or Close button.
   for (auto *value:findChildren<QPushButton *>()) {
      value->setAutoDefault(false); value->setDefault(false);
   }
   openTopic(topic);
}
void HelpDialog::openTopic(const QString &topic)
{
   setWindowTitle("GMAT Help — "+topic); home=helpUrl(topic); load(home);
}
void HelpDialog::load(const QUrl &url)
{
   requested=url;
   if (url.scheme()=="https" || url.scheme()=="http") {
      if (!QDesktopServices::openUrl(url)) status->setText("Cannot open this help link: "+url.toString());
      return;
   }
   if (!url.isLocalFile()) { status->setText("No local help page is configured for this topic."); return; }
   QFile file(url.toLocalFile());
   if (!file.open(QIODevice::ReadOnly)) { status->setText("Cannot read help page: "+file.fileName()+"\n"+file.errorString()+"\nRestore the help files and select Retry, or open Contents."); return; }
   const auto bytes=file.readAll(); QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);
   const QString contents=decoder(bytes);
   if (file.error()!=QFileDevice::NoError || decoder.hasError()) { status->setText("Cannot read this help page as UTF-8: "+file.fileName()); return; }
   browser->setSource(url); status->clear();
}
void HelpDialog::find(bool backwards)
{
   if (search->text().isEmpty()) return;
   const auto flags=backwards ? QTextDocument::FindBackward : QTextDocument::FindFlags();
   bool found=browser->find(search->text(),flags);
   if (!found) {
      auto cursor=browser->textCursor(); cursor.movePosition(backwards ? QTextCursor::End : QTextCursor::Start);
      browser->setTextCursor(cursor); found=browser->find(search->text(),flags);
   }
   status->setText(found ? QString() : "Text not found in this page.");
}
HelpController::HelpController(QWidget *widget) : QObject(widget),owner(widget) { qApp->installEventFilter(this); }
QString HelpController::topic(QWidget *widget) const
{
   for (auto *current=widget;current;current=current->parentWidget()) {
      const auto value=current->property("helpTopic").toString(); if (!value.isEmpty()) return value;
   }
   return "index";
}
void HelpController::attach(QWidget *widget,const QString &value)
{
   widget->setProperty("helpTopic",value);
   auto *buttons=widget->findChild<QDialogButtonBox *>();
   if (!buttons || buttons->button(QDialogButtonBox::Help)) return;
   auto *help=buttons->addButton(QDialogButtonBox::Help); help->setObjectName("contextHelp"); help->setToolTip("Help for this editor (F1)");
   connect(help,&QPushButton::clicked,this,[this,widget] { show(topic(widget)); });
}
void HelpController::show(const QString &value)
{
   // A modal editor must own its Help dialog so desktop modality permits focus.
   auto *parent=QApplication::activeModalWidget(); if (!parent) parent=owner;
   if (dialog && dialog->parentWidget()!=parent) { delete dialog; }
   if (!dialog) { dialog=new HelpDialog(value,parent); dialog->setAttribute(Qt::WA_DeleteOnClose); }
   else dialog->openTopic(value);
   dialog->show(); dialog->raise(); dialog->activateWindow();
}
bool HelpController::eventFilter(QObject *object,QEvent *event)
{
   auto *widget=qobject_cast<QWidget *>(object);
   if (!widget || !owner) return false;
   // QWidget::isAncestorOf stops at a window boundary. Modal editors are
   // parented top-level windows, so follow their ownership chain explicitly.
   bool owned=false;
   for (auto *current=widget;current;current=current->parentWidget()) if (current==owner) { owned=true; break; }
   if (!owned) return false;
   if (event->type()==QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key()==Qt::Key_F1) {
      show(topic(widget)); return true;
   }
   if (event->type()==QEvent::Show && qobject_cast<QDialog *>(widget) && !qobject_cast<QMessageBox *>(widget) &&
       !qobject_cast<QFileDialog *>(widget) && !qobject_cast<QColorDialog *>(widget) && !qobject_cast<QFontDialog *>(widget) &&
       widget->objectName()!="helpDialog" && !widget->property("helpTopic").isValid()) {
      static const QMap<QString,QString> topics={{"ballisticsMassDialog","SpacecraftBallisticMass"},
         {"attitudeDialog","SpacecraftAttitude"},{"visualModelDialog","SpacecraftVisualizationProperties"},
         {"propagationStopsDialog","Propagate"},{"propagationGroupsDialog","Propagate"},
         {"scriptEventDialog","BeginScript"}};
      const auto value=topics.value(widget->objectName(),topic(widget)); if (value!="index") attach(widget,value);
   }
   return false;
}
