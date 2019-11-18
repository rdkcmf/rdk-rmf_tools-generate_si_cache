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

/**
 * @file si_cache_app.cpp
 * @brief SICache is used for tuning and showing video.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "simgr.h"

using namespace std;


/**
* @defgroup rmf_tools rmf tools
*
* @defgroup SI_CACHE   Generate SI cache
* @ingroup  rmf_tools
*
* @defgroup SI_CACHE_TYPES SI Cache Data Types
* @ingroup  SI_CACHE
*
* @defgroup SI_CACHE_APIS  SI Cache APIs
* @ingroup  SI_CACHE
*
**/


/**
 * @addtogroup SI_CACHE_TYPES
 * @{
 */

#define MODE_XML_TO_BINARY "xtob"
#define MODE_BINARY_TO_XML "btox"
#define MODE_DUMP_TO_CONSOLE "dump"

/**
 * @}
 */

/**
 * @addtogroup SI_CACHE_APIS
 * @{
 */

/**
 * @brief This function displays the brief usage message for the tool.
 *
 * It includes the usage of the tool, description, options and examples.
 
 * @param[in] app_name  Indicates the options for the tools
 */
void printUsage(char *app_name)
{
    printf("\nRDK SI CACHE CONVERTER \t- User's Manual\n");
    printf("\nUSAGE: \n\n\t%s [-mode MODE] [-input INPUT_FILE] [-output OUTPUT_FILE] [-sioutput SIOUTPUT_FILE] [-snsoutput SNSOUTPUT_FILE] [-help] [-version]\n", app_name);
    printf("\nDESCRIPTION:\n\n\tRDK SI Cache Converter is a tool to convert the the given SI Cache file from binary to XML format or vice versa based on the specified mode. This tool helps to enhance the existing channel map\n\tto support the development activities at locations where Comcast head-end connectivity is not available. This tool uses Libxml2 XML toolkit (http://xmlsoft.org/) for all XML related operations.\n\nOPTIONS:\n\n");
    printf("\t-mode\t\t: Operation mode. It can be one of the following:\n");
    printf("\t\t\t\txtob - Generate SI Cache binaries from the given XML file\n");
    printf("\t\t\t\tbtox - Generate SI Cache XML file from the given binary file\n");
    printf("\t\t\t\tdump - Dump channel map from the given binary file to console\n");
    printf("\t-input    \t: Input file. It can be an XML file or a binary SI Cache file depending on the operation mode\n");
    printf("\t-output   \t: Output file. It can be a binary SI Cache file or an XML file depending on the operation mode\n");
    printf("\t-sioutput \t: Binary SI Cache file to be generated. To use this option, the mode should be set to 'xtob'\n");
    printf("\t-snsoutput\t: Binary SNS Cache file to be generated. To use this option, the mode should be set to 'xtob'\n");
    printf("\t-version\t: Prints the tool's version details\n");
    printf("\t-help\t\t: Prints this user's manual\n");
    printf("\nEXAMPLES:\n");
    printf("\n(i)   Command to generate binary SI cache from given XML file:\n");
    printf("\t%s -mode xtob -input si.xml -output SICache\n", app_name);
    printf("\n(ii)  Command to generate both binary SI cache and binary SNS cache from given XML file:\n");
    printf("\t%s -mode xtob -input si.xml -sioutput SICache -snsoutput SNSCache \n",app_name);
    printf("\n(iii) Command to generate XML file from binary SI cache:\n");
    printf("\t%s -mode btox -input SICache -output si.xml\n", app_name);
    printf("\n(iv) Command to dump channel map from binary SI cache to console:\n");
    printf("\t%s -mode dump -input SICache \n\n", app_name);
 
}

/**
 * @brief This API scans parameters from the command line. 
 *
 * @param[in]  argc             Number of parameters
 * @param[in]  argv             Parameter list
 * @param[in]  mode             Operation mode. It can be one of the following:
 *                                xtob - Generate SI Cache binaries from the given XML file
 *                                btox - Generate SI Cache XML file from the given binary file
 *                                dump - Dump channel map from the given binary file to console
 * @param[in]  input_file        Input file. It can be an XML file or a binary SI Cache file depending on the operation mode
 * @param[out] output_file       Output file. It can be a binary SI Cache file or an XML file depending on the operation mode
 * @param[out] output_file_sns   Binary SNS Cache file to be generated. To use this option, the mode should be set to 'xtob'.
 *
 * @return Returns status of the operation.
 * @retval True on sucess, False on failure.
 */
bool processCommandLine(int argc, char** argv, string& mode, string& input_file, string& output_file, string& output_file_sns)
{
    int idx;

    if(argc<=1)
    {
        printUsage(argv[0]);
        return false;
    }

    for(int i=1; i < argc; ++i)
    {
        if(std::string(argv[i]) == "-help")
        {
            printUsage(argv[0]);
            return false;
        }
	else if(std::string(argv[i]) == "-version")
	{
    	    printf("RDK SI CACHE CONVERTER - %s\nCopyright 2016 RDK Management, Licensed under the Apache License,Version 2.0\n", PACKAGE_VERSION);
	    return false;
	}
        else if(std::string(argv[i]) == "-mode")
        {
            if(i < argc-1 && argv[i+1][0] != '-')
                mode = argv[++i];
            //verify the mode matches one of our modes
            if((mode != MODE_XML_TO_BINARY) && (mode != MODE_BINARY_TO_XML) && (mode != MODE_DUMP_TO_CONSOLE))
            {
                printf("argv[0]: invalid mode option '%s'.  Try '%s -help' for more info.\n", mode.c_str(),argv[0]);
                return false;
            }
            if(i+1 > idx)
                idx = i+1;
        }
        else if(std::string(argv[i]) == "-input")
        {
            if(i < argc-1 && argv[i+1][0] != '-')
                input_file = argv[++i];
        }
        else if((std::string(argv[i]) == "-output") && (mode != MODE_DUMP_TO_CONSOLE))
        {
            if(i < argc-1 && argv[i+1][0] != '-')
                output_file = argv[++i];
        }
        else if((std::string(argv[i]) == "-sioutput") && (mode == MODE_XML_TO_BINARY))
        {
            if(i < argc-1 && argv[i+1][0] != '-')
                output_file = argv[++i];
        }
        else if((std::string(argv[i]) == "-snsoutput") && (mode == MODE_XML_TO_BINARY))
        {
            if(i < argc-1 && argv[i+1][0] != '-')
                output_file_sns = argv[++i];
        }
        else
        {
            printf("argv[0]: invalid option '%s'.  Try '%s -help' for more info.\n", argv[i], argv[0]);
            return false;
       }

    }

    if(mode.empty())
    {
        printf("Mode: unset \n");
        return false;
    }

    if(input_file.empty())
    {
        printf("Input file: unset \n");
        return false;
    }

    if(output_file.empty() && (mode != MODE_DUMP_TO_CONSOLE))
    {
        printf("Output file: unset \n");
        return false;
    }

    return true;
}

int main(int argc, char ** argv)
{
    string mode;
    string input_file;
    string output_file;
    string output_sns_file;
    SiManager *pSiMgr = NULL;
    bool ret = FALSE;


    if(!processCommandLine(argc, argv, mode, input_file, output_file, output_sns_file))
        return -1;

    if(mode == MODE_XML_TO_BINARY)
    {
        pSiMgr = new SiManager();

	if(pSiMgr->Load(input_file, SI_FILETYPE_XML))
        {
                if(output_sns_file.empty())
                    ret = pSiMgr->GenerateSICache(output_file);
                else
                    ret = pSiMgr->GenerateSICache(output_file, output_sns_file);
        }
    }
    else if(mode == MODE_BINARY_TO_XML)
    {
        pSiMgr = new SiManager();

        if(pSiMgr->Load(input_file, SI_FILETYPE_BINARY))
        {
                ret = pSiMgr->GenerateXML(output_file);
        }
    }
    else if(mode == MODE_DUMP_TO_CONSOLE)
    {
        pSiMgr = new SiManager();

        if(pSiMgr->Load(input_file, SI_FILETYPE_BINARY))
        {
                pSiMgr->DumpChannelMap();
        }

    }

    return ret;
}

/**
 * @}
 */


