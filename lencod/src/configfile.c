
#define INCLUDED_BY_CONFIGFILE_C
// *************************************************************************************
// *************************************************************************************
// Configfile.c	 Configuration handling
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Stephan Wenger				   <stewe@cs.tu-berlin.de>
// *************************************************************************************
// *************************************************************************************
//
// Note: In the future this module should hide the Parameters and offer only
// Functions for their access.  Modules which make frequent use of some parameters
// (e.g. picture size in macroblocks) are free to buffer them on local variables.
// This will not only avoid global variable and make the code more readable, but also
// speed it up.  It will also greatly facilitate future enhancements such as the
// handling of different picture sizes in the same sequence.
//
// For now, everything is just copied to the inp_par structure (gulp)
//
// *************************************************************************************
// *************************************************************************************
// New configuration File Format
// *************************************************************************************
// *************************************************************************************
//
// Format is line oriented, maximum of one parameter per line
//
// Lines have the following format:
// <ParameterName> = <ParameterValue> # Comments \n
// Whitespace is space and \t
//
// <ParameterName> are the predefined names for Parameters and are case sensitive.
//   See configfile.h for the definition of those names and their mapping to configinput-> 
//   values.
// <ParameterValue> are either integers [0..9]* or strings.  
//   Integers must fit into the wordlengths, signed values are generally assumed.
//   Strings containing no whitespace characters can be used directly.  Strings containing
//   whitespace characters are to be inclosed in double quotes ("string with whitespace") 
//   The double quote character is forbidden (may want to implement something smarter here).
//
// Any Parameters whose ParameterName is undefined lead to the termination of the program
// with an error message.
//
// Known bug/Shortcoming:	zero-length strings (i.e. to signal an non-existing file
//							have to be coded as "".
//
//
// Rules for using command files
//
// All Parameters are initially taken from DEFAULTCONFIGFILENAME, defined in configfile.h.  
// If an -f <config> parameter is present in the command line then this file is used to 
// update the defaults of DEFAULTCONFIGFILENAME.  There can be more than one -f parameters 
// present.  If -p <ParameterName = ParameterValue> parameters are present then these 
// overide the default and the additional config file's settings, and are themselfes 
// overridden by future -p parameters.  There must be whitespace between -f and -p commands 
// and their respecitive parameters
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "configfile.h"


static char *GetConfigFileContent (char *Filename);
static void ParseContent (char *buf, int bufsize);
static int ParameterNameToMapIndex (char *s);
static void PatchInp ();


#define MAX_ITEMS_TO_PARSE	10000



void Configure (int ac, char *av[]) {

	char *content;
	int CLcount, ContentLen, NumberParams;

	memset (&configinput, 0, sizeof (InputParameters));
	// Process default config file

	printf ("Parsing Configfile %s", DEFAULTCONFIGFILENAME);
	content = GetConfigFileContent (DEFAULTCONFIGFILENAME);
	ParseContent (content, strlen(content));
	printf ("\n");
	free (content);
	
	// Parse the command line

	CLcount = 1;
	while (CLcount < ac) {
// printf ("checking command line parameter %d\n", CLcount);
		if (0 == strncmp (av[CLcount], "-f", 2)) {			// A file parameter?
			content = GetConfigFileContent (av[CLcount+1]);
			printf ("Parsing Configfile %s", av[CLcount+1]);
			ParseContent (content, strlen (content));
			printf ("\n");
			free (content);
			CLcount += 2;
		} else {
			if (0 == strncmp (av[CLcount], "-p", 2)) {	// A config change?
				// Collect all data until next parameter (starting with -<x> (x is any character)),
				// put it into content, and parse content.

				CLcount++;
				ContentLen = 0;
				NumberParams = CLcount;	

				// determine the necessary size for content
				while (NumberParams < ac && av[NumberParams][0] != '-') 
					ContentLen += strlen (av[NumberParams++]);				// Space for all the strings
				ContentLen += 1000;											// Additional 1000 bytes for spaces and \0s


// printf ("Malloc %d for content\n", ContentLen);

				content = malloc (ContentLen);
				content[0] = '\0';

				// concatenate all parameters itendified before

				while (CLcount < NumberParams) {
					char *source = &av[CLcount][0];
					char *destin = &content[strlen (content)];

					while (*source != '\0') {
						if (*source == '=') {		// The Parser expects whitespace before and after '='
							*destin++=' '; *destin++='='; *destin++=' ';	// Hence make sure we add it
						} else
							*destin++=*source;
						source++;
					}
					*destin = '\0';
					CLcount++;
				}
				printf ("Parsing command line string '%s'", content);
				ParseContent (content, strlen(content));
				free (content);
				printf ("\n");
			} 
			else {
				printf ("Error in command line, ac %d, around string '%s', missing -f or -p parameters?", CLcount, av[CLcount]);
				exit (-300);
			}
		}
	}
	printf ("\n");
}


char *GetConfigFileContent (char *Filename) {
//	allocates memory buf, opens file Filename in f, reads contents into buf and returnz buf 

	unsigned FileSize;
	FILE *f;
	char *buf;

	if (NULL == (f = fopen (Filename, "r"))) {
		printf ("Cannot open configuration file %s, exit -300", Filename);
		exit (-300);
	}

	if (0 != fseek (f, 0, SEEK_END)) {
		printf ("Cannot fseek in configuration file %s, exit -301", Filename);
		exit (-301);
	}
		;
	FileSize = ftell (f);
	if (FileSize < 0 || FileSize > 60000) {
		printf ("Unreasonable Filesize %d reported by ftell for configuration file %s, exit -302\n", FileSize, Filename);
// getchar();
		exit (-302);
	}
	if (0 != fseek (f, 0, SEEK_SET)) {
		printf ("Cannot fseek in configuration file %s, exit -301", Filename);
		exit (-303);
	}

	buf = malloc (FileSize);
	assert (NULL != buf);

	// Note that ftell() gives us the file size as teh file system sees it.  The actual file size,
	// as reported by fread() below will be often smaller due to CR/LF to CR conversion and/or
	// control characters after the dos EOF marker in the file.  

	FileSize = fread (buf, 1, FileSize, f);
	

	fclose (f);
	return buf;
}



void ParseContent (char *buf, int bufsize) {
// Parses the character array buf and writes global variable input, which is defined in 
// configfile.h.  This hack will continue to be necessary to facilitate the addition of
// new parameters through the Map[] mechanism (Need compiler-generated addresses in map[]).

	char *items[MAX_ITEMS_TO_PARSE];
	int MapIdx;
	int item = 0;
	int InString = 0, InItem = 0;
	char *p = buf;
	char *bufend = &buf[bufsize];
	int IntContent; 
	int i;
	
// Stage one: Generate an argc/argv-type list in items[], without comments and whitespace.
// This is context insensitive and could be done most easily with lex(1).

// printf ("In ParseContent, string is '%s'\n", buf);
// printf ("strlen(content)=%d\n", strlen(buf));
// getchar();
	while (p < bufend) {
		switch (*p) {
			case 13:
				p++;
				break;
			case '#': 								// Found comment
				*p = '\0';							// Replace '#' with '\0' in case of comment immediately following integer or string
				while (*p != '\n' && p < bufend)	// Skip till EOL or EOF, whichever comes first
					p++;
				InString = 0;
				InItem = 0;
				break;
			case '\n':
				InItem = 0;
				InString = 0;
				*p++='\0';
				break;
			case ' ':
			case '\t':						// Skip whitespace, leave state unchanged
				if (InString)
					p++;
				else {						// Terminate non-strings once whitespace is found
					*p++ = '\0';
					InItem = 0;
				}
				break;

			case '"':						// Begin/End of String
				*p++ = '\0';
				if (!InString)  {
					items[item++] = p;
					InItem = ~InItem;
				}
				else
					InItem = 0;
				InString = ~InString;		// Toggle
				break;

			default:
				if (!InItem) {
					items[item++] = p;
					InItem = ~InItem;
				}
				p++;
		}
	}

	item--;
/*
	for (i=0; i<item; i+=3) {
		printf ("%s\t\t%s\t%s\n", items[i], items[i+1], items[i+2]);
		getchar();
	}
*/
	for (i=0; i<item; i+= 3) {
		if (0 > (MapIdx = ParameterNameToMapIndex (items[i]))) {
			printf (" Parsing error in config file: Parameter Name '%s' not recognized, exiting\n", items[0]);
			getchar();
			exit (-308);
		}
		if (strcmp ("=", items[i+1])) {
			printf (" Parsing error in config file: '\"' expected as the second token in each line\n");
			exit (-303);
		}

		// Now interprete the Value, context sensitive...

		switch (Map[MapIdx].Type) {
			case 0:						// Numerical
				if (1 != sscanf (items[i+2], "%d", &IntContent)) {
					printf (" Parsing error: Expected numerical value for Parameter of %s, found '%s', exiting\n", items[i], items[i+2]);
					exit (-304);
				}
				* (int *) (Map[MapIdx].Place) = IntContent;
				printf (".");
				break;
			case 1:
				strcpy ((char *) Map[MapIdx].Place, items [i+2]);
				printf (".");
				break;
			default:
				assert ("Unknown value type in the map definition of configfile.h");
		}
	}
	memcpy (input, &configinput, sizeof (InputParameters));
	PatchInp();
}


static int ParameterNameToMapIndex (char *s) {
	int i = 0;

	while (Map[i].TokenName != NULL)
		if (0==strcmp (Map[i].TokenName, s))
			return i;
		else
			i++;
	return -1;
};


static void PatchInp () {


	// consistency check of QPs
	if (input->qp0 > 31 || input->qp0 <= 0)
	{
		printf("Error input parameter quant_0,check configuration file\n");
		exit (-401);
	}

	if (input->qpN > 31 || input->qpN <= 0)
	{
		printf("Error input parameter quant_n,check configuration file\n");
		exit (-402);
	}

	if (input->qpB > 31 || input->qpB <= 0)             
	{
		printf("Error input parameter quant_B,check configuration file\n");
		exit (0);
	}

	// consistency check Search_range

	if (input->search_range > 39)		// StW
	{
		printf("Error in input parameter search_range, check configuration file\n");
		exit (-403);
	}

	// consistency check no_multpred
	if (input->no_multpred<1) input->no_multpred=1;
#if 0   // Do not check anymore
	// consistency check no_multpred
	if(input->no_multpred > MAX_MULT_PRED)
	{
		printf("Error : input parameter 'no_multpred' exceeds limit (1...5), check configuration file \n");
		exit(-404);
	}
#endif

	// consistency check size information

	if (input->img_height % 16 != 0 || input->img_width % 16 != 0)
	{
		printf("Unsupported image format x=%d,y=%d, must be a multiple of 16\n",input->img_width,input->img_height);
		exit(0);
	}
	
	// Set block sizes

	// First, initialize input->blc_size to all zeros
	memset (input->blc_size,0, 4*8*2);

	// set individual items
	if (input->InterSearch16x16) {
		input->blc_size[1][0]=16;
		input->blc_size[1][1]=16;
	}
	if (input->InterSearch16x8) {
		input->blc_size[2][0]=16;
		input->blc_size[2][1]= 8;
	}
	if (input->InterSearch8x16) {
		input->blc_size[3][0]= 8;
		input->blc_size[3][1]=16;
	}
	if (input->InterSearch16x16) {
		input->blc_size[4][0]= 8;
		input->blc_size[4][1]= 8;
	}
	if (input->InterSearch8x4) {
		input->blc_size[5][0]= 8;
		input->blc_size[5][1]= 4;
	}
	if (input->InterSearch4x8) {
		input->blc_size[6][0]= 4;
		input->blc_size[6][1]= 8;
	}
	if (input->InterSearch4x4) {
		input->blc_size[7][0]= 4;
		input->blc_size[7][1]= 4;
	}

	if (input->partition_mode < 0 || input->partition_mode > 2) {
		printf("Unsupported Partition mode, must be between 0 and 2\n");
		exit(-405);
	}

	if (input->of_mode < 0 || input->of_mode > 1) {
		printf("Unsupported Outpout file mode, must be between 0 and 1\n");
		exit(-405);
	}

	// B picture consistency check
	if(input->successive_Bframe > input->jumpd) 
	{
		printf("Number of B-frames %d can not exceed the number of frames skipped\n", input->successive_Bframe);
		exit (-406);
	}

	// Cabac/UVLC consistency check
	if (input->symbol_mode != UVLC && input->symbol_mode != CABAC)
	{
		printf ("Unsupported symbol mode=%d, use UVLC=0 or CABAC=1\n",input->symbol_mode);
		exit (-407);
	}
	
	

	//Open Files

	if ((p_in=fopen(input->infile,"rb"))==NULL)  
	{
		printf("Input file %s does not exist \n",input->infile);
		exit(0);
	}

	if (strlen (input->ReconFile) > 0 && (p_dec=fopen(input->ReconFile, "wb"))==NULL)
	{
		printf("Error open file test.dec \n");
		exit(0);
	}

	if (strlen (input->TraceFile) > 0 && (p_trace=fopen(input->TraceFile,"w"))==NULL)
	{
		printf("Error open file trace_enc.txt\n");
		exit(0);
	}
}






