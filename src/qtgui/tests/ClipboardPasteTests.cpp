#include "MainWindow.hpp"
#include "MatrixTableWidget.hpp"
#include "ResourceEditor.hpp"
#include "VectorLineEdit.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDir>
#include <QDoubleValidator>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
void events() { QApplication::processEvents(); QTest::qWait(10); }
void key(QWidget *widget,Qt::Key code)
{
   widget->setFocus();
   if (code==Qt::Key_Y) QTest::keySequence(widget,QKeySequence(QKeySequence::Redo));
   else QTest::keyClick(widget,code,Qt::ControlModifier);
   events();
}
QStringList values(const std::array<VectorLineEdit *,3> &fields)
{
   return {fields[0]->text(),fields[1]->text(),fields[2]->text()};
}
QStringList cells(QTableWidget &grid)
{
   QStringList result;
   for (int row=0;row<grid.rowCount();++row) for (int column=0;column<grid.columnCount();++column)
      result.append(grid.item(row,column) ? grid.item(row,column)->text() : QString());
   return result;
}
void contextAction(QWidget *widget,const QString &name)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,widget,[&] {
      auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
      try {
         if (!menu) throw std::runtime_error(("Clipboard context menu did not open for "+name).toStdString());
         QAction *selected=nullptr;
         for (auto *action:menu->actions())
            if (action->objectName()==name || action->text().remove('&').section('\t',0,0)==name) selected=action;
         require(selected && selected->isEnabled(),"Clipboard context action is unavailable");
         QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(selected).center());
      } catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
   });
   auto *scroll=qobject_cast<QAbstractScrollArea *>(widget);
   auto *target=scroll ? scroll->viewport() : widget;
   QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(3,3),target->mapToGlobal(QPoint(3,3)));
   QApplication::sendEvent(target,&event); events();
   if (failure) std::rethrow_exception(failure);
}
void basicPasteChecks()
{
   QWidget host; QHBoxLayout layout(&host);
   std::array<VectorLineEdit *,3> fields;
   for (auto &field:fields) { field=new VectorLineEdit(&host); layout.addWidget(field); }
   VectorLineEdit::link({fields[0],fields[1],fields[2]});
   MatrixTableWidget matrix(3,3,&host); layout.addWidget(&matrix);
   for (int row=0;row<3;++row) for (int column=0;column<3;++column)
      matrix.setItem(row,column,new QTableWidgetItem(QString::number(row*3+column+10)));
   host.show(); require(QTest::qWaitForWindowExposed(&host),"Paste fixture was not exposed");
   const QStringList before{"10","20","30"};
   const auto reset=[&] { for (int index=0;index<3;++index) fields[index]->setText(before[index]); };
   const QStringList inputs{"1, 2, 3","[1 2 3]","(1;2;3)","{1\t2\t3}","1\n2\n3","+1e0, 2.00, -3E-2"};
   for (const auto &input:inputs) {
      reset(); QApplication::clipboard()->setText(input); key(fields[0],Qt::Key_V);
      const QStringList expected=input.startsWith('+') ? QStringList{"+1e0","2.00","-3E-2"} : QStringList{"1","2","3"};
      require(values(fields)==expected,"Delimited vector paste did not fill all components");
      require(QApplication::clipboard()->text()==input,"Vector paste changed the clipboard");
      key(fields[0],Qt::Key_Z); require(values(fields)==before,"One Undo did not restore all vector components");
      key(fields[0],Qt::Key_Y); require(values(fields)==expected,"One Redo did not restore the vector paste");
   }
   reset(); fields[0]->setCursorPosition(2); fields[0]->insert("9");
   QApplication::clipboard()->setText("1 2 3"); key(fields[0],Qt::Key_V);
   key(fields[0],Qt::Key_Z); require(fields[0]->text()=="109","Vector undo lost earlier field history");
   key(fields[0],Qt::Key_Z); require(values(fields)==before,"Earlier ordinary field edit could not be undone");
   reset(); QApplication::clipboard()->setText("1 2 3"); key(fields[0],Qt::Key_V);
   fields[0]->insert("4"); key(fields[0],Qt::Key_Z);
   require(values(fields)==QStringList{"1","2","3"},"Ordinary edit after vector paste merged with the paste");
   key(fields[0],Qt::Key_Z); require(values(fields)==before,"Vector undo failed after an ordinary edit");
   for (const auto &input:QStringList{"5","1 2","1 2 3 4","[1 2 3","1 nan 3","1 inf 3"}) {
      reset(); fields[0]->selectAll(); QApplication::clipboard()->setText(input); key(fields[0],Qt::Key_V);
      require(fields[1]->text()=="20" && fields[2]->text()=="30","Non-vector input changed companion fields");
      key(fields[0],Qt::Key_Z); require(values(fields)==before,"Ordinary paste Undo changed companion fields");
   }
   reset(); fields[2]->setReadOnly(true); QApplication::clipboard()->setText("1 2 3"); key(fields[0],Qt::Key_V);
   require(fields[1]->text()=="20" && fields[2]->text()=="30","Read-only vector accepted a partial group update");
   fields[2]->setReadOnly(false); reset();
   fields[2]->setValidator(new QDoubleValidator(0,10,3,fields[2]));
   QApplication::clipboard()->setText("1 2 300"); key(fields[0],Qt::Key_V);
   require(fields[1]->text()=="20" && fields[2]->text()=="30","Rejected vector component caused a partial group update");
   fields[2]->setValidator(nullptr);
   reset(); QApplication::clipboard()->setText("4,5,6");
   contextAction(fields[0],"edit-paste");
   require(values(fields)==QStringList{"4","5","6"},"Context-menu Paste did not fill the vector");
   contextAction(fields[1],"edit-undo");
   require(values(fields)==before,"Context-menu Undo did not restore the whole vector");

   const auto initial=cells(matrix);
   const QStringList matrices{"1\t2\n3\t4","[1 2; 3 4]","1,2\r\n3,4"};
   for (const auto &input:matrices) {
      matrix.setCurrentCell(1,1); QApplication::clipboard()->setText(input); key(&matrix,Qt::Key_V);
      require(matrix.item(1,1)->text()=="1" && matrix.item(1,2)->text()=="2" &&
              matrix.item(2,1)->text()=="3" && matrix.item(2,2)->text()=="4" && matrix.item(0,0)->text()==initial[0],
              "Matrix paste did not fill the selected rectangular block");
      key(&matrix,Qt::Key_Z); require(cells(matrix)==initial,"One Undo did not restore the whole matrix block");
      key(&matrix,Qt::Key_Y); require(matrix.item(2,2)->text()=="4","Matrix block Redo failed");
      key(&matrix,Qt::Key_Z);
   }
   for (const auto &input:QStringList{"1 2\n3","1 nan\n3 4","1 2 3 4\n5 6 7 8"}) {
      matrix.setCurrentCell(0,0); QApplication::clipboard()->setText(input); key(&matrix,Qt::Key_V);
      require(cells(matrix)==initial,"Invalid or overflowing matrix paste changed cells");
   }
   matrix.setCurrentCell(0,0); matrix.editItem(matrix.item(0,0)); events();
   auto *editor=matrix.findChild<QLineEdit *>(); require(editor,"Matrix cell editor did not open");
   QApplication::clipboard()->setText("5 6;7 8"); key(editor,Qt::Key_V);
   require(matrix.item(0,0)->text()=="5" && matrix.item(1,1)->text()=="8","Paste from an open cell editor did not fill the matrix");
   key(&matrix,Qt::Key_Z); require(cells(matrix)==initial,"Open-cell matrix paste could not be undone");
   QApplication::clipboard()->setText("[5 6;7 8]"); matrix.setCurrentCell(0,0);
   contextAction(&matrix,"Paste");
   require(matrix.item(1,1)->text()=="8","Context-menu Paste did not fill the matrix");
   contextAction(&matrix,"Undo");
   require(cells(matrix)==initial,"Context-menu Undo did not restore the matrix");
   matrix.setRangeSelected(QTableWidgetSelectionRange(0,0,1,1),true); key(&matrix,Qt::Key_C);
   require(QApplication::clipboard()->text()=="10\t11\n13\t14","Matrix Copy did not preserve row and column delimiters");
}
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests"); application.setApplicationName("ClipboardPaste");
   TestSettings settings;
   try {
      require(argc==2,"Supply a startup file");
      basicPasteChecks();
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Paste runtime initialization failed");
      require(QTest::qWaitForWindowExposed(&window),"Paste application was not exposed");
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString baseline="Create Spacecraft Sat;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nCreate Array A[3,3];\nBeginMissionSequence;\n";
      source->setPlainText(baseline); require(window.buildScript(),"Paste fixture did not build");
      auto *tree=window.findChild<QTreeWidget *>("Resources");
      const auto open=[&](const QString &name) {
         const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
         require(items.size()==1,"Paste fixture resource is missing");
         tree->itemDoubleClicked(items.first(),0); events();
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
            if (child->property("resourceName")==name) return dynamic_cast<ResourceEditor *>(child->widget());
         return static_cast<ResourceEditor *>(nullptr);
      };
      auto *panel=open("Sat"); require(panel,"Spacecraft panel did not open");
      std::array<VectorLineEdit *,3> position;
      const QStringList names{"X","Y","Z"};
      for (int index=0;index<3;++index) {
         position[index]=panel->findChild<VectorLineEdit *>("resource_"+names[index]);
         require(position[index],"Spacecraft component input is missing");
      }
      const auto original=values(position);
      position[0]->setFocus(); events(); QApplication::clipboard()->setText("[7100, 25, 50]");
      window.findChild<QAction *>("edit_paste")->trigger(); events();
      require(values(position)==QStringList{"7100","25","50"} && source->toPlainText()==baseline,
              "Spacecraft menu Paste did not stage the vector without changing the mission");
      window.findChild<QAction *>("edit_undo")->trigger(); events();
      require(values(position)==original,"Spacecraft menu Undo did not restore all components");
      window.findChild<QAction *>("edit_redo")->trigger(); events();
      require(values(position)==QStringList{"7100","25","50"},"Spacecraft menu Redo failed");
      window.findChild<QAction *>("edit_undo")->trigger(); events();
      auto *vx=panel->findChild<VectorLineEdit *>("resource_VX");
      auto *vy=panel->findChild<VectorLineEdit *>("resource_VY");
      auto *vz=panel->findChild<VectorLineEdit *>("resource_VZ");
      require(vx && vy && vz,"Velocity component inputs are missing");
      const QStringList velocity{vx->text(),vy->text(),vz->text()};
      QApplication::clipboard()->setText("0.1;7.5;-0.2"); key(vx,Qt::Key_V);
      require(QStringList{vx->text(),vy->text(),vz->text()}==QStringList{"0.1","7.5","-0.2"},"Velocity paste did not fill its components");
      key(vx,Qt::Key_Z); require(QStringList{vx->text(),vy->text(),vz->text()}==velocity,"Velocity paste Undo failed");
      auto *representation=panel->findChild<QComboBox *>("resource_DisplayStateType");
      require(representation,"Orbit representation selector is missing");
      representation->setCurrentText("Keplerian"); events();
      const auto keplerian=values(position);
      QApplication::clipboard()->setText("1 2 3"); key(position[0],Qt::Key_V);
      require(position[1]->text()==keplerian[1] && position[2]->text()==keplerian[2],
              "Non-Cartesian elements were treated as vector components");
      key(position[0],Qt::Key_Z); representation->setCurrentText("Cartesian"); events();

      auto *array=open("A"); require(array,"Array panel did not open");
      for (const auto &target:QList<QPair<ResourceEditor *,QString>>{{array,"RmatValue"},{panel,"OrbitErrorCovariance"}}) {
      auto *properties=target.first->findChild<QTableWidget *>("resourceProperties");
      QTableWidgetItem *matrixValue=nullptr;
      for (int row=0;row<properties->rowCount();++row)
         if (properties->item(row,0)->text()==target.second) matrixValue=properties->item(row,1);
      require(matrixValue,"Numeric matrix value is missing");
      const auto initialMatrix=matrixValue->text(); std::exception_ptr failure; bool visited=false;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=target.first->findChild<QDialog *>("numericGridDialog");
         try {
            require(dialog,"Numeric matrix dialog did not open");
            auto *grid=dynamic_cast<MatrixTableWidget *>(dialog->findChild<QTableWidget *>("numericGrid"));
            require(grid,"Numeric matrix paste support is missing");
            const auto before=cells(*grid); grid->setCurrentCell(0,0);
            QApplication::clipboard()->setText("[1 2;3 4]"); key(grid,Qt::Key_V);
            require(grid->item(0,0)->text()=="1" && grid->item(1,1)->text()=="4","Array matrix paste failed");
            window.findChild<QAction *>("edit_undo")->trigger(); events();
            require(cells(*grid)==before,"Global Undo did not restore the matrix paste");
            visited=true;
         } catch (...) { failure=std::current_exception(); }
         if (dialog) dialog->reject();
      });
      properties->itemDoubleClicked(matrixValue); events();
      if (failure) std::rethrow_exception(failure);
      require(visited && matrixValue->text()==initialMatrix && source->toPlainText()==baseline,"Matrix Cancel changed pending data or source");
      }
      std::cout<<"PASS delimited vector/matrix paste, whole-operation Undo/Redo, ordinary editing history, invalid/overflow isolation, inline and context-menu paste, matrix Copy, spacecraft position/velocity, non-Cartesian guard, array and covariance dialogs\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
