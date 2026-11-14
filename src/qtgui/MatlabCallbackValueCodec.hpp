#pragma once
#include "MatlabValue.hpp"
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace MatlabCallbackValueCodec
{
inline void require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
inline Integer integer(const QJsonValue &value)
{
   require(value.isDouble(), "Missing MATLAB value dimension");
   const double number=value.toDouble();
   require(std::isfinite(number) && number>=0 && number<=std::numeric_limits<Integer>::max() &&
      std::floor(number)==number,"Invalid MATLAB value dimension");
   return static_cast<Integer>(number);
}
inline size_t count(Integer rows,Integer columns)
{
   require(rows>=0 && columns>=0,"Negative MATLAB value dimensions");
   require(!columns || static_cast<size_t>(rows)<=static_cast<size_t>(std::numeric_limits<Integer>::max())/static_cast<size_t>(columns),"MATLAB value exceeds native element capacity");
   const size_t size=static_cast<size_t>(rows)*static_cast<size_t>(columns);
   require(size<=static_cast<size_t>(std::numeric_limits<Integer>::max()),"MATLAB value exceeds native element capacity");
   return size;
}
inline QByteArray bytes(const QJsonValue &value)
{
   require(value.isString(),"Missing MATLAB value byte data");
   const auto decoded=QByteArray::fromBase64Encoding(value.toString().toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
   require(bool(decoded),"Invalid MATLAB value base64 data");
   return decoded.decoded;
}
inline QJsonObject encode(const MatlabValue &value,int depth=0)
{
   require(depth<=128,"MATLAB value nesting exceeds the callback transport limit");
   const auto size=count(value.rows,value.columns);
   require(value.numbers.size()<=static_cast<size_t>(std::numeric_limits<Integer>::max()),"MATLAB numeric payload exceeds native capacity");
   require(value.numbers.size()<=static_cast<size_t>(std::numeric_limits<qsizetype>::max())/8,"MATLAB byte payload exceeds native capacity");
   QByteArray numbers(static_cast<qsizetype>(value.numbers.size()*8),Qt::Uninitialized);
   static_assert(sizeof(Real)==8,"The MATLAB callback transport uses binary64 values");
   for(size_t i=0;i<value.numbers.size();++i) {
      quint64 bits;std::memcpy(&bits,&value.numbers[i],8);
      qToLittleEndian(bits,numbers.data()+i*8);
   }
   QJsonArray fields,children;
   for(const auto &field:value.fieldNames) fields.append(QString::fromStdString(field));
   for(const auto &child:value.children) children.append(encode(child,depth+1));
   if(value.type==MatlabValue::Type::NUMERIC || value.type==MatlabValue::Type::LOGICAL)
      require(size==value.numbers.size(),"MATLAB numeric dimensions do not match data");
   if(value.type==MatlabValue::Type::CELL) require(size==value.children.size(),"MATLAB cell dimensions do not match data");
   if(value.type==MatlabValue::Type::STRUCTURE)
      require(size==1 && value.children.size()==value.fieldNames.size(),"MATLAB structure must be scalar with named fields");
   return {{"type",static_cast<int>(value.type)},{"className",QString::fromStdString(value.className)},
      {"rows",value.rows},{"columns",value.columns},{"numbers",QString::fromLatin1(numbers.toBase64())},
      {"text",QString::fromLatin1(QByteArray(value.text.data(),static_cast<qsizetype>(value.text.size())).toBase64())},
      {"fields",fields},{"children",children}};
}
inline MatlabValue decode(const QJsonObject &object,int depth=0)
{
   require(depth<=128 && object.size()==8,"Invalid MATLAB value packet");
   const auto type=integer(object.value("type"));require(type<=5,"Unsupported MATLAB value packet type");
   require(object.value("className").isString() && object.value("fields").isArray() &&
      object.value("children").isArray(),"Incomplete MATLAB value packet");
   MatlabValue value;value.type=static_cast<MatlabValue::Type>(type);
   value.className=object.value("className").toString().toStdString();
   value.rows=integer(object.value("rows"));value.columns=integer(object.value("columns"));
   const auto size=count(value.rows,value.columns);
   const auto numbers=bytes(object.value("numbers"));require(numbers.size()%8==0,"Invalid MATLAB binary64 payload");
   require(static_cast<size_t>(numbers.size()/8)<=static_cast<size_t>(std::numeric_limits<Integer>::max()),"MATLAB numeric payload exceeds native capacity");
   value.numbers.resize(static_cast<size_t>(numbers.size()/8));
   for(size_t i=0;i<value.numbers.size();++i) {
      const auto bits=qFromLittleEndian<quint64>(numbers.constData()+i*8);
      std::memcpy(&value.numbers[i],&bits,8);
   }
   const auto text=bytes(object.value("text"));value.text.assign(text.constData(),static_cast<size_t>(text.size()));
   for(const auto &field:object.value("fields").toArray()) {
      require(field.isString(),"Invalid MATLAB structure field name");value.fieldNames.push_back(field.toString().toStdString());
   }
   for(const auto &child:object.value("children").toArray()) {
      require(child.isObject(),"Invalid MATLAB child packet");value.children.push_back(decode(child.toObject(),depth+1));
   }
   if(value.type==MatlabValue::Type::NUMERIC || value.type==MatlabValue::Type::LOGICAL)
      require(size==value.numbers.size(),"MATLAB numeric dimensions do not match data");
   if(value.type==MatlabValue::Type::LOGICAL)
      for(const auto number:value.numbers) require(number==0 || number==1,"Invalid MATLAB logical value");
   if(value.type==MatlabValue::Type::CELL) require(size==value.children.size(),"MATLAB cell dimensions do not match data");
   if(value.type==MatlabValue::Type::STRUCTURE)
      require(size==1 && value.children.size()==value.fieldNames.size(),"Invalid MATLAB scalar structure fields");
   return value;
}
}
