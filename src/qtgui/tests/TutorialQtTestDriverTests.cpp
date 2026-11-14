#include "TutorialQtTestDriver.hpp"
#include <QtTest/QTest>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc,char **argv)
{
   QApplication app(argc,argv); QTemporaryDir directory;
   QMainWindow root; auto *central=new QWidget; auto *layout=new QVBoxLayout(central);
   auto *area=new QMdiArea; layout->addWidget(area);
   int first=0,second=0,context=0; QString typed,chosen;
   auto panel=[&](int &counter) {
      auto *widget=new QWidget; auto *box=new QVBoxLayout(widget);
      auto *apply=new QPushButton("Apply"); box->addWidget(apply);
      QObject::connect(apply,&QPushButton::clicked,&root,[&counter] { ++counter; });
      return area->addSubWindow(widget);
   };
   auto *a=panel(first),*b=panel(second); a->setWindowTitle("A"); b->setWindowTitle("B");
   auto *switcher=new QPushButton("Move window and switch panel"); switcher->setObjectName("switchPanel"); layout->addWidget(switcher);
   QObject::connect(switcher,&QPushButton::clicked,&root,[&] { root.move(270,140); root.resize(980,760); area->setActiveSubWindow(a); });
   auto *open=new QPushButton("Open dialog"); open->setObjectName("openDialog"); layout->addWidget(open);
   QObject::connect(open,&QPushButton::clicked,&root,[&] {
      QDialog dialog(&root); auto *box=new QVBoxLayout(&dialog);
      auto *input=new QLineEdit("old"); input->setObjectName("dialogInput"); box->addWidget(input);
      auto *options=new QComboBox; options->setObjectName("dialogOptions"); options->addItems({"Third","First","Second"}); box->addWidget(options);
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok); box->addWidget(buttons);
      QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
      if (dialog.exec()==QDialog::Accepted) { typed=input->text(); chosen=options->currentText(); }
   });
   auto *readonly=new QLineEdit("protected"); readonly->setObjectName("readonly"); readonly->setReadOnly(true); layout->addWidget(readonly);
   auto *wide=new QCheckBox("Wide checkbox"); wide->setObjectName("wideCheck"); wide->setMinimumWidth(600); layout->addWidget(wide);
   auto *editableChoice=new QComboBox; editableChoice->setObjectName("editableChoice"); editableChoice->setEditable(true);
   editableChoice->addItem("old"); layout->addWidget(editableChoice);
   auto *tree=new QTreeWidget; tree->setHeaderHidden(true); tree->setObjectName("fixtureTree");
   new QTreeWidgetItem(tree,{"Resource 1"}); new QTreeWidgetItem(tree,{"Resource 2"});
   qobject_cast<QVBoxLayout *>(a->widget()->layout())->addWidget(tree);
   tree->setContextMenuPolicy(Qt::CustomContextMenu);
   QObject::connect(tree,&QWidget::customContextMenuRequested,&root,[&](const QPoint &point) {
      QMenu menu(tree); auto *inspect=menu.addAction("Inspect resource");
      QObject::connect(inspect,&QAction::triggered,&root,[&] { ++context; }); menu.exec(tree->viewport()->mapToGlobal(point));
   });
   root.setCentralWidget(central); root.resize(800,620); root.show(); a->show(); b->show(); area->tileSubWindows(); area->setActiveSubWindow(b);
   TutorialQtTestDriver driver(root,directory.path());
   QString failure;
   std::thread producer([&] {
      try {
         int sequence=0;
         const auto send=[&](const QJsonObject &action,bool expected=true,bool rejected=false) {
            const auto id=QString::number(++sequence); QSaveFile file(directory.path()+"/input-request.json");
            if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error("Cannot write input request");
            file.write(QJsonDocument(QJsonObject{{"id",id},{"action",action}}).toJson()); file.commit();
            const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(8);
            while (std::chrono::steady_clock::now()<until) {
               for (const auto &kind:{"input-finished/","input-suspended/"}) {
                  QFile receipt(directory.path()+"/"+kind+id+".json");
                  if (receipt.open(QIODevice::ReadOnly)) {
                     const auto result=QJsonDocument::fromJson(receipt.readAll()).object();
                     if (result.contains("ok") && result["ok"].toBool()!=expected) throw std::runtime_error(QJsonDocument(result).toJson().toStdString());
                     return;
                  }
               }
               if (rejected) {
                  QFile pending(directory.path()+"/input-pending.json");
                  if (pending.open(QIODevice::ReadOnly)) {
                     const auto result=QJsonDocument::fromJson(pending.readAll()).object();
                     if (result["id"]==id && result["error"].toString().contains("coordinate")) return;
                  }
               }
               std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            throw std::runtime_error("QtTest driver contract timed out");
         };
         send({{"action","click"},{"target_text","Apply"},{"target_role","button"}});
         send({{"action","check"},{"target_name","wideCheck"},{"checked",true}});
         send({{"action","key"},{"target_name","editableChoice"},{"keys","Ctrl+A"}});
         send({{"action","text"},{"target_name","editableChoice"},{"text","Sat.Earth.Altitude"}});
         send({{"action","click"},{"target_name","switchPanel"}});
         send({{"action","click"},{"target_text","Apply"},{"target_role","button"}});
         send({{"action","click"},{"target_name","openDialog"}});
         send({{"action","key"},{"target_name","dialogInput"},{"keys","Ctrl+A"}});
         send({{"action","text"},{"target_name","dialogInput"},{"text","typed by QtTest"}});
         send({{"action","select"},{"target_name","dialogOptions"},{"value","Second"}});
         send({{"action","click"},{"target_text","OK"},{"target_role","button"}});
         send({{"action","click"},{"x",400},{"y",300}},false,true);
         send({{"action","text"},{"target_name","readonly"},{"text","must not change"}},false);
         send({{"action","click"},{"target_text","Resource 2"},{"target_role","tree"},{"button",3}});
         send({{"action","click"},{"target_text","Inspect resource"},{"target_role","menu"}});
      } catch (const std::exception &error) { failure=error.what(); }
      QMetaObject::invokeMethod(&app,&QCoreApplication::quit,Qt::QueuedConnection);
   });
   app.exec(); producer.join();
   if (!failure.isEmpty() || first!=1 || second!=1 || context!=1 || typed!="typed by QtTest" || chosen!="Second" || readonly->text()!="protected" || !wide->isChecked() || editableChoice->currentText()!="Sat.Earth.Altitude") {
      std::cerr<<"QtTest semantic driver failed: "<<failure.toStdString()<<" A="<<first<<" B="<<second<<" context="<<context<<" typed="<<typed.toStdString()<<" choice="<<chosen.toStdString()<<" editable="<<editableChoice->currentText().toStdString()<<" wide="<<wide->isChecked()<<'\n'; return 1;
   }
   std::cout<<"QtTest semantic input, moved windows, active-panel scoping, modal input, combo choices, context menus, coordinate rejection and read-only protection passed.\n";
   return 0;
}
