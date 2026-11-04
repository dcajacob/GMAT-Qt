#pragma once
// Same isolated startup policy as the sealed creation witness.
static QString isolatedStartup(const QString &original,const QString &directory,const QString &plugin) {
 const QDir installation(QFileInfo(original).absolutePath());
 QStringList lines=QString::fromUtf8(formaccept::read(original)).split('\n');
 const QRegularExpression assignment("^(\\w+)\\s*=\\s*(.*)$");
 const QMap<QString,QString> settings{{"OUTPUT_PATH",directory+"/output/"},{"LOG_FILE",directory+"/output/GmatLog.txt"},{"PERSONALIZATION_FILE",directory+"/MyGmat.ini"},{"MATLAB_MODE","SHARED"}};
 QSet<QString> changed;
 for(auto &line:lines) {
  const auto match=assignment.match(line);if(!match.hasMatch())continue;
  const auto key=match.captured(1);auto value=match.captured(2).trimmed();
  if(settings.contains(key)){line=key+" = "+settings[key];changed.insert(key);}
  else if(value.startsWith("../")||value.startsWith("./")) {
   const bool slash=value.endsWith('/');value=QDir::cleanPath(installation.absoluteFilePath(value));if(slash)value+='/';line=key+" = "+value;
  }
 }
 for(auto i=settings.begin();i!=settings.end();++i)if(!changed.contains(i.key()))lines<<i.key()+" = "+i.value();
 bool present=false;for(const auto &line:lines)if(line.startsWith("PLUGIN")&&line.contains("libMatlabInterface"))present=true;
 if(!present)lines<<"PLUGIN = "+plugin;
 formaccept::require(QDir().mkpath(directory+"/output"),"MATLAB private output directory failed");
 const auto path=directory+"/startup.txt";formaccept::write(path,lines.join('\n').toUtf8());return path;
}

