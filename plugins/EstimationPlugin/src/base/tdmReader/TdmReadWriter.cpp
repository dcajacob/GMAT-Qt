//------------------------------------------------------------------------------
//                            TdmReadWriter
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
// Author: Farideh Farahnak
// Created: 2014/8/20
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// FDSS.
//
/**
 * TdmReadWriter class will be used by the TdmObType class.
 */
//------------------------------------------------------------------------------

#include "TdmReadWriter.hpp"
#include "MessageInterface.hpp"
#include "xercesc/framework/LocalFileInputSource.hpp"
#include "MeasurementException.hpp"
#include "DateUtil.hpp"
#include "CCSDSEMSegment.hpp"
#include "StringUtil.hpp"
#include "UtcDate.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace
{
struct XmlTextRelease
{
   template <typename Char> void operator()(Char *text) const
   {
      XMLString::release(&text);
   }
};

template <typename Char>
using OwnedXmlText = std::unique_ptr<Char, XmlTextRelease>;

std::string TdmText(const XMLCh *text)
{
   if (text == NULL)
      throw MeasurementException("TDM XML content is missing");
   OwnedXmlText<char> converted(XMLString::transcode(text));
   return converted ? converted.get() : "";
}

const XMLCh *TdmNodeName(const DOMNode *node)
{
   return node->getLocalName() ? node->getLocalName() : node->getNodeName();
}

DOMElement *CopiedTdmElement(const DOMElement *source, DOMDocument *document)
{
   if (source == NULL)
      return NULL;
   std::vector<XMLSize_t> path;
   for (const DOMNode *node = source; node->getNodeType() != DOMNode::DOCUMENT_NODE;
         node = node->getParentNode())
   {
      const DOMNode *parent = node->getParentNode();
      if (parent == NULL)
         throw MeasurementException("TDM cursor is detached from its document");
      const DOMNodeList *children = parent->getChildNodes();
      XMLSize_t index = 0;
      while (index < children->getLength() && children->item(index) != node)
         ++index;
      if (index == children->getLength())
         throw MeasurementException("TDM cursor is absent from its parent");
      path.push_back(index);
   }
   DOMNode *node = document;
   for (auto index = path.rbegin(); index != path.rend(); ++index)
   {
      node = node ? node->getChildNodes()->item(*index) : NULL;
      if (node == NULL)
         throw MeasurementException("Copied TDM cursor is absent");
   }
   if (node->getNodeType() != DOMNode::ELEMENT_NODE ||
         !XMLString::equals(node->getNodeName(), source->getNodeName()))
      throw MeasurementException("Copied TDM cursor changed node identity");
   return static_cast<DOMElement*>(node);
}
}


//------------------------------------------------------------------------------
// Public Methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// TdmReadWriter()
//------------------------------------------------------------------------------
/**
 * Constructor
 */
//------------------------------------------------------------------------------
TdmReadWriter::TdmReadWriter()
{
   theErrorHandler = new TdmErrorHandler();
   theDOMParser = NULL;
   theDocument = NULL;
  
   theBody = NULL;
   theSegment = NULL;
   theData = NULL;
   theObservation = NULL;
   xercesInitialized = false;
   observationIndex = 0;
   
   // Fill in the map
   mapTransmitBand["S"] = 1.0;
   mapTransmitBand["X"] = 2.0;
   mapTransmitBand["KA"] = 3.0;
   mapTransmitBand["KU"] = 4.0;
   mapTransmitBand["L"] = 5.0;
}


//------------------------------------------------------------------------------
// TdmReadWriter(const TdmReadWriter &trw)
//------------------------------------------------------------------------------
/**
 * Copy Constructor
 */
//------------------------------------------------------------------------------
TdmReadWriter::TdmReadWriter(const TdmReadWriter &trw) : TdmReadWriter()
{
   *this = trw;
}


//------------------------------------------------------------------------------
// operator=(const TdmReadWriter &trw)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 */
//------------------------------------------------------------------------------
TdmReadWriter& TdmReadWriter::operator=(const TdmReadWriter &trw)
{
   if (this != &trw)
   {
      TdmReadWriter copy;
      copy.mapTransmitBand = trw.mapTransmitBand;
      copy.sourceTimeSystem = trw.sourceTimeSystem;
      copy.theTemplate = trw.theTemplate;
      copy.observationIndex = trw.observationIndex;
      if (trw.theErrorHandler != NULL)
      {
         std::unique_ptr<TdmErrorHandler> handler(new TdmErrorHandler(*trw.theErrorHandler));
         delete copy.theErrorHandler;
         copy.theErrorHandler = handler.release();
      }
      if (trw.xercesInitialized)
         copy.Initialize();
      if (trw.theDocument != NULL)
      {
         copy.Initialize();
         copy.theDocument = static_cast<DOMDocument*>(trw.theDocument->cloneNode(true));
         if (copy.theDocument == NULL)
            throw MeasurementException("Unable to copy TDM document");
         copy.theDocument->setDocumentURI(trw.theDocument->getDocumentURI());
         copy.theBody = CopiedTdmElement(trw.theBody, copy.theDocument);
         copy.theSegment = CopiedTdmElement(trw.theSegment, copy.theDocument);
         copy.theData = CopiedTdmElement(trw.theData, copy.theDocument);
         copy.theObservation = CopiedTdmElement(trw.theObservation, copy.theDocument);
      }

      using std::swap;
      swap(theTemplate, copy.theTemplate);
      swap(mapTransmitBand, copy.mapTransmitBand);
      swap(sourceTimeSystem, copy.sourceTimeSystem);
      swap(theErrorHandler, copy.theErrorHandler);
      swap(theDOMParser, copy.theDOMParser);
      swap(theDocument, copy.theDocument);
      swap(xercesInitialized, copy.xercesInitialized);
      swap(theBody, copy.theBody);
      swap(theSegment, copy.theSegment);
      swap(theData, copy.theData);
      swap(theObservation, copy.theObservation);
      swap(observationIndex, copy.observationIndex);
   }
   
   return *this;
}


//------------------------------------------------------------------------------
// ~TdmReadWriter()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
TdmReadWriter::~TdmReadWriter()
{
   Finalize();
}


//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Initializes TdmReadWriter.
 *
 * This method will initialize the DOM Parser, and configure it for error handling
 * and Schema validation.
 *
 * @param none
 *
 * @return bool
 */
//------------------------------------------------------------------------------
bool TdmReadWriter::Initialize()
{
   if (!xercesInitialized)
   {
      std::string initializationError;
      try
      {
         XMLPlatformUtils::Initialize();
         xercesInitialized = true;
         if (theErrorHandler == NULL)
            theErrorHandler = new TdmErrorHandler();
         theDOMParser = new XercesDOMParser();

         theDOMParser->setErrorHandler(theErrorHandler);
	      theDOMParser->setValidationScheme(XercesDOMParser::Val_Auto);
	      theDOMParser->setDoNamespaces(true);
	      theDOMParser->setDoSchema(true);
	      theDOMParser->setValidationConstraintFatal(true);

      }
      catch(const XMLException &xe)
      {
         initializationError = "Xerces failed to initialize: " + TdmText(xe.getMessage());
      }
      catch (...)
      {
         Finalize();
         throw;
      }
      if (!initializationError.empty())
      {
         // The Xerces exception must be destroyed before terminating Xerces.
         Finalize();
         throw MeasurementException(initializationError);
      }
   }

   return xercesInitialized;
}


//------------------------------------------------------------------------------
// ObservationData *ProcessMetadata()
//------------------------------------------------------------------------------
/**
 * Processes metadata and returns it to the caller.
 *
 * This method called each time a new segment is encountered, it loads the metadata 
 * into the ObservationData template and returns a pointer to it for use by the 
 * TdmObType that called the method.
 *
 * @param none
 *
 * @return ObsData pointer
 */
//------------------------------------------------------------------------------
//ObsData *TdmReadWriter::ProcessMetadata()
ObservationData *TdmReadWriter::ProcessMetadata()
{
   if (theSegment != NULL)
   {
      // Clear observation Data if it has been filled in with data.
      theTemplate.Clear();
      theTemplate.epochSystem = TimeSystemConverter::A1MJD;
      theTemplate.epochAtEnd = true;
      theTemplate.epochAtIntegrationEnd = true;
      theTemplate.epochReference = ObservationData::EpochReference::Configured;
      theTemplate.integrationReference = ObservationData::IntegrationReference::Configured;
      sourceTimeSystem.clear();

      DOMElement *pMetaData = theSegment->getFirstElementChild();
      if (pMetaData == NULL || TdmText(TdmNodeName(pMetaData)) != "metadata")
         throw MeasurementException("TDM segment has no metadata element");
      DOMNodeList *pChilds = pMetaData->getChildNodes();
					
		for (UnsignedInt i = 0; i < pChilds->getLength(); i++)
		{
			DOMNode *node = pChilds->item(i);
			if (node->getNodeType() == DOMNode::ELEMENT_NODE)
         {
            DOMElement *pChild = static_cast<DOMElement*>(node);
            //Fill in the observation data theTemplate for each
            // attributes.
            switch(HashIt(TdmNodeName(pChild)))
            {
               case TIME_SYSTEM:
               {
                  if (!sourceTimeSystem.empty())
                     throw MeasurementException("TDM segment declares more than one TIME_SYSTEM");
                  sourceTimeSystem = GmatStringUtil::Trim(TdmText(pChild->getTextContent()));
                  const Integer system = TimeSystemConverter::Instance()->GetTimeTypeID(sourceTimeSystem);
                  if (sourceTimeSystem != "GPS" &&
                        (system < TimeSystemConverter::A1 || system >= TimeSystemConverter::TimeSystemCount))
                     throw MeasurementException("TDM TIME_SYSTEM is not supported without a clock correlation: " + sourceTimeSystem);
                  break;
               }
               case PARTICIPANT_1:
               case PARTICIPANT_2:
               case PARTICIPANT_3:
               case PARTICIPANT_4:
               case PARTICIPANT_5:
               {
                  theTemplate.participantIDs.push_back(TdmText(pChild->getTextContent()));
                  break;
               }
               case MODE:
                  break;
               case PATH:
               {
                  StringArray IDs;
                  OwnedXmlText<char> path(XMLString::transcode(pChild->getTextContent()));
                  char *pPath = path.get();
                  char *pTok;
                  pTok = strtok(pPath, ",");
                  
                  while (pTok != NULL)
                  {
                     const int participant = atoi(pTok) - 1;
                     if (participant < 0 || static_cast<size_t>(participant) >= theTemplate.participantIDs.size())
                        throw MeasurementException("TDM PATH participant index is out of range");
                     IDs.push_back(theTemplate.participantIDs.at(participant));
                     pTok = strtok(NULL, ","); 
                  }

                  theTemplate.strands.push_back(IDs);
                  break;
               }
               case PATH_1:
                  break;
               case PATH_2:
                  break;
               case TRANSMIT_BAND:
               {
                  std::string strT(TdmText(pChild->getTextContent()));

                  std::map<std::string, Real>::iterator it;
                  it = mapTransmitBand.find(strT);
                  
                  if (it != mapTransmitBand.end())
                     theTemplate.value.push_back(it->second);
                  else
                     theTemplate.value.push_back(0.0);

                  theTemplate.dataMap.push_back(TdmText(TdmNodeName(pChild)));

                  break;
               }
               case RECEIVE_BAND:
                  break;
               case TIMETAG_REF:
               {
                  std::string strT(TdmText(pChild->getTextContent()));
                  if (strT.compare("RECEIVE") == 0 || strT.compare("receive") == 0)
                  {
                     theTemplate.epochAtEnd = true;
                     theTemplate.epochReference = ObservationData::EpochReference::Receive;
                  }
                  else if (strT.compare("TRANSMIT") == 0 || strT.compare("transmit") == 0)
                  {
                     theTemplate.epochAtEnd = false;
                     theTemplate.epochReference = ObservationData::EpochReference::Transmit;
                  }
                  else
                     throw MeasurementException("Unsupported TDM TIMETAG_REF: " + strT);
                  
                  break;
               }
//               case INTEGRATION_INTERVAL:
//                  {
//                     theTemplate.data.push_back(atof(XMLString::transcode(pChild->getTextContent())));
//                     theTemplate.dataMap.push_back(XMLString::transcode(pChild->getNodeName()));
//                  }
//                  break;
               case INTEGRATION_REF:
                  {
                     std::string strT(TdmText(pChild->getTextContent()));
                     if (strT.compare("END") == 0 || strT.compare("end") == 0)
                     {
                        theTemplate.epochAtIntegrationEnd = true;
                        theTemplate.integrationReference = ObservationData::IntegrationReference::End;
                     }
                     else if (strT.compare("START") == 0 || strT.compare("start") == 0)
                     {
                        theTemplate.epochAtIntegrationEnd = false;
                        theTemplate.integrationReference = ObservationData::IntegrationReference::Start;
                     }
                     else if (strT.compare("MIDDLE") == 0 || strT.compare("middle") == 0)
                     {
                        theTemplate.epochAtIntegrationEnd = false;
                        theTemplate.integrationReference = ObservationData::IntegrationReference::Middle;
                     }
                     else
                        throw MeasurementException("Unsupported TDM INTEGRATION_REF: " + strT);
                  }
                  break;
               case RANGE_MODE:
                  break;
               case RANGE_MODULUS:
               case FREQ_OFFSET:
               case INTEGRATION_INTERVAL:
               {
                  theTemplate.value.push_back(atof(TdmText(pChild->getTextContent()).c_str()));
                  theTemplate.dataMap.push_back(TdmText(TdmNodeName(pChild)));
                  break;
               }
               case RANGE_UNITS:
               {
                  theTemplate.unit = TdmText(pChild->getTextContent());
                  break;
               }
               default:
                  break;
            }
			}
      }

      if (sourceTimeSystem.empty())
         throw MeasurementException("TDM segment is missing required TIME_SYSTEM metadata");
      theData = theSegment->getLastElementChild();
      if (theData == NULL || TdmText(TdmNodeName(theData)) != "data")
         throw MeasurementException("TDM segment has no data element");
      theObservation = theData->getFirstElementChild();
      if (theObservation == NULL)
         throw MeasurementException("TDM data element contains no observations");
      observationIndex = 0;

      return &theTemplate;
   }
   
   theData = theObservation = NULL;
   observationIndex = 0;
   return NULL;
}


//------------------------------------------------------------------------------
// bool LoadRecord()
//------------------------------------------------------------------------------
/**
 * Loads Data section of XML file.
 *
 * This method retrieves the observation data and fills in the relevant fields in
 * the ObservationData record that is passed to it, by pushing the observation data
 * to the data member and the associated field tags to the dataMap in the input 
 * ObservationData record.
 *
 * @param ObsData *
 *
 * @return ObsData
 */
//------------------------------------------------------------------------------
//ObsData *TdmReadWriter::LoadRecord(ObsData *newData)
ObservationData *TdmReadWriter::LoadRecord(ObservationData *newData)
{
   if (theSegment == NULL)
      return NULL;
   if (newData == NULL || theData == NULL || theObservation == NULL ||
         theObservation->getFirstElementChild() == NULL)
      throw MeasurementException("TDM observation cursor is not initialized");
   std::string strCurrEpoch;
   std::string strObs;
   std::string strNodeName;

   const GmatTime recordEpoch = ParseEpochGT(TdmText(theObservation->getFirstElementChild()->getTextContent()));
  
   for (UnsignedInt i = observationIndex; i < theData->getChildNodes()->getLength(); i++)
   {
      DOMNode *node = theData->getChildNodes()->item(i);
      if (node->getNodeType() == DOMNode::ELEMENT_NODE)
		{
         theObservation = static_cast<DOMElement*>(node);
         DOMElement *epoch = theObservation->getFirstElementChild();
         DOMElement *value = theObservation->getLastElementChild();
         if (epoch == NULL || value == NULL || epoch == value)
            throw MeasurementException("TDM observation is missing its epoch or value");
			strCurrEpoch = TdmText(epoch->getTextContent());
         strObs = TdmText(value->getTextContent());
         strNodeName = TdmText(TdmNodeName(value));
         const GmatTime observationEpoch = ParseEpochGT(strCurrEpoch);

         if (theTemplate.typeName == "")
            theTemplate.typeName = newData->typeName = strNodeName;
         
         if (recordEpoch != observationEpoch)
         {
            //set the index for next call
            observationIndex = i;

            return &theTemplate;
         }
         else
         {
            // push data into newData     
            newData->epochGT = recordEpoch;
            newData->epoch = recordEpoch.GetMjd();
            newData->epochSystem = TimeSystemConverter::A1MJD;
            newData->value.push_back(atof(strObs.c_str()));
            newData->dataMap.push_back(strNodeName);
         }
           
      }
   }
   
   theSegment = theSegment->getNextElementSibling();
   // reset the index back to zero
   observationIndex = 0;
   return (ProcessMetadata());
}


//------------------------------------------------------------------------------
// bool Validate()
//------------------------------------------------------------------------------
/**
 * Validates the XML file.
 *
 * This method called when a new TDM file is loaded for the first data read, it uses 
 * Xerces to validate the data file against the TDM schema.
 *
 * @param TDM XML filename
 *
 * @return bool
 */
//------------------------------------------------------------------------------
bool TdmReadWriter::Validate(const std::string &tdmFileName)
{
   Initialize();
   theBody = theSegment = theData = theObservation = NULL;
   observationIndex = 0;
   theTemplate.Clear();
   sourceTimeSystem.clear();
   if (theDocument != NULL)
   {
      theDocument->release();
      theDocument = NULL;
   }
   theDOMParser->resetDocumentPool();

   // Load the TDM XML file
   try
   {
      OwnedXmlText<XMLCh> filename(XMLString::transcode(tdmFileName.c_str()));
      LocalFileInputSource xmlFile(filename.get());
      theDOMParser->parse(xmlFile);
   }
   catch(const XMLException &xe)
   {
      throw MeasurementException("Xerces failed to load the file: " + TdmText(xe.getMessage()));
   }

   if (theDOMParser->getErrorCount() != 0)
      throw MeasurementException("Xerces failed validation: XML file does not conform to Schema: ");
   theDocument = theDOMParser->adoptDocument();
   if (theDocument == NULL)
      throw MeasurementException("TDM XML parser produced no document");
   MessageInterface::ShowMessage("XML file is validated against the Schema file successfully.\n");

   return true;
}


//------------------------------------------------------------------------------
// bool Finalize()
//------------------------------------------------------------------------------
/**
 * Finalizes the TdmReadWriter object.
 *
 * This method cleans up the TDM file and Xerces interface if needed, and
 * any other artifacts still in memory.
 *
 * @param none
 *
 * @return bool
 */
//------------------------------------------------------------------------------
bool TdmReadWriter::Finalize()
{
   theBody = theSegment = theData = theObservation = NULL;
   observationIndex = 0;
   theTemplate.Clear();
   sourceTimeSystem.clear();
   if (theDocument != NULL)
   {
      theDocument->release();
      theDocument = NULL;
   }
   if (theDOMParser)
   {
      delete theDOMParser;
      theDOMParser = NULL;
   }
   if (theErrorHandler)
   {
      delete theErrorHandler;
      theErrorHandler = NULL;
   }
   if (xercesInitialized)
   {
      XMLPlatformUtils::Terminate();
      xercesInitialized = false;
   }

   return true;
}


//------------------------------------------------------------------------------
// bool SetBody()
//------------------------------------------------------------------------------
/**
 * Reads Header section of XML file (for checking the version number),
 * set the Body element and first Segment node.
 *
 * This method reads the header in XML file, and set the Body element.
 * 
 *
 * @param none
 *
 * @return bool
 */
//------------------------------------------------------------------------------
bool TdmReadWriter::SetBody()
{
   if (theDocument == NULL || theDocument->getDocumentElement() == NULL)
      throw MeasurementException("No validated TDM document is open");
   DOMElement *pTdm = theDocument->getDocumentElement();
   const XMLCh idName[] = {'i', 'd', 0};
   const XMLCh versionName[] = {'v', 'e', 'r', 's', 'i', 'o', 'n', 0};
   DOMAttr *pAttrId = pTdm->getAttributeNode(idName);
   if (pAttrId == NULL || TdmText(pAttrId->getValue()) != "CCSDS_TDM_VERS")
      throw MeasurementException("CCSDS_TDM_VERS id is not correct");
   DOMAttr *pAttrVer = pTdm->getAttributeNode(versionName);
   if (pAttrVer == NULL || TdmText(pAttrVer->getValue()) != "1.0")
      throw MeasurementException("The TDM VERSION is not correct.\n");

   //header
   /********** No NEED to parse these.*********** /
	DOMElement *pHeader = pTdm->getFirstElementChild();
	DOMElement *pComments = pHeader->getFirstElementChild();

	while (pComments != NULL && XMLString::equals(XMLString::transcode("COMMENT"), pComments->getTagName()))  //comments
	{
		pComments = pComments->getNextElementSibling();	
	}
	
	//whether or not there are comments or not, we get here.
	DOMElement * pDate = (DOMElement *)pHeader->getElementsByTagName(XMLString::transcode("CREATION_DATE"))->item(0);
	DOMElement * pOrig = (DOMElement *)pHeader->getElementsByTagName(XMLString::transcode("ORIGINATOR"))->item(0);
   **********************************************/

   theBody = pTdm->getLastElementChild();
   if (theBody == NULL || TdmText(TdmNodeName(theBody)) != "body")
      throw MeasurementException("TDM document has no body element");
   theSegment = theBody->getFirstElementChild();
   theData = theObservation = NULL;
   observationIndex = 0;

   return true;
}


//------------------------------------------------------------------------------
// MetaData HashIt()
//------------------------------------------------------------------------------
/**
 * Hashes a string name to a corresponding enum value.
 * 
 *
 * This method hashes a string to a number.
 * 
 *
 * @param char * pointing to node name
 *
 * @return an enumeration value
 */
//------------------------------------------------------------------------------
TdmReadWriter::MetaData TdmReadWriter::HashIt(const XMLCh *xmlNodeName)
{
   std::string strN(TdmText(xmlNodeName));

   if (strN == "TIME_SYSTEM")
      return TIME_SYSTEM;
   if (strN == "PARTICIPANT_1")
      return PARTICIPANT_1;
   if (strN == "PARTICIPANT_2")
      return PARTICIPANT_2;
   if (strN == "PARTICIPANT_3")
      return PARTICIPANT_3;
   if (strN == "PARTICIPANT_4")
      return PARTICIPANT_4;
   if (strN == "PARTICIPANT_5")
      return PARTICIPANT_5;
   if (strN == "MODE")
      return MODE;
   if (strN == "PATH")
      return PATH;
   if (strN == "PATH_1")
      return PATH_1;
   if (strN == "PATH_2")
      return PATH_2;
   if (strN == "TRANSMIT_BAND")
      return TRANSMIT_BAND;
   if (strN == "RECEIVE_BAND")
      return RECEIVE_BAND;
   if (strN == "TIMETAG_REF")
      return TIMETAG_REF;
   if (strN == "INTEGRATION_INTERVAL")
      return INTEGRATION_INTERVAL;
   if (strN == "INTEGRATION_REF")
      return INTEGRATION_REF;
   if (strN == "RANGE_MODE")
      return RANGE_MODE;
   if (strN == "RANGE_MODULUS")
      return RANGE_MODULUS;
   if (strN == "RANGE_UNITS")
      return RANGE_UNITS;
   if (strN == "FREQ_OFFSET")
      return FREQ_OFFSET;

   return NONE;
}


//------------------------------------------------------------------------------
// GmatEpoch ParseEpoch()
//------------------------------------------------------------------------------
/**
 * Parse and Convert Epoch datetime string.
 * 
 *
 * This method parses and converts the Epoch datetime string
 * passed in to the GmatEpoch that will be used in ObsData structure.
 * 
 * Two datetime formats :
 * 1.) YYYY-MM-DDThh:mm:ss[d->d][Z]
 * 2.) YYYY-DDDThh:mm:ss[d->d][Z]
 * [d->d] is an optional fraction seconds; 'Z" is an optional time code terminator.
 * refer to CCSDS503.0-B-1_TDM.pdf document page 52. 
 * 
 *
 * @param const std::string
 *
 * @return GmatEpoch
 */
//------------------------------------------------------------------------------
GmatEpoch TdmReadWriter::ParseEpoch(const std::string strEpoch)
{
   return ParseEpochGT(strEpoch).GetMjd();
}

// Parse calendar components without forming a Real MJD. Observations are
// normalized to A1, matching their epochSystem tag and the estimator clock.
GmatTime TdmReadWriter::ParseEpochGT(const std::string &strEpoch)
{
   std::string text = GmatStringUtil::Trim(strEpoch);
   if (!text.empty() && text.back() == 'Z')
      text.pop_back();
   const auto split = text.find_first_of("Tt");
   if (split == std::string::npos || text.find_first_of("Tt", split + 1) != std::string::npos)
      throw MeasurementException("Invalid TDM epoch: " + strEpoch);
   const std::string date = text.substr(0, split);
   const std::string time = text.substr(split + 1);
   const auto digits = [](const std::string &part) {
      return !part.empty() && part.find_first_not_of("0123456789") == std::string::npos;
   };
   const bool calendarDate = date.size() == 10 && date[4] == '-' && date[7] == '-';
   const bool ordinalDate = date.size() == 8 && date[4] == '-';
   if ((!calendarDate && !ordinalDate) || !digits(date.substr(0, 4)) ||
         (calendarDate && (!digits(date.substr(5, 2)) || !digits(date.substr(8, 2)))) ||
         (ordinalDate && !digits(date.substr(5, 3))) || time.size() < 8 ||
         time[2] != ':' || time[5] != ':' || !digits(time.substr(0, 2)) ||
         !digits(time.substr(3, 2)) || !digits(time.substr(6, 2)) ||
         (time.size() > 8 && (time[8] != '.' || !digits(time.substr(9)))))
      throw MeasurementException("Invalid TDM epoch: " + strEpoch);

   const Integer year = std::stoi(date.substr(0, 4));
   const Integer hour = std::stoi(time.substr(0, 2));
   const Integer minute = std::stoi(time.substr(3, 2));
   const Integer second = std::stoi(time.substr(6, 2));
   UtcDate calendar = calendarDate ?
         UtcDate(year, std::stoi(date.substr(5, 2)), std::stoi(date.substr(8, 2)), hour, minute, second) :
         UtcDate(year, std::stoi(date.substr(5, 3)), hour, minute, second);

   if (sourceTimeSystem == "UTC")
      return CCSDSEMSegment::ParseEpochGT(text);
   if (second >= 60)
      throw MeasurementException("A TDM leap-second epoch requires TIME_SYSTEM=UTC");

   // Keep the fraction separate from whole seconds, as well as from MJD.
   Real fraction = 0.0;
   Real place = 0.1;
   for (size_t i = 9; i < time.size(); ++i)
   {
      fraction += (time[i] - '0') * place;
      place *= 0.1;
   }
   GmatTime epoch = ModifiedJulianDateGT(calendar.GetYear(), calendar.GetMonth(),
         calendar.GetDay(), 0, 0, 0.0);
   epoch.AddSeconds(hour * 3600.0 + minute * 60.0 + second);
   epoch.AddSeconds(fraction);

   Integer system = TimeSystemConverter::Instance()->GetTimeTypeID(sourceTimeSystem);
   if (sourceTimeSystem == "GPS")
   {
      // Nominal GPS time is TAI minus 19 seconds (USNO GPS time-scale definition).
      epoch.AddSeconds(19.0);
      system = TimeSystemConverter::TAIMJD;
   }
   if (system < 0 || system >= TimeSystemConverter::TimeSystemCount)
      throw MeasurementException("Unsupported TDM time system: " + sourceTimeSystem);
   return TimeSystemConverter::Instance()->Convert(epoch, system, TimeSystemConverter::A1MJD);
}
