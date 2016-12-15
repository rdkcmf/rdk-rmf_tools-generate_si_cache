/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef _SIMGR_H_
#define _SIMGR_H_

#include <list>
#include <string>
#include <stdint.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

using namespace std;

#define TRUE 1
#define FALSE 0

#define SI_CACHE_FILE_VERSION           0x101u
#define SI_CACHE_MAX_FREQUENCIES 255
#define SI_CACHE_CRC_QUOTIENT 0x04C11DB7
#define HERTZ 1000000

#define DEFAULT_SI_CACHE_FILENAME "SICache"

/**
 * @addtogroup sifields
 * The following macros represents the default values for SI fields (OCAP specified)
 * sourceId and tsId are set by default to -1
 * @{
 */
/* Default values for SI fields (OCAP specified) */
/* sourceId and tsId are set by default to -1 */
#define SOURCEID_UNKNOWN                (0xFFFFFFFF)
#define TSID_UNKNOWN                    (0xFFFFFFFF) // ECR 1072
#define SI_DEFAULT_PROGRAM_NUMBER           (0xFFFFFFFF)
#define PROGRAM_NUMBER_UNKNOWN                  (0xFFFF)
#define VIRTUAL_CHANNEL_UNKNOWN                 (0xFFFF)
#define SI_DEFAULT_CHANNEL_NUMBER           0xFFFFFFFF

typedef enum _SIFileType_t_ {
	SI_FILETYPE_XML,    
	SI_FILETYPE_BINARY,    
}SIFileType_t;

/**
 * @enum siEntryState_
 * @brief This enumeration represents the state of a service entry based on of SVCT-DCM, SVCT-VCM signaling.
 * @ingroup Outofbanddatastructures
 */
typedef enum _siEntryState_
{
    SIENTRY_UNSPECIFIED = 0, 
    SIENTRY_PRESENT_IN_DCM = 0x01, // Present in DCM
    SIENTRY_DEFINED = 0x02, // Defined in DCM
    SIENTRY_MAPPED = 0x04, // Mapped in VCM
    SIENTRY_DEFINED_MAPPED = SIENTRY_PRESENT_IN_DCM | SIENTRY_DEFINED
            | SIENTRY_MAPPED, // Marked defined in SVCT-DCM, mapped in SVCT-VCM
    SIENTRY_DEFINED_UNMAPPED = SIENTRY_PRESENT_IN_DCM | SIENTRY_DEFINED, // Marked defined in SVCT-DCM, not mapped in SVCT-VCM
    SIENTRY_UNDEFINED_MAPPED = SIENTRY_PRESENT_IN_DCM | SIENTRY_MAPPED, // Marked undefined in SVCT-DCM, mapped in SVCT-VCM
    SIENTRY_UNDEFINED_UNMAPPED = SIENTRY_PRESENT_IN_DCM
// Marked undefined SVCT-DCM, not mapped in SVCT-VCM
} siEntryState;

/**
 * @enum siChannelType
 * @brief This enumeration represents the service information channel types such as normal, hidden and so on.
 * @ingroup Outofbanddatastructures
 */
typedef enum _siChannelType
{
    CHANNEL_TYPE_NORMAL = 0x00,
    CHANNEL_TYPE_HIDDEN = 0x01,
    CHANNEL_TYPE_RESERVED_2 = 0x02,
    CHANNEL_TYPE_RESERVED_3 = 0x03,
    CHANNEL_TYPE_RESERVED_4 = 0x04,
    CHANNEL_TYPE_RESERVED_5 = 0x05,
    CHANNEL_TYPE_RESERVED_6 = 0x06,
    CHANNEL_TYPE_RESERVED_7 = 0x07,
    CHANNEL_TYPE_RESERVED_8 = 0x08,
    CHANNEL_TYPE_RESERVED_9 = 0x09,
    CHANNEL_TYPE_RESERVED_10 = 0x0A,
    CHANNEL_TYPE_RESERVED_11 = 0x0B,
    CHANNEL_TYPE_RESERVED_12 = 0x0C,
    CHANNEL_TYPE_RESERVED_13 = 0x0D,
    CHANNEL_TYPE_RESERVED_14 = 0x0E,
    CHANNEL_TYPE_RESERVED_15 = 0x0F,
} siChannelType;

typedef enum _siVideoStandard
{
    VIDEO_STANDARD_NTSC = 0x00,
    VIDEO_STANDARD_PAL625 = 0x01,
    VIDEO_STANDARD_PAL525 = 0x02,
    VIDEO_STANDARD_SECAM = 0x03,
    VIDEO_STANDARD_MAC = 0x04,
    VIDEO_STANDARD_UNKNOWN = 0x05
} siVideoStandard;

/**
 * @enum siServiceType
 * @brief Service type values represents "digital television",
 * "digital radio", "NVOD reference service", "NVOD time-shifted service",
 * "analog television", "analog radio", "data broadcast" and "application".
 *
 * (These values are mappable to the ATSC service type in the VCT table and
 * the DVB service type in the Service Descriptor.)
 * @ingroup Outofbanddatastructures
 */
typedef enum _siServiceType
{
    SI_SERVICE_TYPE_UNKNOWN = 0x00,
    SI_SERVICE_ANALOG_TV = 0x01,
    SI_SERVICE_DIGITAL_TV = 0x02,
    SI_SERVICE_DIGITAL_RADIO = 0x03,
    SI_SERVICE_DATA_BROADCAST = 0x04,
    SI_SERVICE_NVOD_REFERENCE = 0x05,
    SI_SERVICE_NVOD_TIME_SHIFTED = 0x06,
    SI_SERVICE_ANALOG_RADIO = 0x07,
    SI_SERVICE_DATA_APPLICATION = 0x08
} siServiceType;

typedef struct ProgramDetails_s
{
    uint32_t virtual_channel_number;
    uint32_t source_id;
    uint32_t modulation_mode;
    uint32_t carrier_frequency;
    uint32_t  program_number;
}ProgramDetails;

#if 0
typedef struct _LangSpecificStringList
{
    char language[4]; /* 3-character ISO 639 language code + null terminator */
    char *string;
    struct LangSpecificStringList *next;
} LangSpecificStringList;
#endif

typedef struct _SourceNameEntry
{
  bool appType;
  uint32_t id;              // if appType=FALSE, sourceID otherwise appID
  bool mapped;          // true if mapped by a virtual channel false for DSG service name
  void *source_names;      // NTT_SNS, CVCT
  void *source_long_names; // NTT_SNS
  struct _SourceNameEntry* next;              // next source name entry
} SourceNameEntry;

struct siTableEntry
{
    uint32_t ref_count;
    uint32_t activation_time; // SVCT
    long long ptime_service;
    uint32_t ts_handle;
    uint32_t program; // Program info for this service. Can be shared with other rmf_SiTableEntry elements,
    uint32_t tuner_id; // '0' for OOB, start from '1' for IB
    int32_t valid;
    uint16_t virtual_channel_number;
    int32_t isAppType; // for DSG app
    uint32_t source_id; // SVCT
    uint32_t app_id; //
    uint32_t dsgAttached;
    uint32_t dsg_attach_count; // DSG attach count
    uint32_t state;
    uint32_t channel_type; // SVCT
    uint32_t video_standard; // SVCT (NTSC, PAL etc.)
    uint32_t service_type; // SVCT (desc)

    void * source_name_entry;      // Reference to the corresponding NTT_SNS entry
    void *descriptions; // LVCT/CVCT (desc)
    uint8_t freq_index; // SVCT
    uint8_t mode_index; // SVCT
    uint32_t major_channel_number;
    uint32_t minor_channel_number; // SVCT (desc)

    uint16_t program_number;       // SVCT
    uint8_t  transport_type;       // SVCT (0==MPEG2, 1==non-MPEG2)
    int32_t scrambled;            // SVCT
    void* hn_stream_session;  // HN stream session handle
    uint32_t hn_attach_count; // HN stream session PSI attach / registration count
    siTableEntry* next; // next service entry
} ;

class SiManager
{
    protected:
	list<ProgramDetails> m_program_list;
        int numOfServices;

    private:
	bool load_xml_file(string xml_file);
        int xml_parse_attributes(xmlNodePtr parent);    
        void init_mpeg2_crc(void);
        uint32_t calc_mpeg2_crc(uint8_t * data, uint32_t len);
        void init_si_entry(siTableEntry *si_entry);
        bool parse_program_details(siTableEntry **si_entry_head, uint32_t frequency[], uint32_t modulation[], int &f_index, int &m_index);
        bool cache_si_data ( string sidbFileName);
        bool cache_sns_data ( string snsdbFileName);
        bool write_crc_for_si_cache(string sidbFileName);
        bool write_crc_for_si_and_sns_cache(string sidbFileName, string snsdbFileName);
        unsigned int get_file_size(const char * location);
        bool load_si_data(string sidbFileName);

    public:
	SiManager();
	bool Load(string file_name, int mode);
        bool GenerateSICache ( string sidbFileName );
        bool GenerateSICache ( string sidbFileName, string snsdbFileName );
        bool GenerateXML (string sixmlFileName);
	void DumpChannelMap();		
};

#endif // _SIMGR_H_
