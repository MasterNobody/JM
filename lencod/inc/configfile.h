#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_


#define DEFAULTCONFIGFILENAME "encoder.cfg"

typedef struct {
	char *TokenName;
	void *Place;
	int Type;
} Mapping;



InputParameters configinput;


#ifdef INCLUDED_BY_CONFIGFILE_C

Mapping Map[] = {
	{"FramesToBeEncoded",		&configinput.no_frames,			0},
	{"QPFirstFrame",			&configinput.qp0,				0},
	{"QPRemainingFrame",		&configinput.qpN,				0},
	{"FrameSkip",				&configinput.jumpd,				0},
	{"MVResolution",			&configinput.mv_res,			0},
	{"UseHadamard",				&configinput.hadamard,			0},
	{"SearchRange",				&configinput.search_range,		0},
	{"NumberRefereceFrames",	&configinput.no_multpred,			0},
	{"SourceWidth",				&configinput.img_width,			0},
	{"SourceHeight",			&configinput.img_height,			0},
	{"MbLineIntraUpdate",		&configinput.intra_upd,			0},
	{"SliceMode",				&configinput.slice_mode,			0},
	{"SliceArgument",			&configinput.slice_argument,		0},
	{"InputFile",				&configinput.infile,				1},
	{"OutputFile",				&configinput.outfile,				1},
	{"ReconFile",				&configinput.ReconFile,			1},
	{"TraceFile",				&configinput.TraceFile,			1},
	{"NumberBFrames",			&configinput.successive_Bframe,	0},
	{"QPBPicture",				&configinput.qpB,					0},
	{"SymbolMode",				&configinput.symbol_mode,			0},
	{"OutFileMode",				&configinput.of_mode,				0},
	{"PartitionMode",			&configinput.partition_mode,		0},
	{"SequenceHeaderType",		&configinput.SequenceHeaderType,	0},
	{"TRModulus",				&configinput.TRModulus,			0},
	{"PicIdModulus",			&configinput.PicIdModulus,			0},
	{"PictureTypeSequence",		&configinput.PictureTypeSequence,	1},
	{"InterSearch16x16",		&configinput.InterSearch16x16,		0},
	{"InterSearch16x8",			&configinput.InterSearch16x8 ,		0},
	{"InterSearch8x16",			&configinput.InterSearch8x16,		0},
	{"InterSearch8x8",			&configinput.InterSearch8x8 ,		0},
	{"InterSearch8x4",			&configinput.InterSearch8x4,		0},
	{"InterSearch4x8",			&configinput.InterSearch4x8,		0},
	{"InterSearch4x4",			&configinput.InterSearch4x4,		0},
#ifdef _FULL_SEARCH_RANGE_
	{"RestrictSearchRange",            &configinput.full_search,      0},
#endif
#ifdef _ADAPT_LAST_GROUP_
	{"LastFrameNumber",                &configinput.last_frame,       0},
#endif
#ifdef _CHANGE_QP_
	{"ChangeQPP",                      &configinput.qpN2,             0},
	{"ChangeQPB",                      &configinput.qpB2,             0},
	{"ChangeQPStart",                  &configinput.qp2start,         0},
#endif
#ifdef _RD_OPT_
	{"RDOptimization",                 &configinput.rdopt,            0},
#endif
#ifdef _ADDITIONAL_REFERENCE_FRAME_
	{"AdditionalReferenceFrame",	   &configinput.add_ref_frame,	  0},
#endif
	{NULL,						NULL,						-1}					
};

#endif

#ifndef INCLUDED_BY_CONFIGFILE_C
extern Mapping Map[];
#endif


void Configure (int ac, char *av[]);

#endif
