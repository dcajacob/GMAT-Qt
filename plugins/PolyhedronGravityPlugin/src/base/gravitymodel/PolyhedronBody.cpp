// Polyhedron.cpp
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
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
// number S-67573-G
//
//  Created on: Aug 27, 2012
//      Author: tdnguye2


#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <map>
#include <algorithm>
#include "FileUtil.hpp"
#include "UtilityException.hpp"
#include "StringUtil.hpp"
#include "Rvector3.hpp"
#include "RealUtilities.hpp"

#include "MessageInterface.hpp"

#include "PolyhedronBody.hpp"

using namespace GmatMathUtil;
using namespace GmatStringUtil;

//#define DEBUG_CONSTRUCTION
//#define DEBUG_CALCULATION
//#define DEBUG_READ_DATAFILE
//#define DEBUG_INCENTERS_CALCULATION
//#define DEBUG_FACENORMALS_CALCULATION

//------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------

PolyhedronBody::PolyhedronBody(const std::string &filename):
   isLoad       (false)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("PolyhedronBody default construction <%p>\n", this);
#endif
   bodyShapeFilename = filename;
}


PolyhedronBody::~PolyhedronBody()
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("PolyhedronBody destruction <%p>\n", this);
#endif
   // clean up vertices list
   verticesList.clear();
   
   // clean up faces list
   for (UnsignedInt i = 0; i < facesList.size(); ++i)
   {
      facesList[i].clear();
   }
   facesList.clear();

   fn.clear();
   ic.clear();
   E.clear();
   attachmentA.clear();
   attachmentB.clear();

   edgeMap.clear();
   attachmentAMap.clear();
   attachmentBMap.clear();

}


PolyhedronBody::PolyhedronBody(const PolyhedronBody& polybody):
   verticesList  (polybody.verticesList),
   facesList 	  (polybody.facesList),
   isLoad        (polybody.isLoad)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("PolyhedronBody copy construction <%p>\n", this);
#endif
}


PolyhedronBody& PolyhedronBody::operator= (const PolyhedronBody& polybody)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("PolyhedronBody operator = <%p>\n", this);
#endif
   if (&polybody == this)
      return *this;

   verticesList = polybody.verticesList;
   facesList 	 = polybody.facesList;
   isLoad       = polybody.isLoad;

   return *this;
}


bool PolyhedronBody::Initialize()
{
	return true;
}


PolyhedronBody* PolyhedronBody::Clone() const
{
   return (new PolyhedronBody(*this));
}


void PolyhedronBody::Copy(const PolyhedronBody* orig)
{
   operator=(*(orig));
}


//-------------------------------------------------------------------------------
// bool PolyhedronBody::LoadBodyShape()
//-------------------------------------------------------------------------------
/*
 * This function is used to load shape of a body from data file.
*/
//-------------------------------------------------------------------------------
bool PolyhedronBody::LoadBodyShape()
{
   if (isLoad)
      return true;
   
   #ifdef DEBUG_READ_DATAFILE
      MessageInterface::ShowMessage("PolyhedronBody::LoadBodyShape() loading "
            "data from %s\n", bodyShapeFilename.c_str());
   #endif

   // Parse into temporary storage. A failed read must not leave a partial
   // mesh that a later retry can append to or index during gravity evaluation.
   const std::string resolved = GmatFileUtil::FindFile(bodyShapeFilename);
   std::ifstream input(resolved.empty() ? bodyShapeFilename : resolved);
   if (!input)
      throw UtilityException("Error opening body shape file: '" + bodyShapeFilename + "'");
   Integer lineNumber = 0;
   const auto fail = [&](const std::string &reason)
   {
      throw UtilityException("Invalid body shape file '" + bodyShapeFilename +
         "' at line " + std::to_string(lineNumber) + ": " + reason);
   };
   const auto row = [&]()
   {
      std::string line;
      ++lineNumber;
      if (!std::getline(input, line)) fail("missing mesh data");
      return std::istringstream(line);
   };
   const auto complete = [&](std::istringstream &record)
   {
      record >> std::ws;
      if (!record.eof()) fail("unexpected data after record");
   };
   const auto count = [&]()
   {
      auto record = row();
      Integer value = 0;
      if (!(record >> value) || value < 4) fail("expected a count of at least four");
      complete(record);
      return value;
   };
   PointsList points;
   FacesList faces;
   const Integer vertexCount = count();
   for (Integer i = 0; i < vertexCount; ++i)
   {
      auto record = row();
      Integer index = 0;
      Real x = 0.0, y = 0.0, z = 0.0;
      if (!(record >> index >> x >> y >> z) ||
          !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
         fail("expected a vertex record index and three finite coordinates in km");
      complete(record);
      points.push_back(Rvector3(x, y, z));
   }
   // Every edge needs two oppositely directed incident faces for the existing
   // face-normal and edge-attachment gravity calculations to be well defined.
   std::map<std::pair<Integer, Integer>, std::pair<Integer, Integer>> edges;
   const Integer faceCount = count();
   Real signedVolume = 0.0;
   for (Integer i = 0; i < faceCount; ++i)
   {
      auto record = row();
      Integer index = 0, a = 0, b = 0, c = 0;
      if (!(record >> index >> a >> b >> c) ||
          a < 1 || b < 1 || c < 1 || a > vertexCount || b > vertexCount ||
          c > vertexCount || a == b || b == c || a == c)
         fail("expected a face record index and three distinct valid vertex indices");
      complete(record);
      --a; --b; --c;
      const Rvector3 u = points[b] - points[a], v = points[c] - points[a];
      const Rvector3 normal(u[1]*v[2] - u[2]*v[1], u[2]*v[0] - u[0]*v[2],
         u[0]*v[1] - u[1]*v[0]);
      const Real area = normal * normal;
      if (!std::isfinite(area) || area <= 0.0) fail("degenerate triangular face");
      signedVolume += points[a] * normal / 6.0;
      const Integer indices[] = {a, b, c, a};
      for (Integer j = 0; j < 3; ++j)
      {
         const Integer from = indices[j], to = indices[j + 1];
         auto &edge = edges[std::make_pair((std::min)(from, to), (std::max)(from, to))];
         ++edge.first;
         edge.second += from < to ? 1 : -1;
      }
      faces.push_back(PolygonFace{a, b, c});
   }
   for (const auto &edge : edges)
      if (edge.second.first != 2 || edge.second.second != 0)
         fail("mesh must be closed with consistently oriented triangular faces");
   if (!std::isfinite(signedVolume) || signedVolume <= 0.0)
      fail("mesh must enclose positive volume with outward face winding");
   input >> std::ws;
   if (!input.eof()) fail("unexpected data after mesh");
   verticesList.swap(points);
   facesList.swap(faces);
   isLoad = true;

   return true;
}


//-------------------------------------------------------------------------------
// bool PolyhedronBody::Incenters()
//-------------------------------------------------------------------------------
/*
 * This function is used to calculate incenters for all triangular faces
*/
//-------------------------------------------------------------------------------
bool PolyhedronBody::Incenters()
{
	PolygonFace face;
   Rvector3 ict;
	Rvector3 AB, BC, CA, A, B, C;

	ic.clear();

	// Create incenters list based on facesList:
	Real a, b, c, p;
	for (UnsignedInt i = 0; i < facesList.size(); ++i)
	{
		// Get indices of triangle ith:
		face = facesList[i];
		A.Set(verticesList[face[0]].Get(0), verticesList[face[0]].Get(1), verticesList[face[0]].Get(2));		// vertex A of triangle ABC
		B.Set(verticesList[face[1]].Get(0), verticesList[face[1]].Get(1), verticesList[face[1]].Get(2));		// vertex B of triangle ABC
		C.Set(verticesList[face[2]].Get(0), verticesList[face[2]].Get(1), verticesList[face[2]].Get(2));		// vertex C of triangle ABC
		AB = B - A;							// vector AB
		BC = C - B;							// vector BC
		CA = A - C;							// vector CA
		a = BC.Norm(); b = CA.Norm(); c = AB.Norm();
		p = a + b + c;

		ict = (a/p)*A + (b/p)*B + (c/p)*C;	// incenter O
		ic.push_back(ict);				// add incenter to the incenters list
	}

#ifdef DEBUG_INCENTERS_CALCULATION
	MessageInterface::ShowMessage("List of incenters:\n");
	for (UnsignedInt i = 0; i < ic.size(); ++i)
	{
		MessageInterface::ShowMessage("%.15lf   %.15lf   %.15lf\n", ic[i][0], ic[i][1], ic[i][2]);
	}
#endif
	return true;
}



//-------------------------------------------------------------------------------
// bool PolyhedronBody::FaceNormals()
//-------------------------------------------------------------------------------
/*
 * This function is used to calculate normals for all faces
*/
//-------------------------------------------------------------------------------
bool PolyhedronBody::FaceNormals()
{
	PolygonFace face;
   Rvector3 n;
	Rvector3 r1, r2, A, B, C;

	fn.clear();

	// Create normal vector list based on facesList:
	Real x, y, z;
	for (UnsignedInt i = 0; i < facesList.size(); ++i)
	{
		// Get indices of triangle ith:
		face = facesList[i];
		A.Set(verticesList[face[0]].Get(0), verticesList[face[0]].Get(1), verticesList[face[0]].Get(2));		// vertex A of triangle ABC
		B.Set(verticesList[face[1]].Get(0), verticesList[face[1]].Get(1), verticesList[face[1]].Get(2));		// vertex B of triangle ABC
		C.Set(verticesList[face[2]].Get(0), verticesList[face[2]].Get(1), verticesList[face[2]].Get(2));		// vertex C of triangle ABC
		r1 = B - A;						// vector AB
		r2 = C - B;						// vector BC
		x = r1[1]*r2[2] - r1[2]*r2[1];
		y = r1[2]*r2[0] - r1[0]*r2[2];
		z = r1[0]*r2[1] - r1[1]*r2[0];
      n.Set(x,y,z);		         // n = AB x BC
		if (n.Norm() < 1.0e-15)
			return false;

		n.Normalize();					// normal vector of triangle ABC
		fn.push_back(n);				// add normal vector to the normal vectors list
	}

   #ifdef DEBUG_FACENORMALS_CALCULATION
      MessageInterface::ShowMessage("List of normal vectors:\n");
      for (UnsignedInt i = 0; i < fn.size(); ++i)
      {
         MessageInterface::ShowMessage("%.15lf   %.15lf   %.15lf\n", fn[i][0], fn[i][1], fn[i][2]);
      }
   #endif

	return true;
}



//-------------------------------------------------------------------------------
// bool PolyhedronBody::IsInEdgesList(Edge& edge, bool& isAttachmentB)
//-------------------------------------------------------------------------------
/*
 * This function is used to verify an edge is in the current edges list
 *
 *  @param  edge           the edge that needs to verify
 *  @param  isAttachmentB  ouput true if the edge is attached to faceB
 *  return true if the edge is in the edges list          
*/
//-------------------------------------------------------------------------------
bool PolyhedronBody::IsInEdgesList(Edge& edge, bool& isAttachmentB)
{
   isAttachmentB = false;

   // Specify edge's indexKey 
   Integer indexKey = (Integer)Min((Real)edge.vertex1,(Real)edge.vertex2)*100000 +
                      (Integer)Max((Real)edge.vertex1,(Real)edge.vertex2);

   for (std::map<Integer, Edge>::iterator i = edgeMap.begin(); i != edgeMap.end(); ++i)
	{ 
      if ((*i).first == indexKey)
      {
         Edge e = (*i).second;

         if (edge.vertex1 == e.vertex2)
            isAttachmentB = true;
         return true;
      }
	}
	return false;
}


//-------------------------------------------------------------------------------
// bool PolyhedronBody::Edges()
//-------------------------------------------------------------------------------
/*
 * This function is used to create edges list and 2 attachment faces to each edge
*/
//-------------------------------------------------------------------------------
bool PolyhedronBody::Edges()
{
   bool isAttachmentB;
   Integer indexKey;

   PolygonFace face;
   Edge e1, e2, e3;

   edgeMap.clear();
   attachmentAMap.clear();
   attachmentBMap.clear();
   E.clear();
   attachmentA.clear();
   attachmentB.clear();

   for(unsigned int i=0; i < facesList.size(); ++i)
   {
      face = facesList[i];

      // For a triangular face <face[0], face[1], face[2]>, it has 3 edges e1, e2, and e3
      e1.vertex1 = face[0]; e1.vertex2 = face[1];
      e2.vertex1 = face[1]; e2.vertex2 = face[2];
      e3.vertex1 = face[2]; e3.vertex2 = face[0];

	   // Add e1 to edges list if it has not existed in edges list:
      indexKey = (Integer)Min((Real)e1.vertex1,(Real)e1.vertex2)*100000 +
                 (Integer)Max((Real)e1.vertex1,(Real)e1.vertex2);
      if (!IsInEdgesList(e1, isAttachmentB))
      {
         edgeMap[indexKey] = e1;
         attachmentAMap[indexKey] = i;                      // add atachment face A
         attachmentBMap[indexKey] = -1;                     // add null value to attachmentB list
      }
      else
      {
         if (isAttachmentB)
            attachmentBMap[indexKey] = i;                   // add attachment to face B
      }

      // Add e2 to edges list if it has not existed in edges list:
      indexKey = (Integer)Min((Real)e2.vertex1,(Real)e2.vertex2)*100000 +
                 (Integer)Max((Real)e2.vertex1,(Real)e2.vertex2);
      if (!IsInEdgesList(e2, isAttachmentB))
      {
         edgeMap[indexKey] = e2;
         attachmentAMap[indexKey] = i;                      // add atachment face A
         attachmentBMap[indexKey] = -1;                     // add null value to attachmentB list
      }
      else
      {
         if (isAttachmentB)
            attachmentBMap[indexKey] = i;                   // add attachment to face B
      }

      // Add e3 to edges list if it has not existed in edges list:
      indexKey = (Integer)Min((Real)e3.vertex1,(Real)e3.vertex2)*100000 +
                 (Integer)Max((Real)e3.vertex1,(Real)e3.vertex2);
      if (!IsInEdgesList(e3, isAttachmentB))
      {
         edgeMap[indexKey] = e3;
         attachmentAMap[indexKey] = i;                      // add atachment face A
         attachmentBMap[indexKey] = -1;                     // add null value to attachmentB list
      }
      else
      {
         if (isAttachmentB)
            attachmentBMap[indexKey] = i;                   // add attachment to face B
      }
   }

   for (std::map<Integer, Edge>::iterator i = edgeMap.begin(); i != edgeMap.end(); ++i)
   {
      E.push_back((*i).second);
   }

   for (std::map<Integer, Integer>::iterator i = attachmentAMap.begin(); i != attachmentAMap.end(); ++i)
   {
      attachmentA.push_back((*i).second);
   }

   for (std::map<Integer, Integer>::iterator i = attachmentBMap.begin(); i != attachmentBMap.end(); ++i)
   {
      attachmentB.push_back((*i).second);
   }
   

#ifdef DEBUG_CALCULATION
   MessageInterface::ShowMessage(" Edges list and face attachment A and B:\n");
   for(UnsignedInt i=0; i < E.size(); ++i)
   {
      MessageInterface::ShowMessage("%d  %d :  %d    %d    %d    %d\n", min(E[i].vertex1, E[i].vertex2), max(E[i].vertex1, E[i].vertex2), E[i].vertex1, E[i].vertex2, attachmentA[i], attachmentB[i]);
   }
#endif
   return true;
}


//------------------------------------------------------------------------------
// bool PolyhedronBody::EdgeAttachments(Integer edgeindex,
//                  Integer& faceA_index, Integer& faceB_index)
//------------------------------------------------------------------------------
/*
 * This function is used to get 2 faces attached to a given edge
 *
 *  @param edgeindex       index of a given edge
 *  @param faceA_index     index of the first face of the edge
 *  @param faceB_index     index of the second face of the edge
 *
*/
//------------------------------------------------------------------------------
bool PolyhedronBody::EdgeAttachments(Integer edgeindex, Integer& faceA_index, Integer& faceB_index)
{
   faceA_index = attachmentA[edgeindex];
   faceB_index = attachmentB[edgeindex];
   return true;
}


