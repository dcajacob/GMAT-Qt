// Author: Darrel Conway
// Created: 2019/09/26
//
// Copyright (c) 2019, Thinking Systems, Inc.
// All rights reserved

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Edited by Jairus Elarbee 10/05/2021 
//   -Allow user to use TLE NORAD ID in addition to spacecraft name in Spacecraft.Id

#include "TLEReader.hpp"
#include <fstream>
#include <cstring>
#include <sstream>
#include <map>
#include <cmath>
#include <locale>
#include "gmatdefs.hpp"
#include "PropagatorException.hpp"

#include <iostream>

extern "C"  
{
   #include "SpiceUsr.h"    // for CSPICE routines
}

namespace
{
std::string Trim(const std::string &text)
{
   const auto first=text.find_first_not_of(" \t\r\n");
   return first==std::string::npos ? "" : text.substr(first,text.find_last_not_of(" \t\r\n")-first+1);
}

TLEData ReadOmm(std::istream &input,const std::string &satellite)
{
   std::map<std::string,std::string> fields;
   std::string line;
   while (std::getline(input,line))
   {
      const auto separator=line.find('=');
      if (separator!=std::string::npos)
         fields[Trim(line.substr(0,separator))]=Trim(line.substr(separator+1));
   }
   TLEData result;
   if (satellite!="SatId" && fields["NORAD_CAT_ID"]!=satellite && fields["OBJECT_NAME"]!=satellite)
      return result;
   if ((fields["CCSDS_OMM_VERS"]!="2.0" && fields["CCSDS_OMM_VERS"]!="3.0") ||
       fields["CENTER_NAME"]!="EARTH" || fields["REF_FRAME"]!="TEME" ||
       fields["TIME_SYSTEM"]!="UTC" || fields["MEAN_ELEMENT_THEORY"]!="SGP4")
      throw PropagatorException("The OMM must contain Earth-centered TEME/UTC mean elements using SGP4.");
   auto number=[&](const std::string &key) {
      std::istringstream value(fields[key]); value.imbue(std::locale::classic());
      double parsed;
      if (!(value>>parsed) || !std::isfinite(parsed) || !(value>>std::ws).eof())
         throw PropagatorException("Missing or invalid OMM field: "+key);
      return parsed;
   };
   const double radians=std::acos(-1.0)/180.0;
   const double revolutions=2*std::acos(-1.0);
   result.elements[0]=number("MEAN_MOTION_DOT")*revolutions/(1440.0*1440.0);
   result.elements[1]=number("MEAN_MOTION_DDOT")*revolutions/(1440.0*1440.0*1440.0);
   result.elements[2]=number("BSTAR");
   result.elements[3]=number("INCLINATION")*radians;
   result.elements[4]=number("RA_OF_ASC_NODE")*radians;
   result.elements[5]=number("ECCENTRICITY");
   result.elements[6]=number("ARG_OF_PERICENTER")*radians;
   result.elements[7]=number("MEAN_ANOMALY")*radians;
   result.elements[8]=number("MEAN_MOTION")*revolutions/1440.0;
   if (result.elements[3]<0 || result.elements[3]>std::acos(-1.0) ||
       result.elements[5]<0 || result.elements[5]>=1 || result.elements[8]<=0)
      throw PropagatorException("The OMM contains an invalid SGP4 inclination, eccentricity or mean motion.");
   for (const int index:{4,6,7}) if (result.elements[index]<0 || result.elements[index]>=revolutions)
      throw PropagatorException("OMM orbital angles must be in the range [0, 360) degrees.");
   if (fields["EPOCH"].empty()) throw PropagatorException("The OMM epoch is missing.");
   str2et_c(fields["EPOCH"].c_str(),&result.secFromJ2k);
   if (failed_c())
   {
      char message[1841]; getmsg_c("LONG",sizeof(message),message); reset_c();
      throw PropagatorException("Invalid OMM epoch: "+std::string(message));
   }
   result.elements[9]=result.secFromJ2k;
   result.satName=fields["OBJECT_NAME"]; result.isOmm=true;
   return result;
}
}


TLEReader::TLEReader(const std::string &tleFile) :
   filename(tleFile)
{
}


TLEReader::~TLEReader()
{
}


TLEData TLEReader::GetTLEData(const std::string &forSatellite)
{
   TLEData theData;
   std::string line;

   if (filename != "")
   {
      std::string prev = "";
      std::ifstream infile(filename);
      std::getline(infile,line);
      infile.clear(); infile.seekg(0);
      if (Trim(line).find("CCSDS_OMM_VERS")==0)
         return ReadOmm(infile,forSatellite);
      while (std::getline(infile, line))
      {
         if (line.find(forSatellite) == 0)
         {
            theData.tleLines[0] = line;
            std::getline(infile, line);
            if (line.find('\r'))
            {
               int end = line.find('\r');
               line = line.substr(0, end);
            }
            theData.tleLines[1] = line;
            std::getline(infile, line);
            if (line.find('\r'))
            {
               int end = line.find('\r');
               line = line.substr(0, end);
            }
            theData.tleLines[2] = line;

            break;
         }
         else if (line.find(forSatellite) == 2)
         {
            if (prev.length() > 65)
            {
               prev = "GMAT TLE Sat";
            }
            else if (prev.find('\r'))
            {
               int end = prev.find('\r');
               prev = prev.substr(0, end);
            }
            theData.tleLines[0] = prev;
            if (line.find('\r'))
            {
               int end = line.find('\r');
               line = line.substr(0, end);
            }
            theData.tleLines[1] = line;
            std::getline(infile, line);
            if (line.find('\r'))
            {
               int end = line.find('\r');
               line = line.substr(0, end);
            }
            theData.tleLines[2] = line;
            break;
         }
         else if (forSatellite =="SatId" && line.find("1 ") == 0 && line.size() > 68 && line.size() < 72)
         {
            if (prev.length() > 65)
            {
               prev = "GMAT TLE Sat";
            }
            else if (prev.find('\r'))
            {
               int end = prev.find('\r');
               prev = prev.substr(0, end);
            }
            theData.tleLines[0] = prev;
            if (line.find('\r'))
            {
               int end = line.find('\r');
               line = line.substr(0, end);
            }
            prev = line;
            std::getline(infile, line);
            if (line.find("2 ") == 0 && line.size() > 68 && line.size() < 72)
            {
               if (line.find('\r'))
               {
                  int end = line.find('\r');
                  line = line.substr(0, end);
               }
               theData.tleLines[1] = prev;
               theData.tleLines[2] = line;
            }
            break;
         }
         prev = line;
      }
   }

   return theData;
}


void TLEReader::ParseForSpice(TLEData &theData)
{
   if (theData.isOmm) return;
   int linelen = theData.tleLines[1].length() + 1;
   char lines[180];
   strcpy(lines, theData.tleLines[1].c_str());
   strcpy(lines + linelen, theData.tleLines[2].c_str());

   getelm_c(1957, linelen, lines, &(theData.secFromJ2k), theData.elements);
}
