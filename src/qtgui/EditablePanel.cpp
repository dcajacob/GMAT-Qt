#include "EditablePanel.hpp"
#include <QCloseEvent>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPointer>
#include <QPushButton>
#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTableWidget>

void EditablePanel::addOkCancelActions(QDialogButtonBox &buttons)
{
   auto *apply=buttons.button(QDialogButtonBox::Apply);
   if (!apply || buttons.button(QDialogButtonBox::Ok) || buttons.button(QDialogButtonBox::Cancel)) return;
   auto *ok=buttons.addButton(QDialogButtonBox::Ok);
   auto *cancel=buttons.addButton(QDialogButtonBox::Cancel);
   completionButtons=&buttons;
   ok->setObjectName("okPanel"); cancel->setObjectName("cancelPanel");
   ok->setDefault(true);
   cancel->setText("Discard changes");
   ok->setToolTip("Apply valid changes and close");
   cancel->setToolTip("Discard unapplied changes and close");
   if (auto *close=buttons.button(QDialogButtonBox::Close)) close->hide();
   completionStatus=new QLabel(this);
   completionStatus->setObjectName("panelCompletionStatus");
   completionStatus->setWordWrap(true);
   if (auto *box=qobject_cast<QBoxLayout *>(layout())) box->insertWidget(box->indexOf(&buttons),completionStatus);
   const auto refresh=[this] { refreshCompletionActions(); };
   for (auto *input:findChildren<QLineEdit *>()) connect(input,&QLineEdit::textChanged,this,refresh);
   for (auto *input:findChildren<QComboBox *>()) connect(input,&QComboBox::currentTextChanged,this,refresh);
   for (auto *input:findChildren<QAbstractButton *>()) connect(input,&QAbstractButton::toggled,this,refresh);
   for (auto *input:findChildren<QPlainTextEdit *>()) connect(input,&QPlainTextEdit::textChanged,this,refresh);
   for (auto *input:findChildren<QTableWidget *>()) connect(input,&QTableWidget::itemChanged,this,refresh);
   connect(ok,&QPushButton::clicked,this,[this,apply] {
      QPointer<QMdiSubWindow> owner=qobject_cast<QMdiSubWindow *>(parentWidget());
      if (!owner) return;
      if (!hasUnappliedInput()) { owner->close(); return; }
      // A successful Apply may replace this widget. The success callback
      // captures its owning window before the replacement and closes that one.
      QPointer<EditablePanel> guarded=this;
      closeAfterApply=true;
      apply->click();
      if (guarded) {
         guarded->closeAfterApply=false;
         // A valid no-op can clear dirty input without replacing the panel or
         // invoking the commit callback. OK still closes its original owner.
         if (owner && !guarded->hasUnappliedInput()) owner->close();
      }
   });
   connect(cancel,&QPushButton::clicked,this,[this] {
      auto *owner=qobject_cast<QMdiSubWindow *>(parentWidget());
      if (!owner) return;
      discardChanges();
      owner->close();
   });
   refreshCompletionActions();
}

void EditablePanel::refreshCompletionActions()
{
   if (!completionButtons) return;
   const bool pending=hasUnappliedInput();
   completionButtons->button(QDialogButtonBox::Ok)->setText(pending ? "Apply and close" : "Close");
   completionButtons->button(QDialogButtonBox::Cancel)->setVisible(pending);
   completionStatus->setText(pending ? "Unapplied changes" : "Applied to the mission script. Save the script to update its file.");
}

void EditablePanel::appliedSuccessfully()
{
   // Copy everything needed before calling a refresh that can retire this panel.
   const auto callback=onApplied;
   QPointer<QWidget> owner=parentWidget();
   const bool shouldClose=closeAfterApply;
   if (callback) callback();
   if ((!callback || shouldClose) && owner) owner->close();
}
void EditorSubWindow::closeEvent(QCloseEvent *event)
{
   const auto *panel=dynamic_cast<EditablePanel *>(widget());
   if (panel && panel->hasUnappliedInput() && QMessageBox::question(this,"Unapplied changes",
       "Discard the unapplied changes in this panel?",QMessageBox::Discard|QMessageBox::Cancel,
       QMessageBox::Cancel)!=QMessageBox::Discard) {
      event->ignore(); return;
   }
   QMdiSubWindow::closeEvent(event);
}
