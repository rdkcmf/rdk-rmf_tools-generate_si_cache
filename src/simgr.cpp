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
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "simgr.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>


uint32_t crctab[256];

//#if ((ULONG_MAX) == (UINT_MAX))
//#define IS32BIT
//#else
//#define IS64BIT
//#endif

#define HERTZ 1000000

#define DEFAULT_SI_CACHE_FILENAME "SICache"

#define XML_DECLARATION "1.0", "", ""
#define XML_SIDB_ELEMENT_NAME "SIDB"
#define XML_PGM_ELEMENT_NAME "program"
#define XML_SOURCE_ID_KEY "sourceid"
#define XML_FREQ_KEY "carrier_frequency"
#define XML_MODULATION_KEY "modulation_mode"
#define XML_PGM_NO_KEY "program_number"
#define XML_VIRT_CHAN_NO_KEY "virtual_channel_number"
#define XML_NO_OF_ATRIBUTES 5

SiManager::SiManager()
{
    numOfServices = 0;
}

int SiManager::xml_parse_attributes(xmlNodePtr parent)
{
        xmlChar* pVal = NULL;
        xmlNodePtr cur_node = parent;
        ProgramDetails programDetails; 
      
        while (cur_node != NULL)
        {
            //printf("cur_node->name: %s\n", cur_node->name);
            if ( ( !xmlStrcmp ( cur_node->name,  ( const xmlChar* ) XML_PGM_ELEMENT_NAME ) ) )
            {
                pVal = xmlGetProp(cur_node, (const xmlChar*)XML_SOURCE_ID_KEY);
		if (pVal)
		{
			//printf("%s: %s\n", XML_SOURCE_ID_KEY, pVal);
			programDetails.source_id = atoi((const char*)pVal);
			xmlFree(pVal);
		}
		else
		{
			printf(XML_SOURCE_ID_KEY" pVal NULL\n");
		}

		pVal = xmlGetProp(cur_node, (const xmlChar*)XML_PGM_NO_KEY);
		if (pVal)
		{
			//printf("%s: %s\n", XML_PGM_NO_KEY, pVal);
			programDetails.program_number = atoi((const char*)pVal);
			xmlFree(pVal);
		}
		else
		{
			printf(XML_PGM_NO_KEY" pVal NULL\n");
		}

		pVal = xmlGetProp(cur_node, (const xmlChar*)XML_FREQ_KEY);
		if (pVal)
		{
			//printf("%s: %s\n", XML_FREQ_KEY, pVal);
			programDetails.carrier_frequency = atoi((const char*)pVal)*HERTZ;
			xmlFree(pVal);
		}
		else
		{
			printf(XML_FREQ_KEY" pVal NULL\n");
		}

		pVal = xmlGetProp(cur_node, (const xmlChar*)XML_MODULATION_KEY);
		if (pVal)
		{
			//printf("%s: %s\n", XML_MODULATION_KEY, pVal);
			programDetails.modulation_mode = atoi((const char*)pVal);
			xmlFree(pVal);
		}
		else
		{
			printf(XML_MODULATION_KEY" pVal NULL\n");
		}

		pVal = xmlGetProp(cur_node, (const xmlChar*)XML_VIRT_CHAN_NO_KEY);
		if (pVal)
		{
			//printf("%s: %s\n", XML_VIRT_CHAN_NO_KEY, pVal);
			programDetails.virtual_channel_number = atoi((const char*)pVal);
			xmlFree(pVal);
		}
		else
		{
			printf(XML_VIRT_CHAN_NO_KEY" pVal NULL\n");
		}

                m_program_list.push_back(programDetails);
            }
 
            cur_node = cur_node->next;
	}

       numOfServices = m_program_list.size();

       return 0;
}

bool SiManager::load_xml_file(string xml_file)
{
    xmlDocPtr doc;
    xmlNodePtr root_element;
    xmlNodePtr parent;

    doc = xmlParseFile ( xml_file.c_str() );
    if ( doc == NULL )
    {
        fprintf ( stderr, "Document not parsed successfully. \n" );
        return FALSE;
    }

    root_element = xmlDocGetRootElement ( doc );
    if ( root_element == NULL )
    {
        fprintf ( stderr,"empty document\n" );
        xmlFreeDoc ( doc );
        return FALSE;
    }

    if ( xmlStrcmp ( root_element->name, ( const xmlChar * )XML_SIDB_ELEMENT_NAME) )
    {
        fprintf ( stderr, "document of the wrong type, root node != %s", XML_SIDB_ELEMENT_NAME);
        xmlFreeDoc ( doc );
        return FALSE;
    }

    parent = root_element->xmlChildrenNode;


    xml_parse_attributes(parent);

    xmlFreeDoc ( doc );
    return TRUE;
}

void SiManager::init_mpeg2_crc(void)
{
    uint16_t i, j;
    uint32_t crc;

    for (i = 0; i < 256; i++)
    {
        crc = i << 24;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ SI_CACHE_CRC_QUOTIENT;
            else
                crc = crc << 1;
        }
        crctab[i] = crc;
    }
} // END crc32_init()

uint32_t SiManager::calc_mpeg2_crc(uint8_t * data, uint32_t len)
{
    uint32_t result;
    uint32_t i;

    if (len < 4)
        return 0;

    result = *data++ << 24;
    result |= *data++ << 16;
    result |= *data++ << 8;
    result |= *data++;
    result = ~result;
    len -= 4;

    for (i = 0; i < len; i++)
    {
        result = (result << 8 | *data++) ^ crctab[result >> 24];
    }

    return ~result;
} // END calc_mpeg2_crc()

/**
 * The <i>si_get_file_size()</i> function will get the size the specified file.
 *
 * @param fileName Is a pointer for the name of the file
 * @param size Is the size to file to be checked.
 * @return The size of the given file. -1 if the file not present or any other failure.
 *          is returned.
 */
unsigned int SiManager::get_file_size(const char * location)
{
    struct stat buff;
    unsigned int size;
    if (0 == stat(location, &buff))
    {
        size = (unsigned int) buff.st_size;
        return size;
    }
    return -1;
}

bool SiManager::write_crc_for_si_cache(string sidbFileName)
{
    int fd=0;
    uint32_t sizeOfSICache = 0;
    uint32_t crcFileSize = 0;
    uint32_t readSISize = 0;
    uint32_t crcValue = 0xFFFFFFFF;
    unsigned char *pCRCData = NULL;

    init_mpeg2_crc();

    sizeOfSICache = get_file_size(sidbFileName.c_str());

    readSISize = sizeOfSICache;
    crcFileSize = sizeOfSICache;

    pCRCData = (unsigned char*)malloc(crcFileSize*sizeof(unsigned char));
    memset(pCRCData, 0 , (crcFileSize*sizeof(unsigned char)));

    fd = open( sidbFileName.c_str(), O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
    if ( -1 == fd)
    {
            perror("File open failed");
            free(pCRCData);
            pCRCData=NULL;
            return FALSE;
    }

    readSISize = read(fd, pCRCData, readSISize);
    close(fd);
    if(readSISize < 0)
    {
        perror("SNS File Read Failed");
        free(pCRCData);
        pCRCData=NULL;
        return FALSE;
    }

    /* Find the CRC and write it at the end of SICache file */
    crcValue = calc_mpeg2_crc (pCRCData, crcFileSize);
    printf("Calculated CRC is [%u]\n", crcValue);
    free(pCRCData);
    pCRCData=NULL;

    fd = open( sidbFileName.c_str(), O_RDWR | O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
    if ( 0 > fd)
    {
            perror("File open failed");
            return FALSE;
    }

    crcFileSize = write( fd, &crcValue, sizeof (crcValue ));
    close(fd);
    if ( -1 == crcFileSize)
    {
	    perror("File append failed");
	    return FALSE;
    }

    return TRUE;

}

bool SiManager::write_crc_for_si_and_sns_cache(string sidbFileName, string snsdbFileName)
{
    int fd=0;
    uint32_t sizeOfSICache = 0;
    uint32_t sizeOfSISNSCache = 0;
    uint32_t crcFileSize = 0;
    uint32_t readSNSSize = 0;
    uint32_t readSISize = 0;
    uint32_t crcValue = 0xFFFFFFFF;
    unsigned char *pCRCData = NULL;

    init_mpeg2_crc();

    sizeOfSICache = get_file_size(sidbFileName.c_str());
    sizeOfSISNSCache = get_file_size(snsdbFileName.c_str());

    readSNSSize = sizeOfSISNSCache;
    readSISize = sizeOfSICache;

    crcFileSize = sizeOfSISNSCache + sizeOfSICache;

    pCRCData = (unsigned char*)malloc(crcFileSize*sizeof(unsigned char));
    memset(pCRCData, 0 , (crcFileSize*sizeof(unsigned char)));

    fd = open( snsdbFileName.c_str(), O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
    if ( -1 == fd)
    {
	    perror("File open failed");
            free(pCRCData);
            pCRCData=NULL;
	    return FALSE;
    }

    readSNSSize = read(fd, pCRCData, readSNSSize);
    close(fd);
    if(readSNSSize < 0)
    {
        perror("SNS File Read Failed");
        free(pCRCData);
        pCRCData=NULL;
        return FALSE;
    }

    fd = open( sidbFileName.c_str(), O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
    if ( -1 == fd)
    {
            perror("File open failed");
            free(pCRCData);
            pCRCData=NULL;
            return FALSE;
    }

    readSISize = read(fd, (pCRCData+readSNSSize), readSISize);
    close(fd);
    if(readSISize < 0)
    {
        perror("SNS File Read Failed");
        free(pCRCData);
        pCRCData=NULL;
        return FALSE;
    }

    /* Find the CRC and write it at the end of SICache file */
    crcValue = calc_mpeg2_crc (pCRCData, crcFileSize);
    printf("Calculated CRC is [%u]\n", crcValue);
    free(pCRCData);
    pCRCData=NULL;

    fd = open( sidbFileName.c_str(), O_RDWR | O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
    if ( -1 == fd)
    {
            perror("File open failed");
            return FALSE;
    }

    crcFileSize = write( fd, &crcValue, sizeof (crcValue ));
    close(fd);
    if ( -1 == crcFileSize)
    {
	    perror("File append failed");
	    return FALSE;
    }

    return TRUE;

}

/**
 *  Internal Method to initialize service entries to default values
 *
 * <i>init_si_entry()</i>
 *
 * @param si_entry is the service entry to set the default values for
 *
 * @return  None
 *
 */
void SiManager::init_si_entry(siTableEntry *si_entry)
{
    // TODO: use static struct initialization
    /* set default values as defined in OCAP spec (annex T) */
    /* These are in the order they appear in the struct, to make it
     easier to verify all have been initialized. --mlb */
    si_entry->ref_count = 1;
    si_entry->activation_time = 0;
    si_entry->ptime_service = 0LL;

    si_entry->next = NULL;
    si_entry->isAppType = FALSE;
    si_entry->source_id = SOURCEID_UNKNOWN;
    si_entry->app_id = 0;
    si_entry->channel_type = CHANNEL_TYPE_HIDDEN; /* */
    si_entry->video_standard = VIDEO_STANDARD_UNKNOWN;
    si_entry->source_name_entry = NULL; /* default value */
    si_entry->descriptions = NULL; /* default value */
    si_entry->major_channel_number = SI_DEFAULT_CHANNEL_NUMBER; /* default value */
    si_entry->minor_channel_number = SI_DEFAULT_CHANNEL_NUMBER; /* default value */
    si_entry->service_type = SI_SERVICE_TYPE_UNKNOWN; /* default value */
    si_entry->freq_index = 0;
    si_entry->mode_index = 0;

    si_entry->state = SIENTRY_UNSPECIFIED;
    si_entry->virtual_channel_number = VIRTUAL_CHANNEL_UNKNOWN;
    si_entry->program_number = 0;
    si_entry->transport_type = 0; // Default is MPEG2
    si_entry->scrambled = FALSE;
}

bool SiManager::parse_program_details(siTableEntry **si_entry_head, uint32_t frequency[], uint32_t modulation[], int& f_index, int& m_index)
{
    int i;
    list<ProgramDetails>::iterator pProgramDetails;
    siTableEntry* si_entry = NULL;
    siTableEntry* si_walker = NULL;
    bool ret = TRUE;

    for( i=0,pProgramDetails=m_program_list.begin(); i<m_program_list.size(); i++,pProgramDetails++ )
    {
	    si_entry = (siTableEntry*)malloc(sizeof(siTableEntry));
            init_si_entry(si_entry);
	    //memset(si_entry, 0 , sizeof(siTableEntry));
	    int foundfreq = 0;
	    int foundmod = 0;

	    si_entry->virtual_channel_number = pProgramDetails->virtual_channel_number;
	    si_entry->program_number = pProgramDetails->program_number;
	    si_entry->source_id = pProgramDetails->source_id;
            si_entry->state |= SIENTRY_MAPPED;

	    for (int i = 0; i < f_index; i++)
	    {
		    if ( pProgramDetails->carrier_frequency == frequency[i])
		    {
			    si_entry->freq_index = i;
			    foundfreq = 1;
			    break;
		    }
	    }

	    if ( foundfreq == 0)
	    {
		    frequency[ f_index ] = pProgramDetails->carrier_frequency;
		    si_entry->freq_index = f_index;
		    f_index++;
	    }

	    for (int i = 0; i <m_index; i++)
	    {
		    if ( pProgramDetails->modulation_mode == modulation[i])
		    {
			    si_entry->mode_index = i;
			    foundmod= 1;
			    break;
		    }
	    }

	    if ( foundmod == 0)
	    {
		    modulation[ m_index ] = pProgramDetails->modulation_mode;
		    si_entry->mode_index = m_index;
		    m_index++;
	    }

	    if (*si_entry_head == NULL)
	    {
		    *si_entry_head = si_entry;
	    }
	    else
	    {
           	    for(si_walker=*si_entry_head; si_walker->next!=NULL; si_walker = si_walker->next);

                    si_walker->next = si_entry;
	    }

    }

    return ret;
}

bool SiManager::cache_si_data ( string sidbFileName)
{
	int ret, fd;
	uint32_t frequency[SI_CACHE_MAX_FREQUENCIES + 1];
	uint32_t modulation[SI_CACHE_MAX_FREQUENCIES + 1];
	int f_index = 0, m_index=0;
	siTableEntry* si_entry, *si_entry_head = NULL;
	unsigned char * buf = NULL;
	uint32_t version;


	buf = (unsigned char*) malloc(sizeof(version)+sizeof(frequency)+ sizeof(modulation)+ 500* sizeof(siTableEntry));
	memset( frequency, 0, sizeof(frequency));
	memset( modulation, 0, sizeof(modulation));

        if(!parse_program_details(&si_entry_head,frequency, modulation, f_index, m_index))
        {
		perror("cache_si_data() failed");
		return FALSE;
        }

	fd = open( sidbFileName.c_str(), O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
	if ( -1 == fd)
	{
		perror("File open failed");
		return FALSE;
	}
	version = SI_CACHE_FILE_VERSION;

	ret = write( fd, &version, sizeof (version ));
	if ( -1 == ret)
	{
		perror("File write failed");
	        close(fd);
		return FALSE;
	}
	memcpy(buf, &version, sizeof (version ));
	buf+=sizeof (version );

	ret = write( fd, frequency, sizeof (frequency ));
	if ( -1 == ret)
	{
		perror("File write failed");
	        close(fd);
		return FALSE;
	}
	memcpy(buf, frequency, sizeof (frequency ));
	buf+=sizeof (frequency );

	ret = write( fd, modulation, sizeof (modulation ));
	if ( -1 == ret)
	{
		perror("File write failed");
	        close(fd);
		return FALSE;
	}
	memcpy(buf, modulation, sizeof (modulation ));
	buf+=sizeof (modulation );
	
	si_entry = si_entry_head;

	while (si_entry)
	{
		ret = write( fd, si_entry, sizeof (*si_entry ));
		if ( -1 == ret)
		{
			perror("File write failed");
	                close(fd);
			return FALSE;
		}
		memcpy(buf, si_entry, sizeof (*si_entry ));
		buf+=sizeof (*si_entry  );
		si_entry = si_entry->next;
	}

	close(fd);

        return TRUE;
}

bool SiManager::cache_sns_data ( string snsdbFileName )
{
    SourceNameEntry *sn_entry=NULL, *sn_entry_head=NULL;
    int i, fd, ret;
    list<ProgramDetails>::iterator pProgramDetails;

    if(m_program_list.size()==0)
    {
	    perror("File open failed");
	    return FALSE;
    }

    for( i=0,pProgramDetails=m_program_list.begin(); i<m_program_list.size()-1; i++,pProgramDetails++ )
    {
        sn_entry = (SourceNameEntry*)malloc(sizeof(SourceNameEntry));
        memset(sn_entry, 0 , sizeof(SourceNameEntry));
        sn_entry->appType = FALSE;
        sn_entry->mapped = false;
        sn_entry->id = pProgramDetails->source_id;
        sn_entry->source_long_names = NULL;
	sn_entry->source_names = NULL;

	if (sn_entry_head == NULL)
	{
		sn_entry_head = sn_entry;
	}
	else
	{
		sn_entry->next = sn_entry_head;
		sn_entry_head = sn_entry;
	}
    }

    fd = open( snsdbFileName.c_str(), O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
    if ( -1 == fd)
    {
	    perror("File open failed");
	    return FALSE;
    }


    sn_entry = sn_entry_head;

    while (sn_entry)
    {
        ret = write( fd, sn_entry, sizeof (*sn_entry ));
        if ( -1 == ret)
        {
            perror("File write failed");
            close(fd);
            return FALSE;
        }

        sn_entry = sn_entry->next;
    }

    close(fd);

    return TRUE;
}

bool SiManager::load_si_data(string sidbFileName)
{
    int fd=0, version=0, count=0, size=0;
    unsigned char *buf=(unsigned char*)malloc(sizeof(siTableEntry)*sizeof(unsigned char));
    siTableEntry *si_entry;
    ProgramDetails programDetails;
    uint32_t frequency[SI_CACHE_MAX_FREQUENCIES + 1];
    uint32_t modulation[SI_CACHE_MAX_FREQUENCIES + 1];
//#ifdef IS64BIT 
//    long name_count=0;
//#else
//    int name_count=0;
//#endif

    uint32_t crcValue=0;
    bool ret = TRUE;

    fd = open( sidbFileName.c_str(), O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH  );
    if ( -1 == fd)
    {
            perror("File open failed");
            return FALSE;
    }

    count = sizeof(version);
    count = read(fd, (void *)&version, count);

    if (count == sizeof(version))
    {
	    if (version != SI_CACHE_FILE_VERSION)
	    {
		    printf("Invalid cache version 0x%x, expected 0x%x\n", version, SI_CACHE_FILE_VERSION);
                    close(fd);   
                    return FALSE;
	    }
	    else
	    {
		    printf("Read version 0x%x\n", version);
	    }
    }
    else
    {
	    printf("reading Version [%s] failed count:%d version:0x%x\n", sidbFileName.c_str(), count, version);
            close(fd);   
            return FALSE;
    }

    count = sizeof(frequency);
    count = read(fd, (void *)frequency, count);
    if (count != sizeof(frequency))
    {
	printf("reading FREQ from [%s] failed\n", sidbFileName.c_str());
        close(fd);   
	return FALSE;
    }

    count = sizeof(modulation);
    count = read(fd, (void *)modulation, count);
    if (count != sizeof(modulation))
    {
        printf("reading MODULATION from [%s] failed\n", sidbFileName.c_str());
        close(fd);
        return FALSE;
    }

    count = size = sizeof(siTableEntry);

    while(count == size)
    {
	count = read(fd, (void*)buf, size);
	if (count == 0 )
	{
	    printf("Reached end of file [%s]\n", sidbFileName.c_str());
            ret = TRUE;
            break;
	}
        else if((count < size) && (count == sizeof(crcValue)))
        {
            //crcValue = (uint32_t)buf;
            crcValue = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | (buf[0] & 0xFF);
            printf("CRC Value: [%u]\n", crcValue); 
            count=size;
            continue;
        }
        else if((count < 0) || (count < size))
	{
	    printf("Error in reading file [%s] \n", sidbFileName.c_str());
            perror("Reason: ");
            ret = FALSE;
            break;
	}

        si_entry = (siTableEntry*)buf;
        programDetails.source_id = si_entry->source_id;
        programDetails.program_number = si_entry->program_number;
        programDetails.carrier_frequency = frequency[si_entry->freq_index]/HERTZ;
        programDetails.modulation_mode = modulation[si_entry->mode_index];
        programDetails.virtual_channel_number =  si_entry->virtual_channel_number;

        m_program_list.push_back(programDetails);

    	long name_count = (long)(si_entry->descriptions);

        if(name_count != 0)
        {
            //TODO : Skip the description data
            for(int i=0 ; i< name_count; i++)
            {
                unsigned char name_len_buf[4] = {0};
                int buf_count = sizeof(name_len_buf);

                count = read(fd, (void *)name_len_buf, buf_count); 

                int name_len = (name_len_buf[0]) | (name_len_buf[1] << 8) | (name_len_buf[2] << 16) | (name_len_buf[3] << 24);
                lseek(fd, name_len, SEEK_CUR); 
            }
        }
    }

    numOfServices = m_program_list.size();

    close(fd);
    free(buf);
    buf=NULL;
    return ret;
}

bool SiManager::Load(string file_name, int mode)
{
	bool ret = FALSE;
	switch (mode)
	{
		case SI_FILETYPE_XML:
		{
                    printf("Loading program data from XML file [%s]\n", file_name.c_str());
		    ret = load_xml_file(file_name);
                    if(ret == TRUE)
                        printf("No:of Services Available: %d\n", numOfServices);
		}
		break;
                case SI_FILETYPE_BINARY:
                {
                    printf("Loading program data from binary cache file [%s]\n", file_name.c_str());
                    ret = load_si_data(file_name);
                    if(ret == TRUE)
                        printf("No:of Services Available: %d\n", numOfServices);

                }
                break;

		default:
		{
			cout<<"Mode not implemented"<<endl;
		}
	}
	return ret;
}

void SiManager::DumpChannelMap()
{
    list<ProgramDetails>::iterator p;
    int i;

    for( i=0,p=m_program_list.begin(); i<m_program_list.size(); i++,p++ )
    {
	printf("RChannelVCN[%06d]-SRCID[%06x]-Freq[%09d]-Mode[%04d]-Prog[%08d]\n",p->virtual_channel_number, p->source_id, p->carrier_frequency*HERTZ,p->modulation_mode,p->program_number);
    }
    cout << endl;
}

bool SiManager::GenerateSICache ( string sidbFileName )
{
    if(!cache_si_data(sidbFileName))
    {
        perror("Failed to cache si data");
        return FALSE;
    }

    if(!write_crc_for_si_cache(sidbFileName))
    {
        perror("Failed to append CRC");
        return FALSE;
    }

    printf("Binary file [%s] generated successfully\n", sidbFileName.c_str()); 
}

bool SiManager::GenerateSICache ( string sidbFileName, string snsdbFileName )
{
    printf("Generating SI Cache [%s] from extracted program details\n", sidbFileName.c_str());
    if(!cache_si_data(sidbFileName))
    {
        perror("Failed to cache si data");
        return FALSE;
    }

    printf("Generating SNS Cache [%s] from extracted program details\n", snsdbFileName.c_str());
    if(!cache_sns_data(snsdbFileName))
    {
        perror("Failed to cache si data");
        return FALSE;
    }

    if(!write_crc_for_si_and_sns_cache(sidbFileName, snsdbFileName))
    {
        perror("Failed to append CRC");
        return FALSE;
    }

    printf("Binary files [%s] and [%s] generated successfully\n", sidbFileName.c_str(), snsdbFileName.c_str()); 
}

bool SiManager::GenerateXML (string sixmlFileName)
{
	xmlTextWriterPtr writer;
        list<ProgramDetails>::iterator p;
        int i=0;

        printf("Generating XML file [%s] from extracted program data\n", sixmlFileName.c_str());
	writer = xmlNewTextWriterFilename(sixmlFileName.c_str(), 0);
	xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);

	xmlTextWriterStartElement(writer, BAD_CAST XML_SIDB_ELEMENT_NAME);
        xmlTextWriterSetIndent(writer, 1);
        xmlTextWriterSetIndentString(writer, BAD_CAST "");

        for( i=0,p=m_program_list.begin(); i<m_program_list.size(); p++, i++ )
        {
	    xmlTextWriterStartElement(writer, BAD_CAST XML_PGM_ELEMENT_NAME);
            xmlTextWriterSetIndentString(writer, BAD_CAST "");

            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST XML_SOURCE_ID_KEY, "%d", p->source_id);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST XML_PGM_NO_KEY, "%d", p->program_number);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST XML_FREQ_KEY, "%d", p->carrier_frequency);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST XML_MODULATION_KEY, "%d", p->modulation_mode);
            xmlTextWriterWriteFormatAttribute(writer, BAD_CAST XML_VIRT_CHAN_NO_KEY, "%d",p->virtual_channel_number);

	    xmlTextWriterEndElement(writer);
        }
	xmlTextWriterEndElement(writer);
	xmlTextWriterEndDocument(writer);
	xmlFreeTextWriter(writer);
        printf("XML file [%s] generated successfully\n", sixmlFileName.c_str()); 
}
