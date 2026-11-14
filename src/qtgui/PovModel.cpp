// SPDX-License-Identifier: Apache-2.0
#include "PovModel.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringConverter>
#include <QScopedValueRollback>
#include <QDebug>
#include <QThread>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/Program>
#include <osg/Shader>
#include <osg/ValueObject>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <functional>
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <variant>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
class ImportCanceled final : public std::runtime_error {
public:
   ImportCanceled() : std::runtime_error("POV model import canceled.") {}
};
class ImportControl {
public:
   explicit ImportControl(const PovModelCancel &predicate) : cancel(predicate) {}
   void check() const {
      if (QThread::currentThread()->isInterruptionRequested() || (cancel && cancel())) throw ImportCanceled();
   }
private:
   const PovModelCancel &cancel;
};
using Tuple=std::vector<double>;
constexpr double PovEpsilon=1.0e-10;
bool povTrue(double value) { return std::abs(value)>PovEpsilon; }
double promotedComponent(const Tuple &value,size_t index)
{
   return value.size()==1 ? value.front() : index<value.size() ? value[index] : 0.0;
}
bool compareValues(const QString &operation,double left,double right)
{
   const bool equal=!povTrue(left-right);
   if (operation=="=") return equal;
   if (operation=="!=") return !equal;
   if (operation=="<=") return left<=right || equal;
   if (operation==">=") return left>=right || equal;
   return operation=="<" ? left<right : left>right;
}
QString terminatedString(QString value)
{
   const auto end=value.indexOf(QChar(u'\0'));
   if (end>=0) value.truncate(end);
   return value;
}
struct SymbolTable;
struct Binding;
struct Token {
   QString text;
   int line=1;
   enum Kind { End,Word,Number,Symbol,String,MacroEnd } kind=End;
   QString file;
   std::shared_ptr<SymbolTable> scope;
   qsizetype offset=0;
   std::shared_ptr<Binding> reference;
   std::shared_ptr<SymbolTable> referenceOwner;
};
bool isComparison(const Token &token)
{
   return token.kind==Token::Symbol && (token.text=="<" || token.text=="<=" ||
      token.text=="=" || token.text=="!=" || token.text==">=" || token.text==">");
}
QString sourceLocation(const QString &file,int line)
{
   return file.isEmpty() ? QString("POV line %1").arg(line) : QString("POV %1:%2").arg(file).arg(line);
}
[[noreturn]] void sourceError(const Token &token,const QString &message)
{
   throw std::runtime_error((sourceLocation(token.file,token.line)+": "+message).toStdString());
}

struct Material {
   std::optional<osg::Vec4> color;
   std::optional<float> ambient,diffuse,specular,power;
   void overlay(const Material &other) {
      if (other.color) color=other.color;
      if (other.ambient) ambient=other.ambient;
      if (other.diffuse) diffuse=other.diffuse;
      if (other.specular) specular=other.specular;
      if (other.power) power=other.power;
   }
};
Material displayMaterial()
{
   Material result; result.color=osg::Vec4(0.75f,0.75f,0.75f,1);
   result.ambient=0.2f; result.diffuse=0.8f; result.specular=0.1f; result.power=32.0f;
   return result;
}
struct Triangle {
   std::array<osg::Vec3d,3> vertices,normals;
   bool smooth=false;
   int line=1;
   QString file;
   Material material;
   std::shared_ptr<const std::array<Material,3>> cornerMaterials;
   std::optional<std::array<osg::Vec3d,3>> materialWeights;
};
struct Model {
   enum class Kind { Patch,Primitive,Mesh,Union,Intersection,Difference,Merge,Empty };
   osg::Matrixd matrix=osg::Matrixd::identity();
   std::vector<Triangle> faces;
   // Some primitives retain visible internal surfaces while their solid
   // membership is defined by a smaller boundary (the spindle torus union).
   std::shared_ptr<const std::vector<Triangle>> solidFaces;
   std::vector<std::shared_ptr<Model>> children;
   Material material;
   bool textureLayer=false;
   bool instance=false;
   Kind kind=Kind::Patch;
   bool closed=true,inverse=false;
   std::optional<osg::Vec3d> insideVector;
   QString file;
   int line=1;
};
using Node=std::shared_ptr<Model>;

using MaterialSymbol=std::pair<QString,Material>;
struct ArrayData;
using Array=std::shared_ptr<ArrayData>;
struct MacroData {
   struct Parameter { QString name; bool optional=false; };
   std::vector<Parameter> parameters;
   QString text,file;
   int line=1;
};
using Macro=std::shared_ptr<MacroData>;
using SymbolValue=std::variant<Tuple,Node,MaterialSymbol,osg::Matrixd,QString,Array,Macro>;
struct Binding {
   Binding() : cell(std::make_shared<std::optional<SymbolValue>>()) {}
   explicit Binding(SymbolValue value) : cell(std::make_shared<std::optional<SymbolValue>>(std::move(value))) {}
   std::shared_ptr<std::optional<SymbolValue>> cell;
   // Only formal aliases retain an element's owning array for type checks.
   // Array entries themselves have no owner link, avoiding ownership cycles.
   Array elementArray;
   const SymbolValue *get() const { return *cell ? &**cell : nullptr; }
};
struct ArrayData {
   std::vector<int> dimensions;
   std::optional<QString> elementType;
   std::map<size_t,Binding> elements;
   bool resizable=false,mixed=false;
};
QString arrayElementType(const SymbolValue &value)
{
   QString type=QString::number(value.index());
   if (const auto *tuple=std::get_if<Tuple>(&value)) type+=':'+QString::number(tuple->size());
   if (const auto *material=std::get_if<MaterialSymbol>(&value)) type+=':'+material->first;
   return type;
}
SymbolValue copyValue(const SymbolValue &value,const Token &origin,ImportControl &control,int depth=0)
{
   control.check();
   if (depth>128) sourceError(origin,"Array value nesting exceeds the model reader's limit.");
   if (const auto *array=std::get_if<Array>(&value)) {
      if (!*array) sourceError(origin,"Missing array value.");
      auto result=std::make_shared<ArrayData>();
      result->dimensions=(*array)->dimensions; result->elementType=(*array)->elementType;
      result->resizable=(*array)->resizable; result->mixed=(*array)->mixed;
      for (const auto &[index,element]:(*array)->elements) {
         control.check();
         if (const auto *value=element.get())
            result->elements.emplace(index,Binding(copyValue(*value,origin,control,depth+1)));
      }
      return result;
   }
   // Model prototypes are immutable; instances retain their own modifiers.
   return value;
}
void storeBinding(Binding &binding,const SymbolValue &value,const Token &origin,ImportControl &control)
{
   const auto type=arrayElementType(value);
   if (binding.elementArray && !binding.elementArray->mixed && binding.elementArray->elementType && *binding.elementArray->elementType!=type)
      sourceError(origin,"An array's initialized elements must have the same type.");
   auto owned=copyValue(value,origin,control);
   *binding.cell=std::move(owned);
   if (binding.elementArray && !binding.elementArray->mixed) binding.elementArray->elementType=type;
}
void storeArrayElement(const Array &array,size_t index,const SymbolValue &value,const Token &origin,ImportControl &control)
{
   Binding alias=array->elements[index]; alias.elementArray=array;
   storeBinding(alias,value,origin,control);
}
struct ArraySlot { Array array; size_t index; };
struct SymbolTable {
   explicit SymbolTable(std::shared_ptr<SymbolTable> outer={}) : parent(std::move(outer)) {}
   std::shared_ptr<SymbolTable> parent;
   std::map<QString,Binding> values;
};
std::shared_ptr<SymbolTable> symbolOwner(std::shared_ptr<SymbolTable> scope,const QString &name)
{
   for (;scope;scope=scope->parent) if (scope->values.count(name)) return scope;
   return {};
}
template<class T> const T *symbol(const Token &token,const QString &name)
{
   const auto owner=symbolOwner(token.scope,name);
   const auto *value=owner ? owner->values.at(name).get() : nullptr;
   return value ? std::get_if<T>(value) : nullptr;
}
bool isStringFunction(const QString &name)
{
   return name=="concat" || name=="chr" || name=="str" || name=="vstr" ||
          name=="substr" || name=="strlwr" || name=="strupr";
}
bool isVectorFunction(const QString &name)
{
   return name=="vcross" || name=="vnormalize" || name=="vrotate" || name=="vaxis_rotate";
}
bool isNumericFunction(const QString &name)
{
   static const QStringList scalarFunctions{
      "abs","acos","acosh","asin","asinh","atan","atanh","atan2",
      "ceil","cos","cosh","degrees","div","exp","floor","int","ln","log",
      "max","min","mod","pow","radians","sin","sinh","sqrt","tan","tanh",
      "vdot","vlength","defined","dimensions","dimension_size","select","strcmp","strlen"};
   return isVectorFunction(name) || scalarFunctions.contains(name);
}

// Each file is a separate token stream. EOF resumes its including file without
// joining neighboring numbers/identifiers or changing their source locations.
class SourceLexer {
   struct Frame {
      QString text,file;
      qsizetype cursor=0;
      int line=1;
      std::shared_ptr<SymbolTable> scope;
      bool macro=false;
   };
public:
   struct Bookmark { std::vector<Frame> frames; std::optional<Token> pending; };
   enum class Skip { Branches,Labels,End };
   SourceLexer(const QString &path,const QStringList &paths,ImportControl &control) : includePaths(paths),control(control) {
      rootDirectory=QFileInfo(path).absolutePath();
      pushFile(QFileInfo(path).absoluteFilePath(),{});
   }
   const std::shared_ptr<SymbolTable> &globals() const { return global; }
   void setIncludeEvaluator(std::function<QString(const Token &)> evaluator) { includeEvaluator=std::move(evaluator); }
   Token next() {
      if (pending) { auto result=*pending; pending.reset(); return result; }
      if (rawOnly) return raw(true);
      auto result=raw();
      while (result.text=="#" && result.kind==Token::Symbol) {
         const auto directive=raw();
         if (directive.file!=result.file || directive.kind!=Token::Word)
            sourceError(result,"Expected a directive name in the same file.");
         if (directive.text!="include") { pending=directive; return result; }
         if (frames.size()>=11) sourceError(result,"Include nesting exceeds 10 files.");
         if (!includeEvaluator) sourceError(result,"The include expression evaluator is unavailable.");
         QString filename;
         {
            // Evaluate this file's expression without expanding a following
            // directive or leaving its EOF. The expression stops on its last
            // token, so the parent has no prematurely consumed lookahead.
            QScopedValueRollback<bool> mode(rawOnly,true);
            filename=includeEvaluator(result);
         }
         if (filename.isEmpty()) sourceError(result,"#include requires a nonempty filename.");
         if (filename.contains(QChar(u'\0'))) sourceError(result,"An include filename cannot contain a NUL character.");
         const auto path=resolve(filename,result);
         pushFile(path,result);
         result=raw();
      }
      return result;
   }
   Token peekRawToken() {
      // Peek without expanding includes or executing a following directive.
      // Include-filename evaluation must also retain its same-file EOF bound.
      auto saved=frames;
      try {
         auto result=raw(rawOnly); frames=std::move(saved); return result;
      } catch (...) { frames=std::move(saved); throw; }
   }
   bool nextIsSymbol(const QString &text) {
      const auto next=peekRawToken();
      return next.kind==Token::Symbol && next.text==text;
   }
   Bookmark bookmark() const { return {frames,pending}; }
   void restore(const Bookmark &state) { frames=state.frames; pending=state.pending; }
   bool atSource(const Token &origin) const { return frames.back().file==origin.file && frames.back().scope==origin.scope; }
   void captureMacro(MacroData &macro,const Token &opening,size_t activeDepth) {
      // The header's ')' is already read. Scan the defining file only, with
      // no include expansion, and retain source text without scope bindings.
      const auto begin=frames.back().cursor;
      macro.file=frames.back().file; macro.line=frames.back().line;
      size_t nested=0;
      for (;;) {
         const auto item=raw(true);
         if (item.kind==Token::End || item.kind==Token::MacroEnd)
            sourceError(opening,"Unterminated macro definition.");
         if (item.kind!=Token::Symbol || item.text!="#") continue;
         const auto kind=raw(true);
         if (kind.kind!=Token::Word || kind.file!=item.file)
            sourceError(item,"Expected a directive name in the same file.");
         if (kind.text=="if" || kind.text=="ifdef" || kind.text=="ifndef" ||
             kind.text=="switch" || kind.text=="while" || kind.text=="for" || kind.text=="macro") {
            if (++nested+activeDepth>200) sourceError(item,"Control directive nesting exceeds 200 levels.");
         } else if (kind.text=="end") {
            if (nested) --nested;
            else { macro.text=frames.back().text.mid(begin,item.offset-begin); return; }
         }
      }
   }
   void enterMacro(const MacroData &macro,std::shared_ptr<SymbolTable> scope) {
      Frame frame; frame.text=macro.text; frame.file=macro.file;
      frame.line=macro.line; frame.scope=std::move(scope); frame.macro=true;
      frames.clear(); frames.push_back(std::move(frame)); pending.reset();
   }
   Token skipControl(const Token &opening,size_t activeDepth,Skip stop=Skip::Branches,size_t nested=0) {
      for (;;) {
         const auto item=raw(rawOnly);
         if (item.kind==Token::End || item.kind==Token::MacroEnd) sourceError(opening,"Unterminated control directive.");
         if (item.kind!=Token::Symbol || item.text!="#") continue;
         const auto kind=raw(rawOnly);
         if (kind.kind!=Token::Word || kind.file!=item.file)
            sourceError(item,"Expected a directive name in the same file.");
         if (kind.text=="if" || kind.text=="ifdef" || kind.text=="ifndef" ||
             kind.text=="switch" || kind.text=="while" || kind.text=="for" || kind.text=="macro") {
            if (++nested+activeDepth>200) sourceError(item,"Control directive nesting exceeds 200 levels.");
         } else if (kind.text=="end") {
            if (nested) --nested;
            else { pending=kind; return item; }
         } else if (!nested &&
            ((stop==Skip::Branches && (kind.text=="else" || kind.text=="elseif")) ||
             (stop==Skip::Labels && (kind.text=="else" || kind.text=="elseif" || kind.text=="case" || kind.text=="range")))) {
            pending=kind; return item;
         }
         // Raw scanning deliberately leaves include files unopened and all
         // skipped declarations/messages/expressions unevaluated.
      }
   }
private:
   std::vector<Frame> frames;
   QString rootDirectory;
   QStringList includePaths;
   std::optional<Token> pending;
   std::shared_ptr<SymbolTable> global=std::make_shared<SymbolTable>();
   std::function<QString(const Token &)> includeEvaluator;
   bool rawOnly=false;
   ImportControl &control;

   QString resolve(const QString &filename,const Token &directive) const {
      if (QDir::isAbsolutePath(filename)) return QDir::cleanPath(filename);
      QStringList directories{QFileInfo(directive.file).absolutePath(),rootDirectory};
      directories.append(includePaths); directories.removeDuplicates();
      for (const auto &directory:directories) {
         const auto candidate=QDir(directory).absoluteFilePath(filename);
         if (QFileInfo(candidate).isFile()) return candidate;
      }
      sourceError(directive,"Cannot locate include '"+filename+"'. Searched: "+directories.join(", "));
   }
   void pushFile(const QString &path,const Token &including) {
      QFile file(path);
      const Token origin=including.file.isEmpty() ? Token{{},1,Token::End,path} : including;
      if (!file.open(QIODevice::ReadOnly)) sourceError(origin,"Cannot open '"+path+"': "+file.errorString());
      const auto canonical=QFileInfo(file).canonicalFilePath();
      const auto identity=canonical.isEmpty() ? QFileInfo(file).absoluteFilePath() : canonical;
      for (const auto &frame:frames)
         if (frame.file==identity) sourceError(origin,"Recursive include of '"+identity+"'.");
      QByteArray encoded;
      for (;;) {
         control.check(); const auto bytes=file.read(65536);
         if (file.error()!=QFile::NoError) sourceError(origin,"Cannot read '"+path+"': "+file.errorString());
         if (bytes.isEmpty()) break;
         encoded+=bytes;
      }
      control.check();
      QStringDecoder decoder(QStringDecoder::Utf8); QString text=decoder(encoded);
      control.check();
      if (decoder.hasError()) sourceError(Token{{},1,Token::End,identity},"Source is not valid UTF-8.");
      if (text.startsWith(QChar(0xfeff))) text.remove(0,1);
      Frame frame;
      frame.text=std::move(text); frame.file=identity;
      frame.scope=including.scope ? std::make_shared<SymbolTable>(including.scope) : global;
      frames.push_back(std::move(frame));
   }
   Token raw(bool stopAtBoundary=false) {
      for (;;) {
         control.check();
         auto &frame=frames.back();
         const auto &source=frame.text; auto &cursor=frame.cursor; auto &line=frame.line;
         auto lastPoll=cursor;
         const auto scanCheck=[&] { if (cursor-lastPoll>=4096) { control.check(); lastPoll=cursor; } };
         while (cursor<source.size() && source[cursor].isSpace()) { scanCheck(); if (source[cursor]=='\n') ++line; ++cursor; }
         if (source.mid(cursor,2)=="//") {
            while (cursor<source.size() && source[cursor]!='\n') { scanCheck(); ++cursor; }
            continue;
         }
         if (source.mid(cursor,2)=="/*") {
            const Token start{{},line,Token::Symbol,frame.file,frame.scope};
            cursor+=2; int nesting=1;
            while (cursor<source.size() && nesting) {
               scanCheck();
               if (source.mid(cursor,2)=="/*") { ++nesting; cursor+=2; }
               else if (source.mid(cursor,2)=="*/") { --nesting; cursor+=2; }
               else { if (source[cursor]=='\n') ++line; ++cursor; }
            }
            if (nesting) sourceError(start,"Unterminated comment.");
            continue;
         }
         Token result{{},line,Token::End,frame.file,frame.scope};
         if (cursor>=source.size()) {
            if (frame.macro) { result.kind=Token::MacroEnd; return result; }
            if (frames.size()==1 || stopAtBoundary) return result;
            frames.pop_back(); continue;
         }
         const auto begin=cursor; const auto first=source[cursor++];
         result.offset=begin;
         if (first.isLetter() || first=='_') {
            while (cursor<source.size() && (source[cursor].isLetterOrNumber() || source[cursor]=='_')) { scanCheck(); ++cursor; }
            result.text=source.mid(begin,cursor-begin); result.kind=Token::Word;
         } else if (first.isDigit() || (first=='.' && cursor<source.size() && source[cursor].isDigit())) {
            while (cursor<source.size() && (source[cursor].isDigit() || source[cursor]=='.')) { scanCheck(); ++cursor; }
            if (cursor<source.size() && (source[cursor]=='e' || source[cursor]=='E')) {
               ++cursor;
               if (cursor<source.size() && (source[cursor]=='+' || source[cursor]=='-')) ++cursor;
               while (cursor<source.size() && source[cursor].isDigit()) { scanCheck(); ++cursor; }
            }
            result.text=source.mid(begin,cursor-begin); result.kind=Token::Number;
         } else if (first=='"') {
            bool closed=false; result.kind=Token::String;
            while (cursor<source.size()) {
               scanCheck();
               const auto character=source[cursor++];
               if (character=='"') { closed=true; break; }
               if (character=='\n') ++line;
               if (character=='\\' && cursor<source.size()) {
                  const auto escaped=source[cursor++];
                  if (escaped=='\n') ++line;
                  switch (escaped.unicode()) {
                     case 'a': result.text+=QChar(7); break;
                     case 'b': result.text+=QChar(8); break;
                     case 'f': result.text+=QChar(12); break;
                     case 'n': result.text+='\n'; break;
                     case 'r': result.text+='\r'; break;
                     case 't': result.text+='\t'; break;
                     case 'v': result.text+=QChar(11); break;
                     case 'u': {
                        if (source.size()-cursor<4) sourceError(result,"A Unicode escape needs four hexadecimal digits.");
                        unsigned code=0;
                        for (int index=0;index<4;++index) {
                           const auto digit=source[cursor++].unicode();
                           const int value=digit>='0' && digit<='9' ? digit-'0' :
                                 digit>='a' && digit<='f' ? digit-'a'+10 : digit>='A' && digit<='F' ? digit-'A'+10 : -1;
                           if (value<0) sourceError(result,"A Unicode escape needs four hexadecimal digits.");
                           code=code*16+unsigned(value);
                        }
                        result.text+=QChar(static_cast<ushort>(code)); break;
                     }
                     default: result.text+=escaped; break;
                  }
               } else result.text+=character;
            }
            if (!closed) sourceError(result,"Unterminated string.");
         } else {
            result.text=QString(first); result.kind=Token::Symbol;
            if ((first=='<' || first=='>' || first=='!') && cursor<source.size() && source[cursor]=='=')
               result.text+=source[cursor++];
         }
         return result;
      }
   }
};

bool isFinitePrimitive(const QString &kind)
{
   return kind=="box" || kind=="sphere" || kind=="cylinder" || kind=="cone" || kind=="torus";
}
bool isCsgObject(const QString &kind)
{
   return kind=="union" || kind=="intersection" || kind=="difference" || kind=="merge";
}

class Parser {
public:
   Parser(const QString &path,const QStringList &includePaths,ImportControl &control) : lexer(path,includePaths,control),control(control) {
      auto &values=lexer.globals()->values;
      values["pi"]=Binding(Tuple{3.14159265358979323846});
      values["x"]=Binding(Tuple{1,0,0}); values["y"]=Binding(Tuple{0,1,0}); values["z"]=Binding(Tuple{0,0,1});
      values["t"]=Binding(Tuple{0,0,0,1}); values["u"]=Binding(Tuple{1,0}); values["v"]=Binding(Tuple{0,1});
      values["on"]=Binding(Tuple{1}); values["off"]=Binding(Tuple{0}); values["true"]=Binding(Tuple{1}); values["false"]=Binding(Tuple{0});
      values["yes"]=Binding(Tuple{1}); values["no"]=Binding(Tuple{0});
      lexer.setIncludeEvaluator([this](const Token &origin) { return includeFilename(origin); });
      next();
   }
   Node parse();
private:
   SourceLexer lexer;
   ImportControl &control;
   int depth=0;
   bool rawIdentifier=false;
   size_t macroBoundaryFloor=0;
   enum class ControlKind { If,For,While,Switch,Macro };
   struct Control {
      Token opening;
      quint64 identity=0;
      bool taken=false,elseSeen=false;
      bool labelSeen=false;
      ControlKind kind=ControlKind::If;
      std::optional<SourceLexer::Bookmark> bookmark;
      Token replay;
      std::shared_ptr<SymbolTable> counterScope;
      QString counter;
      double bound=0,step=1;
      double selection=0;
   };
   std::vector<Control> controls;
   quint64 controlIdentity=0;
   Token token;
   [[noreturn]] void fail(const QString &message) const {
      sourceError(token,message);
   }
   void next();
   void expandToken();
   void defineMacro(const Token &origin);
   void invokeMacro();
   std::optional<Binding> macroArgument(bool optional,const std::shared_ptr<SymbolTable> &callerScope);
   void returnMacro();
   void expandArrayReference();
   ArraySlot arrayReference(bool allowMissing);
   std::optional<SymbolValue> identifierValue(bool allowMissing);
   int arrayInteger(double value,const Token &origin);
   Array arrayDeclaration();
   void arrayInitializer(const Array &array,size_t dimension,size_t base);
   SymbolValue declarationValue(bool &leftEnd);
   void prepareDirectives();
   bool take(const QString &text) { prepareDirectives(); if (token.kind==Token::String || token.text!=text) return false; next(); return true; }
   bool closeBlock(bool leaveEnd) {
      prepareDirectives();
      if (token.kind!=Token::Symbol || token.text!="}") return false;
      if (!leaveEnd) next();
      return true;
   }
   void expect(const QString &text) { if (!take(text)) fail("Expected '"+text+"', found '"+token.text+"'."); }
   QString word() { prepareDirectives(); if (token.kind!=Token::Word) fail("Expected an identifier."); const auto value=token.text; next(); return value; }
   Tuple value();
   Tuple expression();
   Tuple logical();
   Tuple relation();
   Tuple product();
   Tuple atom();
   Tuple primary();
   Tuple numericFunction();
   Tuple combine(const Tuple &left,const Tuple &right,QChar operation);
   Tuple vectorValue(size_t dimensions) {
      auto result=value();
      if (result.size()==1) result.resize(dimensions,result.front());
      else if (result.size()<dimensions) result.resize(dimensions,0.0);
      if (result.size()!=dimensions) fail("Too many vector components for this field.");
      return result;
   }
   double scalar() { const auto result=value(); if (result.size()!=1) fail("Expected a scalar."); return result[0]; }
   int count() {
      const double result=scalar();
      if (result<0 || result!=std::floor(result) || result>std::numeric_limits<int>::max())
         fail("Invalid list count.");
      return static_cast<int>(result);
   }
   osg::Vec3d vector() {
      const auto result=vectorValue(3);
      return {result[0],result[1],result[2]};
   }
   Tuple colorValue();
   QString stringValue(bool leaveEnd=false);
   bool startsString() const { return token.kind==Token::String || (token.kind==Token::Word && (isStringFunction(token.text) || symbol<QString>(token,token.text))); }
   QString includeFilename(const Token &origin);
   int stringInteger();
   QString numberString(double value,int width,int precision);
   Material materialBlock(const QString &kind,bool leaveEnd=false);
   Node object(bool leaveEnd=false,bool meshTriangle=false);
   Node primitive(bool leaveEnd=false);
   Node mesh2(bool leaveEnd=false);
   bool modifier(Model &model);
   osg::Matrixd transformBlock(bool leaveEnd=false);
   void transform(Model &model,const QString &kind,bool leaveEnd=false);
   void directive();
   bool conditionalPredicate(const QString &kind,bool repeatedWhile=false);
   void conditional(const QString &kind,const Token &origin);
   void validateControlContext(size_t count,const std::optional<Control> &owner,const Token &origin);
   void loop(const QString &kind,const Token &origin);
   void finishLoop(const Token &origin);
   void breakControl(const Token &origin);
   double loopCounter(const Control &loop) const;
   void switchBlock(const Token &origin);
   void switchLabel(const QString &kind,const Token &origin);
   void switchDefault(const Token &origin);
   void nextDirectiveToken();
   void skipRenderBlock();
};

void Parser::next()
{
   for (;;) {
      token=lexer.next();
      if (token.kind!=Token::MacroEnd) break;
      returnMacro();
   }
   if (!rawIdentifier) {
      // A final emitted token retains its own scope, while its following
      // delimiter belongs to the caller. Directive keywords remain pending
      // until directive() has read them under rawIdentifier mode.
      if (!(token.kind==Token::Symbol && token.text=="#"))
         while (controls.size()>macroBoundaryFloor && lexer.peekRawToken().kind==Token::MacroEnd) returnMacro();
      expandToken();
   }
}

void Parser::expandToken()
{
   if (token.kind==Token::Word && symbol<Macro>(token,token.text)) invokeMacro();
   else expandArrayReference();
}

void Parser::defineMacro(const Token &origin)
{
   if (controls.size()>=200) sourceError(origin,"Control directive nesting exceeds 200 levels.");
   auto macro=std::make_shared<MacroData>();
   {
      QScopedValueRollback<bool> raw(rawIdentifier,true);
      next();
      if (token.kind!=Token::Word) fail("Expected a macro name.");
      const auto name=token.text;
      if (const auto owner=symbolOwner(token.scope,name))
         if (owner->values.at(name).get() && !symbol<Macro>(token,name))
            fail("A macro cannot replace a non-macro identifier without #undef.");
      next();
      if (token.kind!=Token::Symbol || token.text!="(") fail("Expected '(' after a macro name.");
      next();
      while (!(token.kind==Token::Symbol && token.text==")")) {
         MacroData::Parameter parameter;
         if (token.kind==Token::Word && token.text=="optional") { parameter.optional=true; next(); }
         if (token.kind!=Token::Word) fail("Expected a macro parameter name.");
         parameter.name=token.text;
         if (std::any_of(macro->parameters.begin(),macro->parameters.end(),[&](const auto &p) { return p.name==parameter.name; }))
            fail("Duplicate macro parameter name.");
         macro->parameters.push_back(std::move(parameter)); next();
         if (token.kind==Token::Symbol && token.text==",") next();
         else if (token.kind!=Token::Symbol || token.text!=")") fail("Expected ',' or ')' in a macro header.");
      }
      lexer.captureMacro(*macro,origin,controls.size()+1);
      // Definitions retain text and parameter names, never their defining
      // scope. They are global, including definitions made by another macro.
      lexer.globals()->values[name]=Binding(macro);
   }
   nextDirectiveToken();
}

std::optional<Binding> Parser::macroArgument(bool optional,const std::shared_ptr<SymbolTable> &callerScope)
{
   prepareDirectives();
   const Token origin=token;
   const auto delimiter=[](const Token &item) { return item.kind==Token::Symbol && (item.text=="," || item.text==")"); };
   if (delimiter(token)) {
      if (!optional) sourceError(origin,"A required macro argument cannot be omitted.");
      return {};
   }
   if (token.kind==Token::Word && !symbol<Macro>(token,token.text)) {
      std::optional<Binding> actual;
      std::optional<ArraySlot> slot;
      auto actualOwner=origin.reference ? origin.referenceOwner : symbolOwner(origin.scope,origin.text);
      if (token.reference && !lexer.nextIsSymbol("[")) {
         actual=*token.reference; actualOwner=token.referenceOwner;
      } else if (lexer.nextIsSymbol("[")) {
         slot=arrayReference(true);
         actual=slot->array->elements[slot->index];
         actual->elementArray=slot->array;
      } else if (const auto owner=symbolOwner(origin.scope,origin.text)) actual=owner->values.at(origin.text);
      if (delimiter(lexer.peekRawToken())) {
         if (!actual || !actual->get()) {
            if (!optional) sourceError(origin,"A required macro argument must be initialized.");
            actual.reset();
         } else {
            bool callerOwned=false;
            for (auto scope=callerScope;scope;scope=scope->parent)
               if (scope==actualOwner) { callerOwned=true; break; }
            // A value emitted from a nested macro's local table cannot be a
            // reference into that shorter-lived invocation. Native parsing
            // likewise compares its table context with the calling context.
            if (!callerOwned) actual=Binding(copyValue(*actual->get(),origin,control));
         }
         QScopedValueRollback<bool> raw(rawIdentifier,true); next();
         return actual;
      }
      if (slot) {
         if (!actual->get()) sourceError(origin,"Reference to an uninitialized array element.");
         // Subscripts have already been evaluated once. Preserve that typed
         // read while parsing the rest of a by-value expression.
         auto view=std::make_shared<SymbolTable>(origin.scope);
         view->values.emplace(origin.text,Binding(*actual->get()));
         token=origin; token.scope=std::move(view);
      }
   }
   bool leftEnd=false; const auto value=declarationValue(leftEnd);
   Binding owned(copyValue(value,origin,control));
   if (leftEnd) { QScopedValueRollback<bool> raw(rawIdentifier,true); next(); }
   return owned;
}

void Parser::invokeMacro()
{
   const Token origin=token; const Macro macro=*symbol<Macro>(token,token.text);
   if (!macro) sourceError(origin,"Missing macro definition.");
   if (controls.size()>=200) sourceError(origin,"Control directive nesting exceeds 200 levels.");
   const auto count=controls.size();
   const std::optional<Control> owner=controls.empty() ? std::nullopt : std::optional<Control>(controls.back());
   std::vector<std::optional<Binding>> arguments;
   {
      // The caller's final ')' still belongs to this invocation header.
      // Only macros expanded while reading its arguments may retire here.
      QScopedValueRollback<size_t> boundary(macroBoundaryFloor,count);
      {
         QScopedValueRollback<bool> raw(rawIdentifier,true); next();
         if (token.kind!=Token::Symbol || token.text!="(") fail("Expected '(' after a macro name.");
         next();
      }
      for (size_t index=0;index<macro->parameters.size();++index) {
         if (index+1<macro->parameters.size() && token.kind==Token::Symbol && token.text==")")
            fail("The macro argument list ended before its final parameter.");
         arguments.push_back(macroArgument(macro->parameters[index].optional,origin.scope));
         if (token.kind==Token::Symbol && token.text==",") {
            QScopedValueRollback<bool> raw(rawIdentifier,true); next();
         } else if (index+1<macro->parameters.size()) fail("Expected ',' between macro arguments.");
      }
      if (token.kind!=Token::Symbol || token.text!=")") fail("Unexpected or excess macro arguments; expected ')'.");
   }
   validateControlContext(count,owner,origin);
   if (controls.size()>=200) sourceError(origin,"Control directive nesting exceeds 200 levels.");
   if (controlIdentity==std::numeric_limits<quint64>::max()) sourceError(origin,"Control identity capacity exceeded.");
   auto scope=std::make_shared<SymbolTable>(origin.scope);
   // All arguments are evaluated in the caller before formal bindings exist.
   // Missing optional arguments add no entry, matching native outer lookup.
   for (size_t index=0;index<arguments.size();++index)
      if (arguments[index]) scope->values.emplace(macro->parameters[index].name,*arguments[index]);
   Control frame; frame.kind=ControlKind::Macro; frame.opening=origin;
   frame.identity=++controlIdentity; frame.bookmark=lexer.bookmark();
   controls.push_back(std::move(frame));
   lexer.enterMacro(*macro,std::move(scope)); next();
}

void Parser::returnMacro()
{
   if (controls.empty() || controls.back().kind!=ControlKind::Macro || !controls.back().bookmark)
      fail("A macro ended with an unclosed control directive.");
   lexer.restore(*controls.back().bookmark); controls.pop_back();
}

int Parser::arrayInteger(double value,const Token &origin)
{
   // POV array dimensions/subscripts use integer conversion with its small
   // boundary allowance, rather than the mesh count/index integer policy.
   const double truncated=std::trunc(value+PovEpsilon);
   if (!std::isfinite(value) || value < -PovEpsilon || truncated<0 ||
       truncated>std::numeric_limits<int>::max())
      sourceError(origin,"Invalid array dimension or subscript.");
   return static_cast<int>(truncated);
}

ArraySlot Parser::arrayReference(bool allowMissing)
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Array reference nesting exceeds the model reader's limit.");
   const Token origin=token;
   const auto *named=symbol<Array>(origin,origin.text);
   if (!named || !*named) sourceError(origin,"Indexed identifier is not an array.");
   Array array=*named;
   for (;;) {
      size_t index=0;
      next();
      for (size_t dimension=0;dimension<array->dimensions.size();++dimension) {
         expect("["); const Token subscript=token;
         const int coordinate=arrayInteger(scalar(),subscript);
         if (token.kind!=Token::Symbol || token.text!="]") fail("Expected ']' after an array subscript.");
         if (coordinate>=array->dimensions[dimension]) {
            if (!array->resizable) sourceError(subscript,"Array subscript is out of range.");
            if (coordinate==std::numeric_limits<int>::max()) sourceError(subscript,"Dynamic array size exceeds the dimension limit.");
            // Native dynamic arrays grow on every access, including defined,
            // undef and an uninitialized read which subsequently fails.
            array->dimensions[0]=coordinate+1;
         }
         index=index*static_cast<size_t>(array->dimensions[dimension])+static_cast<size_t>(coordinate);
         if (dimension+1<array->dimensions.size()) next();
      }
      const auto element=array->elements.find(index);
      if (!lexer.nextIsSymbol("[")) {
         if (!allowMissing && (element==array->elements.end() || !element->second.get()))
            sourceError(origin,"Reference to an uninitialized array element.");
         return {array,index};
      }
      if (element==array->elements.end() || !element->second.get()) sourceError(origin,"Reference to an uninitialized nested array.");
      const auto *child=std::get_if<Array>(element->second.get());
      if (!child || !*child) sourceError(origin,"Extra subscripts require a nested array element.");
      array=*child;
   }
}

void Parser::expandArrayReference()
{
   if (token.kind!=Token::Word || !symbol<Array>(token,token.text) || !lexer.nextIsSymbol("[")) return;
   const Token origin=token;
   const auto slot=arrayReference(false);
   // A resolved read is one typed token for all existing value consumers.
   // Its closing ']' is already read, but the next token/include is untouched.
   auto view=std::make_shared<SymbolTable>(origin.scope);
   view->values.emplace(origin.text,Binding(*slot.array->elements.at(slot.index).get()));
   token=origin; token.scope=std::move(view);
   auto alias=slot.array->elements.at(slot.index); alias.elementArray=slot.array;
   token.reference=std::make_shared<Binding>(std::move(alias));
   token.referenceOwner=origin.reference ? origin.referenceOwner : symbolOwner(origin.scope,origin.text);
}

std::optional<SymbolValue> Parser::identifierValue(bool allowMissing)
{
   if (token.kind!=Token::Word) fail("Expected an identifier.");
   const Token origin=token;
   std::optional<SymbolValue> result;
   if (lexer.nextIsSymbol("[")) {
      const auto slot=arrayReference(allowMissing);
      const auto found=slot.array->elements.find(slot.index);
      if (found!=slot.array->elements.end() && found->second.get()) result=*found->second.get();
   } else if (const auto owner=symbolOwner(origin.scope,origin.text)) {
      if (const auto *value=owner->values.at(origin.text).get()) result=*value;
   }
   if (!result && !allowMissing) sourceError(origin,"Undefined identifier '"+origin.text+"'.");
   return result;
}

Array Parser::arrayDeclaration()
{
   const Token origin=token;
   auto array=std::make_shared<ArrayData>();
   // Retain the final declaration token until its value has been committed.
   // Raw structural lookahead cannot execute the next include or directive.
   const auto next=lexer.peekRawToken();
   if (next.kind==Token::Word && next.text=="mixed") { this->next(); array->mixed=true; }
   size_t total=1;
   while (lexer.nextIsSymbol("[")) {
      this->next();
      if (array->dimensions.size()==5) sourceError(origin,"An array can have at most five dimensions.");
      expect("["); const int size=arrayInteger(scalar(),origin);
      if (size==0 || total>std::numeric_limits<size_t>::max()/static_cast<size_t>(size))
         sourceError(origin,"Invalid or excessive array dimensions.");
      total*=static_cast<size_t>(size); array->dimensions.push_back(size);
      if (token.kind!=Token::Symbol || token.text!="]") fail("Expected ']' after an array dimension.");
   }
   if (array->dimensions.empty()) { array->dimensions.push_back(0); array->resizable=true; }
   const auto initializer=lexer.peekRawToken();
   if ((initializer.kind==Token::Symbol && initializer.text=="{") ||
       (initializer.kind==Token::Word && initializer.text=="optional")) {
      this->next(); arrayInitializer(array,0,0);
   }
   return array;
}

void Parser::arrayInitializer(const Array &array,size_t dimension,size_t base)
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Array initializer nesting exceeds the model reader's limit.");
   const bool optional=take("optional");
   expect("{");
   if (dimension+1<array->dimensions.size()) {
      const auto size=static_cast<size_t>(array->dimensions[dimension]);
      for (size_t index=0;index<size;++index) {
         control.check();
         arrayInitializer(array,dimension+1,base*size+index); next();
         // Native comma parsing is optional between complete row values.
         take(",");
      }
   } else {
      const auto size=static_cast<size_t>(array->dimensions[dimension]);
      bool properlyDelimited=true;
      for (size_t index=0;array->resizable || index<size;++index) {
         control.check(); prepareDirectives();
         const bool closing=token.kind==Token::Symbol && token.text=="}";
         if (array->resizable) {
            if (closing) {
               // The native parser reserves and then removes one end probe;
               // a sparse array can set the resulting size directly.
               array->dimensions[0]=static_cast<int>(index); break;
            }
            if (index>=static_cast<size_t>(std::numeric_limits<int>::max()))
               fail("Dynamic array size exceeds the dimension limit.");
            array->dimensions[0]=static_cast<int>(index+1);
         }
         const bool empty=closing || (token.kind==Token::Symbol && token.text==",");
         if (closing && (index+1!=size || !properlyDelimited))
            fail("The array initializer ended before its final position.");
         const Token origin=token;
         bool missing=empty;
         if (!missing && optional && token.kind==Token::Word && !lexer.nextIsSymbol("[")) {
            const auto owner=symbolOwner(token.scope,token.text);
            const auto following=lexer.peekRawToken();
            // Optional omission accepts an undeclared plain identifier.
            // Uninitialized indexed values still use normal read errors.
            if ((!owner || !owner->values.at(token.text).get()) &&
                following.kind==Token::Symbol && (following.text=="," || following.text=="}")) {
               missing=true; next();
            }
         }
         if (missing) {
            if (!optional) sourceError(origin,"An ordinary array initializer requires every value.");
         } else {
            bool leftEnd=false; const auto value=declarationValue(leftEnd);
            storeArrayElement(array,base*size+index,value,origin,control);
            if (leftEnd) next();
         }
         properlyDelimited=take(",");
      }
   }
   // A final comma can precede the end of a generating control/macro. Finish
   // those already active directives before checking the enclosing brace.
   prepareDirectives();
   if (token.kind!=Token::Symbol || token.text!="}") fail("Array initializer does not match its dimensions.");
}

void Parser::prepareDirectives()
{
   for (;;) {
      if (!rawIdentifier && token.kind==Token::Word && symbol<Macro>(token,token.text)) invokeMacro();
      else if (token.kind==Token::Symbol && token.text=="#") directive();
      else return;
   }
}

QString Parser::includeFilename(const Token &origin)
{
   QScopedValueRollback<Token> saved(token);
   QScopedValueRollback<size_t> boundary(macroBoundaryFloor,controls.size());
   next();
   const auto filename=stringValue(true);
   if (!lexer.atSource(origin))
      sourceError(origin,"The include filename expression must finish in its including source context.");
   return filename;
}

int Parser::stringInteger()
{
   const double result=std::trunc(scalar());
   if (result<std::numeric_limits<int>::min() || result>std::numeric_limits<int>::max())
      fail("String-function integer argument exceeds its supported range.");
   return static_cast<int>(result);
}

QString Parser::numberString(double value,int width,int precision)
{
   if (!std::isfinite(value)) fail("Cannot format a nonfinite number.");
   std::ostringstream output; output.imbue(std::locale::classic());
   output<<std::fixed<<std::setprecision(precision<0 ? 6 : precision);
   if (width<0) output<<std::internal<<std::setfill('0');
   const auto length=width<0 ? -static_cast<long long>(width) : static_cast<long long>(width);
   output.width(static_cast<std::streamsize>(length)); output<<value;
   if (!output) fail("Unable to format a number as a string.");
   return QString::fromStdString(output.str());
}

QString Parser::stringValue(bool leaveEnd)
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Nesting exceeds the static model reader's limit.");
   prepareDirectives();
   if (token.kind==Token::String) {
      const auto result=token.text; if (!leaveEnd) next(); return result;
   }
   if (token.kind!=Token::Word) fail("Expected a string expression.");
   if (const auto *declared=symbol<QString>(token,token.text)) {
      const auto result=*declared; if (!leaveEnd) next(); return result;
   }
   const auto function=word();
   if (!isStringFunction(function)) fail("Unsupported or undefined string identifier '"+function+"'.");
   expect("("); QString result;
   if (function=="concat") {
      result=stringValue(); expect(","); result+=stringValue();
      while (take(",")) result+=stringValue();
   } else if (function=="strlwr" || function=="strupr") {
      const auto input=stringValue();
      result=function=="strlwr" ? input.toLower() : input.toUpper();
   } else if (function=="substr") {
      const auto input=stringValue(); expect(",");
      const int start=stringInteger(); expect(","); const int length=stringInteger();
      if (start<1 || length<0 || qsizetype(start-1)>input.size() || qsizetype(length)>input.size()-qsizetype(start-1))
         fail("Substring position/length exceeds its source string.");
      result=input.mid(start-1,length);
   } else if (function=="chr") {
      const int code=stringInteger();
      if (code<0 || code>65535) fail("chr requires a character code from 0 to 65535 in the UTF-8 model reader.");
      result=QString(QChar(static_cast<ushort>(code)));
   } else if (function=="str") {
      const double number=scalar(); expect(","); const int width=stringInteger(); expect(",");
      const int precision=stringInteger(); result=numberString(number,width,precision);
   } else {
      const int components=std::clamp(stringInteger(),2,5); expect(",");
      const auto values=vectorValue(static_cast<size_t>(components)); expect(",");
      const auto separator=stringValue(); expect(","); const int width=stringInteger(); expect(",");
      const int precision=stringInteger();
      for (int index=0;index<components;++index) {
         if (index) result+=separator;
         result+=numberString(values[index],width,precision);
      }
   }
   if (token.kind!=Token::Symbol || token.text!=")") fail("Expected ')' after string-function arguments.");
   if (!leaveEnd) next();
   return result;
}

Tuple Parser::combine(const Tuple &left,const Tuple &right,QChar operation)
{
   const auto size=std::max(left.size(),right.size());
   Tuple result(size);
   for (size_t i=0;i<size;++i) {
      // POV scalar operands repeat in every component. Shorter vectors grow
      // with zero components instead of repeating their last component.
      const double a=left.size()==1 ? left[0] : i<left.size() ? left[i] : 0.0;
      const double b=right.size()==1 ? right[0] : i<right.size() ? right[i] : 0.0;
      if (operation=='/' && b==0.0) fail("Division by zero.");
      result[i]=operation=='+' ? a+b : operation=='-' ? a-b : operation=='*' ? a*b : a/b;
      if (!std::isfinite(result[i])) fail("Nonfinite expression result.");
   }
   return result;
}
Tuple Parser::value()
{
   auto result=product();
   for (;;) {
      prepareDirectives();
      if (token.text!="+" && token.text!="-") break;
      const auto op=token.text[0]; next(); result=combine(result,product(),op);
   }
   return result;
}
Tuple Parser::expression()
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Expression nesting exceeds the model reader's limit.");
   auto result=logical();
   if (take("?")) {
      if (result.size()!=1) fail("A conditional predicate must be a scalar.");
      const bool selected=povTrue(result.front());
      // Native scene parsing evaluates both arms, then retains the selected
      // value and its actual width. Preserve errors in either arm as well.
      auto yes=expression(); expect(":"); auto no=expression();
      result=selected ? std::move(yes) : std::move(no);
   }
   return result;
}
Tuple Parser::logical()
{
   auto result=relation();
   for (;;) {
      prepareDirectives();
      if (token.kind!=Token::Symbol || (token.text!="&" && token.text!="|")) break;
      const auto operation=token.text; next(); const auto right=relation();
      const size_t width=std::max(result.size(),right.size()); Tuple combined(width);
      for (size_t index=0;index<width;++index) {
         const bool a=povTrue(promotedComponent(result,index));
         const bool b=povTrue(promotedComponent(right,index));
         combined[index]=operation=="&" ? a && b : a || b;
      }
      result=std::move(combined);
   }
   return result;
}
Tuple Parser::relation()
{
   prepareDirectives();
   if (startsString()) {
      const auto left=terminatedString(stringValue());
      if (!isComparison(token)) fail("Expected a string comparison operator.");
      const auto operation=token.text; next();
      const auto right=terminatedString(stringValue());
      const int order=QString::compare(left,right,Qt::CaseSensitive);
      return {compareValues(operation,order,0) ? 1.0 : 0.0};
   }
   auto result=value();
   while (isComparison(token)) {
      const auto operation=token.text; next(); const auto right=value();
      const size_t width=std::max(result.size(),right.size()); Tuple compared(width);
      for (size_t index=0;index<width;++index)
         compared[index]=compareValues(operation,promotedComponent(result,index),promotedComponent(right,index));
      result=std::move(compared);
   }
   return result;
}
Tuple Parser::product()
{
   auto result=atom();
   for (;;) {
      prepareDirectives();
      if (token.text!="*" && token.text!="/") break;
      const auto op=token.text[0]; next(); result=combine(result,atom(),op);
   }
   return result;
}
Tuple Parser::atom()
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Nesting exceeds the static mesh reader's limit.");
   prepareDirectives();
   if (take("+")) return atom();
   if (take("-")) { auto result=atom(); for (auto &number:result) number=-number; return result; }
   if (take("!")) { auto result=atom(); for (auto &number:result) number=povTrue(number) ? 0.0 : 1.0; return result; }
   auto result=primary();
   while (take(".")) {
      const Token origin=token;
      const auto component=word();
      int index=-1;
      if (component=="x" || component=="u" || component=="red") index=0;
      else if (component=="y" || component=="v" || component=="green") index=1;
      else if (component=="z" || component=="blue") index=2;
      else if (component=="t" || component=="filter") index=3;
      else if (component=="transmit") index=4;
      if (index<0) sourceError(origin,"Unsupported vector/color component '"+component+"'.");
      // Use the same scalar broadcast and shorter-vector zero extension as
      // other numeric vector operands before extracting a component.
      const double selected=result.size()==1 ? result.front() :
         static_cast<size_t>(index)<result.size() ? result[index] : 0.0;
      result={selected};
   }
   return result;
}

Tuple Parser::primary()
{
   if (take("(")) { auto result=expression(); expect(")"); return result; }
   if (take("<")) {
      Tuple result;
      do { result.push_back(scalar()); if (result.size()>5) fail("Too many vector components."); } while (take(","));
      expect(">"); return result;
   }
   if (token.kind==Token::Number) {
      bool valid=false; const auto number=token.text.toDouble(&valid);
      if (!valid || !std::isfinite(number)) fail("Invalid finite number.");
      next(); return {number};
   }
   if (token.kind==Token::Word) {
      if (isNumericFunction(token.text)) return numericFunction();
      const auto *found=symbol<Tuple>(token,token.text);
      if (!found) fail("Unsupported or undefined numeric identifier '"+token.text+"'.");
      auto result=*found; next(); return result;
   }
   fail("Expected a static number or vector.");
}

Tuple Parser::numericFunction()
{
   const Token origin=token;
   const auto function=word();
   if (function=="defined" || function=="dimensions" || function=="dimension_size") {
      {
         // defined must be able to inspect an uninitialized array element.
         // Read its root as an identifier before ordinary typed expansion.
         QScopedValueRollback<bool> raw(rawIdentifier,true);
         expect("(");
      }
      const auto identifier=identifierValue(function=="defined");
      next();
      double result=identifier ? 1.0 : 0.0;
      if (function!="defined") {
         const auto *array=identifier ? std::get_if<Array>(&*identifier) : nullptr;
         if (!array || !*array) sourceError(origin,function+" requires an array identifier.");
         result=static_cast<double>((*array)->dimensions.size());
         if (function=="dimension_size") {
            expect(","); const int dimension=arrayInteger(scalar(),origin);
            if (dimension<1 || static_cast<size_t>(dimension)>(*array)->dimensions.size())
               sourceError(origin,"Array dimensions are numbered from one through the array's rank.");
            result=(*array)->dimensions[dimension-1];
         }
      }
      expect(")"); return {result};
   }
   expect("(");
   if (function=="strcmp" || function=="strlen") {
      const auto left=terminatedString(stringValue()); double result=left.size();
      if (function=="strcmp") {
         expect(","); const auto right=terminatedString(stringValue());
         result=QString::compare(left,right,Qt::CaseSensitive);
      }
      expect(")"); return {result};
   }
   std::vector<Tuple> arguments;
   if (!take(")")) {
      do { arguments.push_back(value()); } while (take(","));
      expect(")");
   }
   const auto arity=[&](size_t count) {
      if (arguments.size()!=count)
         sourceError(origin,function+" requires "+QString::number(count)+" arguments.");
   };
   const auto number=[&](size_t index) {
      if (arguments[index].size()!=1)
         sourceError(origin,function+" requires scalar arguments.");
      return arguments[index].front();
   };
   const auto vectorArgument=[&](size_t index) {
      const auto &tuple=arguments[index];
      if (tuple.size()>3)
         sourceError(origin,function+" requires three-component vectors.");
      osg::Vec3d result;
      for (size_t axis=0;axis<3;++axis)
         result[axis]=tuple.size()==1 ? tuple.front() : axis<tuple.size() ? tuple[axis] : 0.0;
      return result;
   };
   const auto unit=[&](osg::Vec3d vector) {
      const double scale=std::max({std::abs(vector.x()),std::abs(vector.y()),std::abs(vector.z())});
      if (scale==0.0) sourceError(origin,function+" requires a nonzero direction.");
      vector/=scale;
      vector/=std::hypot(vector.x(),vector.y(),vector.z());
      return vector;
   };
   constexpr double radians=3.14159265358979323846/180.0;
   Tuple result;
   if (function=="select") {
      if (arguments.size()!=3 && arguments.size()!=4)
         sourceError(origin,"select requires three or four scalar arguments.");
      for (size_t index=0;index<arguments.size();++index) number(index);
      const double predicate=number(0);
      const size_t selected=predicate<0.0 ? 1U : arguments.size()==3 || predicate==0.0 ? 2U : 3U;
      result={number(selected)};
   } else if (isVectorFunction(function) || function=="vdot" || function=="vlength") {
      arity(function=="vnormalize" || function=="vlength" ? 1U : function=="vaxis_rotate" ? 3U : 2U);
      const auto first=vectorArgument(0);
      if (function=="vlength") result={std::hypot(first.x(),first.y(),first.z())};
      else if (function=="vdot") result={first*vectorArgument(1)};
      else {
         osg::Vec3d value;
         if (function=="vnormalize") value=unit(first);
         else if (function=="vcross") value=first^vectorArgument(1);
         else if (function=="vaxis_rotate")
            value=first*osg::Matrixd::rotate(number(2)*radians,unit(vectorArgument(1)));
         else {
            const auto angles=vectorArgument(1);
            value=first*(osg::Matrixd::rotate(angles.x()*radians,osg::Vec3d(1,0,0))*
               osg::Matrixd::rotate(angles.y()*radians,osg::Vec3d(0,1,0))*
               osg::Matrixd::rotate(angles.z()*radians,osg::Vec3d(0,0,1)));
         }
         result={value.x(),value.y(),value.z()};
      }
   } else if (function=="min" || function=="max") {
      if (arguments.size()<2) sourceError(origin,function+" requires at least two arguments.");
      double value=number(0);
      for (size_t index=1;index<arguments.size();++index)
         value=function=="min" ? std::min(value,number(index)) : std::max(value,number(index));
      result={value};
   } else if (function=="atan2" || function=="div" || function=="mod" || function=="pow") {
      arity(2); const double a=number(0),b=number(1);
      if ((function=="div" || function=="mod") && b==0.0)
         sourceError(origin,"Division by zero in "+function+".");
      if (function=="atan2") result={std::atan2(a,b)};
      else if (function=="div") result={std::trunc(a/b)};
      else if (function=="mod") result={std::fmod(a,b)};
      else {
         if (a==0.0 && b==0.0) sourceError(origin,"Invalid zero-to-zero power in pow.");
         result={std::pow(a,b)};
      }
   } else {
      arity(1); const double a=number(0); double value=0;
      if (function=="abs") value=std::abs(a);
      else if (function=="acos") value=std::acos(a);
      else if (function=="acosh") value=std::acosh(a);
      else if (function=="asin") value=std::asin(a);
      else if (function=="asinh") value=std::asinh(a);
      else if (function=="atan") value=std::atan(a);
      else if (function=="atanh") value=std::atanh(a);
      else if (function=="ceil") value=std::ceil(a);
      else if (function=="cos") value=std::cos(a);
      else if (function=="cosh") value=std::cosh(a);
      else if (function=="degrees") value=a/radians;
      else if (function=="exp") value=std::exp(a);
      else if (function=="floor") value=std::floor(a);
      else if (function=="int") value=std::trunc(a);
      else if (function=="ln") value=std::log(a);
      else if (function=="log") value=std::log10(a);
      else if (function=="radians") value=a*radians;
      else if (function=="sin") value=std::sin(a);
      else if (function=="sinh") value=std::sinh(a);
      else if (function=="sqrt") value=std::sqrt(a);
      else if (function=="tan") value=std::tan(a);
      else if (function=="tanh") value=std::tanh(a);
      else sourceError(origin,"Unsupported numeric function '"+function+"'.");
      result={value};
   }
   for (const double value:result)
      if (!std::isfinite(value)) sourceError(origin,"Invalid domain or nonfinite result in "+function+".");
   return result;
}

Tuple Parser::colorValue()
{
   take("color");
   QString kind;
   if (token.text=="rgb" || token.text=="rgbf" || token.text=="rgbt" || token.text=="rgbft") { kind=token.text; next(); }
   auto channels=value();
   if (kind.isEmpty() && channels.size()==5) return channels;
   if (kind.isEmpty()) kind="rgb";
   const size_t needed=kind=="rgb" ? 3 : kind=="rgbft" ? 5 : 4;
   if (channels.size()==1) channels.resize(needed,channels.front());
   if (channels.size()!=needed) fail("Wrong component count for "+kind+".");
   if (needed==3) channels.resize(5,0.0);
   else if (kind=="rgbf") channels.push_back(0.0);
   else if (kind=="rgbt") channels.insert(channels.begin()+3,0.0);
   return channels;
}

Material Parser::materialBlock(const QString &kind,bool leaveEnd)
{
   expect(kind); expect("{"); Material result;
   prepareDirectives();
   if (token.kind==Token::Word) {
      if (const auto *named=symbol<MaterialSymbol>(token,token.text)) {
         if (named->first!=kind && !(kind=="texture" && (named->first=="pigment" || named->first=="finish")))
            fail("Material identifier has a different declaration type.");
         result=named->second; next();
      }
   }
   while (!closeBlock(leaveEnd)) {
      if (token.kind==Token::End) fail("Unterminated material.");
      const auto *namedComponent=token.kind==Token::Word ? symbol<MaterialSymbol>(token,token.text) : nullptr;
      if (kind=="texture" && (token.text=="pigment" || token.text=="finish")) {
         const auto part=token.text; result.overlay(materialBlock(part));
      } else if (kind=="texture" && namedComponent &&
                 (namedComponent->first=="pigment" || namedComponent->first=="finish")) {
         result.overlay(namedComponent->second); next();
      } else if (kind=="pigment") {
         const auto c=colorValue();
         result.color=osg::Vec4(float(c[0]),float(c[1]),float(c[2]),float(std::clamp(1.0-c[3]-c[4],0.0,1.0)));
      } else if (kind=="finish") {
         const auto property=word(); const auto number=scalar();
         if (property=="ambient") result.ambient=float(number);
         else if (property=="diffuse") result.diffuse=float(number);
         else if (property=="phong" || property=="specular") result.specular=float(number);
         else if (property=="phong_size") result.power=float(number);
         else if (property=="roughness" && number>0) result.power=float(std::clamp(2.0/(number*number)-2.0,0.0,128.0));
         else fail("Unsupported finish property '"+property+"'.");
      } else fail("Unsupported texture construct '"+token.text+"'.");
   }
   return result;
}

void Parser::transform(Model &model,const QString &kind,bool leaveEnd)
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Nesting exceeds the static mesh reader's limit.");
   expect(kind); osg::Matrixd matrix;
   if (kind=="transform") {
      expect("{");
      prepareDirectives();
      if (token.kind==Token::Word) if (const auto *named=symbol<osg::Matrixd>(token,token.text)) { matrix=*named; next(); }
      while (!closeBlock(leaveEnd)) {
         Model item;
         if (token.text!="translate" && token.text!="rotate" && token.text!="scale" && token.text!="matrix" && token.text!="transform")
            fail("Unsupported transform '"+token.text+"'.");
         const auto operation=token.text; transform(item,operation); matrix*=item.matrix;
      }
   } else if (kind=="matrix") {
      expect("<"); std::array<double,12> entries;
      for (size_t i=0;i<entries.size();++i) { if (i) expect(","); entries[i]=scalar(); }
      expect(">");
      matrix.set(entries[0],entries[1],entries[2],0,entries[3],entries[4],entries[5],0,
                 entries[6],entries[7],entries[8],0,entries[9],entries[10],entries[11],1);
   } else {
      const auto values=vectorValue(3);
      osg::Vec3d v(values[0],values[1],values[2]);
      if (kind=="translate") matrix=osg::Matrixd::translate(v);
      else if (kind=="scale") {
         // POV uses zero here to mean no scaling along that coordinate.
         for (int axis=0;axis<3;++axis) if (v[axis]==0) v[axis]=1;
         matrix=osg::Matrixd::scale(v);
      } else {
         constexpr double radians=3.14159265358979323846/180.0;
         matrix=osg::Matrixd::rotate(v.x()*radians,osg::Vec3d(1,0,0))*
                osg::Matrixd::rotate(v.y()*radians,osg::Vec3d(0,1,0))*
                osg::Matrixd::rotate(v.z()*radians,osg::Vec3d(0,0,1));
      }
   }
   osg::Matrixd inverse;
   if (!inverse.invert(matrix)) fail("A model transform must be nonsingular.");
   for (int row=0;row<4;++row) for (int column=0;column<4;++column)
      if (!std::isfinite(inverse(row,column))) fail("A model transform exceeds display precision.");
   model.matrix*=matrix;
   for (int row=0;row<4;++row) for (int column=0;column<4;++column)
      if (!std::isfinite(model.matrix(row,column))) fail("Nonfinite model transform.");
}

bool Parser::modifier(Model &model)
{
   const auto kind=token.text;
   if (kind=="translate" || kind=="rotate" || kind=="scale" || kind=="matrix" || kind=="transform") {
      transform(model,kind); return true;
   }
   if (kind=="texture" || kind=="pigment" || kind=="finish") {
      if (kind=="texture" && model.textureLayer) fail("Layered textures are unsupported.");
      if (!model.textureLayer) model.material=displayMaterial();
      model.material.overlay(materialBlock(kind));
      model.textureLayer=true;
      return true;
   }
   if (kind=="uv_mapping" || kind=="double_illuminate" || kind=="no_shadow" || kind=="hierarchy") {
      next();
      const auto *number=symbol<Tuple>(token,token.text);
      if (token.kind==Token::Number || (number && number->size()==1) ||
          (token.kind==Token::Symbol && (token.text=="(" || token.text=="!" || token.text=="+" || token.text=="-")) ||
          (token.kind==Token::Word && isNumericFunction(token.text))) scalar();
      return true;
   }
   if (kind=="uv_vectors" && model.faces.size()==1) {
      next();
      for (int i=0;i<3;++i) { if (i) expect(","); vectorValue(2); }
      return true;
   }
   if (kind=="inverse") {
      if (model.kind==Model::Kind::Patch && !model.instance)
         fail("Inverse requires a supported closed solid, not an individual triangle patch.");
      model.inverse=!model.inverse; next(); return true;
   }
   if (kind=="inside_vector") {
      next(); const auto direction=vector();
      if (std::hypot(direction.x(),direction.y(),direction.z())==0)
         fail("An inside_vector must be nonzero.");
      model.insideVector=direction; return true;
   }
   return false;
}

Node Parser::object(bool leaveEnd,bool meshTriangle)
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Nesting exceeds the static mesh reader's limit.");
   prepareDirectives();
   if (token.kind!=Token::Word) fail("Expected an object type or identifier.");
   const auto kind=token.text; const int sourceLine=token.line; const auto sourceFile=token.file;
   if (const auto *named=symbol<Node>(token,kind)) {
      auto result=std::make_shared<Model>(); const auto prototype=*named;
      result->file=sourceFile; result->line=sourceLine;
      result->children.push_back(prototype); result->instance=true;
      result->material=prototype->material; result->textureLayer=prototype->textureLayer;
      if (!leaveEnd) next(); return result;
   }
   if (kind=="mesh2") return mesh2(leaveEnd);
   if (isFinitePrimitive(kind)) return primitive(leaveEnd);
   if (kind!="triangle" && kind!="smooth_triangle" && kind!="mesh" && !isCsgObject(kind) && kind!="object")
      fail("Unsupported object '"+kind+"'; use static meshes or supported finite primitives.");
   next(); expect("{"); auto result=std::make_shared<Model>();
   result->file=sourceFile; result->line=sourceLine;
   if (kind=="mesh") result->kind=Model::Kind::Mesh;
   else if (kind=="union") result->kind=Model::Kind::Union;
   else if (kind=="intersection") result->kind=Model::Kind::Intersection;
   else if (kind=="difference") result->kind=Model::Kind::Difference;
   else if (kind=="merge") result->kind=Model::Kind::Merge;
   if (kind=="triangle" || kind=="smooth_triangle") {
      Triangle face; face.smooth=kind=="smooth_triangle"; face.line=sourceLine; face.file=sourceFile;
      for (int i=0;i<3;++i) {
         if (i) expect(","); face.vertices[i]=vector();
         if (face.smooth) { expect(","); face.normals[i]=vector(); }
      }
      result->faces.push_back(face);
   } else if (kind=="object") result=object();
   while (!closeBlock(leaveEnd)) {
      if (token.kind==Token::End) fail("Unterminated object.");
      if (meshTriangle && token.text=="texture_list") {
         if (result->textureLayer) fail("A mesh triangle cannot combine corner textures with another material statement.");
         expect("texture_list"); expect("{");
         auto corners=std::make_shared<std::array<Material,3>>();
         for (int corner=0;corner<3;++corner) {
            if (corner) take(",");
            prepareDirectives();
            const auto *named=token.kind==Token::Word ? symbol<MaterialSymbol>(token,token.text) : nullptr;
            if (!named || named->first!="texture") fail("A mesh triangle texture list needs three declared texture identifiers.");
            (*corners)[corner]=displayMaterial(); (*corners)[corner].overlay(named->second);
            next();
         }
         expect("}");
         result->faces.front().cornerMaterials=std::move(corners);
         result->textureLayer=true;
         continue;
      }
      if (meshTriangle && result->faces.front().cornerMaterials &&
          (token.text=="texture" || token.text=="pigment" || token.text=="finish"))
         fail("A mesh triangle cannot combine corner textures with another material statement.");
      if (modifier(*result)) continue;
      if (token.text=="#") { directive(); continue; }
      if (isCsgObject(kind)) result->children.push_back(object());
      else if (kind=="mesh" && (token.text=="triangle" || token.text=="smooth_triangle")) {
         auto child=object(false,true);
         if (!child->matrix.isIdentity()) fail("Transforms belong on the mesh, not an individual mesh triangle.");
         for (auto face:child->faces) {
            control.check(); face.material.overlay(child->material); result->faces.push_back(std::move(face));
         }
      } else fail("Unsupported object modifier '"+token.text+"'.");
   }
   if (result->faces.empty() && result->children.empty() && result->kind!=Model::Kind::Empty)
      sourceError(Token{{},sourceLine,Token::Word,sourceFile},"Empty mesh object.");
   return result;
}

Node Parser::primitive(bool leaveEnd)
{
   const auto kind=token.text; const int sourceLine=token.line; const auto sourceFile=token.file;
   next(); expect("{");
   osg::Vec3d first,second;
   double firstRadius=0.0,secondRadius=0.0;
   QString spindleMode="union";
   bool radiusInverse=false;
   if (kind=="torus") {
      firstRadius=scalar(); expect(","); secondRadius=scalar();
      radiusInverse=(firstRadius<0)!=(secondRadius<0);
      if (firstRadius<0 || secondRadius<0)
         qWarning().noquote()<<sourceLocation(sourceFile,sourceLine)+": Signed torus radii toggle inversion; magnitudes set the dimensions.";
      firstRadius=std::abs(firstRadius); secondRadius=std::abs(secondRadius);
      if (!std::isfinite(firstRadius+secondRadius) || firstRadius+secondRadius>std::numeric_limits<float>::max())
         fail("Torus dimensions exceed display precision.");
      prepareDirectives();
      if (isCsgObject(token.text)) { spindleMode=token.text; next(); }
   } else {
      first=vector(); expect(",");
      if (kind=="sphere") {
         firstRadius=scalar();
         if (firstRadius<=0) fail("A sphere radius must be positive.");
      } else if (kind=="box") {
         second=vector();
         for (int axis=0;axis<3;++axis)
            if (first[axis]==second[axis]) fail("A box must have nonzero extent on every axis.");
      } else if (kind=="cylinder") {
         second=vector(); expect(","); firstRadius=secondRadius=scalar();
         if (firstRadius<=0) fail("A cylinder radius must be positive.");
      } else {
         firstRadius=scalar(); expect(","); second=vector(); expect(","); secondRadius=scalar();
         if (firstRadius<0 || secondRadius<0 || (firstRadius==0 && secondRadius==0))
            fail("Cone radii must be nonnegative, with at least one positive radius.");
      }
   }
   auto result=std::make_shared<Model>(); bool open=false;
   result->kind=Model::Kind::Primitive; result->file=sourceFile; result->line=sourceLine;
   result->inverse=radiusInverse;
   while (!closeBlock(leaveEnd)) {
      if (token.kind==Token::End) fail("Unterminated primitive.");
      if (token.text=="open" && (kind=="cylinder" || kind=="cone")) { open=true; next(); continue; }
      if (kind=="torus" && token.text=="sturm") {
         // This selects a ray-root solver in POV-Ray; display tessellation
         // has no polynomial root-solving mode, but consumes its optional flag.
         next(); const auto *number=symbol<Tuple>(token,token.text);
         if (token.kind==Token::Number || (number && number->size()==1) ||
             (token.kind==Token::Symbol && (token.text=="(" || token.text=="!" || token.text=="+" || token.text=="-")) ||
             (token.kind==Token::Word && isNumericFunction(token.text))) scalar();
         continue;
      }
      if (modifier(*result)) continue;
      if (token.text=="#") { directive(); continue; }
      fail("Unsupported "+kind+" modifier '"+token.text+"'.");
   }
   result->closed=!open;
   auto flat=[&](const osg::Vec3d &a,const osg::Vec3d &b,const osg::Vec3d &c) {
      Triangle face; face.line=sourceLine; face.file=sourceFile; face.vertices={a,b,c};
      result->faces.push_back(std::move(face));
   };
   auto smooth=[&](const osg::Vec3d &a,const osg::Vec3d &b,const osg::Vec3d &c,
                   const osg::Vec3d &na,const osg::Vec3d &nb,const osg::Vec3d &nc) {
      Triangle face; face.line=sourceLine; face.file=sourceFile; face.vertices={a,b,c};
      face.normals={na,nb,nc}; face.smooth=true;
      result->faces.push_back(std::move(face));
   };
   if (kind=="box") {
      std::array<osg::Vec3d,8> corners;
      for (int index=0;index<8;++index) for (int axis=0;axis<3;++axis)
         corners[index][axis]=(index&(1<<axis)) ? std::max(first[axis],second[axis]) : std::min(first[axis],second[axis]);
      // Each quad is counterclockwise as seen from outside in model space.
      constexpr int faces[6][4]={{0,4,6,2},{1,3,7,5},{0,1,5,4},
                                  {2,6,7,3},{0,2,3,1},{4,5,7,6}};
      for (const auto &face:faces) {
         control.check();
         flat(corners[face[0]],corners[face[1]],corners[face[2]]);
         flat(corners[face[0]],corners[face[2]],corners[face[3]]);
      }
      return result;
   }
   // Display tessellation is deterministic. Smooth normals come from the
   // analytic surface; native model transforms and instance reuse stay shared
   // with imported triangle meshes. Caps retain separate flat normals.
   constexpr int slices=64,stacks=32;
   constexpr double pi=3.14159265358979323846;
   if (kind=="torus") {
      const double major=firstRadius,minor=secondRadius;
      const bool spindle=minor>major;
      if (minor==0 || (!spindle && spindleMode=="intersection") ||
          (major==0 && spindleMode=="difference")) {
         result->kind=Model::Kind::Empty;
         if (!spindle && spindleMode=="intersection")
            qWarning().noquote()<<sourceLocation(sourceFile,sourceLine)+": Intersection spindle mode has no surface for this torus.";
         return result;
      }
      const double ratio=minor>=major ? major/minor : 1.0;
      const double tipY=minor*std::sqrt(std::max(0.0,(1-ratio)*(1+ratio)));
      // Revolve a circular section about Y. Splitting at exact axis tips
      // avoids collapsed or inverted strips in horn and spindle variants.
      auto sheet=[&](double center,double limit,bool periodic) {
         std::vector<Triangle> faces;
         struct Corner { osg::Vec3d point,normal; };
         auto corner=[&](int slice,int stack) {
            const double theta=2*pi*(slice%slices)/slices;
            const osg::Vec3d radial(std::cos(theta),0,std::sin(theta));
            if (!periodic && (stack==0 || stack==stacks)) {
               const double sign=stack==0 ? -1.0 : 1.0;
               const auto n=tipY>0 ? osg::Vec3d(0,sign,0) : radial*(-center/minor);
               return Corner{osg::Vec3d(0,sign*tipY,0),n};
            }
            const int section=periodic ? stack%stacks : stack;
            const double phi=-limit+2*limit*section/stacks;
            const double cosine=periodic && section==0 ? -1.0 : std::cos(phi);
            const double sine=periodic && section==0 ? 0.0 : std::sin(phi);
            return Corner{radial*(center+minor*cosine)+osg::Vec3d(0,minor*sine,0),
                          radial*cosine+osg::Vec3d(0,sine,0)};
         };
         auto triangle=[&](const Corner &a,const Corner &b,const Corner &c) {
            Triangle face; face.file=sourceFile; face.line=sourceLine; face.smooth=true;
            face.vertices={a.point,b.point,c.point}; face.normals={a.normal,b.normal,c.normal};
            faces.push_back(std::move(face));
         };
         for (int stack=0;stack<stacks;++stack) for (int slice=0;slice<slices;++slice) {
            control.check();
            const auto a=corner(slice,stack),b=corner(slice+1,stack);
            const auto c=corner(slice+1,stack+1),d=corner(slice,stack+1);
            if (periodic || stack>0) triangle(a,d,b);
            if (periodic || stack+1<stacks) triangle(b,d,c);
         }
         return faces;
      };
      auto outer=sheet(major,spindle ? std::acos(-ratio) : pi,major>minor);
      if (!spindle || major==0 || spindleMode=="merge") {
         result->faces=std::move(outer); return result;
      }
      auto inner=sheet(-major,std::acos(ratio),false);
      if (spindleMode=="intersection") { result->faces=std::move(inner); return result; }
      if (spindleMode=="union")
         result->solidFaces=std::make_shared<const std::vector<Triangle>>(outer);
      else for (auto &face:inner) {
         // The spindle is outside a difference's solid, while its authored
         // shading normal convention remains directed out of the spindle.
         std::swap(face.vertices[1],face.vertices[2]);
         std::swap(face.normals[1],face.normals[2]);
      }
      result->faces=std::move(outer);
      result->faces.insert(result->faces.end(),std::make_move_iterator(inner.begin()),std::make_move_iterator(inner.end()));
      return result;
   }
   if (kind=="sphere") {
      auto direction=[&](int stack,int slice) {
         if (stack==0) return osg::Vec3d(0,1,0);
         if (stack==stacks) return osg::Vec3d(0,-1,0);
         const double theta=pi*stack/stacks,phi=2*pi*(slice%slices)/slices;
         return osg::Vec3d(std::sin(theta)*std::cos(phi),std::cos(theta),std::sin(theta)*std::sin(phi));
      };
      for (int stack=0;stack<stacks;++stack) for (int slice=0;slice<slices;++slice) {
         control.check();
         const auto a=direction(stack,slice),b=direction(stack+1,slice);
         const auto c=direction(stack+1,slice+1),d=direction(stack,slice+1);
         // Do not emit collapsed triangles at either exact pole.
         if (stack>0) smooth(first+a*firstRadius,first+d*firstRadius,first+b*firstRadius,a,d,b);
         if (stack+1<stacks) smooth(first+d*firstRadius,first+c*firstRadius,first+b*firstRadius,d,c,b);
      }
      return result;
   }
   const auto displacement=second-first;
   const double height=std::hypot(displacement.x(),displacement.y(),displacement.z());
   if (!std::isfinite(height) || height==0)
      sourceError(Token{{},sourceLine,Token::Word,sourceFile},"A cylinder or cone needs distinct finite endpoints.");
   const auto axis=displacement/height;
   // Choose the least aligned coordinate axis to avoid an unstable basis.
   int referenceAxis=0;
   if (std::abs(axis.y())<std::abs(axis[referenceAxis])) referenceAxis=1;
   if (std::abs(axis.z())<std::abs(axis[referenceAxis])) referenceAxis=2;
   osg::Vec3d reference(0,0,0); reference[referenceAxis]=1;
   auto u=axis^reference; u/=std::hypot(u.x(),u.y(),u.z());
   const auto v=axis^u;
   const double radiusDifference=firstRadius-secondRadius;
   const double slant=std::hypot(height,radiusDifference);
   if (!std::isfinite(slant))
      sourceError(Token{{},sourceLine,Token::Word,sourceFile},"Primitive dimensions exceed display precision.");
   auto radial=[&](double slice) {
      const double angle=2*pi*slice/slices;
      return u*std::cos(angle)+v*std::sin(angle);
   };
   auto sideNormal=[&](const osg::Vec3d &direction) {
      return direction*(height/slant)+axis*(radiusDifference/slant);
   };
   for (int slice=0;slice<slices;++slice) {
      control.check();
      const auto r0=radial(slice),r1=radial((slice+1)%slices);
      const auto a=first+r0*firstRadius,d=first+r1*firstRadius;
      const auto b=second+r0*secondRadius,c=second+r1*secondRadius;
      const auto n0=sideNormal(r0),n1=sideNormal(r1),tipNormal=sideNormal(radial(slice+0.5));
      // A cone tip has one surface triangle, not a degenerate second face.
      if (firstRadius>0) smooth(a,d,b,n0,n1,secondRadius>0 ? n0 : tipNormal);
      if (secondRadius>0) smooth(d,c,b,firstRadius>0 ? n1 : tipNormal,n1,n0);
      if (!open) {
         if (firstRadius>0) flat(first,d,a);
         if (secondRadius>0) flat(second,b,c);
      }
   }
   return result;
}

Node Parser::mesh2(bool leaveEnd)
{
   const Token origin=token;
   const int sourceLine=token.line; const auto sourceFile=token.file; expect("mesh2"); expect("{");
   auto result=std::make_shared<Model>();
   result->kind=Model::Kind::Mesh; result->file=sourceFile; result->line=sourceLine;
   std::vector<osg::Vec3d> vertices,normals;
   std::vector<Tuple> uv;
   using Indices=std::array<int,3>;
   struct Face { Indices vertices; std::optional<Indices> textures; };
   std::vector<Face> faces;
   std::vector<Indices> normalIndices,uvIndices;
   std::vector<Material> textures;
   bool haveVertices=false,haveNormals=false,haveUv=false,haveFaces=false;
   bool haveNormalIndices=false,haveUvIndices=false,haveTextures=false;
   auto indexTuple=[&](const Tuple &tuple) {
      if (tuple.size()!=3) fail("An index tuple needs three components.");
      Indices result;
      for (int i=0;i<3;++i) {
         if (tuple[i]<0 || tuple[i]!=std::floor(tuple[i]) || tuple[i]>std::numeric_limits<int>::max()) fail("Invalid mesh index.");
         result[i]=static_cast<int>(tuple[i]);
      }
      return result;
   };
   auto indices=[&]() { return indexTuple(value()); };
   auto textureIndex=[&](const Tuple &tuple) {
      if (tuple.size()!=1 || tuple[0]<0 || tuple[0]!=std::floor(tuple[0]) ||
          tuple[0]>std::numeric_limits<int>::max()) fail("Invalid texture index.");
      return static_cast<int>(tuple[0]);
   };
   while (!closeBlock(leaveEnd)) {
      const auto part=token.text;
      if (part=="vertex_vectors" || part=="normal_vectors" || part=="uv_vectors") {
         if ((part=="vertex_vectors" && haveVertices) || (part=="normal_vectors" && haveNormals) || (part=="uv_vectors" && haveUv))
            fail("Duplicate mesh vector list.");
         next(); expect("{"); const int size=count(); take(",");
         for (int i=0;i<size;++i) {
            const auto tuple=vectorValue(part=="uv_vectors" ? 2U : 3U);
            if (part=="uv_vectors") uv.push_back(tuple);
            else (part=="vertex_vectors" ? vertices : normals).emplace_back(tuple[0],tuple[1],tuple[2]);
            if (i+1<size) expect(","); else take(",");
         }
         expect("}"); if (part=="vertex_vectors") haveVertices=true;
         if (part=="normal_vectors") haveNormals=true;
         if (part=="uv_vectors") haveUv=true;
      } else if (part=="texture_list") {
         if (haveTextures) fail("Duplicate texture list.");
         next(); expect("{"); const int size=count(); take(",");
         for (int i=0;i<size;++i) { textures.push_back(materialBlock("texture")); take(","); }
         expect("}"); haveTextures=true;
      } else if (part=="face_indices" || part=="normal_indices" || part=="uv_indices") {
         if ((part=="face_indices" && haveFaces) || (part=="normal_indices" && haveNormalIndices) || (part=="uv_indices" && haveUvIndices))
            fail("Duplicate mesh index list.");
         next(); expect("{"); const int size=count(); take(",");
         std::optional<Indices> pendingFace;
         for (int i=0;i<size;++i) {
            const auto entry=pendingFace ? *pendingFace : indices();
            pendingFace.reset();
            if (part=="face_indices") {
               Face face{entry,{}};
               if (take(",") && token.text!="}") {
                  // Parse an expression once to distinguish the next face from
                  // a texture selector. Its first token need not reveal its
                  // type (a vector function can also have a scalar .x suffix).
                  auto candidate=value();
                  if (candidate.size()==3) pendingFace=indexTuple(candidate);
                  else {
                     const int texture=textureIndex(candidate);
                     face.textures=Indices{texture,texture,texture};
                     if (take(",") && token.text!="}") {
                        candidate=value();
                        if (candidate.size()==3) pendingFace=indexTuple(candidate);
                        else {
                           const int second=textureIndex(candidate); expect(",");
                           const int third=count(); face.textures=Indices{texture,second,third};
                           take(",");
                        }
                     }
                  }
               }
               faces.push_back(face);
            } else { (part=="normal_indices" ? normalIndices : uvIndices).push_back(entry); take(","); }
         }
         if (pendingFace) fail("Too many face index entries.");
         expect("}");
         if (part=="face_indices") haveFaces=true;
         if (part=="normal_indices") haveNormalIndices=true;
         if (part=="uv_indices") haveUvIndices=true;
      } else if (!modifier(*result)) fail("Unsupported mesh2 field '"+part+"'.");
   }
   if (!haveVertices || !haveFaces || vertices.empty() || faces.empty()) sourceError(origin,"mesh2 needs vertices and faces.");
   if (normalIndices.size()>faces.size() || uvIndices.size()>faces.size()) sourceError(origin,"Too many normal or UV index entries.");
   if (!haveNormalIndices && !normals.empty() && normals.size()!=vertices.size()) sourceError(origin,"Explicit normal indices are required for this mesh.");
   for (size_t i=0;i<faces.size();++i) {
      control.check();
      Triangle face; face.line=sourceLine; face.file=sourceFile;
      for (int corner=0;corner<3;++corner) {
         const int index=faces[i].vertices[corner];
         if (size_t(index)>=vertices.size()) sourceError(origin,"Vertex index exceeds its list.");
         face.vertices[corner]=vertices[index];
      }
      if (i<normalIndices.size() || (!haveNormalIndices && !normals.empty())) {
         face.smooth=true; const auto &list=i<normalIndices.size() ? normalIndices[i] : faces[i].vertices;
         for (int corner=0;corner<3;++corner) {
            if (size_t(list[corner])>=normals.size()) sourceError(origin,"Normal index exceeds its list.");
            face.normals[corner]=normals[list[corner]];
         }
      }
      if (i<uvIndices.size()) for (const int index:uvIndices[i]) if (size_t(index)>=uv.size()) sourceError(origin,"UV index exceeds its list.");
      if (faces[i].textures) {
         const auto &indices=*faces[i].textures;
         for (const int index:indices)
            if (size_t(index)>=textures.size()) sourceError(origin,"Texture index exceeds its list.");
         if (indices[0]==indices[1] && indices[0]==indices[2]) {
            face.material=displayMaterial(); face.material.overlay(textures[indices[0]]);
         } else {
            auto corners=std::make_shared<std::array<Material,3>>();
            for (int corner=0;corner<3;++corner) {
               (*corners)[corner]=displayMaterial(); (*corners)[corner].overlay(textures[indices[corner]]);
            }
            face.cornerMaterials=std::move(corners);
         }
      }
      result->faces.push_back(std::move(face));
   }
   return result;
}

osg::Matrixd Parser::transformBlock(bool leaveEnd)
{
   Model model; transform(model,"transform",leaveEnd); return model.matrix;
}
void Parser::nextDirectiveToken()
{
   next();
   if (token.kind==Token::Symbol && token.text==";") next();
}
bool Parser::conditionalPredicate(const QString &kind,bool repeatedWhile)
{
   bool result;
   if (kind=="ifdef" || kind=="ifndef") {
      {
         QScopedValueRollback<bool> raw(rawIdentifier,true);
         next(); expect("(");
      }
      const auto identifier=identifierValue(true);
      result=identifier.has_value();
      if (kind=="ifndef") result=!result;
      next();
   } else {
      next(); expect("("); const auto predicate=expression();
      if (predicate.size()!=1) fail("A conditional directive requires a scalar predicate.");
      result=repeatedWhile ? std::abs(predicate.front())>=PovEpsilon : povTrue(predicate.front());
   }
   // Keep the closing token unread by the next stage until the chosen branch
   // is known. Advancing here could already expand an inactive include.
   if (token.kind!=Token::Symbol || token.text!=")") fail("Expected ')' after a conditional predicate.");
   return result;
}
void Parser::validateControlContext(size_t count,const std::optional<Control> &owner,const Token &origin)
{
   if (controls.size()!=count || (owner &&
       (controls.back().identity!=owner->identity || controls.back().kind!=owner->kind || controls.back().taken!=owner->taken ||
        controls.back().elseSeen!=owner->elseSeen || controls.back().labelSeen!=owner->labelSeen ||
        controls.back().selection!=owner->selection)))
      sourceError(origin,"A predicate changed its enclosing control directive.");
}
void Parser::conditional(const QString &kind,const Token &origin)
{
   if (kind=="if" || kind=="ifdef" || kind=="ifndef") {
      if (controls.size()>=200) sourceError(origin,"Control directive nesting exceeds 200 levels.");
      const size_t previousCount=controls.size();
      const std::optional<Control> owner=controls.empty() ? std::nullopt : std::optional<Control>(controls.back());
      const bool selected=conditionalPredicate(kind);
      validateControlContext(previousCount,owner,origin);
      if (controlIdentity==std::numeric_limits<quint64>::max()) sourceError(origin,"Conditional identity capacity exceeded.");
      Control frame; frame.opening=origin; frame.identity=++controlIdentity; frame.taken=selected;
      controls.push_back(std::move(frame));
      if (selected) nextDirectiveToken();
      else token=lexer.skipControl(origin,controls.size());
      return;
   }
   if (controls.empty()) sourceError(origin,"Unmatched #"+kind+" directive.");
   if (kind=="end") {
      if (controls.back().kind==ControlKind::Macro)
         sourceError(origin,"A control #end cannot close an invoking macro.");
      if (controls.back().kind==ControlKind::If || controls.back().kind==ControlKind::Switch) {
         if (controls.back().kind==ControlKind::Switch && !controls.back().labelSeen)
            sourceError(origin,"A switch must contain a case or range label.");
         controls.pop_back(); nextDirectiveToken();
      }
      else finishLoop(origin);
      return;
   }
   if (controls.back().kind==ControlKind::Switch && kind=="else") { switchDefault(origin); return; }
   if (controls.back().kind!=ControlKind::If) sourceError(origin,"Conditional branch marker is not in an if directive.");
   if (controls.back().elseSeen) sourceError(origin,"A conditional cannot have a branch after #else.");
   if (kind=="else") {
      controls.back().elseSeen=true;
      if (!controls.back().taken) {
         controls.back().taken=true; nextDirectiveToken(); return;
      }
   } else if (!controls.back().taken) {
      const size_t previousCount=controls.size(); const auto owner=controls.back();
      const bool selected=conditionalPredicate(kind);
      validateControlContext(previousCount,owner,origin);
      if (selected) {
         controls.back().taken=true; nextDirectiveToken(); return;
      }
   }
   token=lexer.skipControl(controls.back().opening,controls.size());
}
double Parser::loopCounter(const Control &loop) const
{
   if (!loop.counterScope) sourceError(loop.opening,"Missing for-loop counter scope.");
   const auto found=loop.counterScope->values.find(loop.counter);
   const auto *number=found==loop.counterScope->values.end() ? nullptr : std::get_if<Tuple>(found->second.get());
   if (!number || number->size()!=1 || !std::isfinite(number->front()))
      sourceError(loop.opening,"A for-loop counter must remain defined and scalar.");
   return number->front();
}
void Parser::loop(const QString &kind,const Token &origin)
{
   if (controls.size()>=200) sourceError(origin,"Control directive nesting exceeds 200 levels.");
   const size_t previousCount=controls.size();
   const std::optional<Control> owner=controls.empty() ? std::nullopt : std::optional<Control>(controls.back());
   Control frame; frame.opening=origin;
   if (kind=="while") {
      frame.kind=ControlKind::While; frame.replay=token; frame.bookmark=lexer.bookmark();
      frame.taken=conditionalPredicate(kind);
   } else {
      frame.kind=ControlKind::For;
      {
         QScopedValueRollback<bool> raw(rawIdentifier,true);
         next(); expect("(");
      }
      if (token.kind!=Token::Word || lexer.nextIsSymbol("["))
         fail("A for-loop counter must be an identifier, not an array element.");
      frame.counter=token.text; frame.counterScope=token.scope;
      // Native #for introduces/replaces a local float before reading its
      // initial expression; an outer same-name symbol remains shadowed.
      storeBinding(frame.counterScope->values[frame.counter],Tuple{0.0},origin,control);
      next(); expect(",");
      const double start=scalar(); storeBinding(frame.counterScope->values[frame.counter],Tuple{start},origin,control);
      expect(","); frame.bound=scalar();
      if (take(",") && !(token.kind==Token::Symbol && token.text==")")) frame.step=scalar();
      if (token.kind!=Token::Symbol || token.text!=")") fail("Expected ')' after a for-loop header.");
      if (std::abs(frame.step)<PovEpsilon) sourceError(origin,"A for-loop increment must be nonzero.");
      const double current=loopCounter(frame);
      frame.taken=frame.step>0 ? current<frame.bound+PovEpsilon : current>frame.bound-PovEpsilon;
      frame.replay=token; frame.bookmark=lexer.bookmark();
   }
   validateControlContext(previousCount,owner,origin);
   if (controlIdentity==std::numeric_limits<quint64>::max()) sourceError(origin,"Control identity capacity exceeded.");
   frame.identity=++controlIdentity;
   const bool selected=frame.taken; controls.push_back(std::move(frame));
   if (selected) nextDirectiveToken();
   else token=lexer.skipControl(origin,controls.size(),SourceLexer::Skip::End);
}
void Parser::finishLoop(const Token &origin)
{
   const Control frame=controls.back();
   if (!frame.taken) { controls.pop_back(); nextDirectiveToken(); return; }
   if (origin.file!=frame.opening.file) sourceError(origin,"A running loop must end in the file where it started.");
   if (!frame.bookmark) sourceError(origin,"Missing loop source bookmark.");
   lexer.restore(*frame.bookmark); token=frame.replay;
   bool selected;
   if (frame.kind==ControlKind::For) {
      const double current=loopCounter(frame),advanced=current+frame.step;
      if (!std::isfinite(advanced)) sourceError(origin,"A for-loop counter became nonfinite.");
      selected=frame.step>0 ? !(advanced>frame.bound+PovEpsilon) : !(advanced<frame.bound-PovEpsilon);
      if (selected && advanced==current) sourceError(origin,"A for-loop increment does not advance its counter.");
      storeBinding(frame.counterScope->values[frame.counter],Tuple{advanced},origin,control);
   } else {
      const size_t count=controls.size();
      selected=conditionalPredicate("while",true);
      validateControlContext(count,frame,origin);
   }
   controls.back().taken=selected;
   if (selected) nextDirectiveToken();
   else token=lexer.skipControl(frame.opening,controls.size(),SourceLexer::Skip::End);
}
void Parser::breakControl(const Token &origin)
{
   size_t target=controls.size();
   while (target && controls[target-1].kind==ControlKind::If) --target;
   if (!target) sourceError(origin,"#break requires an enclosing loop, switch or macro.");
   if (controls[target-1].kind==ControlKind::Macro) {
      controls.resize(target); returnMacro(); next(); return;
   }
   if (controls[target-1].kind==ControlKind::Switch && !controls[target-1].labelSeen)
      sourceError(origin,"A switch must begin with a case or range label before #break.");
   const size_t nested=controls.size()-target;
   controls.resize(target); controls.back().taken=false;
   token=lexer.skipControl(controls.back().opening,controls.size(),SourceLexer::Skip::End,nested);
}
void Parser::switchBlock(const Token &origin)
{
   if (controls.size()>=200) sourceError(origin,"Control directive nesting exceeds 200 levels.");
   const size_t count=controls.size();
   const std::optional<Control> owner=controls.empty() ? std::nullopt : std::optional<Control>(controls.back());
   next(); expect("("); const auto selection=expression();
   if (selection.size()!=1) fail("A switch selector must be scalar.");
   if (token.kind!=Token::Symbol || token.text!=")") fail("Expected ')' after a switch selector.");
   validateControlContext(count,owner,origin);
   if (controlIdentity==std::numeric_limits<quint64>::max()) sourceError(origin,"Control identity capacity exceeded.");
   Control frame; frame.kind=ControlKind::Switch; frame.opening=origin;
   frame.identity=++controlIdentity; frame.selection=selection.front();
   const auto identity=frame.identity;
   controls.push_back(std::move(frame)); nextDirectiveToken();
   // Native switch parsing permits intervening directives, but its first
   // ordinary clause must be a case or range at this actual control level.
   for (;;) {
      if (controls.size()<=count || controls[count].identity!=identity)
         sourceError(origin,"The switch changed before its first label.");
      if (controls[count].labelSeen) return;
      if (token.kind!=Token::Symbol || token.text!="#")
         sourceError(origin,"A switch must begin with a case or range label.");
      directive();
   }
}
void Parser::switchLabel(const QString &kind,const Token &origin)
{
   if (controls.empty() || controls.back().kind!=ControlKind::Switch)
      sourceError(origin,"#"+kind+" requires the current control to be a switch.");
   const size_t count=controls.size(); const auto owner=controls.back();
   next(); expect("("); const auto first=expression();
   if (first.size()!=1) fail("A switch label must be scalar.");
   double second=first.front();
   if (kind=="range") {
      expect(","); const auto bound=expression();
      if (bound.size()!=1) fail("A switch range bound must be scalar.");
      second=bound.front();
   }
   if (token.kind!=Token::Symbol || token.text!=")") fail("Expected ')' after a switch label.");
   validateControlContext(count,owner,origin);
   auto &frame=controls.back(); frame.labelSeen=true;
   const bool matched=kind=="case" ? std::abs(frame.selection-first.front())<PovEpsilon :
      frame.selection>=first.front() && frame.selection<=second;
   // A selected native switch falls through subsequent clauses. Their test
   // expressions are still parsed, but do not deactivate the selected chain.
   frame.taken=frame.taken || matched;
   if (frame.taken) nextDirectiveToken();
   else token=lexer.skipControl(frame.opening,controls.size(),SourceLexer::Skip::Labels);
}
void Parser::switchDefault(const Token &origin)
{
   if (!controls.back().labelSeen) sourceError(origin,"A switch needs a case or range before its default clause.");
   controls.back().elseSeen=true; controls.back().taken=true; nextDirectiveToken();
}
void Parser::directive()
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Nesting exceeds the static model reader's limit.");
   const Token origin=token;
   // Do not call take/expect here: they prepare directives themselves.
   if (token.kind!=Token::Symbol || token.text!="#") fail("Expected a directive.");
   QString kind;
   {
      QScopedValueRollback<bool> raw(rawIdentifier,true);
      next();
      if (token.kind!=Token::Word) fail("Expected a directive name.");
      kind=token.text;
   }
   if (kind=="if" || kind=="ifdef" || kind=="ifndef" || kind=="elseif" || kind=="else" || kind=="end") {
      conditional(kind,origin); return;
   }
   if (kind=="for" || kind=="while") { loop(kind,origin); return; }
   if (kind=="switch") { switchBlock(origin); return; }
   if (kind=="case" || kind=="range") { switchLabel(kind,origin); return; }
   if (kind=="break") { breakControl(origin); return; }
   if (kind=="macro") { defineMacro(origin); return; }
   const bool messageDirective=kind=="error" || kind=="warning" || kind=="debug" || kind=="render" || kind=="statistics";
   if (kind!="version" && kind!="undef" && kind!="declare" && kind!="local" && !messageDirective)
      fail("Unsupported #"+kind+" directive; procedural generation is not imported.");
   {
      QScopedValueRollback<bool> raw(rawIdentifier,true);
      next();
   }
   if (messageDirective) {
      expandToken();
      const auto message=stringValue(true);
      const auto diagnostic=sourceLocation(origin.file,origin.line)+" #"+kind+": "+message;
      if (kind=="error") sourceError(origin,"#error: "+message);
      if (kind=="warning") qWarning().noquote()<<diagnostic;
      else qInfo().noquote()<<diagnostic;
      nextDirectiveToken(); return;
   }
   if (kind=="version") { expandArrayReference(); scalar(); if (token.kind==Token::Symbol && token.text==";") next(); return; }
   if (kind=="undef") {
      if (token.kind!=Token::Word) fail("Expected an identifier after #undef.");
      const auto name=token.text;
      if (lexer.nextIsSymbol("[")) {
         const auto slot=arrayReference(true);
         slot.array->elements[slot.index].cell->reset();
      } else if (const auto owner=symbolOwner(origin.scope,name)) owner->values.erase(name);
      // Commit before reading a following #include whose filename may use
      // this identifier. The directive's original scope survives EOF lookahead.
      next(); if (token.kind==Token::Symbol && token.text==";") next(); return;
   }
   if (kind!="declare" && kind!="local") fail("Unsupported #"+kind+" directive; procedural generation is not imported.");
   if (token.kind!=Token::Word) fail("Expected an identifier after #"+kind+".");
   const auto name=token.text;
   if (symbol<Macro>(token,name)) fail("A declaration cannot replace a macro without #undef.");
   auto destination=kind=="local" ? origin.scope : symbolOwner(origin.scope,name);
   if (!destination) destination=lexer.globals();
   std::optional<ArraySlot> slot;
   if (lexer.nextIsSymbol("[")) {
      if (kind=="local" && symbolOwner(token.scope,name)!=origin.scope)
         fail("#local cannot assign an element of an array from an outer scope.");
      slot=arrayReference(true);
   }
   next(); expect("=");
   bool leftEnd=false; auto parsed=declarationValue(leftEnd);
   if (slot) storeArrayElement(slot->array,slot->index,parsed,origin,control);
   else storeBinding(destination->values[name],parsed,origin,control);
   // Commit before reading a following include whose filename may use the
   // new binding or array element, including a final token at include EOF.
   if (leftEnd) next();
   if (token.kind==Token::Symbol && token.text==";") next();
}

SymbolValue Parser::declarationValue(bool &leftEnd)
{
   QScopedValueRollback<int> nested(depth,depth+1);
   if (depth>128) fail("Declared value nesting exceeds the model reader's limit.");
   prepareDirectives(); const auto type=token.text;
   SymbolValue parsed; leftEnd=false;
   if (startsString()) {
      parsed=stringValue(true); leftEnd=true;
   } else if (type=="array") {
      parsed=arrayDeclaration(); leftEnd=true;
   } else if (const auto *array=symbol<Array>(token,type)) {
      parsed=*array; leftEnd=true;
   } else if (type=="triangle" || type=="smooth_triangle" || type=="mesh" || type=="mesh2" || isCsgObject(type) || type=="object" || isFinitePrimitive(type) || symbol<Node>(token,type)) {
      parsed=object(true); leftEnd=true;
   } else if (type=="texture" || type=="pigment" || type=="finish") {
      parsed=MaterialSymbol{type,materialBlock(type,true)}; leftEnd=true;
   } else if (type=="transform") {
      parsed=transformBlock(true); leftEnd=true;
   } else if (const auto *material=symbol<MaterialSymbol>(token,type)) {
      parsed=*material; leftEnd=true;
   } else if (const auto *matrix=symbol<osg::Matrixd>(token,type)) {
      parsed=*matrix; leftEnd=true;
   } else if (type=="color" || type=="rgb" || type=="rgbf" || type=="rgbt" || type=="rgbft") {
      parsed=colorValue();
   } else parsed=value();
   return parsed;
}
void Parser::skipRenderBlock()
{
   next(); expect("{"); int nesting=1;
   while (nesting) {
      prepareDirectives();
      if (token.kind==Token::End) fail("Unterminated render-only block.");
      if (token.kind==Token::Symbol) {
         if (token.text=="{") ++nesting;
         if (token.text=="}") --nesting;
      }
      next();
   }
}
Node Parser::parse()
{
   auto result=std::make_shared<Model>();
   while (token.kind!=Token::End) {
      if (token.text=="#") directive();
      else if (token.kind==Token::Word && (token.text=="camera" || token.text=="light_source" || token.text=="background" || token.text=="global_settings")) skipRenderBlock();
      else if (!take(";")) result->children.push_back(object());
   }
   if (!controls.empty()) sourceError(controls.back().opening,"Unterminated control directive.");
   if (result->children.empty()) fail("No instantiated static model objects.");
   return result;
}

using MaterialKey=std::array<float,8>;
MaterialKey materialKey(const Material &material)
{
   const auto color=material.color.value_or(osg::Vec4(0.75f,0.75f,0.75f,1));
   MaterialKey result={color.r(),color.g(),color.b(),color.a(),material.ambient.value_or(0.2f),
                       material.diffuse.value_or(0.8f),material.specular.value_or(0.1f),material.power.value_or(32.0f)};
   for (size_t i=0;i<result.size();++i) result[i]=std::clamp(result[i],0.0f,i==7?128.0f:1.0f);
   return result;
}
osg::Vec3 normal(const osg::Vec3d &value,const QString &file,int line)
{
   const double length=std::hypot(value.x(),value.y(),value.z());
   if (!std::isfinite(length) || length==0)
      throw std::runtime_error((sourceLocation(file,line)+": Degenerate triangle or zero/nonfinite normal.").toStdString());
   return {float(value.x()/length),float(value.y()/length),float(value.z()/length)};
}
#include "PovSolidGeometry.inc"
struct Batch {
   osg::ref_ptr<osg::Vec3Array> vertices=new osg::Vec3Array;
   osg::ref_ptr<osg::Vec3Array> normals=new osg::Vec3Array;
};
struct CornerBatch:Batch {
   osg::ref_ptr<osg::Vec2Array> weights=new osg::Vec2Array;
   std::array<osg::ref_ptr<osg::Vec4Array>,6> materials;
   CornerBatch() { for (auto &array:materials) array=new osg::Vec4Array; }
};
osg::ref_ptr<osg::Geometry> geometryFor(const Batch &batch)
{
   if (batch.vertices->size()>size_t(std::numeric_limits<GLsizei>::max()))
      throw std::runtime_error("POV mesh exceeds the graphics vertex-count limit.");
   osg::ref_ptr<osg::Geometry> geometry=new osg::Geometry;
   geometry->setVertexArray(batch.vertices);
   geometry->setNormalArray(batch.normals,osg::Array::BIND_PER_VERTEX);
   geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(batch.vertices->size())));
   geometry->setUseDisplayList(false); geometry->setUseVertexBufferObjects(true);
   return geometry;
}
void materialState(osg::StateSet &state,bool transparent)
{
   state.setMode(GL_CULL_FACE,osg::StateAttribute::OFF);
   if (transparent) {
      state.setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA));
      state.setMode(GL_BLEND,osg::StateAttribute::ON); state.setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
      state.setAttributeAndModes(new osg::Depth(osg::Depth::LESS,0,1,false));
   }
}
osg::ref_ptr<osg::Program> cornerMaterialProgram()
{
   // Evaluate all three solid finishes at the same fragment before blending.
   // Interpolating pigment and finish separately would introduce cross terms.
   // Eight vec4 varyings fit the GLSL 1.20 minimum; weights share spare lanes.
   const char *vertex=R"glsl(#version 120
varying vec4 color0, finish0, color1, finish1, color2, finish2;
varying vec4 normalWeight, eyeWeight;
void main()
{
   gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
   normalWeight=vec4(gl_NormalMatrix*gl_Normal,gl_MultiTexCoord0.x);
   eyeWeight=vec4((gl_ModelViewMatrix*gl_Vertex).xyz,gl_MultiTexCoord0.y);
   color0=gl_MultiTexCoord1; finish0=gl_MultiTexCoord2;
   color1=gl_MultiTexCoord3; finish1=gl_MultiTexCoord4;
   color2=gl_MultiTexCoord5; finish2=gl_MultiTexCoord6;
}
)glsl";
   const char *fragment=R"glsl(#version 120
varying vec4 color0, finish0, color1, finish1, color2, finish2;
varying vec4 normalWeight, eyeWeight;
vec3 unitVector(vec3 v)
{
   float lengthSquared=dot(v,v);
   return lengthSquared>0.0 ? v*inversesqrt(lengthSquared) : vec3(0.0);
}
vec4 shade(vec4 color,vec4 finish,float diffuse,float halfDot)
{
   float highlight=0.0;
   if (diffuse>0.0)
      highlight=finish.w==0.0 ? 1.0 : pow(halfDot,finish.w);
   vec3 ambient=gl_LightModel.ambient.rgb+gl_LightSource[0].ambient.rgb;
   vec3 lit=color.rgb*(finish.x*ambient+finish.y*diffuse*gl_LightSource[0].diffuse.rgb);
   lit+=finish.z*highlight*gl_LightSource[0].specular.rgb;
   return vec4(lit,color.a);
}
void main()
{
   vec3 n=unitVector(normalWeight.xyz);
   vec3 light=unitVector(gl_LightSource[0].position.xyz-eyeWeight.xyz*gl_LightSource[0].position.w);
   vec3 view=unitVector(-eyeWeight.xyz);
   float diffuse=max(dot(n,light),0.0);
   float halfDot=max(dot(n,unitVector(light+view)),0.0);
   float a=normalWeight.w, b=eyeWeight.w;
   gl_FragColor=a*shade(color0,finish0,diffuse,halfDot)
               +b*shade(color1,finish1,diffuse,halfDot)
               +(1.0-a-b)*shade(color2,finish2,diffuse,halfDot);
}
)glsl";
   osg::ref_ptr<osg::Program> program=new osg::Program;
   program->addShader(new osg::Shader(osg::Shader::VERTEX,vertex));
   program->addShader(new osg::Shader(osg::Shader::FRAGMENT,fragment));
   return program;
}
class Builder {
public:
   explicit Builder(ImportControl &control) : control(control) {}
   bool emptySolidGeometry() const { return sawSolid && !hasSurfaces; }
   osg::ref_ptr<osg::Node> build(const Node &model,const Material &inherited={},const Material *rootOverride=nullptr) {
      control.check();
      QScopedValueRollback<int> nested(depth,depth+1);
      if (depth>128) throw std::runtime_error("POV instance hierarchy exceeds the static mesh reader's nesting limit.");
      const Material &rootMaterial=rootOverride ? *rootOverride : model->material;
      Material material=inherited; material.overlay(rootMaterial);
      const auto key=materialKey(material);
      auto &known=cache[model.get()];
      if (known.count(key)) return known.at(key);
      osg::ref_ptr<osg::MatrixTransform> result=new osg::MatrixTransform;
      result->setMatrix(model->matrix);
      const bool solidGeometry=model->inverse || model->kind==Model::Kind::Intersection ||
            model->kind==Model::Kind::Difference || model->kind==Model::Kind::Merge || model->kind==Model::Kind::Empty;
      if (model->instance && !solidGeometry) {
         // Object modifiers edit the copied object's root texture. A union's
         // children still retain their explicit local textures. Keep the mesh
         // prototype shared instead of copying every face for each instance.
         result->addChild(build(model->children.at(0),inherited,&rootMaterial));
         known[key]=result; return result;
      }
      std::map<MaterialKey,Batch> batches;
      std::map<bool,CornerBatch> cornerBatches;
      const auto solidFaces=solidGeometry ? PovSolid::Builder(control).faces(model,inherited,rootOverride) : std::vector<Triangle>{};
      sawSolid|=solidGeometry;
      hasSurfaces|=!(solidGeometry ? solidFaces : model->faces).empty();
      for (const auto &face:solidGeometry ? solidFaces : model->faces) {
         control.check();
         Material surface=material; surface.overlay(face.material);
         std::array<MaterialKey,3> values;
         for (int corner=0;corner<3;++corner) {
            Material cornerSurface=surface;
            if (face.cornerMaterials) cornerSurface.overlay((*face.cornerMaterials)[corner]);
            values[corner]=materialKey(cornerSurface);
         }
         const bool mixed=values[0]!=values[1] || values[0]!=values[2];
         CornerBatch *corners=nullptr;
         if (mixed) {
            const bool transparent=std::any_of(values.begin(),values.end(),[](const MaterialKey &v) { return v[3]<1.0f; });
            corners=&cornerBatches[transparent];
         }
         auto &batch=corners ? static_cast<Batch&>(*corners) : batches[values[0]];
         const auto faceNormal=normal((face.vertices[1]-face.vertices[0])^(face.vertices[2]-face.vertices[0]),face.file,face.line);
         // The outer Y/Z exchange changes handedness. Reverse triangle winding
         // while retaining the authored normals, then apply that exchange once.
         for (const int index:{0,2,1}) {
            const auto &v=face.vertices[index];
            const osg::Vec3 point(float(v.x()),float(v.y()),float(v.z()));
            if (!std::isfinite(point.x()) || !std::isfinite(point.y()) || !std::isfinite(point.z()))
               throw std::runtime_error((sourceLocation(face.file,face.line)+": Vertex exceeds display precision.").toStdString());
            batch.vertices->push_back(point);
            batch.normals->push_back(face.smooth ? normal(face.normals[index],face.file,face.line) : faceNormal);
            if (corners) {
               const auto weights=face.materialWeights ? (*face.materialWeights)[index] :
                     osg::Vec3d(index==0 ? 1.0 : 0.0,index==1 ? 1.0 : 0.0,index==2 ? 1.0 : 0.0);
               corners->weights->push_back(osg::Vec2(float(weights.x()),float(weights.y())));
               for (int corner=0;corner<3;++corner) {
                  const auto &v=values[corner];
                  corners->materials[2*corner]->push_back(osg::Vec4(v[0],v[1],v[2],v[3]));
                  corners->materials[2*corner+1]->push_back(osg::Vec4(v[4],v[5],v[6],v[7]));
               }
            }
         }
      }
      osg::ref_ptr<osg::Geode> geode=new osg::Geode;
      for (const auto &[values,batch]:batches) {
         control.check();
         auto geometry=geometryFor(batch);
         osg::ref_ptr<osg::Material> finish=new osg::Material;
         finish->setColorMode(osg::Material::OFF);
         finish->setAmbient(osg::Material::FRONT_AND_BACK,{values[0]*values[4],values[1]*values[4],values[2]*values[4],values[3]});
         finish->setDiffuse(osg::Material::FRONT_AND_BACK,{values[0]*values[5],values[1]*values[5],values[2]*values[5],values[3]});
         finish->setSpecular(osg::Material::FRONT_AND_BACK,{values[6],values[6],values[6],values[3]});
         finish->setShininess(osg::Material::FRONT_AND_BACK,values[7]);
         auto *state=geometry->getOrCreateStateSet(); state->setAttributeAndModes(finish,osg::StateAttribute::ON);
         materialState(*state,values[3]<1.0f);
         geode->addDrawable(geometry);
      }
      for (const auto &[transparent,batch]:cornerBatches) {
         control.check();
         auto geometry=geometryFor(batch);
         geometry->setTexCoordArray(0,batch.weights);
         for (unsigned i=0;i<batch.materials.size();++i) geometry->setTexCoordArray(i+1,batch.materials[i]);
         if (!interpolationProgram) interpolationProgram=cornerMaterialProgram();
         auto *state=geometry->getOrCreateStateSet();
         state->setAttributeAndModes(interpolationProgram,osg::StateAttribute::ON);
         materialState(*state,transparent);
         geode->addDrawable(geometry);
      }
      if (geode->getNumDrawables()) result->addChild(geode);
      if (!solidGeometry)
         for (const auto &child:model->children) result->addChild(build(child,material));
      known[key]=result;
      return result;
   }
private:
   ImportControl &control;
   int depth=0;
   bool sawSolid=false,hasSurfaces=false;
   osg::ref_ptr<osg::Program> interpolationProgram;
   std::map<const Model*,std::map<MaterialKey,osg::ref_ptr<osg::Node>>> cache;
};
}

osg::ref_ptr<osg::Node> readPovModel(const QString &path,QString *error)
{
   return readPovModel(path,QStringList{},error);
}

osg::ref_ptr<osg::Node> readPovModel(const QString &path,const QStringList &includePaths,QString *error)
{
   return readPovModel(path,includePaths,error,PovModelCancel{});
}

osg::ref_ptr<osg::Node> readPovModel(const QString &path,const QStringList &includePaths,QString *error,const PovModelCancel &cancel)
{
   if (error) error->clear();
   try {
      ImportControl control(cancel); control.check();
      Parser parser(path,includePaths,control); Builder builder(control);
      osg::ref_ptr<osg::MatrixTransform> axes=new osg::MatrixTransform;
      // Match the legacy GMAT POV model convention: POV Y-up becomes GMAT Z-up.
      axes->setMatrix(osg::Matrixd(1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,0,1));
      axes->addChild(builder.build(parser.parse()));
      if (builder.emptySolidGeometry()) axes->setUserValue("gmat.pov.emptyGeometry",true);
      control.check();
      return axes;
   } catch (const std::exception &failure) {
      if (error) *error=QString::fromUtf8(failure.what());
      return {};
   }
}
