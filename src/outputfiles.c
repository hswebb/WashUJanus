/*******************************************************************************
*
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the
* software, documentation and results solely at his own risk.
******************************************************************************/

#include <stdio.h>
#include <inttypes.h>

#include "FERSlib.h"
#include "console.h"
#include "configure.h"
#include "JanusC.h"
#include "Statistics.h"


// ****************************************************
// Global Variables
// ****************************************************
FILE *of_raw_b = NULL, *of_raw_a = NULL;
FILE *of_list_b = NULL, *of_list_a = NULL, *of_list_c;
FILE *of_sync = NULL;
FILE* of_servInfo = NULL;
uint8_t fnumFVer = 0;
uint8_t snumFVer = 0;
uint8_t type_file = 0x0; // XXXX XCTS
uint8_t datatype = 0x0;	// XXTA XCHL	C=Counting T=ToT A=ToA (timestamp) H=HG L=LG - not use in Counting/Timimng mode alone for the moment
uint8_t fnumSW = 0;
uint8_t snumSW = 0;
uint8_t tnumSW = 0;
char dtq_mode_ch[6][15] = { " ", "Spectroscopy", "Timing_CStart", "Spect_Timing", "Counting", "Timing_CStop"};
static uint64_t bin_size = 0, ascii_size = 0, csv_size = 0;
static int bin_srun = 0, ascii_srun = 0, csv_srun = 0;
static int LocalRunNum = 0;

// ****************************************************
// Local functions
// ****************************************************
static uint16_t rebin_energy(uint16_t energy) {
	if		(WDcfg.EHistoNbin == 8192) return energy;
	else if (WDcfg.EHistoNbin == 4096) return (energy >> 1);
	else if (WDcfg.EHistoNbin == 2048) return (energy >> 2);
	else if (WDcfg.EHistoNbin == 1024) return (energy >> 3);
	else if (WDcfg.EHistoNbin == 512)  return (energy >> 4);
	else if (WDcfg.EHistoNbin == 256)  return (energy >> 5);
	else return energy;
}

// Type: 0=Binary, 1=CSV, 2=ascii/txt
static void CreateOutFileName(char *radix, int RunNumber, int type, char *filename) {
	if (RunNumber >= 0) {
		if (strcmp(radix, "list") == 0 && WDcfg.EnableMaxFileSize)
			sprintf(filename, "%sRun%d.%d_%s", WDcfg.DataFilePath, RunNumber, 0, radix);
		else
			sprintf(filename, "%sRun%d_%s", WDcfg.DataFilePath, RunNumber, radix);
	} else sprintf(filename, "%s%s", WDcfg.DataFilePath, radix);
	if (type == 0) strcat(filename, ".dat");
	else if (type == 1) strcat(filename, ".csv");
	else strcat(filename, ".txt");
}

// Type: 0=Binary, 1=CSV, 2=ASCII
static void IncreaseListSubRun(int type) {
	int srun;
	char filename[500];

	if (type == 0) {
		fclose(of_list_b);
		++bin_srun;
		srun = bin_srun;
		bin_size = 0;
	} else if (type == 1) {
		fclose(of_list_c);
		++csv_srun;
		srun = csv_srun;
		csv_size = 0;
	} else {
		fclose(of_list_a);
		++ascii_srun;
		srun = ascii_srun;
		ascii_size = 0;
	}
	if (LocalRunNum >= 0) sprintf(filename, "%sRun%d.%d_%s", WDcfg.DataFilePath, LocalRunNum, srun, "list");
	else sprintf(filename, "%s%s", WDcfg.DataFilePath, "list");
	if (type == 0) {
		strcat(filename, ".dat");
		of_list_b = fopen(filename, "wb");
	} else if (type == 1) {
		strcat(filename, ".csv");
		of_list_c = fopen(filename, "w");
	} else {
		strcat(filename, ".txt");
		of_list_a = fopen(filename, "w");
	}
}


// ****************************************************
// Open/Close Output Files
// ****************************************************
int OpenOutputFiles(int RunNumber)
{
	char filename[500];
	LocalRunNum = RunNumber;

	//if ((WDcfg.OutFileEnableMask & OUTFILE_RAW_DATA_BIN) && (of_raw_b == NULL)) { 
	//	CreateOutFileName("raw_data", RunNumber, 1, filename);
	//	of_raw_b = fopen(filename, "wb");
	//}
	//if ((WDcfg.OutFileEnableMask & OUTFILE_RAW_DATA_ASCII) && (of_raw_a == NULL)) {
	//	CreateOutFileName("raw_data", RunNumber, 0, filename);
	//	of_raw_a = fopen(filename, "w");
	//}
	if ((WDcfg.OutFileEnableMask & OUTFILE_LIST_BIN) && (of_list_b == NULL)) {
		CreateOutFileName("list", RunNumber, 0, filename);
		of_list_b = fopen(filename, "wb");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_LIST_ASCII) && (of_list_a == NULL)) {
		CreateOutFileName("list", RunNumber, 2, filename);
		of_list_a = fopen(filename, "w");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_LIST_CSV) && (of_list_c == NULL)) {
		CreateOutFileName("list", RunNumber, 1, filename);
		of_list_c = fopen(filename, "w");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_SYNC) && (of_sync == NULL)) {
		CreateOutFileName("sync", RunNumber, 2, filename);
		of_sync = fopen(filename, "w");
	}
	if ((WDcfg.OutFileEnableMask & OUTFILE_SERVICE_INFO) && (of_servInfo == NULL)) {
		CreateOutFileName("ServiceInfo", RunNumber, 2, filename);
		of_servInfo = fopen(filename, "w");
	}
	return 0;
}

int CloseOutputFiles()
{
	// char filename[500];
	if (of_raw_b != NULL) fclose(of_raw_b);
	if (of_raw_a != NULL) fclose(of_raw_a);
	if (of_list_b != NULL) fclose(of_list_b);
	if (of_list_a != NULL) fclose(of_list_a);
	if (of_list_c != NULL) fclose(of_list_c);
	if (of_sync != NULL) fclose(of_sync);
	if (of_servInfo != NULL) fclose(of_servInfo);
	of_raw_b = NULL;
	of_raw_a = NULL;
	of_list_c = NULL;
	of_list_b = NULL;
	of_list_a = NULL;
	of_servInfo = NULL;
	of_sync = NULL;
	bin_size = 0;
	bin_srun = 0;
	ascii_size = 0;
	ascii_srun = 0;
	csv_size = 0;
	csv_srun = 0;
	return 0;
}

// ****************************************************
// Save Raw data and Lists to Output Files
// ****************************************************

// CTIN: add check on file size and stop saving when the programmed limit is reached

int SaveRawData(uint32_t *buff, int nw)
{
	int i;
	if (of_raw_b != NULL) {
		fwrite(buff, sizeof(uint32_t), nw, of_raw_b);
	}
	if (of_raw_a != NULL) {
		for(i=0; i<nw; i++)
			fprintf(of_raw_a, "%3d %08X\n", i, buff[i]);
		fprintf(of_raw_a, "----------------------------------\n");
	}
	return 0;
}

// ****************************************************
// Write Header of List Output Files
// ****************************************************
int WriteListfileHeader() {
	// Get software and data file version	
	sscanf(SW_RELEASE_NUM, "%" SCNu8 ".%" SCNu8 ".%" SCNu8, &fnumSW, &snumSW, &tnumSW);
	sscanf(FILE_LIST_VER, "%" SCNu8 ".%" SCNu8, &fnumFVer, &snumFVer);
	uint16_t brdVer = 0;
#ifdef FERS_5203	
	//sscanf("52.03", "%" SCNu8 ".%" SCNu8, &fbrdVer, &sbrdVer);
	sscanf("5203", "%" SCNu16, &brdVer);
#else
	sscanf("5202", "%" SCNu16, &brdVer);
#endif	

	int16_t rn = (int16_t)RunVars.RunNumber;
	// Write headers, common for all the list files
	type_file = WDcfg.AcquisitionMode & 0x0F | (WDcfg.Enable_2nd_tstamp<<7);  // dtq & 0x0F;
	if (of_list_b != NULL) {   // Binary ASCII
		float tmpLSB = float(TOA_LSB_ns);
		uint16_t enbin = WDcfg.EHistoNbin;
		//uint32_t tmask = WDcfg.ChEnableMask1[brd];	// see below
		uint8_t header_size = 
			sizeof(header_size) + 5 * sizeof(fnumFVer) + sizeof(brdVer) + sizeof(rn) + sizeof(type_file) +
			sizeof(enbin) + sizeof(WDcfg.OutFileUnit) + sizeof(tmpLSB) + sizeof(Stats.start_time);

		//fwrite(&header_size, sizeof(header_size), 1, of_list_b);
		fwrite(&fnumFVer, sizeof(fnumFVer), 1, of_list_b);
		fwrite(&snumFVer, sizeof(snumFVer), 1, of_list_b);
		fwrite(&fnumSW, sizeof(fnumSW), 1, of_list_b);
		fwrite(&snumSW, sizeof(snumSW), 1, of_list_b);
		fwrite(&tnumSW, sizeof(tnumSW), 1, of_list_b);
		fwrite(&brdVer, sizeof(brdVer), 1, of_list_b);	// next File Format
		fwrite(&rn, sizeof(rn), 1, of_list_b);
		fwrite(&type_file, sizeof(type_file), 1, of_list_b); // Acquisition Mode
		fwrite(&enbin, sizeof(enbin), 1, of_list_b);
		fwrite(&WDcfg.OutFileUnit, sizeof(WDcfg.OutFileUnit), 1, of_list_b);	// Type of unit used for Time. 0 LSB, 1 ns
		fwrite(&tmpLSB, sizeof(tmpLSB), 1, of_list_b);	// Keep it as float for homogenity with A5203, the value of the LSB of which is not fixed
		//fwrite(&tmask, sizeof(tmask), 1, of_list_b);	// uncomment if we want the Channel Mask
		fwrite(&Stats.start_time, sizeof(Stats.start_time), 1, of_list_b);
		bin_size += header_size;
	}
	char unit[10];
	if (WDcfg.OutFileUnit) strcpy(unit, "ns");
	else strcpy(unit, "LSB");

	char mytime[100];
	//strcpy(mytime, ctime(&Stats.time_of_start));
	strcpy(mytime, asctime(gmtime(&Stats.time_of_start)));
	mytime[strlen(mytime) - 1] = 0;

	if (of_list_a != NULL) {  //  ASCII header
		int t_file = (WDcfg.AcquisitionMode&0xF0)? 5: type_file & 0x0F;
		fprintf(of_list_a, "//************************************************\n");
		fprintf(of_list_a, "// File Format Version %s\n", FILE_LIST_VER);
		//fprintf(of_list_a, "// Janus _%" PRIu16 " Release % s\n", brdVer, SW_RELEASE_NUM);   // For next File Format
		fprintf(of_list_a, "// Janus Release %s\n", SW_RELEASE_NUM);
		fprintf(of_list_a, "// Acquisition Mode: %s\n", dtq_mode_ch[t_file]);
		fprintf(of_list_a, "// Energy Histogram Channels: %d\n", WDcfg.EHistoNbin);
		fprintf(of_list_a, "// ToA/ToT LSB: %.1f ns\n", TOA_LSB_ns);
		char mytime[100];
		//strcpy(mytime, ctime(&Stats.time_of_start));
		strcpy(mytime, asctime(gmtime(&Stats.time_of_start)));
		mytime[strlen(mytime) - 1] = 0;
		//fprintf(of_list_a, "// Run%d start time: %s UTC\n", rn, mytime);	// For next File Format
		fprintf(of_list_a, "// Run start time: %s UTC\n", mytime);
		fprintf(of_list_a, "//************************************************\n");

		int dtqh = WDcfg.AcquisitionMode & 0x0F;
		int en2ts = (WDcfg.Enable_2nd_tstamp & 1);
		if (dtqh == DTQ_SPECT) {
			if (en2ts) fprintf(of_list_a, "Brd  Ch       LG       HG        Tstamp_us       Tstamp2_us        TrgID			NHits\n");
			else fprintf(of_list_a, "Brd  Ch       LG       HG        Tstamp_us        TrgID		NHits\n");
		} else if (dtqh == DTQ_TSPECT) {
			if (WDcfg.EnableToT) fprintf(of_list_a, "Brd  Ch       LG       HG  ToA_%-3s  ToT_%-3s", unit, unit);
			else fprintf(of_list_a, "Brd  Ch       LG       HG  ToA_%-3s", unit);
			if (en2ts) fprintf(of_list_a, "        Tstamp_us       Tstamp2_us        TrgID			NHits\n");
			else fprintf(of_list_a, "        Tstamp_us        TrgID			NHits\n");
		} else if (dtqh == DTQ_TIMING) {
			if (WDcfg.EnableToT) fprintf(of_list_a, "Brd  Ch  ToA_%-3s  ToT_%-3s", unit, unit);
			else fprintf(of_list_a, "Brd  Ch  ToA_%-3s", unit);
			fprintf(of_list_a, "        Tstamp_us		NHits\n");
		} else if (dtqh == DTQ_COUNT) {
			if (en2ts) fprintf(of_list_a, "Brd  Ch          Cnt       Tstamp_us       Tstamp2_us        TrgID		NHits\n");
			else fprintf(of_list_a, "Brd  Ch          Cnt       Tstamp_us        TrgID		NHits\n");
		}

		ascii_size = ftell(of_list_a);
	}
	if (of_list_c != NULL) {  // CSV Header
		// Write Header
		int t_file = (WDcfg.AcquisitionMode & 0xF0) ? 5 : type_file & 0x0F;
		fprintf(of_list_c, "//************************************************\n");
		fprintf(of_list_c, "//Board:5202\n//File_Format_Version:%s\n//Janus_Release:%s\n", FILE_LIST_VER, SW_RELEASE_NUM);
		fprintf(of_list_c, "//Acquisition_Mode:%s\n", dtq_mode_ch[t_file]);
		if (type_file & ACQMODE_SPECT) // Spect or TSpect mode
			fprintf(of_list_c, "//Energy_Histo_NBins:%d\n", WDcfg.EHistoNbin);
		if ((type_file & ACQMODE_TIMING_CSTART) & 0xF) { // Timing or TSpect
			fprintf(of_list_c, "//Time_LSB_Value_ns:0.5\n");
			fprintf(of_list_c, "//Time_Unit:%s\n", unit);
		}
		fprintf(of_list_c, "//Run#:%d\n", rn);
		fprintf(of_list_c, "//Start_Time_Epoch:%" PRId64 "\n", Stats.start_time);
		fprintf(of_list_c, "//Start_Time_DateTime_UTC:%s\n", mytime);
		fprintf(of_list_c, "//************************************************\n");
		// Write parname header
		fprintf(of_list_c, "TStamp_us,");
		if ((type_file & 0x80) && ((type_file & 0xF) != ACQMODE_TIMING_CSTART)) fprintf(of_list_c, "Rel_TStamp_us,");
		if (!(type_file & ACQMODE_TIMING_CSTART)) fprintf(of_list_c, "Trg_Id,");
		fprintf(of_list_c, "Board_Id,Num_Hits,");
		if ((type_file & ACQMODE_SPECT)) {
			fprintf(of_list_c, "ChannelMask,Ch_Id,DataType,PHA_LG,PHA_HG");
			if (type_file & ACQMODE_TIMING_CSTART) {
				fprintf(of_list_c, ",ToA_%s", unit);
				if (WDcfg.EnableToT) fprintf(of_list_c, ",ToT_%s", unit);
			} 
			fprintf(of_list_c, "\n");
		}
		if ((type_file & 0xF) == ACQMODE_TIMING_CSTART) {
			fprintf(of_list_c, "Ch_Id,DataType,ToA_%s", unit);
			if (WDcfg.EnableToT) fprintf(of_list_c, ",ToT_%s", unit);
			fprintf(of_list_c, "\n");
		}
		if ((type_file & 0xF) == ACQMODE_COUNT) {
			fprintf(of_list_c, "ChannelMask,Ch_Id,Counts\n");
		}
		csv_size = ftell(of_list_c);
	}
	if (of_servInfo != NULL) {
		fprintf(of_servInfo, "TimeStamp\t");
		for (int j = 0; j < WDcfg.NumBrd; ++j) {
			fprintf(of_servInfo, "\tBrd\t\tBrdTemp\t\tDetTemp\t\tFPGATemp\tHVTemp\t\tVmon\t\tImon\tHVstatus");
		}
		fprintf(of_servInfo, "\n");
	}
	if (of_sync != NULL)
		fprintf(of_sync, "Brd    Tstamp_us      TrgID \n");

	return 0;
}

// ****************************************************
// Save List
// ****************************************************
int SaveList(int brd, double ts, uint64_t trgid, void *generic_ev, int dtq)
{
	if (bin_size > WDcfg.MaxOutFileSize && WDcfg.EnableMaxFileSize)
		IncreaseListSubRun(0);
	if (ascii_size > WDcfg.MaxOutFileSize && WDcfg.EnableMaxFileSize)
		IncreaseListSubRun(2);
	if (csv_size > WDcfg.MaxOutFileSize && WDcfg.EnableMaxFileSize)
		IncreaseListSubRun(1);

	// ----------------------------------------------------------------------------------
	// SPECT/SPECT_TIMING MODE
	// ----------------------------------------------------------------------------------
	if (dtq & DTQ_SPECT) {
		SpectEvent_t *ev = (SpectEvent_t *)generic_ev;
		int num_of_hits=0, masked[FERSLIB_MAX_NCH] = { 0 };
		int isTSpect = (dtq & 0x2) >> 1;

		// Re-Binning
		// In this case we are re-binning, so, dividing by pow of 2...
		// or shifting the ev->energy >> X
		uint8_t data_t[MAX_NCH] = {};
		uint16_t tmp_enL[MAX_NCH] = {};
		uint16_t tmp_enH[MAX_NCH] = {};

		//char z[2], x[2], q[2];
		uint8_t i, b8 = brd;

		for (i = 0; i < MAX_NCH; ++i) {
			masked[i] = 0;
			tmp_enL[i] = rebin_energy(ev->energyLG[i]);
			tmp_enH[i] = rebin_energy(ev->energyHG[i]);
			if (((ev->chmask >> i) & 1) && (tmp_enL[i] >= 0 || tmp_enH[i] >= 0)) {
				++num_of_hits;
				masked[i] = 1;
			}
		}

		if (of_list_b != NULL && (WDcfg.OutFileEnableMask && OUTFILE_LIST_BIN)) {
			uint16_t size = sizeof(size) + sizeof(b8) + sizeof(ts) + sizeof(trgid) + sizeof(ev->chmask);
			if (dtq & 0x80) size += sizeof(ev->rel_tstamp_us);
			for (i = 0; i < MAX_NCH; i++) {	// DNIN: Is it somehow usefull keeping the condition temp_enL/H >= 0??
				datatype = 0;
				if ((ev->chmask >> i) & 1) size += (sizeof(i) + sizeof(datatype));
				else continue;
				if (tmp_enL[i] >= 0 && ((WDcfg.GainSelect & GAIN_SEL_LOW) || WDcfg.GainSelect == GAIN_SEL_AUTO)) {
					datatype = datatype | 0x01;
					size += sizeof(ev->energyLG[i]);
				}
				if (tmp_enH[i] >= 0 && ((WDcfg.GainSelect & GAIN_SEL_HIGH) || WDcfg.GainSelect == GAIN_SEL_AUTO)) {
					datatype = datatype | 0x02;
					size += sizeof(ev->energyHG[i]);
				}
				if (isTSpect) {
					if (ev->tstamp[i] > 0) {
						datatype = datatype | 0x10;
						if (WDcfg.OutFileUnit) size += sizeof(float);
						else size += sizeof(ev->tstamp[i]);	
					}
					if (WDcfg.EnableToT && (ev->ToT[i] > 0)) {
						datatype = datatype | 0x20;
						if (WDcfg.OutFileUnit) size += sizeof(float);
						else size += sizeof(ev->ToT[i]);
					}
				}
				data_t[i] = datatype;
			}
			bin_size += size;
			fwrite(&size, sizeof(size), 1, of_list_b);
			fwrite(&b8, sizeof(b8), 1, of_list_b);
			fwrite(&ts, sizeof(ts), 1, of_list_b);
			if (dtq & 0x80)	fwrite(&ev->rel_tstamp_us, sizeof(ev->rel_tstamp_us), 1, of_list_b);
			fwrite(&trgid, sizeof(trgid), 1, of_list_b);
			fwrite(&ev->chmask, sizeof(ev->chmask), 1, of_list_b);
			for(i=0; i<MAX_NCH; i++) {
				if ((ev->chmask >> i) & 1) {
					uint8_t tmp_type = data_t[i];
					uint16_t tmp_nrgL = tmp_enL[i];
					uint16_t tmp_nrgH = tmp_enH[i];
					float tmpToA = float(ev->tstamp[i] * TOA_LSB_ns);
					float tmpToT = (tmpToA == 0) ? 0 : float(ev->ToT[i] * TOA_LSB_ns);  // don't write ToT if there is no ToA
					fwrite(&i, sizeof(i), 1, of_list_b);	// Channel	
					fwrite(&tmp_type, sizeof(tmp_type), 1, of_list_b);
					if (data_t[i] & 0x01) fwrite(&tmp_nrgL, sizeof(ev->energyLG[i]), 1, of_list_b);
					if (data_t[i] & 0x02) fwrite(&tmp_nrgH, sizeof(ev->energyHG[i]), 1, of_list_b);
					if (data_t[i] & 0x10) {	
						if (WDcfg.OutFileUnit) fwrite(&tmpToA, sizeof(float), 1, of_list_b);
						else fwrite(&ev->tstamp[i], sizeof(ev->tstamp[i]), 1, of_list_b);
					}
					if (data_t[i] & 0x20) {
						if (WDcfg.OutFileUnit) fwrite(&tmpToT, sizeof(float), 1, of_list_b);
						else fwrite(&ev->ToT[i], sizeof(ev->ToT[i]), 1, of_list_b);
					}
				}
			}
		}
		if (of_list_a != NULL) { 
			int evg = 1;
			for(i=0; i<MAX_NCH; i++) {
				if ((ev->chmask >> i) & 1) {
					fprintf(of_list_a, "%3d  %02d ", brd, i);
					if (tmp_enL[i] >= 0 && ((WDcfg.GainSelect & GAIN_SEL_LOW) || WDcfg.GainSelect == GAIN_SEL_AUTO)) fprintf(of_list_a, "%8d ", tmp_enL[i]);
					else fprintf(of_list_a, "       - "); 
					if (tmp_enH[i] >= 0 && ((WDcfg.GainSelect & GAIN_SEL_HIGH) || WDcfg.GainSelect == GAIN_SEL_AUTO)) fprintf(of_list_a, "%8d ", tmp_enH[i]);
					else fprintf(of_list_a, "       - "); 
					if (isTSpect) {
						if (ev->tstamp[i] > 0) {
							if (WDcfg.OutFileUnit) fprintf(of_list_a, "%8.1f ", 0.5 * ev->tstamp[i]);
							else fprintf(of_list_a, "%8d ", ev->tstamp[i]);
						} else fprintf(of_list_a, "       - ");
						if (WDcfg.EnableToT && (ev->ToT[i] > 0) && (ev->tstamp[i] > 0)) {  // Don't write ToT if there is no ToA
							if (WDcfg.OutFileUnit) fprintf(of_list_a, "%8.1f ", 0.5 * ev->ToT[i]);
							else fprintf(of_list_a, "%8d ", ev->ToT[i]);
						} else fprintf(of_list_a, "       - ");
					}
					if (evg) {
						if (dtq & 0x80) fprintf(of_list_a, "%16.3lf %16.3f %12" PRIu64 "\t\t%d", ts, ev->rel_tstamp_us, trgid, num_of_hits);
						else fprintf(of_list_a, "%16.3lf %12" PRIu64 "\t\t%d", ts, trgid, num_of_hits);
					}
					evg = 0;
					fprintf(of_list_a, "\n");
				}
				fflush(of_list_a);
			}
			ascii_size = ftell(of_list_a);
		}
		if (of_list_c != NULL) {
			for (int j = 0; j < MAX_NCH; ++j) {
				if (!masked[j]) continue;
				datatype = 0;
				if (tmp_enL[i] >= 0 && ((WDcfg.GainSelect & GAIN_SEL_LOW) || WDcfg.GainSelect == GAIN_SEL_AUTO)) datatype = datatype | 0x01;
				if (tmp_enH[i] >= 0 && ((WDcfg.GainSelect & GAIN_SEL_HIGH) || WDcfg.GainSelect == GAIN_SEL_AUTO)) datatype = datatype | 0x02;
				if (isTSpect) {
					if (ev->tstamp[i] > 0) datatype = datatype | 0x10;
					if (WDcfg.EnableToT && (ev->ToT[i] > 0)) datatype = datatype | 0x20;
				}
				fprintf(of_list_c, "%lf,", ts);
				if (dtq & 0x80) fprintf(of_list_c, "%lf,", ev->rel_tstamp_us);
				fprintf(of_list_c, "%" PRIu64 ",%d,%d,0x%" PRIx64 ",%d,0x%" PRIx8 ",", trgid, brd, num_of_hits, ev->chmask, j, datatype);
				if (datatype & 0x1) fprintf(of_list_c, "%" PRIu16, tmp_enL[j]);
				else fprintf(of_list_c, "-1");
				if (datatype & 0x2) fprintf(of_list_c, ",%" PRIu16, tmp_enH[j]);
				else fprintf(of_list_c, ",-1");
				if (isTSpect) {
					if (datatype & 0x10) {
						if (WDcfg.OutFileUnit) fprintf(of_list_c, ",%f", 0.5*ev->tstamp[j]);
						else fprintf(of_list_c, ",%" PRIu32, ev->tstamp[j]);
					} else fprintf(of_list_c, ",-1");
					if (datatype & 0x20) {
						if (WDcfg.OutFileUnit) fprintf(of_list_c, ",%f", 0.5 * ev->ToT[j]);
						else fprintf(of_list_c, ",%" PRIu16, ev->ToT[j]);
					} else fprintf(of_list_c, ",-1");
				}
				fprintf(of_list_c, "\n");
			}
			csv_size = ftell(of_list_c);
		}
	}

	// ----------------------------------------------------------------------------------
	// COUNTING MODE
	// ----------------------------------------------------------------------------------
	else if ((dtq & 0x0F) == DTQ_COUNT) {
		CountingEvent_t *ev = (CountingEvent_t *)generic_ev;
		uint8_t i, b8 = brd;
		
		datatype = 0x0;
		int num_of_hits = 0;
		int masked[MAX_NCH] = { 0 };

		uint64_t mask_chip1 = WDcfg.ChEnableMask1[brd];
		ev->chmask = mask_chip1 << 32 | WDcfg.ChEnableMask0[brd];
		
		for (i = 0; i < MAX_NCH; ++i) {
			if ((ev->chmask >> i) & 1) {
				if (WDcfg.SupprZeroCntListFile == 1 && ev->counts[i] == 0)
					continue;
				else {
					++num_of_hits;
					masked[i] = 1;
				}
			}
		}

		if (of_list_b != NULL) {
			uint16_t size = sizeof(size) + sizeof(b8) + sizeof(ts) + sizeof(trgid) + sizeof(ev->chmask);
			if (dtq & 0x80) size += sizeof(ev->rel_tstamp_us);
			for (i = 0; i < MAX_NCH; i++) {
				if ((ev->chmask >> i) & 1) {
					if (WDcfg.SupprZeroCntListFile == 1 && ev->counts[i] == 0)
						continue;
					else
						size += (sizeof(i) + sizeof(ev->counts[i]));
				}
			}
			bin_size += size;
			fwrite(&size, sizeof(size), 1, of_list_b);
			fwrite(&b8, sizeof(b8), 1, of_list_b);
			fwrite(&ts, sizeof(ts), 1, of_list_b);
			if (dtq & 0x80)	fwrite(&ev->rel_tstamp_us, sizeof(ev->rel_tstamp_us), 1, of_list_b);
			fwrite(&trgid, sizeof(trgid), 1, of_list_b);
			fwrite(&ev->chmask, sizeof(ev->chmask), 1, of_list_b);
			for(i=0; i<MAX_NCH; i++) {
				if ((ev->chmask >> i) & 1) {
					if (WDcfg.SupprZeroCntListFile == 1 && ev->counts[i] == 0)
						continue;
					else {
						fwrite(&i, sizeof(i), 1, of_list_b);
						fwrite(&ev->counts[i], sizeof(ev->counts[i]), 1, of_list_b);
					}
				}
			}
		}
		if (of_list_a != NULL) {
			int evg = 1, cnt_zero = 0;
			for(i=0; i<MAX_NCH; i++) {
				if ((ev->chmask >> i) & 1) {
					if (WDcfg.SupprZeroCntListFile == 1) {
						if (ev->counts[i] > 0) {
							if (evg == 0 && cnt_zero != 0) fprintf(of_list_a, "                              ");
							cnt_zero++;
							fprintf(of_list_a, "%3d  %02d %12d", brd, i, ev->counts[i]);
						}
						if ((cnt_zero == 0) && (i == MAX_NCH - 1)) {
							fprintf(of_list_a, "%03d  --          --", brd);
						}
					} else {
						fprintf(of_list_a, "%3d  %02d %12d", brd, i, ev->counts[i]);
					}
					if (evg) {
						if (dtq & 0x80) fprintf(of_list_a, "%16.3lf %16.3f %12" PRIu64 "\t\t%d", ts, ev->rel_tstamp_us, trgid, num_of_hits);
						else fprintf(of_list_a, "%16.3lf %12" PRIu64 "\t\t%d", ts, trgid, num_of_hits);
					}
					evg = 0;
					fprintf(of_list_a, "\n");
				}
			}
			ascii_size = ftell(of_list_a);
		}
		if (of_list_c != NULL) {
			for (i = 0; i < MAX_NCH; ++i) {
				if (!masked[i]) continue;
				fprintf(of_list_c, "%lf,", ts);
				if (dtq & 0x80) fprintf(of_list_c, "%lf,", ev->rel_tstamp_us);
				fprintf(of_list_c, "%" PRIu64 ",%d,%d,0x%" PRIx64 ",%d,%" PRIu32"\n", trgid, brd, num_of_hits, ev->chmask, i, ev->counts[i]);
			}
			csv_size = ftell(of_list_c);
		}
	}

	// ----------------------------------------------------------------------------------
	// TIMING MODE
	// ----------------------------------------------------------------------------------
	else if ((dtq & 0x0F) == DTQ_TIMING) {
		ListEvent_t *ev = (ListEvent_t *)generic_ev;
		if (ev->nhits <= 0)	return 0;
		datatype = 0x0;
		uint32_t i;
		uint8_t b8 = brd;
		
		if (of_list_b != NULL && (WDcfg.OutFileEnableMask && OUTFILE_LIST_BIN)) {
			uint8_t* mydtype = NULL;
			mydtype = (uint8_t*)malloc(ev->nhits * sizeof(uint8_t));
			uint16_t size = sizeof(size) + sizeof(b8) + sizeof(ts) + sizeof(ev->nhits); // +sizeof(trgid); //trgid = 0 in timing mode
			// DNIN: as in Spect, tstamp and ToT may not be in data simultaneously, or simply ToT is disabled.
			// Size must to be computed hit-wise
			size += ev->nhits * (sizeof(ev->channel[i]) + sizeof(datatype));
			for (int chit = 0; chit < ev->nhits; ++chit) {
				datatype = 0x0;
				if (ev->tstamp[chit] > 0) {
					datatype = datatype | 0x10;
					if (WDcfg.OutFileUnit) size += sizeof(float);
					else size += sizeof(ev->tstamp[chit]);
				}
				if (ev->ToT[chit] > 0 && WDcfg.EnableToT) {
					datatype = datatype | 0x20;
					if (WDcfg.OutFileUnit) size += sizeof(float);
					else size += sizeof(ev->ToT[chit]);
				}
				mydtype[chit] = datatype;
			}

			bin_size += size;
			fwrite(&size, sizeof(size), 1, of_list_b);
			fwrite(&b8, sizeof(b8), 1, of_list_b);
			fwrite(&ts, sizeof(ts), 1, of_list_b);
			//fwrite(&trgid, sizeof(trgid), 1, of_list_b);
			fwrite(&ev->nhits, sizeof(ev->nhits), 1, of_list_b);
			for(i=0; i<ev->nhits; i++) {
				float tmpToA = float(ev->tstamp[i] * TOA_LSB_ns);
				float tmpToT = float(ev->ToT[i] * TOA_LSB_ns);
				datatype = mydtype[i];
				fwrite(&ev->channel[i], 1, sizeof(ev->channel[i]), of_list_b);
				// Write Datatype as specttiming
				fwrite(&datatype, 1, sizeof(datatype), of_list_b);
				if (datatype & 0x10) {
					if (WDcfg.OutFileUnit) fwrite(&tmpToA, sizeof(float), 1, of_list_b);
					else fwrite(&ev->tstamp[i], sizeof(ev->tstamp[i]), 1, of_list_b);
				}
				if (datatype & 0x20) {
					if (WDcfg.OutFileUnit) fwrite(&tmpToT, sizeof(float), 1, of_list_b);
					else fwrite(&ev->ToT[i], sizeof(ev->ToT[i]), 1, of_list_b);
				}
			}
			free(mydtype);	// Deallocating memory
			mydtype = NULL;
		}

		if (of_list_a != NULL) {
			int evg = 1;
			for(i=0; i<ev->nhits; i++) {
				fprintf(of_list_a, "%3d  %02d ", brd, ev->channel[i]);
				if (ev->tstamp[i] > 0) {
					if (WDcfg.AcquisitionMode == ACQMODE_TIMING_CSTART) {
						if (WDcfg.OutFileUnit) fprintf(of_list_a, "%8.1f ", 0.5 * ev->tstamp[i]);
						else fprintf(of_list_a, "%8d ", ev->tstamp[i]);
					} else if (WDcfg.AcquisitionMode == ACQMODE_TIMING_CSTOP) {
						if (WDcfg.OutFileUnit) fprintf(of_list_a, "%8.1f ", (WDcfg.TrefWindow - 0.5 * ev->tstamp[i]));
						else fprintf(of_list_a, "%8d ", (uint32_t)(WDcfg.TrefWindow / (float)CLK_PERIOD) - ev->tstamp[i]);
					}
				}
				else fprintf(of_list_a, "       - ");
				if (WDcfg.EnableToT && (ev->ToT[i] > 0)) {
					if (WDcfg.OutFileUnit) fprintf(of_list_a, "%8.1f ", 0.5 * ev->ToT[i]);
					else fprintf(of_list_a, "%8d ", ev->ToT[i]);
				}
				else fprintf(of_list_a, "       - ");
				if (evg) fprintf(of_list_a, "%16.3lf\t\t%" PRIu16, ts, ev->nhits);  
				evg = 0;
				fprintf(of_list_a, "\n");
			}
			ascii_size = ftell(of_list_a);
		}
		if (of_list_c != NULL) {
			for (int chit = 0; chit < ev->nhits; ++chit) {
				datatype = 0x0;
				if (ev->tstamp[chit] > 0)					datatype = datatype | 0x10;
				if (ev->ToT[chit] > 0 && WDcfg.EnableToT)	datatype = datatype | 0x20;
				fprintf(of_list_c, "%lf,%d,%" PRIu16 ",%" PRIu32 ",0x%" PRIx8 ",", ts, brd, ev->nhits, ev->channel[chit], datatype);
				if (datatype == 0x10) {
					if (WDcfg.AcquisitionMode == ACQMODE_TIMING_CSTART) {
						if (WDcfg.OutFileUnit) fprintf(of_list_c, "%f", 0.5 * ev->tstamp[chit]);
						else fprintf(of_list_c, "%" PRIu32, ev->tstamp[chit]);
					} else if (WDcfg.AcquisitionMode == ACQMODE_TIMING_CSTOP) {
						if (WDcfg.OutFileUnit) fprintf(of_list_c, "%f", (WDcfg.TrefWindow - 0.5 * ev->tstamp[chit]));
						else fprintf(of_list_c, "%" PRIu32, (uint32_t)(WDcfg.TrefWindow / (float)CLK_PERIOD) - ev->tstamp[chit]);
					}
				} else fprintf(of_list_c, "-1");
				if (datatype == 0x20) {
					if (WDcfg.OutFileUnit) fprintf(of_list_c, ",%f", 0.5 * ev->ToT[chit]);
					else fprintf(of_list_c, ",%" PRIu16, ev->ToT[chit]);
				} else fprintf(of_list_c, ",-1");
				fprintf(of_list_c, "\n");
			}
			csv_size = ftell(of_list_c);
		}
	}

	if (of_sync != NULL) { 
		fprintf(of_sync, "%3d %12.3lf %10" PRIu64 "\n", brd, ts, trgid);
	}
	return 0;
}


// ****************************************************
// Save Run Temp HV list
// ****************************************************
int WriteTempHV(ServEvent_t sev[FERSLIB_MAX_NBRD]) {
	if (of_servInfo == NULL) return 0;
	fprintf(of_servInfo, "[%" PRIu64 "]", sev[0].update_time);
	for (int i = 0; i < WDcfg.NumBrd; ++i) {
		int hv_status = sev[i].hv_status_on | sev[i].hv_status_ovc << 1 | sev[i].hv_status_ovv << 1;
		fprintf(of_servInfo, "\t%d\t\t%2.2f\t\t\t%2.2f\t\t%2.2f\t\t%2.2f\t\t%3.2f\t\t%3.2f\t\t%d",i, 
			sev[i].tempBoard, sev[i].tempDetector, sev[i].tempFPGA, sev[i].tempHV, sev[i].hv_Vmon, sev[i].hv_Imon, hv_status);
	}
	fprintf(of_servInfo, "\n");
	return 0;
}


// ****************************************************
// Save Histograms
// ****************************************************
int SaveHistos()
{
	int ch, b, i;
	char fname[500];
	FILE *hf;
	for(b=0; b<WDcfg.NumBrd; b++) {
		for(ch=0; ch<WDcfg.NumCh; ch++) {
			if (WDcfg.OutFileEnableMask & OUTFILE_SPECT_HISTO) {
				if (Stats.H1_PHA_LG[b][ch].H_cnt > 0) {
					sprintf(fname, "%sRun%d_PHA_LG_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						for (i=0; i<(int)Stats.H1_PHA_LG[b][ch].Nbin; i++)	// equivalent maybe better WDcfg.EHistoNbin
							fprintf(hf, "%d\n", Stats.H1_PHA_LG[b][ch].H_data[i]);
						fclose(hf);
					}
				}
				if (Stats.H1_PHA_HG[b][ch].H_cnt > 0) {
					sprintf(fname, "%sRun%d_PHA_HG_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						for (i=0; i<(int)Stats.H1_PHA_HG[b][ch].Nbin; i++)
							fprintf(hf, "%d\n", Stats.H1_PHA_HG[b][ch].H_data[i]);
						fclose(hf);
					}
				}
			}
			if (WDcfg.OutFileEnableMask & OUTFILE_ToA_HISTO) {
				if (Stats.H1_ToA[b][ch].H_cnt > 0) {
					sprintf(fname, "%sRun%d_ToA_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						for (i=0; i<(int)Stats.H1_ToA[b][ch].Nbin; i++)	// WDcfg.ToTHistoNbin
							fprintf(hf, "%" PRId32 "\n", Stats.H1_ToA[b][ch].H_data[i]);
						fclose(hf);
					}
				}
			}
			if (WDcfg.OutFileEnableMask & OUTFILE_TOT_HISTO) {
				if (Stats.H1_ToT[b][ch].H_cnt > 0) {
					sprintf(fname, "%sRun%d_ToT_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						for (i=0; i<(int)Stats.H1_ToT[b][ch].Nbin; i++)
							fprintf(hf, "%" PRId32 "\n", Stats.H1_ToT[b][ch].H_data[i]);
						fclose(hf);
					}
				}
			}
			if ((WDcfg.OutFileEnableMask & OUTFILE_MCS_HISTO) ) { // && (WDcfg.AcquisitionMode == ACQMODE_COUNT)) {
				sprintf(fname, "%sRun%d_MCS_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
				hf = fopen(fname, "w");
				if (hf != NULL) {
					for (i = 0; i < (int)Stats.H1_MCS[b][ch].Nbin; ++i) {
						const int ind = (i + Stats.H1_MCS[b][ch].Bin_set) % (WDcfg.MCSHistoNbin);
						fprintf(hf, "%" PRId32 "\n", Stats.H1_MCS[b][ch].H_data[ind]);
					}
					fclose(hf);
				}
			}
			if ((WDcfg.OutFileEnableMask & OUTFILE_STAIRCASE) && (RunVars.StaircaseCfg[SCPARAM_STEP] > 0)) {
				int nstep = (RunVars.StaircaseCfg[SCPARAM_MAX] - RunVars.StaircaseCfg[SCPARAM_MIN])/RunVars.StaircaseCfg[SCPARAM_STEP] + 1;
				for(i = 0; i < nstep; i++) 
					if (Stats.Staircase[b][ch][i] > 0) break; // check if the staircase is empty (all zeroes)
				if (i < nstep) { // not empty
					sprintf(fname, "%sRun%d_Staircase_%d_%d.txt", WDcfg.DataFilePath, RunVars.RunNumber, b, ch);
					hf = fopen(fname, "w");
					if (hf != NULL) {
						for(i = 0; i < nstep; i++) 
							fprintf(hf, "%d %6e\n", RunVars.StaircaseCfg[SCPARAM_MIN] + i * RunVars.StaircaseCfg[SCPARAM_STEP], Stats.Staircase[b][ch][i]);
						fclose(hf);
					}
				}
			}
		}
	}
	return 0;
}

/******************************************************
* Save Run Info
******************************************************/
int cnc_write(char* path, char cncp[MAX_NCNC][200]) 
{
	for (int i = 0; i < FERSLIB_MAX_NCNC; i++)
		if (strcmp(cncp[i], path) == 0) return 1;
	return 0;
}

int SaveRunInfo()
{
	char str[200];
	char fname[500];
	char read_cnc[MAX_NCNC][200];
	struct tm* t;
	time_t tt;
	int b;
	FILE* cfg;
	FILE* iof;
	uint32_t FPGArev=0, MICrev=0, pid=0;

	sprintf(fname, "%sRun%d_Info.txt", WDcfg.DataFilePath, RunVars.RunNumber);
	iof = fopen(fname, "w");

	uint8_t rr;

	fprintf(iof, "********************************************************************* \n");
	fprintf(iof, "Run n. %d\n\n", RunVars.RunNumber);
	tt = (time_t)(Stats.start_time / 1000); //   RO_Stats.StartTime_ms / 1000);
	t = localtime(&tt);
	strftime(str, sizeof(str) - 1, "%d/%m/%Y %H:%M", t);
	fprintf(iof, "Start Time: %s\n", str);
	tt = (time_t)(Stats.stop_time / 1000); //   RO_Stats.StopTime_ms / 1000);
	t = localtime(&tt);
	strftime(str, sizeof(str) - 1, "%d/%m/%Y %H:%M", t);
	fprintf(iof, "Stop Time:  %s\n", str);
	fprintf(iof, "Elapsed time = %.3f s\n", Stats.current_tstamp_us[0] / 1e6); //     RO_Stats.CurrentTimeStamp_us / 1e6);
	fprintf(iof, "********************************************************************* \n");
	fprintf(iof, "\n\n********************************************************************* \n");
	fprintf(iof, "Setup:\n");
	fprintf(iof, "********************************************************************* \n");
	fprintf(iof, "Software Version: Janus %s\n", SW_RELEASE_NUM);
	fprintf(iof, "Output data format version: %s\n", FILE_LIST_VER);
	FERS_BoardInfo_t BoardInfo;
	FERS_CncInfo_t CncInfo;
	int cnc = 0;

	for (b = 0; b < WDcfg.NumBrd; b++) {
		char* cc, cpath[100];
		if (((cc = strstr(WDcfg.ConnPath[b], "tdl")) != NULL)) {  // TDlink used => Open connection to concentrator (this is not mandatory, it is done for reading information about the concentrator)
			FERS_Get_CncPath(WDcfg.ConnPath[b], cpath);
			if (!cnc_write(cpath, read_cnc)) {
				rr = FERS_ReadConcentratorInfo(cnc_handle[cnc], &CncInfo);
				sprintf(read_cnc[cnc], "%s", cpath);
				if (rr == 0) {
					fprintf(iof, "Concentrator %d:\n", cnc);
					fprintf(iof, "\tFPGA FW revision = %s\n", CncInfo.FPGA_FWrev);
					fprintf(iof, "\tSW revision = %s\n", CncInfo.SW_rev);
					fprintf(iof, "\tPID = %d\n\n", CncInfo.pid);
					if (CncInfo.ChainInfo[0].BoardCount == 0) { 	// Rising error if no board is connected to link 0
						Con_printf("LCSm", "ERROR: read concentrator info failed in SaveRunInfo\n");
						return -2;
					}
					for (int l = 0; l < 8; l++) {
						if (CncInfo.ChainInfo[l].BoardCount > 0)
							Con_printf("LCSm", "Found %d board(s) connected to TDlink n. %d\n", CncInfo.ChainInfo[l].BoardCount, l);
					}
				} else {
					Con_printf("LCSm", "ERROR: Cannot read concentrator %02d info\n", cnc);
					return -2;
				}
				++cnc;
			}
		}

		rr = FERS_ReadBoardInfo(handle[b], &BoardInfo); // Read Board Info
		if (rr != 0)
			return -1;
		char fver[100];
		//if (FPGArev == 0) sprintf(fver, "BootLoader"); DNIN: mixed with an old version checker with register.
		//else 
		sprintf(fver, "%d.%d (Build = %04X)", (BoardInfo.FPGA_FWrev >> 8) & 0xFF, (BoardInfo.FPGA_FWrev) & 0xFF, (BoardInfo.FPGA_FWrev >> 16) & 0xFFFF);
		fprintf(iof, "Board %d:\n", b);
		fprintf(iof, "\tModel = %s\n", BoardInfo.ModelName);
		fprintf(iof, "\tPID = %" PRIu32 "\n", BoardInfo.pid);
		fprintf(iof, "\tFPGA FW revision = %s\n", fver);
		fprintf(iof, "\tuC FW revision = %08X\n", BoardInfo.uC_FWrev);
	}
	// CTIN: save event statistics
	/*
	fprintf(iof, "\n\n********************************************************************* \n");
	fprintf(iof, "Statistics:\n");
	fprintf(iof, "********************************************************************* \n");
	fprintf(iof, "Total Acquired Events: %lld (Rate = %.3f Kcps)\n", (long long)RO_Stats.EventCnt, (float)RO_Stats.EventCnt/(RO_Stats.CurrentTimeStamp_us/1000));
	for (b = 0; b < WDcfg.NumBrd; b++) {
		fprintf(iof, "\nBoard %d (s.n. %d)\n", b, DGTZ_SerialNumber(handle[b]));
		fprintf(iof, "Lost Events: %lld (%.3f %%)\n", (long long)RO_Stats.LostEventCnt[b], PERCENT(RO_Stats.LostEventCnt[b], RO_Stats.LostEventCnt[b] + RO_Stats.EventCnt));
	}
	*/
	if (WDcfg.EnableJobs) {
		sprintf(fname, "%sJanus_Config_Run%d.txt", WDcfg.DataFilePath, RunVars.RunNumber);
		cfg = fopen(fname, "r");
		if (cfg == NULL) 
			sprintf(fname, CONFIG_FILENAME);
	}
	else
		sprintf(fname, CONFIG_FILENAME);

	fprintf(iof, "\n\n********************************************************************* \n");
	fprintf(iof, "Config file: %s\n", fname);
	fprintf(iof, "********************************************************************* \n");
	cfg = fopen(fname, "r");
	if (cfg != NULL) {
		while (!feof(cfg)) {
			char line[500];
			fgets(line, 500, cfg);
			fputs(line, iof);
		}
	}

	fclose(iof);

	return 0;
}