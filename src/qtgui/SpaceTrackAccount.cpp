#include "SpaceTrackAccount.hpp"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QPointer>
#include <QScopeGuard>
#include <QThread>
#include <QTimer>
#include <memory>
#include <mutex>
#include <utility>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#elif defined(Q_OS_MACOS)
#include <Security/Security.h>
#endif

namespace {
struct Reply { QByteArray data;QString error; };
struct Job
{
   Reply reply;
   std::mutex mutex;
   bool cancelled=false;
   std::function<void()> cancelNative;
   void cancel() {
      const std::lock_guard<std::mutex> lock(mutex);
      cancelled=true;if (cancelNative) cancelNative();
   }
};
enum class Action { Read,Write,Remove };

Reply nativeCredential(Action action,const QString &key,const QByteArray &data,Job &job)
{
#ifdef Q_OS_WIN
   const auto target=("GMAT/"+key).toStdWString();
   if (action==Action::Read) {
      PCREDENTIALW credential=nullptr;
      if (!CredReadW(target.c_str(),CRED_TYPE_GENERIC,0,&credential))
         return GetLastError()==ERROR_NOT_FOUND ? Reply{} : Reply{{},"Windows Credential Manager could not read the account."};
      const auto release=qScopeGuard([&] { CredFree(credential); });
      return {QByteArray(reinterpret_cast<const char *>(credential->CredentialBlob),credential->CredentialBlobSize),{}};
   }
   if (action==Action::Remove) {
      if (CredDeleteW(target.c_str(),CRED_TYPE_GENERIC,0) || GetLastError()==ERROR_NOT_FOUND) return {};
      return {{},"Windows Credential Manager could not remove the account."};
   }
   CREDENTIALW credential{};credential.Type=CRED_TYPE_GENERIC;
   credential.TargetName=const_cast<wchar_t *>(target.c_str());
   credential.CredentialBlobSize=static_cast<DWORD>(data.size());
   credential.CredentialBlob=reinterpret_cast<LPBYTE>(const_cast<char *>(data.constData()));
   credential.Persist=CRED_PERSIST_LOCAL_MACHINE;
   if (!CredWriteW(&credential,0)) return {{},"Windows Credential Manager could not save the account."};
   return {};
#elif defined(Q_OS_MACOS)
   const auto serviceBytes=key.toUtf8();
   auto service=CFStringCreateWithBytes(nullptr,reinterpret_cast<const UInt8 *>(serviceBytes.constData()),serviceBytes.size(),kCFStringEncodingUTF8,false);
   auto query=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
   const auto release=qScopeGuard([&] { CFRelease(query);CFRelease(service); });
   CFDictionarySetValue(query,kSecClass,kSecClassGenericPassword);
   CFDictionarySetValue(query,kSecAttrService,service);
   CFDictionarySetValue(query,kSecAttrAccount,CFSTR("default"));
   OSStatus status;
   if (action==Action::Read) {
      CFDictionarySetValue(query,kSecReturnData,kCFBooleanTrue);
      CFDictionarySetValue(query,kSecMatchLimit,kSecMatchLimitOne);
      CFTypeRef result=nullptr;status=SecItemCopyMatching(query,&result);
      if (status==errSecItemNotFound) return {};
      if (status!=errSecSuccess) return {{},"Keychain could not read the account."};
      const auto value=static_cast<CFDataRef>(result);
      QByteArray bytes(reinterpret_cast<const char *>(CFDataGetBytePtr(value)),CFDataGetLength(value));
      CFRelease(result);return {bytes,{}};
   }
   if (action==Action::Remove) {
      status=SecItemDelete(query);
      return status==errSecSuccess || status==errSecItemNotFound ? Reply{} : Reply{{},"Keychain could not remove the account."};
   }
   auto value=CFDataCreate(nullptr,reinterpret_cast<const UInt8 *>(data.constData()),data.size());
   auto update=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
   const auto releaseValue=qScopeGuard([&] { CFRelease(update);CFRelease(value); });
   CFDictionarySetValue(update,kSecValueData,value);
   status=SecItemUpdate(query,update);
   if (status==errSecItemNotFound) {
      CFDictionarySetValue(query,kSecValueData,value);status=SecItemAdd(query,nullptr);
   }
   return status==errSecSuccess ? Reply{} : Reply{{},"Keychain could not save the account."};
#elif defined(Q_OS_LINUX)
   // Resolve the stable libsecret C API so a desktop without a keyring can
   // still run GMAT and use session-only credentials.
   QLibrary secret("secret-1",0),gio("gio-2.0",0),glib("glib-2.0",0),gobject("gobject-2.0",0);
   const auto schemaNew=reinterpret_cast<void *(*)(const char *,int,...)>(secret.resolve("secret_schema_new"));
   const auto schemaFree=reinterpret_cast<void (*)(void *)>(secret.resolve("secret_schema_unref"));
   struct Error { unsigned domain;int code;char *message; };
   const auto read=reinterpret_cast<char *(*)(const void *,void *,Error **,...)>(secret.resolve("secret_password_lookup_sync"));
   const auto write=reinterpret_cast<int (*)(const void *,const char *,const char *,const char *,void *,Error **,...)>(secret.resolve("secret_password_store_sync"));
   const auto remove=reinterpret_cast<int (*)(const void *,void *,Error **,...)>(secret.resolve("secret_password_clear_sync"));
   const auto freePassword=reinterpret_cast<void (*)(char *)>(secret.resolve("secret_password_free"));
   const auto newCancellation=reinterpret_cast<void *(*)()>(gio.resolve("g_cancellable_new"));
   const auto cancel=reinterpret_cast<void (*)(void *)>(gio.resolve("g_cancellable_cancel"));
   const auto unref=reinterpret_cast<void (*)(void *)>(gobject.resolve("g_object_unref"));
   const auto freeError=reinterpret_cast<void (*)(Error *)>(glib.resolve("g_error_free"));
   if (!schemaNew || !schemaFree || !read || !write || !remove || !freePassword || !newCancellation || !cancel || !unref || !freeError)
      return {{},"Desktop keyring support is unavailable. Use the account for this session."};
   const auto keyBytes=key.toUtf8();
   auto *schema=schemaNew("org.gmat.Account",0,"service",0,"account",0,nullptr);
   auto *cancellation=newCancellation();
   if (!schema || !cancellation) {
      if (schema) schemaFree(schema);
      if (cancellation) unref(cancellation);
      return {{},"Desktop keyring could not start the request."};
   }
   {
      const std::lock_guard<std::mutex> lock(job.mutex);
      job.cancelNative=[=] { cancel(cancellation); };
      if (job.cancelled) cancel(cancellation);
   }
   const auto cleanup=qScopeGuard([&] {
      const std::lock_guard<std::mutex> lock(job.mutex);
      job.cancelNative={};unref(cancellation);schemaFree(schema);
   });
   Error *error=nullptr;Reply result;
   if (action==Action::Read) {
      char *password=read(schema,cancellation,&error,"service",keyBytes.constData(),"account","default",nullptr);
      if (password) { result.data=password;freePassword(password); }
   } else if (action==Action::Write) {
      if (!write(schema,nullptr,"GMAT Space-Track account",data.constData(),cancellation,&error,"service",keyBytes.constData(),"account","default",nullptr))
         result.error="Desktop keyring could not save the account.";
   } else remove(schema,cancellation,&error,"service",keyBytes.constData(),"account","default",nullptr);
   if (error) { result.error="Desktop keyring: "+QString::fromUtf8(error->message);freeError(error); }
   return result;
#else
   Q_UNUSED(action);Q_UNUSED(key);Q_UNUSED(data);Q_UNUSED(job);
   return {{},"Secure credential storage is unavailable. Use the account for this session."};
#endif
}
}

SpaceTrackAccount::SpaceTrackAccount(QString storageKey,QObject *parent) : QObject(parent),key(std::move(storageKey)) {}
SpaceTrackAccount &SpaceTrackAccount::instance()
{
   static auto *account=new SpaceTrackAccount("org.gmat.SpaceTrack",QCoreApplication::instance());
   return *account;
}
void SpaceTrackAccount::load(QObject *context,Completion complete)
{
   if (loaded) { QTimer::singleShot(0,context,[complete] { complete({}); });return; }
   run(Operation::Read,{},context,std::move(complete));
}
void SpaceTrackAccount::save(Credentials credentials,QObject *context,Completion complete)
{
   ++generation;run(Operation::Write,std::move(credentials),context,std::move(complete));
}
void SpaceTrackAccount::useForSession(Credentials credentials)
{
   ++generation;current=std::move(credentials);loaded=true;
}
void SpaceTrackAccount::forget(QObject *context,Completion complete)
{
   ++generation;run(Operation::Remove,{},context,std::move(complete));
}
void SpaceTrackAccount::run(Operation operation,Credentials credentials,QObject *context,Completion complete)
{
   pending.push_back({operation,std::move(credentials),context,std::move(complete),generation});
   startNext();
}
void SpaceTrackAccount::startNext()
{
   if (running) return;
   while (!pending.empty() && !pending.front().context) pending.pop_front();
   if (pending.empty()) return;
   auto request=std::move(pending.front());
   pending.pop_front();
   running=true;
   const auto operation=request.operation;
   const auto credentials=request.credentials;
   const auto action=operation==Operation::Read ? Action::Read : operation==Operation::Write ? Action::Write : Action::Remove;
   const auto bytes=QJsonDocument(QJsonObject{{"identity",credentials.identity},{"password",credentials.password}}).toJson(QJsonDocument::Compact);
   auto job=std::make_shared<Job>();
   auto *thread=QThread::create([job,action,key=key,bytes] {
      try { job->reply=nativeCredential(action,key,bytes,*job); }
      catch (...) { job->reply.error="Credential storage could not complete the request."; }
   });
   auto *timeout=new QTimer(thread);timeout->setSingleShot(true);timeout->start(30000);
   connect(timeout,&QTimer::timeout,thread,[job] { job->cancel(); });
   connect(request.context,&QObject::destroyed,thread,[job] { job->cancel(); });
   connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,thread,[job] { job->cancel(); });
   connect(thread,&QThread::finished,this,[this,job,request,timeout] {
      timeout->stop();
      auto error=job->reply.error;
      {
         const std::lock_guard<std::mutex> lock(job->mutex);
         if (job->cancelled) error="Credential request cancelled.";
      }
      if (error.isEmpty() && request.revision==generation) {
         if (request.operation==Operation::Read) {
            const auto object=QJsonDocument::fromJson(job->reply.data).object();
            if (!job->reply.data.isEmpty() && (!object.value("identity").isString() || !object.value("password").isString()))
               error="The saved Space-Track account could not be read. Save the account again to replace it.";
            else current={object.value("identity").toString(),object.value("password").toString()};
         } else current=request.operation==Operation::Write ? request.credentials : Credentials{};
         loaded=error.isEmpty();
      }
      running=false;
      const QPointer<SpaceTrackAccount> guard(this);
      if (request.context) request.complete(error);
      if (guard) startNext();
   });
   connect(thread,&QThread::finished,thread,&QObject::deleteLater);
   thread->start();
}
