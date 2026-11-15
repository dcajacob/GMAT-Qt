//$Id: CallMatlabFunction.cpp 9460 2011-04-21 22:03:28Z lindajun $
//------------------------------------------------------------------------------
//                                 CallMatlabFunction
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); 
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either 
// express or implied.   See the License for the specific language
// governing permissions and limitations under the License.
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number NNG04CC06P
//
// Author: Allison Greene
// Created: 2004/09/22
//
/**
 * Definition for the CallMatlabFunction command class
 */
//------------------------------------------------------------------------------
#include "CallMatlabFunction.hpp"
#include "BeginFunction.hpp"
#include "StringTokenizer.hpp"
#include "StringUtil.hpp"          // for Replace()
#include "ScriptStringLiteral.hpp"
#include "FileManager.hpp"         // for GetAllMatlabFunctionPaths()
#include "FileUtil.hpp"
#include "ObjectMatlabExportScope.hpp"
#include "FormationInterface.hpp"
#include "Spacecraft.hpp"
#include "MatlabInterface.hpp"     // for Matlab Engine functions
#include "InterfaceException.hpp"
#include "GmatInterface.hpp"
#include "MessageInterface.hpp"
#include <algorithm>               // for formal output aliases
#include <sstream>                 // for ostringstream
#include <vector>
#include <cmath>
#include <limits>
#include <locale>

namespace
{
   struct MatlabFunctionSource
   {
      std::string name;
      std::string directory;
      bool explicitFile;
   };

   MatlabFunctionSource ResolveMatlabFunctionSource(const Function &function)
   {
      const std::string path = function.GetStringParameter("FunctionPath");
      // DoesDirectoryExist checks a file's parent unless the path ends in a
      // separator. Query the selected path itself to distinguish .m folders.
      if (GmatFileUtil::ParseFileExtension(path) == "m" &&
          !GmatFileUtil::DoesDirectoryExist(path + GmatFileUtil::GetPathSeparator(), false))
      {
         const std::string name = GmatFileUtil::ParseFileName(path, true);
         if (!GmatStringUtil::IsValidName(name))
            throw CommandException("The MATLAB source filename must contain a valid function name.");
         return {name, GmatFileUtil::ParsePathName(path), true};
      }
      return {function.GetName(), path, false};
   }

   void EvaluateMatlabFunctionExpression(MatlabInterface &interface,
         const std::string &expression, bool checked = true)
   {
      GmatInterface::MatlabFunctionScope scope;
      if (checked) interface.RunMatlabString(expression);
      else interface.EvalString(expression);
   }

   bool OpenMatlabFunctionEngine(MatlabInterface &interface)
   {
      GmatInterface::MatlabFunctionScope scope;
      return interface.Open("GmatMatlab") != 0;
   }

   class MatlabWorkspaceVariables
   {
   public:
      MatlabWorkspaceVariables(MatlabInterface &interface, size_t count,
                               const StringArray &excluded = {})
         : interface(interface), excluded(excluded) { variables.reserve(count); }
      ~MatlabWorkspaceVariables()
      {
         // During a transport failure the engine may already be gone. Keep
         // the original exception; explicit successful cleanup precedes commit.
         try { Clear(); } catch (...) {}
      }
      MatlabWorkspaceVariables(const MatlabWorkspaceVariables &) = delete;
      MatlabWorkspaceVariables &operator=(const MatlabWorkspaceVariables &) = delete;
      void Add()
      {
         GmatInterface::MatlabFunctionScope scope;
         variables.push_back(interface.CreateTemporaryVariable(excluded));
      }
      const StringArray &Names() const { return variables; }
      void Clear()
      {
         if (variables.empty()) return;
         std::string command = "clear";
         for (const auto &name : variables) command += " " + name;
         EvaluateMatlabFunctionExpression(interface, command + ";");
         variables.clear();
      }
   private:
      MatlabInterface &interface;
      StringArray variables, excluded;
   };

   std::string MatlabInvocation(const std::string &function,
      const StringArray &inputs, const StringArray &outputs)
   {
      std::string command;
      if (!outputs.empty())
      {
         command = "[";
         for (size_t i = 0; i < outputs.size(); ++i)
            command += (i ? ", " : "") + outputs[i];
         command += "] = ";
      }
      command += function + "(";
      for (size_t i = 0; i < inputs.size(); ++i)
         command += (i ? ", " : "") + inputs[i];
      return command + ");";
   }

   std::string MatlabRealText(Real value)
   {
      if (std::isnan(value)) return "NaN";
      if (std::isinf(value)) return value < 0 ? "-Inf" : "Inf";
      std::ostringstream text;
      text.imbue(std::locale::classic());
      text.precision(std::numeric_limits<Real>::max_digits10);
      text << value;
      return text.str();
   }

   using MatlabAssignments = std::vector<std::function<void()>>;

   CommandException MatlabObjectError(const std::string &path, const std::string &message)
   {
      return CommandException("Cannot assign MATLAB object field \"" + path + "\": " + message);
   }

   bool MatlabNumeric(const MatlabValue &value)
   {
      return value.type == MatlabValue::Type::NUMERIC || value.type == MatlabValue::Type::LOGICAL;
   }

   Real MatlabScalar(const MatlabValue &value, const std::string &path)
   {
      if (!MatlabNumeric(value) || value.rows != 1 || value.columns != 1 || value.numbers.size() != 1)
         throw MatlabObjectError(path, "a real numeric scalar is required");
      return value.numbers[0];
   }

   template <typename T>
   T MatlabInteger(Real value, const std::string &path)
   {
      if (!std::isfinite(value) || std::floor(value) != value ||
          value < static_cast<Real>(std::numeric_limits<T>::lowest()) ||
          value > static_cast<Real>(std::numeric_limits<T>::max()))
         throw MatlabObjectError(path, "an integer within the native property range is required");
      return static_cast<T>(value);
   }

   std::string MatlabText(const MatlabValue &value, const std::string &path)
   {
      if (value.type != MatlabValue::Type::CHARACTER || (value.rows > 1 && value.columns > 1))
         throw MatlabObjectError(path, "a character vector is required");
      return value.text;
   }

   GmatTime MatlabPreciseTime(const MatlabValue &value, const std::string &path)
   {
      if (value.type != MatlabValue::Type::CHARACTER)
      {
         const Real mjd = MatlabScalar(value, path);
         const Real upper = std::ldexp(1.0, std::numeric_limits<long>::digits);
         if (!std::isfinite(mjd) || mjd < -upper || mjd >= upper)
            throw MatlabObjectError(path, "the MJD exceeds native precise-time capacity");
         return GmatTime(mjd);
      }
      const std::string text = MatlabText(value, path);
      const bool negative = !text.empty() && text[0] == '-';
      const size_t first = !text.empty() && (text[0] == '-' || text[0] == '+') ? 1 : 0;
      const size_t dot = text.find('.', first);
      bool digit = false;
      for (size_t i = first; i < text.size(); ++i)
      {
         if (i == dot) continue;
         if (text[i] < '0' || text[i] > '9')
            throw MatlabObjectError(path, "a signed decimal MJD character vector is required");
         digit = true;
      }
      if (!digit) throw MatlabObjectError(path, "a signed decimal MJD character vector is required");
      const std::string whole = text.substr(first, dot == std::string::npos ? dot : dot - first);
      long days = 0;
      try { days = whole.empty() ? 0 : std::stol(whole); }
      catch (const std::exception &)
      {
         throw MatlabObjectError(path, "the MJD exceeds native precise-time capacity");
      }
      // The native parser uses atoi for its day part. Parse only the validated
      // fractional part there, then set range-checked days without narrowing.
      GmatTime result(0.0);
      result.SetMjdString("0." + (dot == std::string::npos ? "0" : text.substr(dot + 1)));
      const long carry = result.GetDays();
      if (days > std::numeric_limits<long>::max() - carry)
         throw MatlabObjectError(path, "the MJD exceeds native precise-time capacity");
      result.SetDays(days + carry);
      return negative ? GmatTime(0.0) - result : result;
   }

   RealArray MatlabVector(const MatlabValue &value, const std::string &path)
   {
      if (value.type == MatlabValue::Type::CHARACTER)
      {
         const std::string text = MatlabText(value, path);
         if (!GmatStringUtil::IsBracketBalanced(text, "[]"))
            throw MatlabObjectError(path, "the legacy numeric-vector text has unbalanced brackets");
         return GmatStringUtil::ToRealArray(text);
      }
      if (!MatlabNumeric(value) || (value.rows > 1 && value.columns > 1))
         throw MatlabObjectError(path, "a real numeric vector is required");
      return value.numbers;
   }

   BooleanArray MatlabBooleans(const MatlabValue &value, const std::string &path)
   {
      if (value.type == MatlabValue::Type::CHARACTER)
      {
         const std::string text = MatlabText(value, path);
         if (!GmatStringUtil::IsBracketBalanced(text, "[]"))
            throw MatlabObjectError(path, "the legacy Boolean-vector text has unbalanced brackets");
         return GmatStringUtil::ToBooleanArray(text);
      }
      const RealArray numbers = MatlabVector(value, path);
      BooleanArray result;
      result.reserve(numbers.size());
      for (const Real number : numbers)
      {
         if (number != 0.0 && number != 1.0)
            throw MatlabObjectError(path, "Boolean values must be logical, zero or one");
         result.push_back(number == 1.0);
      }
      return result;
   }

   StringArray MatlabStrings(const MatlabValue &value, const std::string &path)
   {
      if (value.type == MatlabValue::Type::CHARACTER)
      {
         // A character vector is one value. Braces and quotes inside it are
         // data; actual MATLAB lists arrive as cells, without literal parsing.
         return {MatlabText(value, path)};
      }
      // Empty [] is an unambiguous empty list as well as the native {} cell.
      if (MatlabNumeric(value) && value.numbers.empty()) return {};
      if (value.type != MatlabValue::Type::CELL || (value.rows > 1 && value.columns > 1))
         throw MatlabObjectError(path, "a cell vector of character vectors is required");
      StringArray result;
      result.reserve(value.children.size());
      for (const auto &child : value.children) result.push_back(MatlabText(child, path));
      return result;
   }

   GmatBase *MatlabOwnedComponent(GmatBase *object, Integer id, const std::string &path)
   {
      if (!object->IsOwnedObject(id))
         throw MatlabObjectError(path, "a nested structure requires a native owned-object property");
      GmatBase *child = object->GetRefObject(object->GetPropertyObjectType(id),
                                           object->GetStringParameter(id));
      if (child)
         for (Integer i = 0; i < object->GetOwnedObjectCount(); ++i)
            if (object->GetOwnedObject(i) == child) return child;
      throw MatlabObjectError(path, "the native owned component is not bound");
   }

   using MatlabLookup = std::function<GmatBase*(const std::string&)>;

   struct MatlabReference
   {
      GmatBase *object = nullptr;
      UnsignedInt type = Gmat::UNKNOWN_OBJECT;
      std::string name;
      std::weak_ptr<const void> identity;
      std::shared_ptr<const FunctionValueContext> lease;
      bool fromBinding = false;
   };

   GmatBase *MatlabCurrentReference(GmatBase *owner, UnsignedInt type, const std::string &name)
   {
      // Some native getters signal a missing/unbound reference with an exception.
      try { return owner->GetRefObject(type, name); }
      catch (BaseException &) { return nullptr; }
   }

   MatlabReference PlanMatlabReference(GmatBase *owner, UnsignedInt type,
      const std::string &name, bool alreadyNamed, const std::string &path,
      const MatlabLookup &lookup)
   {
      MatlabReference result;
      result.type = type;
      result.name = name;
      if (type == Gmat::UNKNOWN_OBJECT) return result;
      GmatBase *bound = alreadyNamed ? MatlabCurrentReference(owner, type, name) : nullptr;
      if (name.empty() || name == "None")
      {
         if (alreadyNamed && !bound) return result;
         throw MatlabObjectError(path, "clearing this native reference requires its provider's clear operation");
      }
      // A same-name private reference may differ from the caller's public map.
      // Preserve the actual binding when the returned name is unchanged.
      result.object = bound ? bound : lookup(name);
      result.fromBinding = bound != nullptr;
      if (!result.object || !result.object->IsOfType(type))
         throw MatlabObjectError(path, "the named reference is unavailable or has an incompatible native type: " + name);
      result.identity = result.object->GetLifetimeToken();
      result.lease = result.object->GetFunctionValueContext();
      return result;
   }

   void ValidateMatlabReference(const MatlabReference &reference, const std::string &path,
                                const MatlabLookup &lookup)
   {
      if (!reference.object) return;
      if (!GmatBase::IsLiveReference(reference.object, reference.identity) ||
          (!reference.fromBinding && lookup(reference.name) != reference.object))
         throw MatlabObjectError(path, "the resolved native reference changed before assignment");
   }

   bool BindMatlabReference(GmatBase *owner, const MatlabReference &reference, const std::string &path)
   {
      if (!reference.object) return false;
      if (MatlabCurrentReference(owner, reference.type, reference.name) == reference.object) return false;
      if (!owner->SetRefObject(reference.object, reference.type, reference.name))
         throw MatlabObjectError(path, "the native reference setter rejected: " + reference.name);
      return true;
   }

   void StageMatlabObject(GmatBase *object, const MatlabValue &value,
                         const std::string &path, const std::function<void()> &validate,
                         const MatlabLookup &lookup, MatlabAssignments &assignments)
   {
      if (!object || value.type != MatlabValue::Type::STRUCTURE ||
          value.rows != 1 || value.columns != 1 || value.children.size() != value.fieldNames.size())
         throw MatlabObjectError(path, "a scalar structure is required");
      // Tanks and thrusters form one owned graph. Plan both lists before
      // assigning either, regardless of MATLAB's structure field order.
      struct PropulsionPacket
      {
         Integer tankId = -1, thrusterId = -1;
         bool returnedTanks = false, returnedThrusters = false;
         StringArray tanks, thrusters;
         std::vector<MatlabReference> tankSources, thrusterSources;
         std::shared_ptr<const FunctionValueContext> previous;
      };
      auto *spacecraft = dynamic_cast<Spacecraft*>(object);
      std::shared_ptr<PropulsionPacket> propulsion;
      if (spacecraft)
      {
         auto packet = std::make_shared<PropulsionPacket>();
         packet->tankId = spacecraft->GetParameterID("Tanks");
         packet->thrusterId = spacecraft->GetParameterID("Thrusters");
         for (size_t i = 0; i < value.fieldNames.size(); ++i)
         {
            Integer id;
            try { id = spacecraft->GetParameterID(value.fieldNames[i]); }
            catch (BaseException &) { continue; }
            if ((id != packet->tankId && id != packet->thrusterId) || spacecraft->IsParameterReadOnly(id))
               continue;
            const std::string fieldPath = path + "." + value.fieldNames[i];
            bool &returned = id == packet->tankId ? packet->returnedTanks : packet->returnedThrusters;
            if (returned) throw MatlabObjectError(fieldPath, "repeated aliases for one propulsion list");
            returned = true;
            (id == packet->tankId ? packet->tanks : packet->thrusters) = MatlabStrings(value.children[i], fieldPath);
         }
         if (packet->returnedTanks || packet->returnedThrusters)
         {
            const StringArray currentTanks = spacecraft->GetStringArrayParameter(packet->tankId);
            const StringArray currentThrusters = spacecraft->GetStringArrayParameter(packet->thrusterId);
            if (!packet->returnedTanks) packet->tanks = currentTanks;
            if (!packet->returnedThrusters) packet->thrusters = currentThrusters;
            const auto plan = [&](const StringArray &names, const StringArray &current,
                                  UnsignedInt type, const std::string &fieldPath,
                                  std::vector<MatlabReference> &sources)
            {
               sources.reserve(names.size());
               for (const auto &name : names)
                  sources.push_back(PlanMatlabReference(spacecraft, type, name,
                     std::find(current.begin(), current.end(), name) != current.end(), fieldPath, lookup));
            };
            plan(packet->tanks, currentTanks, Gmat::FUEL_TANK, path + ".Tanks", packet->tankSources);
            plan(packet->thrusters, currentThrusters, Gmat::THRUSTER, path + ".Thrusters", packet->thrusterSources);
            packet->previous = spacecraft->GetFunctionValueContext();
            propulsion = std::move(packet);
         }
      }
      bool propulsionQueued = false;
      for (size_t i = 0; i < value.fieldNames.size(); ++i)
      {
         const std::string &field = value.fieldNames[i];
         const std::string fieldPath = path + "." + field;
         const MatlabValue &data = value.children[i];
         Integer id;
         try { id = object->GetParameterID(field); }
         catch (BaseException &)
         {
            // Retain the native allowance for additional MATLAB struct fields.
            MessageInterface::ShowMessage("*** WARNING *** MATLAB field \"%s\" is not defined in \"%s\", so ignoring\n",
                                          field.c_str(), path.c_str());
            continue;
         }
         if (id < 0)
         {
            MessageInterface::ShowMessage("*** WARNING *** MATLAB field \"%s\" is not defined in \"%s\", so ignoring\n",
                                          field.c_str(), path.c_str());
            continue;
         }
         const auto type = object->GetParameterType(id);
         if (data.type == MatlabValue::Type::STRUCTURE && type == Gmat::OBJECT_TYPE)
         {
            GmatBase *child = MatlabOwnedComponent(object, id, fieldPath);
            const auto identity = child->GetLifetimeToken();
            auto childValidation = [validate, object, child, identity, id, fieldPath]
            {
               validate();
               if (!GmatBase::IsLiveReference(child, identity) ||
                   MatlabOwnedComponent(object, id, fieldPath) != child)
                  throw MatlabObjectError(fieldPath, "the owned component changed before assignment");
            };
            StageMatlabObject(child, data, fieldPath, childValidation, lookup, assignments);
            continue;
         }
         if (object->IsParameterReadOnly(id))
         {
            MessageInterface::ShowMessage("*** WARNING *** MATLAB field \"%s\" is read-only, so ignoring\n",
                                          fieldPath.c_str());
            continue;
         }
         const auto append = [&](std::function<void()> setter)
         {
            assignments.emplace_back([validate, object, id, type, field, fieldPath, setter]
            {
               validate();
               if (object->GetParameterID(field) != id || object->GetParameterType(id) != type ||
                   object->IsParameterReadOnly(id))
                  throw MatlabObjectError(fieldPath, "the native property changed before assignment");
               try { setter(); }
               catch (BaseException &error)
               {
                  throw MatlabObjectError(fieldPath, error.GetFullMessage());
               }
            });
         };
         if (propulsion && (id == propulsion->tankId || id == propulsion->thrusterId))
         {
            if (!propulsionQueued)
            {
               append([spacecraft, propulsion, lookup, path]
               {
                  // The packet owns its names/plans and leases dependencies
                  // through clone preparation, publication and retirement.
                  if (spacecraft->GetParameterID("Tanks") != propulsion->tankId ||
                      spacecraft->GetParameterID("Thrusters") != propulsion->thrusterId ||
                      spacecraft->GetParameterType(propulsion->tankId) != Gmat::OBJECTARRAY_TYPE ||
                      spacecraft->GetParameterType(propulsion->thrusterId) != Gmat::OBJECTARRAY_TYPE ||
                      (propulsion->returnedTanks && spacecraft->IsParameterReadOnly(propulsion->tankId)) ||
                      (propulsion->returnedThrusters && spacecraft->IsParameterReadOnly(propulsion->thrusterId)))
                     throw MatlabObjectError(path, "the native propulsion properties changed before assignment");
                  ObjectArray tanks, thrusters;
                  tanks.reserve(propulsion->tankSources.size());
                  thrusters.reserve(propulsion->thrusterSources.size());
                  for (const auto &source : propulsion->tankSources)
                  {
                     ValidateMatlabReference(source, path + ".Tanks", lookup);
                     tanks.push_back(source.object);
                  }
                  for (const auto &source : propulsion->thrusterSources)
                  {
                     ValidateMatlabReference(source, path + ".Thrusters", lookup);
                     thrusters.push_back(source.object);
                  }
                  spacecraft->ReplacePropulsionHardware(propulsion->tanks, tanks, propulsion->thrusters, thrusters);
               });
               propulsionQueued = true;
            }
            continue;
         }
         switch (type)
         {
            case Gmat::REAL_TYPE:
            case Gmat::TIME_TYPE:
            {
               const Real number = MatlabScalar(data, fieldPath);
               append([object, id, number] { object->SetRealParameter(id, number); });
               break;
            }
            case Gmat::INTEGER_TYPE:
            {
               const Integer number = MatlabInteger<Integer>(MatlabScalar(data, fieldPath), fieldPath);
               append([object, id, number] { object->SetIntegerParameter(id, number); });
               break;
            }
            case Gmat::UNSIGNED_INT_TYPE:
            {
               const UnsignedInt number = MatlabInteger<UnsignedInt>(MatlabScalar(data, fieldPath), fieldPath);
               append([object, id, number] { object->SetUnsignedIntParameter(id, number); });
               break;
            }
            case Gmat::BOOLEAN_TYPE:
            {
               const Real number = MatlabScalar(data, fieldPath);
               if (number != 0.0 && number != 1.0)
                  throw MatlabObjectError(fieldPath, "a Boolean value must be logical, zero or one");
               const bool flag = number == 1.0;
               append([object, id, flag] { object->SetBooleanParameter(id, flag); });
               break;
            }
            case Gmat::STRING_TYPE:
            case Gmat::ENUMERATION_TYPE:
            case Gmat::FILENAME_TYPE:
            case Gmat::COLOR_TYPE:
            case Gmat::EQUATION_TYPE:
            {
               const std::string text = MatlabText(data, fieldPath);
               append([object, id, text, fieldPath]
               {
                  if (!object->SetStringParameter(id, text))
                     throw MatlabObjectError(fieldPath, "the native string setter rejected the value");
               });
               break;
            }
            case Gmat::OBJECT_TYPE:
            {
               const std::string text = MatlabText(data, fieldPath);
               if (spacecraft && id == spacecraft->GetParameterID("PowerSystem"))
               {
                  const bool clear = text.empty() || text == "None";
                  const auto reference = clear ? MatlabReference() :
                     PlanMatlabReference(spacecraft, Gmat::POWER_SYSTEM, text,
                        spacecraft->GetStringParameter(id) == text, fieldPath, lookup);
                  const auto previous = spacecraft->GetFunctionValueContext();
                  append([spacecraft, text, fieldPath, reference, lookup, previous]
                  {
                     (void)previous;
                     ValidateMatlabReference(reference, fieldPath, lookup);
                     spacecraft->ReplacePowerSystem(text, reference.object);
                  });
                  break;
               }
               const bool owned = object->IsOwnedObject(id);
               const UnsignedInt expected = owned ? Gmat::UNKNOWN_OBJECT : object->GetPropertyObjectType(id);
               const auto reference = PlanMatlabReference(object, expected, text,
                  object->GetStringParameter(id) == text, fieldPath, lookup);
               const auto previous = object->GetFunctionValueContext();
               append([object, id, text, fieldPath, reference, lookup, previous]
               {
                  (void)previous; // Retain the previous dependency context through native assignment.
                  ValidateMatlabReference(reference, fieldPath, lookup);
                  if (reference.type != Gmat::UNKNOWN_OBJECT && object->GetStringParameter(id) == text &&
                      MatlabCurrentReference(object, reference.type, text) == reference.object)
                     return;
                  if (!object->SetStringParameter(id, text))
                     throw MatlabObjectError(fieldPath, "the native object-name setter rejected the value");
                  BindMatlabReference(object, reference, fieldPath);
               });
               break;
            }
            case Gmat::ON_OFF_TYPE:
            {
               const std::string text = MatlabText(data, fieldPath);
               if (text != "On" && text != "Off")
                  throw MatlabObjectError(fieldPath, "On or Off is required");
               append([object, id, text, fieldPath]
               {
                  if (!object->SetOnOffParameter(id, text))
                     throw MatlabObjectError(fieldPath, "the native On/Off setter rejected the value");
               });
               break;
            }
            case Gmat::RVECTOR_TYPE:
            {
               const RealArray numbers = MatlabVector(data, fieldPath);
               if (numbers.size() != static_cast<size_t>(object->GetRvectorParameter(id).GetSize()))
                  throw MatlabObjectError(fieldPath, "the vector size differs from the native property");
               Rvector vector(static_cast<Integer>(numbers.size()));
               for (Integer j = 0; j < vector.GetSize(); ++j) vector[j] = numbers[j];
               append([object, id, vector] { object->SetRvectorParameter(id, vector); });
               break;
            }
            case Gmat::RMATRIX_TYPE:
            {
               const Rmatrix shape = object->GetRmatrixParameter(id);
               if (!MatlabNumeric(data) || data.rows != shape.GetNumRows() || data.columns != shape.GetNumColumns())
                  throw MatlabObjectError(fieldPath, "the numeric matrix shape differs from the native property");
               Rmatrix matrix(data.rows, data.columns);
               for (Integer column = 0; column < data.columns; ++column)
                  for (Integer row = 0; row < data.rows; ++row)
                     matrix(row, column) = data.numbers[static_cast<size_t>(column) * data.rows + row];
               append([object, id, matrix] { object->SetRmatrixParameter(id, matrix); });
               break;
            }
            case Gmat::GMATTIME_TYPE:
            {
               const GmatTime time = MatlabPreciseTime(data, fieldPath);
               append([object, id, time] { object->SetGmatTimeParameter(id, time); });
               break;
            }
            case Gmat::BOOLEANARRAY_TYPE:
            {
               const BooleanArray flags = MatlabBooleans(data, fieldPath);
               append([object, id, flags, fieldPath]
               {
                  if (!object->SetBooleanArrayParameter(id, flags))
                     throw MatlabObjectError(fieldPath, "the native Boolean-array setter rejected the value");
               });
               break;
            }
            case Gmat::INTARRAY_TYPE:
            {
               const RealArray numbers = MatlabVector(data, fieldPath);
               if (numbers.size() != object->GetIntegerArrayParameter(id).size())
                  throw MatlabObjectError(fieldPath, "the integer-array size differs from the native property");
               IntegerArray integers;
               for (const Real number : numbers) integers.push_back(MatlabInteger<Integer>(number, fieldPath));
               append([object, id, integers]
               {
                  for (size_t j = 0; j < integers.size(); ++j)
                     object->SetIntegerParameter(id, integers[j], static_cast<Integer>(j));
               });
               break;
            }
            case Gmat::UNSIGNED_INTARRAY_TYPE:
            {
               const RealArray numbers = MatlabVector(data, fieldPath);
               if (numbers.size() != object->GetUnsignedIntArrayParameter(id).size())
                  throw MatlabObjectError(fieldPath, "the unsigned-array size differs from the native property");
               UnsignedIntArray integers;
               for (const Real number : numbers) integers.push_back(MatlabInteger<UnsignedInt>(number, fieldPath));
               append([object, id, integers]
               {
                  for (size_t j = 0; j < integers.size(); ++j)
                     object->SetUnsignedIntParameter(id, integers[j], static_cast<Integer>(j));
               });
               break;
            }
            case Gmat::STRINGARRAY_TYPE:
            case Gmat::OBJECTARRAY_TYPE:
            {
               const StringArray strings = MatlabStrings(data, fieldPath);
               const StringArray current = object->GetStringArrayParameter(id);
               std::vector<MatlabReference> references;
               if (type == Gmat::OBJECTARRAY_TYPE && !object->IsOwnedObject(id))
                  for (size_t j = 0; j < strings.size(); ++j)
                     references.push_back(PlanMatlabReference(object,
                        object->GetPropertyObjectType(id, static_cast<Integer>(j)), strings[j],
                        std::find(current.begin(), current.end(), strings[j]) != current.end(), fieldPath, lookup));
               const auto previous = object->GetFunctionValueContext();
               append([object, id, strings, fieldPath, references, lookup, previous]
               {
                  (void)previous; // Retain the previous dependency context through native assignment.
                  for (const auto &reference : references) ValidateMatlabReference(reference, fieldPath, lookup);
                  const bool changedNames = object->GetStringArrayParameter(id) != strings;
                  object->SetField(id, strings);
                  bool changedBindings = false;
                  for (const auto &reference : references)
                     changedBindings = BindMatlabReference(object, reference, fieldPath) || changedBindings;
                  if ((changedNames || changedBindings) && !strings.empty())
                     if (auto *formation = dynamic_cast<FormationInterface*>(object)) formation->BuildState();
               });
               break;
            }
            case Gmat::GENERIC_TYPE:
            {
               const Generic previous = object->GetGenericParameter(id);
               Generic result;
               switch (previous.VarIndex())
               {
                  case 0: result = MatlabScalar(data, fieldPath); break;
                  case 1: result = MatlabInteger<Integer>(MatlabScalar(data, fieldPath), fieldPath); break;
                  case 2: result = MatlabText(data, fieldPath); break;
                  case 3: result = MatlabVector(data, fieldPath); break;
                  case 4:
                  {
                     IntegerArray integers;
                     for (const Real number : MatlabVector(data, fieldPath))
                        integers.push_back(MatlabInteger<Integer>(number, fieldPath));
                     result = integers;
                     break;
                  }
                  case 5: result = MatlabStrings(data, fieldPath); break;
                  default: throw MatlabObjectError(fieldPath, "unsupported native Generic subtype");
               }
               append([object, id, result] { object->SetGenericParameter(id, result); });
               break;
            }
            default:
               throw MatlabObjectError(fieldPath, "typed transfer is unavailable for native property type " +
                                                      object->GetParameterTypeString(id));
         }
      }
   }

   MatlabValue NativeMatlabText(const std::string &text)
   {
      MatlabValue value;
      value.type = MatlabValue::Type::CHARACTER;
      value.text = text;
      return value;
   }

   MatlabValue NativeMatlabVector(const RealArray &numbers, bool logical = false)
   {
      if (numbers.size() > static_cast<size_t>(std::numeric_limits<Integer>::max()))
         throw InterfaceException("Native MATLAB input exceeds element capacity.");
      MatlabValue value;
      value.type = logical ? MatlabValue::Type::LOGICAL : MatlabValue::Type::NUMERIC;
      value.rows = numbers.empty() ? 0 : 1;
      value.columns = static_cast<Integer>(numbers.size());
      value.numbers = numbers;
      return value;
   }

   MatlabValue NativeMatlabStrings(const StringArray &strings)
   {
      if (strings.size() > static_cast<size_t>(std::numeric_limits<Integer>::max()))
         throw InterfaceException("Native MATLAB list input exceeds element capacity.");
      MatlabValue value;
      value.type = MatlabValue::Type::CELL;
      value.rows = strings.empty() ? 0 : 1;
      value.columns = static_cast<Integer>(strings.size());
      for (const auto &text : strings) value.children.push_back(NativeMatlabText(text));
      return value;
   }

   bool CaptureNativeMatlabInput(GmatBase &object, Integer id, MatlabValue &value)
   {
      switch (object.GetParameterType(id))
      {
         case Gmat::REAL_TYPE:
         case Gmat::TIME_TYPE:
            value = NativeMatlabVector({object.GetRealParameter(id)}); break;
         case Gmat::INTEGER_TYPE:
            value = NativeMatlabVector({static_cast<Real>(object.GetIntegerParameter(id))}); break;
         case Gmat::UNSIGNED_INT_TYPE:
            value = NativeMatlabVector({static_cast<Real>(object.GetUnsignedIntParameter(id))}); break;
         case Gmat::BOOLEAN_TYPE:
            value = NativeMatlabVector({object.GetBooleanParameter(id) ? 1.0 : 0.0}, true); break;
         case Gmat::STRING_TYPE:
         case Gmat::ENUMERATION_TYPE:
         case Gmat::FILENAME_TYPE:
         case Gmat::OBJECT_TYPE:
         case Gmat::COLOR_TYPE:
         case Gmat::EQUATION_TYPE:
            value = NativeMatlabText(object.GetStringParameter(id)); break;
         case Gmat::ON_OFF_TYPE:
            value = NativeMatlabText(object.GetOnOffParameter(id)); break;
         case Gmat::STRINGARRAY_TYPE:
         case Gmat::OBJECTARRAY_TYPE:
            value = NativeMatlabStrings(object.GetStringArrayParameter(id)); break;
         case Gmat::INTARRAY_TYPE:
         {
            RealArray numbers;
            for (const Integer number : object.GetIntegerArrayParameter(id)) numbers.push_back(number);
            value = NativeMatlabVector(numbers);
            break;
         }
         case Gmat::UNSIGNED_INTARRAY_TYPE:
         {
            RealArray numbers;
            for (const UnsignedInt number : object.GetUnsignedIntArrayParameter(id)) numbers.push_back(number);
            value = NativeMatlabVector(numbers);
            break;
         }
         case Gmat::BOOLEANARRAY_TYPE:
         {
            RealArray numbers;
            for (const bool flag : object.GetBooleanArrayParameter(id)) numbers.push_back(flag ? 1.0 : 0.0);
            value = NativeMatlabVector(numbers, true);
            break;
         }
         case Gmat::RVECTOR_TYPE:
         {
            const Rvector vector = object.GetRvectorParameter(id);
            RealArray numbers;
            for (Integer i = 0; i < vector.GetSize(); ++i) numbers.push_back(vector[i]);
            value = NativeMatlabVector(numbers);
            break;
         }
         case Gmat::RMATRIX_TYPE:
         {
            const Rmatrix matrix = object.GetRmatrixParameter(id);
            value.type = MatlabValue::Type::NUMERIC;
            value.rows = matrix.GetNumRows();
            value.columns = matrix.GetNumColumns();
            if (value.rows < 0 || value.columns < 0 ||
                (value.columns && value.rows > std::numeric_limits<Integer>::max() / value.columns))
               throw InterfaceException("Native MATLAB matrix input exceeds element capacity.");
            for (Integer column = 0; column < value.columns; ++column)
               for (Integer row = 0; row < value.rows; ++row) value.numbers.push_back(matrix(row, column));
            break;
         }
         case Gmat::GMATTIME_TYPE:
            value = NativeMatlabText(object.GetGmatTimeParameter(id).ToString()); break;
         case Gmat::GENERIC_TYPE:
         {
            const Generic generic = object.GetGenericParameter(id);
            switch (generic.VarIndex())
            {
               case 0: value = NativeMatlabVector({VarGet<Real>(generic)}); break;
               case 1: value = NativeMatlabVector({static_cast<Real>(VarGet<Integer>(generic))}); break;
               case 2: value = NativeMatlabText(VarGet<std::string>(generic)); break;
               case 3: value = NativeMatlabVector(VarGet<RealArray>(generic)); break;
               case 4:
               {
                  RealArray numbers;
                  for (const Integer number : VarGet<IntegerArray>(generic)) numbers.push_back(number);
                  value = NativeMatlabVector(numbers);
                  break;
               }
               case 5: value = NativeMatlabStrings(VarGet<StringArray>(generic)); break;
               default: return false;
            }
            break;
         }
         default: return false; // Native class-specific serialization retains unsupported families.
      }
      return true;
   }

   StringArray MatlabDeclaredBindings(const StringArray &inputs, const StringArray &outputs,
                                     const std::string &function)
   {
      StringArray excluded;
      for (const auto *list : {&inputs, &outputs})
         for (const auto &expression : *list)
         {
            excluded.push_back(expression);
            excluded.push_back(expression.substr(0, expression.find_first_of(".(")));
         }
      if (!function.empty()) excluded.push_back(function);
      return excluded;
   }

   bool SupportsMatlabOutput(GmatBase *object)
   {
      if (!object) return false;
      Parameter *parameter = dynamic_cast<Parameter*>(object);
      // Native whole-object results use staged typed structure properties.
      if (!parameter) return true;
      // Native user Variables, Strings and Arrays use writable value setters
      // even though the system-Parameter IsSettable flag defaults to false.
      const std::string &name = parameter->GetTypeName();
      if (name == "Variable" || name == "String" || name == "Array") return true;
      if (!parameter->IsSettable()) return false;
      const auto type = parameter->GetReturnType();
      return type == Gmat::REAL_TYPE ||
         type == Gmat::STRING_TYPE || type == Gmat::RVECTOR_TYPE ||
         type == Gmat::RMATRIX_TYPE;
   }

   template <typename Lookup>
   std::string ResolveMatlabArrayElement(const std::string &expression,
                                        const Lookup &lookup);

   template <typename Lookup>
   Real MatlabIndexValue(const std::string &expression, const Lookup &lookup)
   {
      Real value;
      if (GmatStringUtil::ToReal(expression, value))
         return value;

      if (expression.find('(') != std::string::npos)
      {
         const std::string resolved = ResolveMatlabArrayElement(expression, lookup);
         std::string name, rowText, columnText;
         Integer row, column;
         GmatStringUtil::GetArrayIndex(resolved, rowText, columnText, row, column, name);
         Array *array = dynamic_cast<Array*>(lookup(name));
         return array->GetRealParameter("SingleValue", row, column);
      }

      Parameter *parameter = dynamic_cast<Parameter*>(lookup(expression));
      if (!parameter || parameter->GetReturnType() != Gmat::REAL_TYPE)
         throw CommandException("MATLAB array element index must resolve to a real "
                                "number or Parameter: " + expression);
      return parameter->EvaluateReal();
   }

   Integer MatlabArrayIndex(Real value, Integer limit, const std::string &expression)
   {
      // Match native ArrayElementWrapper's integer, one-based index contract.
      // Check the real value before converting it, including overflow cases.
      if (!std::isfinite(value) || value < 1 || value > limit ||
          std::floor(value) != value)
         throw CommandException("MATLAB array element index must be an integer "
            "within the configured array dimensions: " + expression);
      return static_cast<Integer>(value);
   }

   template <typename Lookup>
   std::string ResolveMatlabArrayElement(const std::string &expression,
                                        const Lookup &lookup)
   {
      // IsParenPartOfArray deliberately recognizes only simple indices.
      // GetArrayIndexVar also handles recursively indexed native Arrays.
      if (expression.find('(') == std::string::npos)
         return expression;

      std::string name, rowText, columnText;
      GmatStringUtil::GetArrayIndexVar(expression, rowText, columnText, name);
      Array *array = dynamic_cast<Array*>(lookup(name));
      if (!array)
         throw CommandException("MATLAB array element does not reference an Array: " + expression);
      const Integer row = MatlabArrayIndex(MatlabIndexValue(rowText, lookup),
                                           array->GetRowCount(), expression);
      const Integer column = MatlabArrayIndex(MatlabIndexValue(columnText, lookup),
                                              array->GetColCount(), expression);
      return name + "(" + std::to_string(row) + "," + std::to_string(column) + ")";
   }
}

// If using MatlabInterface::EvalMatlabString() to send data to Matlab workspace
//#define __USE_EVAL_STRING__

//#define DEBUG_CALL_FUNCTION_PARAM
//#define DEBUG_CALL_FUNCTION_INIT
//#define DEBUG_CALL_FUNCTION_EXEC
//#define DEBUG_SEND_PARAM
//#define DEBUG_UPDATE_VAR
//#define DEBUG_UPDATE_OBJECT
//#define DEBUG_SHOW_ARRAY
//#define DEBUG_GET_OUTPUT
//#define DEBUG_GMAT_FUNCTION_INIT
//#define DEBUG_OBJECT_MAP
//#define DEBUG_GLOBAL_OBJECT_MAP
//#define DEBUG_RUN_COMPLETE
//#define DEBUG_MATLAB_EXEC
//#define DEBUG_MATLAB_EVAL
//#define DEBUG_CALLING_OBJECT

//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif
//#ifndef DEBUG_TRACE
//#define DEBUG_TRACE
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif
#ifdef DEBUG_TRACE
#include <ctime>                 // for clock()
#endif

//---------------------------------
// static data
//---------------------------------

//------------------------------------------------------------------------------
// CallMatlabFunction::CallMatlabFunction()
//------------------------------------------------------------------------------
CallMatlabFunction::CallMatlabFunction() :
   CallFunction("CallMatlabFunction")
{
   objectTypeNames.push_back("CallMatlabFunction");
}


//------------------------------------------------------------------------------
// ~CallMatlabFunction()
//------------------------------------------------------------------------------
CallMatlabFunction::~CallMatlabFunction()
{
}


//------------------------------------------------------------------------------
// CallMatlabFunction(const CallMatlabFunction& cf) :
//------------------------------------------------------------------------------
CallMatlabFunction::CallMatlabFunction(const CallMatlabFunction& cf) :
   CallFunction (cf)
{
}


//------------------------------------------------------------------------------
// CallMatlabFunction& operator=(const CallMatlabFunction& cf)
//------------------------------------------------------------------------------
CallMatlabFunction& CallMatlabFunction::operator=(const CallMatlabFunction& cf)
{
   if (this == &cf)
      return *this;
   
   CallFunction::operator=(cf);
   
   return *this;
}


//------------------------------------------------------------------------------
// std::string FormEvalString()
//  String format
//    [Out1, Out2] = FunctionName(In1, In2, In3) or
//    [arrA(3,3)] = FunctionName(arrB(2,2)) - Implemented on 2013.01.04 by LOJ
//------------------------------------------------------------------------------
std::string CallMatlabFunction::FormEvalString()
{
   StringArray inputs, outputs;
   for (const auto &name : mInputNames)
      inputs.push_back(ResolveMatlabArrayElement(name,
         [this](const std::string &object) { return FindObject(object); }));
   for (const auto &name : mOutputNames)
      outputs.push_back(ResolveMatlabArrayElement(name,
         [this](const std::string &object) { return FindObject(object); }));
   return MatlabInvocation(ResolveMatlabFunctionSource(*mFunction).name, inputs, outputs);
}

//------------------------------------------------------------------------------
//  GmatBase* Clone(void) const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the CallMatlabFunction.
 *
 * @return clone of the CallMatlabFunction.
 *
 */
//------------------------------------------------------------------------------
GmatBase* CallMatlabFunction::Clone() const
{
   return (new CallMatlabFunction(*this));
}


//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
bool CallMatlabFunction::Initialize()
{
   #ifdef DEBUG_CALL_FUNCTION_INIT
   MessageInterface::ShowMessage
      ("CallMatlabFunction::Initialize() this=<%p> entered, command = '%s'\n   "
       "function type is '%s', callingFunction is '%s'\n", this,
       GetGeneratingString(Gmat::NO_COMMENTS).c_str(), mFunction->GetTypeName().c_str(),
       callingFunction? (callingFunction->GetFunctionName()).c_str() : "NULL");
   #endif
   
   if (!GmatGlobal::Instance()->IsMatlabAvailable())
      throw CommandException
         ("MATLAB is not installed.  GMAT will not run if any CallMatlabFunction"
          " uses MATLAB function");
   
   bool rv = true;  // Initialization return value
   
   rv = CallFunction::Initialize();
   
   #ifdef DEBUG_OBJECT_MAP
   ShowObjectMaps("In CallMatlabFunction::Initialize()");
   #endif
   
   // We need to add all Matlab paths to the bottom of the path using path(path, 'newpath')
   // since FileManager::GetAllMatlabFunctionPaths() returns in top to bottom order
   if (isMatlabFunction)
   {
      // Get Matlab and FileManager instance
      matlabIf = MatlabInterface::Instance();
      FileManager *fm = FileManager::Instance();
      
      // Open Matlab engine
      if (!OpenMatlabFunctionEngine(*matlabIf))
         throw CommandException("Cannot open the GMAT MATLAB engine session.");
      
      // Change directory to working directory where GMAT application is so that
      // relative path specified in the startup file works. (LOJ: 2012.09.24)
      std::string workingDir = fm->GetCurrentWorkingDirectory();
      #ifdef DEBUG_CALL_FUNCTION_INIT
      MessageInterface::ShowMessage("   Changing working dir to '%s'\n", workingDir.c_str());
      #endif
      EvalMatlabString("cd(" + MatlabInterface::CharacterLiteral(workingDir) + ");");
      
      #ifdef DEBUG_CALLING_OBJECT
      MessageInterface::ShowMessage
         ("   Setting calling object '%s'\n", mFunctionPathAndName.c_str());
      #endif
      
      // Set calling function name
      matlabIf->SetCallingObjectName(mFunctionPathAndName);
      
      // Get all matlab function paths from the startup file
      StringArray paths = fm->GetAllMatlabFunctionPaths();
      
      #ifdef DEBUG_CALL_FUNCTION_INIT
      MessageInterface::ShowMessage("   Found %d matlab path\n", paths.size());
      #endif
      
      // Add path to the top of the Matlab path in reverse order(loj: 2008.10.16)
      std::string pathName;
      StringArray::reverse_iterator rpos = paths.rbegin();
      std::string addPath;
      while (rpos != paths.rend())
      {
         pathName = *rpos;
         if (pathName != "")
         {
            #ifdef DEBUG_CALL_FUNCTION_INIT
            MessageInterface::ShowMessage
               ("   Adding matlab path '%s' to the top\n", pathName.c_str());
            #endif
            addPath = "path(" + MatlabInterface::CharacterLiteral(pathName) + ", path);";
            EvalMatlabString(addPath);
         }
         rpos++;
      }
   
      // Initialize input parameters only for MatlabFunction
      // Initialization of GmatFunctions are handled by FunctionManager during
      // execution
      
      // add input/output parameters
      mInputList.clear();
      GmatBase *mapObj;
      // need to initialize input parameters
      for (StringArray::iterator i = mInputNames.begin();
           i != mInputNames.end(); ++i)
      {
         Real literal;
         if (GmatScriptString::IsLiteral(*i) || GmatStringUtil::ToReal(*i, literal))
         {
            if (GmatScriptString::IsLiteral(*i)) GmatScriptString::Decode(*i);
            // Constants have no borrowed native object. Execution binds them
            // to private, per-call workspace slots instead of authored text.
            mInputList.push_back(NULL);
            continue;
         }
         if ((mapObj = FindObject(*i))  == NULL)
            throw CommandException("CallMatlabFunction command cannot find Parameter " +
                                   *i + " in script line\n   \"" +
                                   GetGeneratingString(Gmat::SCRIPTING) + "\"");
         
         #ifdef DEBUG_CALL_FUNCTION_INIT
         MessageInterface::ShowMessage("   Adding input parameter '%s'\n", i->c_str());
         #endif
         
         mInputList.push_back(mapObj);
      }
      
      // need to initialize output parameters
      mOutputList.clear();
      
      for (StringArray::iterator i = mOutputNames.begin();
           i != mOutputNames.end();++i)
      {
         if ((mapObj = FindObject(*i))  == NULL)
            throw CommandException("CallMatlabFunction command cannot find Parameter " + (*i));

         if (!SupportsMatlabOutput(mapObj))
            throw CommandException("Unsupported or non-settable MATLAB output: " + *i);
         // Formal inputs can also receive results. Whole-object formals use
         // the same staged typed property validation as other destinations.

         #ifdef DEBUG_CALL_FUNCTION_INIT
         MessageInterface::ShowMessage("   Adding output parameter '%s'\n", i->c_str());
         #endif
         
         mOutputList.push_back(mapObj);
      }
      
      if (mOutputList.size() > 0)
         if (mOutputList[0] == NULL)
         {
            MessageInterface::PopupMessage
               (Gmat::WARNING_,
                "CallMatlabFunction::Initialize() CallMatlabFunction will not be created.\n"
                "The first parameter selected as output for the CallMatlabFunction is NULL\n");
            return false;
         }
   }
   
   #ifdef DEBUG_CALL_FUNCTION_INIT
   MessageInterface::ShowMessage
      ("CallMatlabFunction::Initialize() this=<%p> returning %d\n", this, rv);
   #endif
   return rv;
}


//------------------------------------------------------------------------------
// bool Execute()
//------------------------------------------------------------------------------
bool CallMatlabFunction::Execute()
{
   bool status = false;
   
   if (mFunction == NULL)
      throw CommandException("Function is not defined for CallMatlabFunction");
   
   #ifdef DEBUG_TRACE
   static Integer callCount = 0;
   callCount++;      
   clock_t t1 = clock();
   MessageInterface::ShowMessage
      ("=== CallMatlabFunction::Execute() entered, '%s' Count = %d\n",
       GetGeneratingString(Gmat::NO_COMMENTS).c_str(), callCount);
   #endif
   
   #ifdef DEBUG_CALL_FUNCTION_EXEC
   MessageInterface::ShowMessage
      ("CallMatlabFunction::Execute() this=<%p> entered, command = '%s'\n   "
       "function type is '%s', callingFunction is '%s'\n", this,
       GetGeneratingString(Gmat::NO_COMMENTS).c_str(), mFunction->GetTypeName().c_str(),
       callingFunction? (callingFunction->GetFunctionName()).c_str() : "NULL");
      #ifdef DEBUG_OBJECT_MAP
      ShowObjectMaps("object maps at the start");
      #endif
   #endif
      
   if (isMatlabFunction)
   {
      #ifdef DEBUG_CALL_FUNCTION_EXEC
      MessageInterface::ShowMessage("   calling ExecuteMatlabFunction()\n");
      #endif
      
      for (Integer attempt = 0; attempt < 2; ++attempt)
      {
         try
         {
            status = ExecuteMatlabFunction();
            break;
         }
         catch (MatlabEngineConnectionException &)
         {
            if (attempt != 0) throw;
            // Retry the command boundary once. A nested Execute from an input
            // helper would resume the outer command and execute it again.
            MessageInterface::ShowMessage("MATLAB connection lost; reconnecting once...\n");
            matlabIf->Close();
            if (!Initialize())
               throw CommandException("MATLAB command initialization failed after reconnecting the engine.");
         }
      }
      BuildCommandSummary(true);
      
      #ifdef DEBUG_CALL_FUNCTION_EXEC
      MessageInterface::ShowMessage
         ("CallMatlabFunction::Execute() MatlabFunction exiting with %d\n", status);
         #ifdef DEBUG_OBJECT_MAP
         ShowObjectMaps("object maps at the end");
         #endif
      #endif
   }
   
   
   #ifdef DEBUG_TRACE
   clock_t t2 = clock();
   MessageInterface::ShowMessage
      ("=== CallMatlabFunction::Execute() exiting, '%s' Count = %d, Run Time: %f seconds\n",
       GetGeneratingString(Gmat::NO_COMMENTS).c_str(), callCount, (Real)(t2-t1)/CLOCKS_PER_SEC);
   #endif
   
   return status;
}


//------------------------------------------------------------------------------
// void RunComplete()
//------------------------------------------------------------------------------
void CallMatlabFunction::RunComplete()
{
   #ifdef DEBUG_RUN_COMPLETE
   MessageInterface::ShowMessage
      ("CallMatlabFunction::RunComplete() entered for this=<%p> '%s',\n   "
       "FCS %sfinalized\n", this, GetGeneratingString(Gmat::NO_COMMENTS).c_str(),
       fm.IsFinalized() ? "already " : "NOT ");
   #endif
   
   if (!fm.IsFinalized())
   {
      #ifdef DEBUG_RUN_COMPLETE
      MessageInterface::ShowMessage("   calling FunctionManager::Finalize()\n");
      #endif
      fm.Finalize();
   }
   
   GmatCommand::RunComplete();
}


//------------------------------------------------------------------------------
// bool ExecuteMatlabFunction()
//------------------------------------------------------------------------------
bool CallMatlabFunction::ExecuteMatlabFunction()
{
   #ifdef DEBUG_MATLAB_EXEC
   MessageInterface::ShowMessage
      ("CallMatlabFunction::ExecuteMatlabFunction() entered, mFunctionName='%s', "
       "function=<%p>\n", mFunctionName.c_str(), mFunction);
   if (mFunction)
      MessageInterface::ShowMessage
         ("   functionPath='%s', functionName='%s'\n",
          mFunction->GetStringParameter("FunctionPath").c_str(),
          mFunction->GetStringParameter("FunctionName").c_str());
   #endif
   
   #ifdef DEBUG_CALLING_OBJECT
   MessageInterface::ShowMessage
      ("CallMatlabFunction::ExecuteMatlabFunction() setting calling object '%s'\n",
       mFunctionPathAndName.c_str());
   #endif
   
   matlabIf->SetCallingObjectName(mFunctionPathAndName);
   
   // open Matlab engine
   #ifdef DEBUG_MATLAB_EXEC
   MessageInterface::ShowMessage(".....Opening Matlab Engine\n");
   #endif
   if (!OpenMatlabFunctionEngine(*matlabIf))
      throw CommandException("Cannot open the GMAT MATLAB engine session.");
   
   // set format long so that we don't loose precision between string transmission
   #ifdef DEBUG_MATLAB_EXEC
   MessageInterface::ShowMessage(".....Sending format long\n");
   #endif
   EvaluateMatlabFunctionExpression(*matlabIf, "format long", false);
   
   // Clear last errormsg
   #ifdef DEBUG_MATLAB_EXEC
   MessageInterface::ShowMessage(".....Sending clear errormsg\n");
   #endif
   EvaluateMatlabFunctionExpression(*matlabIf, "clear errormsg", false);
   
   // add the path to the top of the path list using path('newpath', path)
   const auto source = ResolveMatlabFunctionSource(*mFunction);
   const std::string &thePath = source.directory;
   
   if (thePath != "")
   {
      #ifdef DEBUG_MATLAB_EXEC
      MessageInterface::ShowMessage(".....Sending path to use\n");
      #endif
      std::string setPath = "path(" + MatlabInterface::CharacterLiteral(thePath) + ", path);";
      EvalMatlabString(setPath);
   }
   
   StringArray inputs, targets;
   for (const auto &name : mInputNames)
      inputs.push_back(GmatScriptString::IsLiteral(name) ? name :
         ResolveMatlabArrayElement(name,
            [this](const std::string &object) { return FindObject(object); }));
   for (const auto &name : mOutputNames)
      targets.push_back(ResolveMatlabArrayElement(name,
         [this](const std::string &object) { return FindObject(object); }));
   if (inputs.size() != mInputList.size() || targets.size() != mOutputList.size())
      throw CommandException("MATLAB argument bindings do not match the call declaration.");
   for (GmatBase *output : mOutputList)
      if (!SupportsMatlabOutput(output))
         throw CommandException("A MATLAB output is no longer settable or has an unsupported type.");
   MatlabWorkspaceVariables inputVariables(*matlabIf, inputs.size(),
      MatlabDeclaredBindings(mInputNames, mOutputNames, mFunction ? mFunction->GetName() : ""));
   MatlabWorkspaceVariables resultVariables(*matlabIf, targets.size(),
      MatlabDeclaredBindings(mInputNames, mOutputNames, mFunction ? mFunction->GetName() : ""));
   for (size_t i = 0; i < targets.size(); ++i) resultVariables.Add();
   MatlabWorkspaceVariables callableVariables(*matlabIf, 1,
      MatlabDeclaredBindings(mInputNames, mOutputNames, source.name));
   std::string callableName = source.name;
   if (source.explicitFile)
   {
      // A resource alias can differ from its source filename. A handle keeps
      // a same-named input variable from shadowing that source function.
      callableVariables.Add();
      callableName = callableVariables.Names().front();
      EvalMatlabString(callableName + " = @" + source.name + ";");
   }

   // send the in parameters
   for (unsigned int i=0; i<mInputList.size(); i++)
   {
      GmatBase *param = mInputList[i];
      #ifdef DEBUG_MATLAB_EXEC
      MessageInterface::ShowMessage
         (".....Sending input parameter <%p> '%s', %d out of %d\n", param,
          mInputNames[i].c_str(), i+1, mNumInputParams);
      #endif
      if (param)
         SendInParam(param, inputs[i]);
      else
      {
         inputVariables.Add();
         inputs[i] = inputVariables.Names().back();
         if (GmatScriptString::IsLiteral(mInputNames[i]))
            SendInputValue(inputs[i], NativeMatlabText(GmatScriptString::Decode(mInputNames[i])));
         else
         {
            Real value;
            if (!GmatStringUtil::ToReal(mInputNames[i], value))
               throw CommandException("An unbound MATLAB input is not a constant: " + mInputNames[i]);
            EvalMatlabString(inputs[i] + " = " + MatlabRealText(value) + ";");
         }
      }
   }
   
   //  Eval String
   #ifdef DEBUG_MATLAB_EXEC
   MessageInterface::ShowMessage(".....Forming evaluate string\n");
   #endif
   std::string evalString = MatlabInvocation(callableName, inputs, resultVariables.Names());
   EvalMatlabString(evalString);
   
   // get the value for the out parameters
   #ifdef DEBUG_MATLAB_EXEC
   MessageInterface::ShowMessage(".....Getting output parameters\n");
   #endif
   auto assignments = ReadOutParams(resultVariables.Names(), targets);
   // Preserve the documented workspace bindings after validating independent
   // return slots. Frozen indices keep this publication separate from setters.
   if (!targets.empty())
   {
      std::string publish;
      for (size_t i = 0; i < targets.size(); ++i)
         publish += targets[i] + " = " + resultVariables.Names()[i] + ";";
      EvalMatlabString(publish);
   }
   // A failed read, workspace publication or cleanup cannot assign GMAT outputs.
   resultVariables.Clear();
   inputVariables.Clear();
   callableVariables.Clear();
   for (auto &assign : assignments) assign();
   
   return true;
   
}


//------------------------------------------------------------------------------
// void SendInParam(Parameter *param, const std::string &inStr)
//------------------------------------------------------------------------------
void CallMatlabFunction::SendInParam(Parameter *param, const std::string &inStr)
{
   SendInParam(static_cast<GmatBase*>(param), inStr);
}

void CallMatlabFunction::SendInParam(GmatBase *object, const std::string &inStr)
{
   if (!object)
      throw CommandException("A MATLAB input object is not bound: " + inStr);
   Parameter *param = dynamic_cast<Parameter*>(object);
   if (!param)
   {
      SendInputObject(object, inStr);
      return;
   }
   #ifdef DEBUG_SEND_PARAM
   MessageInterface::ShowMessage
      ("CallMatlabFunction::SendInParam() entered, param=<%p>'%s', inStr='%s'\n",
       param, param ? param->GetName().c_str() : "NULL", inStr.c_str());
   #endif
   
   if (param == NULL)
   {
      MessageInterface::ShowMessage("Parameter is null");
      return;
   }
   
   std::string paramName = inStr;
   if (inStr.find('(') != std::string::npos)
      paramName = inStr.substr(0, inStr.find('('));
   
   #ifdef DEBUG_SEND_PARAM
   MessageInterface::ShowMessage
      ("Parameter name=%s, type=%s\n", paramName.c_str(),
       param->GetTypeName().c_str());
   #endif
   
   if (param->GetTypeName() == "Array")
   {
      Array *array = (Array *)param;
      int numRows = array->GetIntegerParameter("NumRows");
      int numCols = array->GetIntegerParameter("NumCols");
      Rmatrix rmatrix = array->GetRmatrix();
      
      #ifdef DEBUG_SEND_PARAM
      MessageInterface::ShowMessage
         ("numRows=%d, numCols=%d\nrmatrix=\n%s\n", numRows, numCols,
          rmatrix.ToString(16).c_str());
      #endif
      
      // If sending array element, use EvalString (GMT-3324 fix, LOJ:2013.01.04)
      if (inStr.find('(') != std::string::npos)
      {
         std::string name, rowStr, colStr;
         Integer row, col;
         const std::string resolved = ResolveMatlabArrayElement(inStr,
            [this](const std::string &name) { return FindObject(name); });
         GmatStringUtil::GetArrayIndex(resolved, rowStr, colStr, row, col, name);
         
         #ifdef DEBUG_SEND_PARAM
         MessageInterface::ShowMessage
            ("'%s' is an array element, rows=%d, cols=%d\n", inStr.c_str(), row, col);
         #endif
         
         if (row == -1 || col == -1)
         {
            std::string msg =
               "Input array element index is invalid, it must be greater than 0 in " + inStr;
            #ifdef DEBUG_SEND_PARAM
            MessageInterface::ShowMessage("CallMatlabFunction::SendInParam() " + msg + "\n");
            #endif
            throw CommandException(msg);
         }
      }
      
      
      //----------------------------------------------------
      #ifdef __USE_EVAL_STRING__
      //----------------------------------------------------
      #ifdef DEBUG_SEND_PARAM
      MessageInterface::ShowMessage("===> Using EvalString\n");
      #endif
      std::ostringstream os;
      os.precision(18);
      
      for (int j=0; j<numRows; j++)
      {
         os << "[";
         for (int k=0; k<numCols; k++)
            os << rmatrix(j, k) << ",";
         os << "], \n";
      }
      
      std::string inParamString = paramName + " = [" + os.str() + "];";
      EvalMatlabString(inParamString);
      
      //----------------------------------------------------
      #else
      //----------------------------------------------------
      #ifdef DEBUG_SEND_PARAM
      MessageInterface::ShowMessage("===> Not using EvalString\n");
      #endif
      
      const Real *realArray = rmatrix.GetDataVector();
      
      #ifdef DEBUG_SEND_PARAM
      MessageInterface::ShowMessage("realArray = \n");
      for (int i = 0; i < numRows; i++)
         for (int j = 0; j < numCols; j++)
         {
            MessageInterface::ShowMessage("%f ", realArray[i*numCols+j]);
            if (j == numCols-1) MessageInterface::ShowMessage("\n");
         }
      MessageInterface::ShowMessage
         (".....Putting RealArray data <%p> to Matlab workspace\n", realArray);
      #endif
      matlabIf->PutRealArray(paramName, numRows, numCols, realArray);
      
      //----------------------------------------------------
      #endif
      //----------------------------------------------------
   }
   else if (param->GetTypeName() == "Variable")
   {
      EvalMatlabString(paramName + " = " + MatlabRealText(param->EvaluateReal()) + ";");
   }
   else if (param->GetTypeName() == "String")
   {
      StringVar *stringVar = (StringVar *)param;
      SendInputValue(paramName, NativeMatlabText(stringVar->GetString()));
   }
   else // Other Parameters or whole object
   {
      std::string inParamString;
      bool paramAllowed = true;
      if (param->IsOfType(Gmat::PARAMETER))
      {
         Gmat::ParameterType returnType = param->GetReturnType();
         #ifdef DEBUG_SEND_PARAM
         MessageInterface::ShowMessage("Parameter return type :%d\n", returnType);
         #endif
         if (returnType == Gmat::REAL_TYPE)
         {
            inParamString = paramName + " = " + MatlabRealText(param->EvaluateReal()) + ";";
         }
         else if (returnType == Gmat::STRING_TYPE)
         {
            // Evaluate Parameter first then get String value
            const std::string value = param->EvaluateString();
            #ifdef DEBUG_SEND_PARAM
            MessageInterface::ShowMessage("   value=<%s>\n", value.c_str());
            #endif
            SendInputValue(paramName, NativeMatlabText(value));
            return;
         }
         else if (returnType == Gmat::RVECTOR_TYPE)
         {
            const Rvector value = param->EvaluateRvector();
            inParamString = paramName + " = [";
            for (Integer i = 0; i < value.GetSize(); ++i)
               inParamString += (i ? " " : "") + MatlabRealText(value[i]);
            inParamString += "];";
         }
         else if (returnType == Gmat::RMATRIX_TYPE)
         {
            const Rmatrix value = param->EvaluateRmatrix();
            inParamString = paramName + " = [";
            for (Integer row = 0; row < value.GetNumRows(); ++row)
            {
               if (row) inParamString += "; ";
               for (Integer column = 0; column < value.GetNumColumns(); ++column)
                  inParamString += (column ? " " : "") + MatlabRealText(value(row, column));
            }
            inParamString += "];";
         }
         else
         {
            paramAllowed = false;
         }
         
         if (!paramAllowed)
         {
            std::string msg =
               "Unsupported MATLAB input Parameter type in " +
               GetGeneratingString(Gmat::NO_COMMENTS);
            #ifdef DEBUG_SEND_PARAM
            MessageInterface::ShowMessage("CallMatlabFunction::SendInParam() " + msg + "\n");
            #endif
            throw CommandException(msg);
         }
      }
      else
      {
         #ifdef DEBUG_SEND_PARAM
         MessageInterface::ShowMessage
            ("It is not a Parameter, so calling GetGeneratingString()\n");
         #endif
         
         if (param->GetTypeName() == "Spacecraft")
            param->TakeAction("UpdateEpoch");
         
         inParamString = param->GetGeneratingString(Gmat::MATLAB_STRUCT);
      }
      
      #ifdef DEBUG_SEND_PARAM
      MessageInterface::ShowMessage
         ("Generated param string :\n%s\n", inParamString.c_str());
      #endif
      
      EvalMatlabString(inParamString);
   }
}


void CallMatlabFunction::SendInputValue(const std::string &target, const MatlabValue &value)
{
   MatlabWorkspaceVariables temporary(*matlabIf, 1,
      MatlabDeclaredBindings(mInputNames, mOutputNames, mFunction ? mFunction->GetName() : ""));
   temporary.Add();
   matlabIf->PutValue(temporary.Names()[0], value);
   EvalMatlabString(target + " = " + temporary.Names()[0] + ";");
   temporary.Clear();
}

void CallMatlabFunction::SendInputObject(GmatBase *object, const std::string &target)
{
   const auto identity = object->GetLifetimeToken();
   if (object->GetTypeName() == "Spacecraft") object->TakeAction("UpdateEpoch");
   MatlabWorkspaceVariables temporary(*matlabIf, 2,
      MatlabDeclaredBindings(mInputNames, mOutputNames, mFunction ? mFunction->GetName() : ""));
   temporary.Add();
   temporary.Add();
   const std::string valuesName = temporary.Names()[0], objectName = temporary.Names()[1];
   MatlabValue values;
   values.type = MatlabValue::Type::CELL;
   std::string source;
   {
      const auto appendValue = [&](MatlabValue value, std::stringstream &text)
      {
         values.children.push_back(std::move(value));
         text << valuesName << "{" << values.children.size() << "}";
      };
      ObjectMatlabExportScope exportValues([&](GmatBase &owner, Integer id, std::stringstream &text)
      {
         MatlabValue value;
         if (!CaptureNativeMatlabInput(owner, id, value)) return false;
         appendValue(std::move(value), text);
         return true;
      }, [&](Real number, std::stringstream &text)
      {
         appendValue(NativeMatlabVector({number}), text);
      });
      source = object->GetGeneratingString(Gmat::MATLAB_STRUCT, "", objectName);
   }
   if (!GmatBase::IsLiveReference(object, identity))
      throw CommandException("MATLAB input object changed during serialization: " + target);
   if (source.empty())
      throw CommandException("Native MATLAB input serialization produced no fields: " + target);
   if (values.children.size() > static_cast<size_t>(std::numeric_limits<Integer>::max()))
      throw InterfaceException("Native MATLAB object input exceeds field capacity.");
   values.rows = values.children.empty() ? 0 : 1;
   values.columns = static_cast<Integer>(values.children.size());
   matlabIf->PutValue(valuesName, values);
   // Build a fresh scalar structure, then publish the completed value. A failed
   // native export or source evaluation cannot reuse stale fields in target.
   EvalMatlabString(objectName + " = struct();\n" + source);
   EvalMatlabString(target + " = " + objectName + ";");
   temporary.Clear();
}

//------------------------------------------------------------------------------
// void GetOutParams()
//------------------------------------------------------------------------------
void CallMatlabFunction::GetOutParams()
{
   // Retain the legacy protected entry point for native callers. Capture its
   // current workspace expressions into independent slots before any setters.
   MatlabWorkspaceVariables variables(*matlabIf, mOutputNames.size(),
      MatlabDeclaredBindings(mInputNames, mOutputNames, mFunction ? mFunction->GetName() : ""));
   StringArray targets;
   for (const auto &name : mOutputNames)
   {
      targets.push_back(ResolveMatlabArrayElement(name,
         [this](const std::string &object) { return FindObject(object); }));
      variables.Add();
      EvalMatlabString(variables.Names().back() + " = " + targets.back() + ";");
   }
   auto assignments = ReadOutParams(variables.Names(), targets);
   variables.Clear();
   for (auto &assign : assignments) assign();
}

std::vector<std::function<void()>> CallMatlabFunction::ReadOutParams(
   const StringArray &variables, const StringArray &targets)
{
   if (variables.size() != mOutputList.size() || targets.size() != mOutputList.size())
      throw CommandException("MATLAB result bindings do not match the call declaration.");
   std::vector<std::function<void()>> assignments;
   assignments.reserve(variables.size());
   try
   {
      for (size_t i = 0; i < variables.size(); ++i)
      {
         GmatBase *object = mOutputList[i];
         Parameter *parameter = dynamic_cast<Parameter*>(object);
         const std::string &target = targets[i];
         const std::string &variable = variables[i];
         if (!SupportsMatlabOutput(object))
            throw CommandException("Unsupported or non-settable MATLAB output: " + target);
         if (!parameter)
         {
            ReadObjectOutput(object, variable, target, assignments);
            continue;
         }
         const auto mismatch = [&]() -> CommandException
         {
            return CommandException("CallMatlabFunction cannot assign result to \"" +
               target + "\": type or dimension mismatch");
         };
         const auto readMatrix = [&](Integer rows, Integer columns) -> Rmatrix
         {
            if (rows <= 0 || columns <= 0 || rows > std::numeric_limits<Integer>::max() / columns)
               throw mismatch();
            const Integer count = rows * columns;
            std::vector<double> values(static_cast<size_t>(count));
            Integer receivedRows = 0, receivedColumns = 0;
            const Integer received = matlabIf->GetRealArray(variable, count,
               values.data(), receivedRows, receivedColumns);
            if (received != count || receivedRows != rows || receivedColumns != columns)
               throw mismatch();
            Rmatrix result(rows, columns);
            for (Integer row = 0; row < rows; ++row)
               for (Integer column = 0; column < columns; ++column)
                  result(row, column) = values[static_cast<size_t>(column) * rows + row];
            return result;
         };
         if (parameter->GetTypeName() == "Array")
         {
            Array *array = static_cast<Array*>(parameter);
            if (target.find('(') != std::string::npos)
            {
               std::string name, rowText, columnText;
               Integer row, column;
               GmatStringUtil::GetArrayIndex(target, rowText, columnText, row, column, name);
               if (row < 0 || column < 0 || row >= array->GetRowCount() || column >= array->GetColCount())
                  throw mismatch();
               const Real result = readMatrix(1, 1)(0, 0);
               assignments.emplace_back([array, result, row, column]
               {
                  array->SetRealParameter("SingleValue", result, row, column);
               });
            }
            else
            {
               const Rmatrix result = readMatrix(array->GetRowCount(), array->GetColCount());
               assignments.emplace_back([array, result]
               {
                  array->SetRmatrixParameter("RmatValue", result);
               });
            }
         }
         else if (parameter->GetReturnType() == Gmat::REAL_TYPE)
         {
            const Real result = readMatrix(1, 1)(0, 0);
            if (parameter->GetTypeName() == "Variable")
            {
               const std::string expression = MatlabRealText(result);
               assignments.emplace_back([parameter, result, expression]
               {
                  parameter->SetReal(result);
                  parameter->SetStringParameter("Expression", expression);
               });
            }
            else assignments.emplace_back([parameter, result] { parameter->SetReal(result); });
         }
         else if (parameter->GetReturnType() == Gmat::STRING_TYPE)
         {
            std::string result;
            if (matlabIf->GetString(variable, result) != 1) throw mismatch();
            if (parameter->GetTypeName() == "String")
               assignments.emplace_back([parameter, result]
               {
                  parameter->SetStringParameter("Expression", result);
               });
            else assignments.emplace_back([parameter, result] { parameter->SetString(result); });
         }
         else if (parameter->GetReturnType() == Gmat::RVECTOR_TYPE)
         {
            const Integer size = parameter->EvaluateRvector().GetSize();
            if (size <= 0) throw mismatch();
            std::vector<double> values(static_cast<size_t>(size));
            Integer rows = 0, columns = 0;
            const Integer received = matlabIf->GetRealArray(variable, size, values.data(), rows, columns);
            if (received != size || (rows != 1 && columns != 1)) throw mismatch();
            Rvector result(size);
            for (Integer j = 0; j < size; ++j) result[j] = values[j];
            assignments.emplace_back([parameter, result] { parameter->SetRvector(result); });
         }
         else if (parameter->GetReturnType() == Gmat::RMATRIX_TYPE)
         {
            const Rmatrix shape = parameter->EvaluateRmatrix();
            const Rmatrix result = readMatrix(shape.GetNumRows(), shape.GetNumColumns());
            assignments.emplace_back([parameter, result] { parameter->SetRmatrix(result); });
         }
         else throw mismatch();
      }
   }
   catch (BaseException &error)
   {
      const std::string message = error.GetFullMessage() + " in \n" +
         GetGeneratingString(Gmat::NO_COMMENTS);
      error.SetMessage("");
      error.SetDetails(message);
      throw;
   }
   return assignments;
}

void CallMatlabFunction::ReadObjectOutput(GmatBase *object,
   const std::string &variable, const std::string &target,
   std::vector<std::function<void()>> &assignments)
{
   if (!object) throw CommandException("MATLAB object output is not bound: " + target);
   const auto identity = object->GetLifetimeToken();
   const auto validate = [object, identity, target]
   {
      if (!GmatBase::IsLiveReference(object, identity))
         throw MatlabObjectError(target, "the output object was replaced before assignment");
   };
   const MatlabValue value = matlabIf->GetValue(variable);
   validate();
   StageMatlabObject(object, value, target, validate,
      [this](const std::string &name) { return FindObject(name); }, assignments);
}

//------------------------------------------------------------------------------
// void EvalMatlabString(const std::string &evalString)
//------------------------------------------------------------------------------
void CallMatlabFunction::EvalMatlabString(const std::string &evalString)
{
   #ifdef DEBUG_MATLAB_EVAL
   MessageInterface::ShowMessage
      ("CallMatlabFunction::EvalMatlabString() calling MI::RunMatlabString() with\n"
       "======================================================================\n"
       "%s\n\n", evalString.c_str());
   #endif
   
   EvaluateMatlabFunctionExpression(*matlabIf, evalString);
}


//------------------------------------------------------------------------------
// void ClearInputParameters()
//------------------------------------------------------------------------------
void CallMatlabFunction::ClearInputParameters()
{
   mInputList.clear();
   mInputNames.clear();
   mNumInputParams = 0;
}


//------------------------------------------------------------------------------
// void ClearOutputParameters()
//------------------------------------------------------------------------------
void CallMatlabFunction::ClearOutputParameters()
{
   mOutputList.clear();
   mOutputNames.clear();
   mNumOutputParams = 0;
}


//------------------------------------------------------------------------------
// void UpdateObject(GmatBase *obj, char *buffer)
//------------------------------------------------------------------------------
void CallMatlabFunction::UpdateObject(GmatBase *obj, char *buffer)
{
   if (obj == NULL)
   {
      #ifdef DEBUG_UPDATE_OBJECT
      MessageInterface::ShowMessage
         ("CallMatlabFunction::UpdateObject() entered, obj is NULL, so just leaving\n");
      #endif
      return;
   }
   
   StringTokenizer st(buffer, "=\n");
   StringArray tokens = st.GetAllTokens();

   // Mac/Linux sometimes return wuth the MATLAB prompt at head of the string
   if (tokens[0] == ">> ")
      tokens.erase(tokens.begin());

   int numTokens = tokens.size();
   
   #ifdef DEBUG_UPDATE_OBJECT
   MessageInterface::ShowMessage
      ("CallMatlabFunction::UpdateObject() entered, obj=<%p><%s>'%s', numTokens=%d\n",
       obj, obj->GetTypeName().c_str(), obj->GetName().c_str(), numTokens);
   for (int i=0; i<numTokens; i++)
      MessageInterface::ShowMessage("tokens[%2d]=<%s>\n", i, tokens[i].c_str());
   #endif
   
   int id;
   Gmat::ParameterType type;
   std::string objName = obj->GetName();
   std::string name, value, newValue;
   
   // If answer is returned for Parameter
   if (tokens[0] == "ans ")
   {
      value = tokens[1];
      value = GmatStringUtil::Strip(value);
      value = GmatStringUtil::RemoveEnclosingString(value,"'");
      #ifdef DEBUG_UPDATE_OBJECT
      MessageInterface::ShowMessage("   Received ans = %s\n", value.c_str());
      #endif
      if (obj->IsOfType(Gmat::PARAMETER))
      {
         Parameter *param = (Parameter*)obj;
         if (param->IsSettable())
         {
            if (param->GetReturnType() == Gmat::REAL_TYPE)
            {
               Real rval;
               if (GmatStringUtil::ToReal(value, rval))
               {
                  #ifdef DEBUG_UPDATE_OBJECT
                  MessageInterface::ShowMessage
                     ("   Setting rval: %f to object named '%s'\n", rval, objName.c_str());
                  #endif
                  param->SetReal(rval);
               }
               else
                  MessageInterface::ShowMessage
                     ("**** ERROR **** Cannot set \"" + value + "\" to Parameter '" +
                      objName + "'; The value cannot be converted to Real number\n");
            }
            else if (param->GetReturnType() == Gmat::STRING_TYPE)
            {
               
               #ifdef DEBUG_UPDATE_OBJECT
               MessageInterface::ShowMessage
                  ("   Setting string: '%s' to object named '%s'\n", value.c_str(), objName.c_str());
               #endif
               param->SetString(value);
            }
            else
            {
               MessageInterface::ShowMessage
                  ("**** ERROR **** Cannot set \"" + value + "\" to Parameter \"" +
                   objName + "\"; Setting value of non-REAL or non-STRING type has "
                   "not been implemented yet\n");
            }
         }
         else
            MessageInterface::ShowMessage
               ("**** ERROR **** Cannot set \"" + value + "\" to Parameter \"" +
                objName + "\"; The Parameter is not settable\n");
      }
      #ifdef DEBUG_UPDATE_OBJECT
      MessageInterface::ShowMessage("CallMatlabFunction::UpdateObject() leaving\n");
      #endif
      return;
   }
   
   //======================================================================
   // The following is the sample spacecraft output received from the MATLAB
   //sat2 = 
   //
   //                    X: 7123
   //         UTCGregorian: '01 Jan 2012 11:59:28.000'
   //                   Q1: 1
   //                   Q2: 0
   //                   Q3: 0
   //                   Q4: 0
   //           Quaternion: '[ 1 0 0 0]'
   //======================================================================
   
   // Handle whole object here
   StringArray names;
   StringArray values;
   std::string line, sval;
   
   // Parse object field names and values
   #ifdef DEBUG_UPDATE_OBJECT
   MessageInterface::ShowMessage("===> Parsing object fields and values\n");
   #endif
   for (int i = 2; i < numTokens; i++)
   {
      line = tokens[i];
      std::string::size_type index = line.find_first_of(":");
      name = line.substr(0, index);
      sval = line.substr(index+1);
      name = GmatStringUtil::Strip(name);
      sval = GmatStringUtil::Strip(sval);
      names.push_back(name);
      values.push_back(sval);
      #ifdef DEBUG_UPDATE_OBJECT
      MessageInterface::ShowMessage
         ("   name = %30s, value = %s\n", name.c_str(), sval.c_str());
      #endif
   }
   
   // Now set object field and its value
   #ifdef DEBUG_UPDATE_OBJECT
   MessageInterface::ShowMessage("===> Setting values to object fields\n");
   #endif
   
   std::string fieldType;
   for (unsigned int i = 0; i < names.size(); i++)
   {
      name = names[i];
      value = values[i];
      
      try
      {
         id = obj->GetParameterID(name);
         type = obj->GetParameterType(id);
         
         #ifdef DEBUG_UPDATE_OBJECT
         MessageInterface::ShowMessage
            ("   name=%s, value=%s, id=%d, type=%d\n", name.c_str(), value.c_str(), id, type);
         #endif
         
         switch (type)
         {
         case Gmat::REAL_TYPE:
         {
            Real rval;
            if (GmatStringUtil::ToReal(value, rval))
            {
               #ifdef DEBUG_UPDATE_OBJECT
               MessageInterface::ShowMessage
                  ("      REAL_TYPE: value=%s, rval=%f\n", value.c_str(), rval);
               #endif
               obj->SetRealParameter(id, rval);
            }
            else
               MessageInterface::ShowMessage
                  ("**** ERROR **** Cannot convert \"" + value + "\" to Real number\n");
            break;
         }
         case Gmat::INTEGER_TYPE:
            Integer ival;
            if (GmatStringUtil::ToInteger(value, ival))
            {
               #ifdef DEBUG_UPDATE_OBJECT
               MessageInterface::ShowMessage
                  ("      INTEGER_TYPE: value=%s, ival=%d\n", value.c_str(), ival);
               #endif
               obj->SetIntegerParameter(id, ival);
            }
            else
               MessageInterface::ShowMessage
                  ("**** ERROR **** Cannot convert \"" + value + "\" to Integer number\n");
            break;
         case Gmat::STRING_TYPE:
            fieldType = "STRING_TYPE";
         case Gmat::RVECTOR_TYPE:
            if (type != Gmat::STRING_TYPE)
               fieldType = "RVECTOR_TYPE";
         case Gmat::ENUMERATION_TYPE:
            if (type != Gmat::STRING_TYPE && type != Gmat::RVECTOR_TYPE)
               fieldType = "ENUMERATION_TYPE";
         case Gmat::OBJECT_TYPE:
            if (type != Gmat::STRING_TYPE && type != Gmat::RVECTOR_TYPE &&
                type != Gmat::ENUMERATION_TYPE)
               fieldType = "OBJECT_TYPE";
         case Gmat::FILENAME_TYPE:
            if (type != Gmat::STRING_TYPE && type != Gmat::RVECTOR_TYPE &&
                type != Gmat::ENUMERATION_TYPE && type != Gmat::OBJECT_TYPE)
               fieldType = "FILENAME_TYPE";
            
            newValue = GmatStringUtil::RemoveEnclosingString(value, "'");
            #ifdef DEBUG_UPDATE_OBJECT
            MessageInterface::ShowMessage
               ("      %s: value=<%s>, newValue=<%s>\n", fieldType.c_str(),
                value.c_str(), newValue.c_str());
            #endif
            obj->SetStringParameter(id, newValue);
            break;
         case Gmat::BOOLEAN_TYPE:
         {
            bool bval = false;
            if (value == "1")
               bval = true;
            #ifdef DEBUG_UPDATE_OBJECT
            MessageInterface::ShowMessage
               ("      BOOLEAN_TYPE: value=<%s>, bval=%d\n", value.c_str(), bval);
            #endif
            obj->SetBooleanParameter(id, bval);
            break;
         }
         default:
            throw CommandException
               ("Currently CallMatlabFunction cannot update output object for " + name + 
                " of parameter type: " + GmatBase::PARAM_TYPE_STRING[type]);
         }
      }
      catch (BaseException &e)
      {
         // Should we ignore unknown fields?
         MessageInterface::ShowMessage
            ("*** WARNING *** The field \"%s\" is not defined in the object named \"%s\", so ignoring\n",
             name.c_str(), objName.c_str());
      }
   }
   
   #ifdef DEBUG_UPDATE_OBJECT
   MessageInterface::ShowMessage
      ("CallMatlabFunction::UpdateObject() leaving, new %s =\n%s\n", obj->GetName().c_str(),
       obj->GetGeneratingString(Gmat::MATLAB_STRUCT).c_str());
   #endif
}


//------------------------------------------------------------------------------
// void HandleError(double **outArray, std::string &errMsg, ...)
//------------------------------------------------------------------------------
void CallMatlabFunction::HandleError(double **outArray, std::string &errMsg, Integer numRows,
                                     Integer numCols, Integer numRowsReceived,
                                     Integer numColsReceived)
{
   MessageInterface::ShowMessage("CallMatlabFunction::HandleError() entered\n");
   #ifdef DEBUG_MEMORY
   MemoryTracker::Instance()->Remove
      (*outArray, "outArray", "CallMatlabFunction::GetOutParams()",
       "deletinig outArray");
   #endif
   delete [] *outArray;
   #ifdef DEBUG_GET_OUTPUT
   MessageInterface::ShowMessage
      ("CallMatlabFunction::GetOutParams() error occurred in "
       "matlabIf->GetRealArray(), dimension mismatch\n   numRows=%d, "
       "numCols=%d, numRowsReceived=%d, numColsReceived=%d\n", numRows, numCols,
       numRowsReceived, numColsReceived);
   #endif
   CommandException ce(errMsg);
   throw ce;
}

