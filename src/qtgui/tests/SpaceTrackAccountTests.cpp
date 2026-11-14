#include "SpaceTrackAccount.hpp"
#include "SpaceTrackAccountDialog.hpp"
#include "SatelliteImportDialog.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTest>
#include <QTimer>
#include <QUuid>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

void sessionTest()
{
   const SpaceTrackAccount::Credentials expected{"account@example.invalid","session-secret-fixture"};
   SpaceTrackAccount account;
   account.useForSession({});
   {
      SpaceTrackAccountDialog dialog(account);
      dialog.show();
      auto *identity=dialog.findChild<QLineEdit *>("savedSpaceTrackIdentity");
      auto *password=dialog.findChild<QLineEdit *>("savedSpaceTrackPassword");
      auto *session=dialog.findChild<QPushButton *>("sessionSpaceTrackAccount");
      require(password->echoMode()==QLineEdit::Password,"Account password is visible");
      require(!session->isEnabled(),"Empty account is accepted");
      identity->setText(expected.identity);password->setText(expected.password);
      session->click();
      require(dialog.result()==QDialog::Accepted,"Session account dialog did not accept");
      require(account.credentials().password==expected.password,"Session account was not retained");
   }
   {
      SpaceTrackAccountDialog dialog(account);
      dialog.show();
      require(QTest::qWaitFor([&] {
         return dialog.findChild<QLineEdit *>("savedSpaceTrackIdentity")->text()==expected.identity;
      }),"Configured account did not reopen");
      dialog.findChild<QLineEdit *>("savedSpaceTrackPassword")->setText("cancelled-edit");
      dialog.reject();
      require(account.credentials().password==expected.password,"Cancel changed the account");
   }
   SpaceTrackAccount::instance().useForSession(expected);
   {
      SatelliteImportDialog dialog({"EarthMJ2000Eq"},{"Earth"},[](const auto &,const auto &) { return QString(); });
      dialog.show();
      dialog.findChild<QComboBox *>("satelliteCatalogSource")->setCurrentIndex(1);
      require(QTest::qWaitFor([&] {
         return dialog.findChild<QLineEdit *>("spaceTrackPassword")->text()==expected.password;
      }),"Historical importer did not use the configured account");
      require(dialog.findChild<QLineEdit *>("spaceTrackIdentity")->text()==expected.identity,"Importer account differs");
      auto *setup=dialog.findChild<QPushButton *>("configureSpaceTrackAccount");
      require(setup && setup->isVisible(),"Importer account setup is missing");
      QTimer::singleShot(0,[] {
         auto *accountDialog=QApplication::activeModalWidget();
         require(accountDialog && accountDialog->objectName()=="spaceTrackAccountDialog","Account setup did not open");
         accountDialog->findChild<QLineEdit *>("savedSpaceTrackIdentity")->setText("updated@example.invalid");
         accountDialog->findChild<QLineEdit *>("savedSpaceTrackPassword")->setText("updated-session-fixture");
         accountDialog->findChild<QPushButton *>("sessionSpaceTrackAccount")->click();
      });
      setup->click();
      require(dialog.findChild<QLineEdit *>("spaceTrackIdentity")->text()=="updated@example.invalid","Account setup did not refresh the importer");
   }
   QSettings().setValue("satellites/spaceTrackUrl","http://127.0.0.1:1");
   {
      SatelliteImportDialog dialog({"EarthMJ2000Eq"},{"Earth"},[](const auto &,const auto &) { return QString(); });
      dialog.findChild<QComboBox *>("satelliteCatalogSource")->setCurrentIndex(1);
      QApplication::processEvents();
      require(dialog.findChild<QLineEdit *>("spaceTrackPassword")->text().isEmpty(),"Saved account was copied to a custom endpoint");
   }
   QSettings settings;
   settings.sync();
   for (const auto &key:settings.allKeys())
      require(!settings.value(key).toString().contains("fixture"),"Account secret was written into application settings");
   SpaceTrackAccount::instance().useForSession({});
   std::cout<<"PASS masked account setup, session reuse, cancel, importer access, official-origin autofill and settings isolation\n";
}

void secureTest()
{
   const auto key="org.gmat.test.SpaceTrack."+QUuid::createUuid().toString(QUuid::WithoutBraces);
   const SpaceTrackAccount::Credentials expected{"secure@example.invalid","keyring-secret-fixture"};
   QObject context;
   SpaceTrackAccount account(key);
   bool complete=false;
   QString error;
   account.save(expected,&context,[&](const QString &result) { error=result;complete=true; });
   require(QTest::qWaitFor([&] { return complete; },35000),"Secure account save timed out");
   if (!error.isEmpty()) throw std::runtime_error(error.toStdString());
   SpaceTrackAccount reader(key);
   complete=false;
   reader.load(&context,[&](const QString &result) { error=result;complete=true; });
   require(QTest::qWaitFor([&] { return complete; },35000),"Secure account load timed out");
   const bool roundTrip=error.isEmpty() && reader.credentials().identity==expected.identity && reader.credentials().password==expected.password;
   complete=false;
   account.forget(&context,[&](const QString &result) { error=result;complete=true; });
   require(QTest::qWaitFor([&] { return complete; },35000),"Secure account removal timed out");
   require(error.isEmpty(),"Secure account removal failed");
   require(roundTrip,"Fresh account did not retrieve the stored credentials");
   SpaceTrackAccount absent(key);
   complete=false;
   absent.load(&context,[&](const QString &result) { error=result;complete=true; });
   require(QTest::qWaitFor([&] { return complete; },35000),"Removed account lookup timed out");
   require(error.isEmpty() && absent.credentials().password.isEmpty(),"Forgotten credentials were retained");
   bool saved=false,removed=false;
   account.save(expected,&context,[&](const QString &result) { require(result.isEmpty(),"Queued save failed");saved=true; });
   account.forget(&context,[&](const QString &result) { require(result.isEmpty(),"Queued removal failed");removed=true; });
   require(QTest::qWaitFor([&] { return saved && removed; },35000),"Queued credential operations timed out");
   SpaceTrackAccount queuedResult(key);
   complete=false;
   queuedResult.load(&context,[&](const QString &result) { error=result;complete=true; });
   require(QTest::qWaitFor([&] { return complete; },35000),"Queued result lookup timed out");
   require(error.isEmpty() && queuedResult.credentials().password.isEmpty(),"Concurrent save and forget completed out of order");
   std::cout<<"PASS secure-store persistence across account instances, removal and ordered save/forget with an isolated test key\n";
}

void forgetDialogTest()
{
   const auto key="org.gmat.test.SpaceTrack."+QUuid::createUuid().toString(QUuid::WithoutBraces);
   SpaceTrackAccount account(key);
   account.useForSession({"temporary@example.invalid","disposable-session-fixture"});
   SpaceTrackAccountDialog dialog(account);
   dialog.show();
   dialog.findChild<QPushButton *>("forgetSpaceTrackAccount")->click();
   require(QTest::qWaitFor([&] { return dialog.result()==QDialog::Accepted; },35000),"Forget did not finish the account dialog");
   require(account.credentials().identity.isEmpty() && account.credentials().password.isEmpty(),"Forget retained the session account");
   std::cout<<"PASS forget closes account setup and clears session credentials using an isolated test key\n";
}
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("SpaceTrackAccount");
   TestSettings settings;
   try {
      if (argc>1 && QString::fromLocal8Bit(argv[1])=="secure") secureTest();
      else if (argc>1 && QString::fromLocal8Bit(argv[1])=="forget") forgetDialogTest();
      else sessionTest();
      return 0;
   } catch (const std::exception &error) {
      std::cerr<<error.what()<<'\n';return 1;
   }
}
