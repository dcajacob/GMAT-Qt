#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "MatlabInterface.hpp"
#include "DataWriterInterface.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QSet>
#include <memory>
using namespace formaccept;
static QString quote(QString value){return "'"+value.replace("'","''")+"'";}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaMatlabLibraryWriter");
   try {
      require(argc==4,"Supply startup, MATLAB plugin and writer profile");const QString mode=argv[3];
      require(mode=="Dense"||mode=="Records","Unknown MATLAB writer profile");const int count=mode=="Records"?2:1;
      QTemporaryDir runtime;require(runtime.isValid(),"MATLAB writer private startup failed");
      const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
      Rig rig(startup);rig.save("startup.txt",read(startup));
      const QString source="% MATLAB writer owner/control witness\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\n";
      rig.source->setPlainText(source);require(rig.window.buildScript(),"MATLAB writer caller build failed");
      const int history=rig.source->document()->availableUndoSteps();
      const auto path=rig.files.filePath("MATLAB writer records Ω.mat");
      std::unique_ptr<DataWriter> writer(DataWriterInterface::Instance()->GetDataWriter("MatWriter"));
      require(bool(writer),"Actual registered MATLAB writer unavailable");
      require(writer->Initialize(path.toStdString(),"w6")&&writer->DescribeData({"Numbers","Labels"},count),"Actual MATLAB writer setup failed");
      QJsonObject expected;expected["shape"]=QJsonArray{count,1};
      for(int row=0;row<count;++row) {
         const double scale=row+1;const Matrix numbers{{scale,2*scale,3*scale},{4*scale,5*scale,6*scale}};
         const StringMatrix labels=mode=="Records"?StringMatrix{{"row"+std::to_string(row)+"-A","row"+std::to_string(row)+"-B"}}:StringMatrix{{"row0-A"}};
         require(writer->AddDataBlock(),"MATLAB writer record block failed");
         auto numeric=std::unique_ptr<WriterData>(writer->GetContainer(Gmat::REAL_TYPE,"Numbers"));
         auto text=std::unique_ptr<WriterData>(writer->GetContainer(Gmat::STRING_TYPE,"Labels"));
         require(numeric&&text&&numeric->AddData(numbers)&&text->AddData(labels),"MATLAB writer input containers failed");
         require(writer->AddData(numeric.get(),row),"MATLAB numeric record was not accepted");numeric.release();
         require(writer->AddData(text.get(),row),"MATLAB string record was not accepted");text.release();
         QJsonArray matrix,labelArray;for(const auto &r:numbers){QJsonArray cells;for(const auto x:r)cells.append(x);matrix.append(cells);}for(const auto &label:labels.front())labelArray.append(QString::fromStdString(label));
         expected["record"+QString::number(row)]=QJsonObject{{"numbers",matrix},{"labels",labelArray}};
      }
      require(writer->WriteData("Records")&&writer->CloseFile()&&writer->ClearData(),"MATLAB writer Write/Close/Clear failed");
      writer.reset();const auto bytes=read(path);require(!bytes.isEmpty(),"Actual MAT-file bytes absent");rig.save("records.mat",bytes);
      auto *matlab=MatlabInterface::Instance();QString evaluate="gmat_writer_loaded = load("+quote(path)+"); gmat_writer_view = struct; gmat_writer_view.shape = size(gmat_writer_loaded.Records);\n";
      for(int row=0;row<count;++row) {
         const auto key="record"+QString::number(row),index=QString::number(row+1);
         evaluate+="gmat_writer_view."+key+".numbers = gmat_writer_loaded.Records("+index+").Numbers;\n";
         evaluate+="gmat_writer_view."+key+".labels = gmat_writer_loaded.Records("+index+").Labels;\n";
      }
      evaluate+="gmat_writer_json = jsonencode(gmat_writer_view);";rig.save("load-and-inspect.m",evaluate.toUtf8());
      matlab->RunMatlabString(evaluate.toStdString());std::string json;require(matlab->GetString("gmat_writer_json",json)==1,"Real MATLAB could not inspect the emitted MAT-file");
      rig.save("actual.json",QByteArray::fromStdString(json));rig.save("expected.json",QJsonDocument(expected).toJson());
      require(QJsonDocument::fromJson(QByteArray::fromStdString(json)).object()==expected,"Actual MATLAB-loaded numeric/string records differ from independent inputs");
      require(read(path)==bytes&&rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==history&&Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==41,"MATLAB writer changed its output bytes or caller/control state");
      acceptanceView(rig.window);
      const QJsonObject proof{{"checkpoint","actual-MATLAB-library-writer"},{"profile",mode},{"records",count},{"expected",expected},{"actual",QJsonDocument::fromJson(QByteArray::fromStdString(json)).object()},{"mat_file_sha256",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &error){std::cerr<<error.GetFullMessage()<<'\n';return 1;}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
