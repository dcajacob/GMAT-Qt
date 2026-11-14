// Qt MAT output keeps native file/array handles inside one serialized operation.
#ifdef GMAT_MATLAB_QT_ENGINE
#include "MatWriter.hpp"
#include "MatWriterValue.hpp"
#include "MatlabArrayConversion.hpp"
#include "MatlabCallbackBridge.hpp"
#include "MatlabLibraryExecutor.hpp"
#include "RealMatData.hpp"
#include "StringMatData.hpp"
#include "MessageInterface.hpp"
#include <algorithm>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace
{
struct Operation
{
   bool &active;
   explicit Operation(bool &flag) : active(flag)
   {
      if (active) throw UtilityException("The MATLAB writer is already executing an operation");
      active = true;
   }
   ~Operation() { active = false; }
};
struct NativeFile
{
   MATFile *file;
   NativeFile(const std::string &path, const char *mode) : file(matOpen(path.c_str(), mode))
   {
      if (!file) throw UtilityException("Cannot open MATLAB output file: " + path);
   }
   ~NativeFile() { if (file) matClose(file); }
   void Close()
   {
      auto handle = file; file = nullptr;
      if (matClose(handle) != 0) throw UtilityException("Cannot close MATLAB output file");
   }
   NativeFile(const NativeFile&) = delete;
   NativeFile& operator=(const NativeFile&) = delete;
};
using ArrayOwner = std::unique_ptr<mxArray, decltype(&mxDestroyArray)>;
void Name(const std::string &name)
{
   auto letter = [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); };
   if (name.empty() || !letter(static_cast<unsigned char>(name.front())))
      throw UtilityException("Invalid MATLAB writer variable or field name: " + name);
   for (unsigned char c : name)
      if (!letter(c) && !(c >= '0' && c <= '9') && c != '_')
         throw UtilityException("Invalid MATLAB writer variable or field name: " + name);
}
MatlabValue Record(const StringArray &fields)
{
   MatWriterValue::Dimension(fields.size());
   MatlabValue value; value.type = MatlabValue::Type::STRUCTURE; value.className = "struct";
   value.rows = value.columns = 1; value.fieldNames = fields;
   value.children.assign(fields.size(), MatWriterValue::Empty());
   return value;
}
std::unique_ptr<WriterData> Clone(WriterData *data)
{
   if (auto real = dynamic_cast<RealMatData*>(data)) return std::make_unique<RealMatData>(*real);
   if (auto text = dynamic_cast<StringMatData*>(data)) return std::make_unique<StringMatData>(*text);
   throw UtilityException("Unsupported MATLAB writer data container");
}
MatlabValue Value(WriterData *data)
{
   if (auto real = dynamic_cast<RealMatData*>(data)) return real->ToMatlabValue();
   if (auto text = dynamic_cast<StringMatData*>(data)) return text->ToMatlabValue();
   throw UtilityException("Unsupported MATLAB writer data container");
}
}

bool MatWriter::OpenQtFile()
{
   Operation operation(qtOperationActive);
   if (filename.empty() || filename.find('\0') != std::string::npos)
      throw UtilityException("MATLAB output filename is empty or contains a NUL character");
   if (format != "w4" && format != "w6" && format != "w7" && format != "w7.3" && format != "w")
   {
      MessageInterface::ShowMessage("MATLAB Writer .mat version %s invalid; defaulting to w6\n", format.c_str());
      format = "w6";
   }
   // Reopening per write must retain the path selected at OpenFile, even if a
   // later callback changes MATLAB's or GMAT's working directory.
   const auto path = std::filesystem::absolute(filename).string(), mode = format;
   qtFileOpen = false;
   if (MatlabCallbackBridge::IsActive())
   {
      MatlabCallbackRequest request; request.operation = MatlabCallbackRequest::Operation::OPEN_MAT_FILE;
      request.name = path; request.expression = mode;
      MatlabCallbackBridge::Execute(request);
   }
   else
   {
      auto create = [path, mode] { NativeFile file(path, mode.c_str()); file.Close(); };
      if (MatlabLibraries().OnWorker()) create(); else MatlabLibraries().Invoke(create);
   }
   filename = path; qtFileOpen = true;
   return true;
}

void MatWriter::DescribeQtData(const StringArray &fields, mwSize size)
{
   Operation operation(qtOperationActive);
   MatWriterValue::Dimension(size); MatWriterValue::Dimension(fields.size());
   StringArray staged = fields;
   std::unordered_set<std::string> names;
   for (const auto &field : staged)
   {
      Name(field);
      if (!names.insert(field).second) throw UtilityException("Duplicate MATLAB writer field: " + field);
   }
   qtStructureFields.swap(staged); qtStructureSize = size; qtStructureReady = true;
}

bool MatWriter::WriteQtData(const std::string &name)
{
   Operation operation(qtOperationActive);
   if (!qtStructureReady) throw UtilityException("Cannot write MATLAB data: structure description not created");
   if (!qtFileOpen) throw UtilityException("Cannot write MATLAB data: file is not open");
   if (format == "w4") throw UtilityException("MAT-file version 4 cannot store structured MATLAB writer data");
   Name(name);
   if (allData.size() > qtStructureSize) throw UtilityException("MATLAB writer has more data blocks than described records");
   const auto prototype = Record(qtStructureFields);
   auto records = MatWriterValue::Cell(qtStructureSize, 1);
   records.children.assign(qtStructureSize, prototype);
   for (size_t i = 0; i < allData.size(); ++i)
      for (auto data : allData[i])
      {
         if (!data) throw UtilityException("MATLAB writer has a null data container");
         const auto field = data->GetName();
         const auto found = std::find(qtStructureFields.begin(), qtStructureFields.end(), field);
         if (found == qtStructureFields.end()) throw UtilityException("Undescribed MATLAB writer field: " + field);
         records.children[i].children[found - qtStructureFields.begin()] = Value(data);
      }
   const auto path = filename, mode = format;
   if (MatlabCallbackBridge::IsActive())
   {
      MatlabCallbackRequest request; request.operation = MatlabCallbackRequest::Operation::WRITE_MAT_FILE;
      request.name = path; request.expression = mode;
      request.value = Record({"name", "prototype", "records"});
      request.value.children = {MatWriterValue::Character(name), prototype, std::move(records)};
      MatlabCallbackBridge::Execute(request);
   }
   else
   {
      auto write = [&records, &prototype, path, name]
      {
         std::vector<const char*> fields;
         for (const auto &field : prototype.fieldNames) fields.push_back(field.c_str());
         ArrayOwner array(mxCreateStructMatrix(records.rows, 1, static_cast<int>(fields.size()), fields.data()), &mxDestroyArray);
         if (!array) throw UtilityException("Cannot allocate MATLAB output structure");
         for (size_t i = 0; i < records.children.size(); ++i)
            for (size_t j = 0; j < prototype.fieldNames.size(); ++j)
            {
               ArrayOwner child(CreateMatlabArray(records.children[i].children[j]), &mxDestroyArray);
               mxSetFieldByNumber(array.get(), i, static_cast<int>(j), child.release());
            }
         NativeFile file(path, "u");
         if (matPutVariable(file.file, name.c_str(), array.get()) != 0)
            throw UtilityException("Cannot write MATLAB output variable: " + name);
         file.Close();
      };
      if (MatlabLibraries().OnWorker()) write(); else MatlabLibraries().Invoke(write);
   }
   return true;
}

void MatWriter::ClearQtData()
{
   if (qtOperationActive) throw UtilityException("Cannot clear an active MATLAB writer");
   // AddData transfers ownership to the writer. An aliased container is deleted
   // only once, even when it appears in more than one record.
   std::unordered_set<WriterData*> unique;
   for (const auto &block : allData) for (auto data : block) if (data) unique.insert(data);
   for (auto data : unique) delete data;
   allData.clear(); qtStructureReady = false; qtStructureSize = 0; qtStructureFields.clear();
}

void MatWriter::CopyQtData(const MatWriter &source)
{
   if (qtOperationActive || source.qtOperationActive) throw UtilityException("Cannot copy an active MATLAB writer");
   // Stage an independent copy before retiring destination data. File handles
   // and a live file-open state never transfer to the copy.
   auto stagedName = source.filename, stagedFormat = source.format;
   auto stagedNames = source.variable_names, stagedFields = source.qtStructureFields;
   std::vector<std::vector<WriterData*>> blocks(source.allData.size());
   std::vector<std::unique_ptr<WriterData>> owners;
   std::unordered_map<WriterData*, WriterData*> copies;
   for (size_t i = 0; i < source.allData.size(); ++i)
      for (auto data : source.allData[i])
      {
         auto found = copies.find(data);
         if (found == copies.end())
         {
            auto value = Clone(data); auto pointer = value.get();
            owners.push_back(std::move(value)); found = copies.emplace(data, pointer).first;
         }
         blocks[i].push_back(found->second);
      }
   ClearQtData();
   allData.swap(blocks); filename.swap(stagedName); format.swap(stagedFormat);
   variable_names.swap(stagedNames); qtStructureFields.swap(stagedFields);
   qtStructureReady = source.qtStructureReady; qtStructureSize = source.qtStructureSize;
   qtFileOpen = false;
   for (auto &value : owners) value.release();
}
#endif
