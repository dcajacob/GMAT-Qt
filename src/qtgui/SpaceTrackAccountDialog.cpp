#include "SpaceTrackAccountDialog.hpp"
#include "SpaceTrackAccount.hpp"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SpaceTrackAccountDialog::SpaceTrackAccountDialog(SpaceTrackAccount &account,QWidget *parent) : QDialog(parent)
{
   setObjectName("spaceTrackAccountDialog");setWindowTitle("Space-Track account");resize(500,240);
   auto *layout=new QVBoxLayout(this);
   auto *form=new QFormLayout;
   auto *identity=new QLineEdit(this);identity->setObjectName("savedSpaceTrackIdentity");
   auto *password=new QLineEdit(this);password->setObjectName("savedSpaceTrackPassword");password->setEchoMode(QLineEdit::Password);
   form->addRow("Space-Track username",identity);form->addRow("Password",password);layout->addLayout(form);
   auto *note=new QLabel("Save securely uses your operating system's credential store. Use for this session keeps the account available until GMAT closes.",this);note->setWordWrap(true);layout->addWidget(note);
   auto *status=new QLabel(this);status->setObjectName("spaceTrackAccountStatus");status->setWordWrap(true);layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this);
   auto *save=buttons->addButton("Save securely",QDialogButtonBox::ActionRole);save->setObjectName("saveSpaceTrackAccount");
   auto *session=buttons->addButton("Use for this session",QDialogButtonBox::ActionRole);session->setObjectName("sessionSpaceTrackAccount");
   auto *forget=buttons->addButton("Forget saved account",QDialogButtonBox::ActionRole);forget->setObjectName("forgetSpaceTrackAccount");
   layout->addWidget(buttons);
   const auto update=[=] {
      const bool complete=identity->isEnabled() && !identity->text().trimmed().isEmpty() && !password->text().isEmpty();
      save->setEnabled(complete);session->setEnabled(complete);
   };
   connect(identity,&QLineEdit::textChanged,this,update);connect(password,&QLineEdit::textChanged,this,update);update();
   account.load(this,[=,&account](const QString &error) {
      if (identity->text().isEmpty() && password->text().isEmpty()) {
         const auto current=account.credentials();identity->setText(current.identity);password->setText(current.password);
      }
      status->setText(error);
   });
   connect(session,&QPushButton::clicked,this,[=,&account] { account.useForSession({identity->text().trimmed(),password->text()});accept(); });
   connect(save,&QPushButton::clicked,this,[=,&account] {
      identity->setEnabled(false);password->setEnabled(false);
      save->setEnabled(false);session->setEnabled(false);forget->setEnabled(false);status->setText("Saving in the credential store…");
      account.save({identity->text().trimmed(),password->text()},this,[=](const QString &error) {
         if (error.isEmpty()) { accept();return; }
         status->setText(error);identity->setEnabled(true);password->setEnabled(true);forget->setEnabled(true);update();
      });
   });
   connect(forget,&QPushButton::clicked,this,[=,&account] {
      identity->setEnabled(false);password->setEnabled(false);
      save->setEnabled(false);session->setEnabled(false);forget->setEnabled(false);status->setText("Removing saved account…");
      account.forget(this,[=](const QString &error) {
         if (error.isEmpty()) { accept();return; }
         status->setText(error);
         identity->setEnabled(true);password->setEnabled(true);forget->setEnabled(true);update();
      });
   });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
}
