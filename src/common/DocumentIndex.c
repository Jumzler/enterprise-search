/*
Rutinner for � jobbe p� en Boitho DocumentIndex fil.

ToDo: trenger en "close" prosedyre for filhandlerene.
*/
#include "DocumentIndex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "bstr.h"
#include "dp.h"

//skrur av filcashn da vi deiver � segge feiler med den
//#define DI_FILE_CASHE

#ifdef DI_FILE_CASHE

	static int openDocumentIndex = -1;
	static char openName[64] = {'\0'};

	FILE *DocumentIndexHA;

	void closeDICache(void)
	{
		if (openDocumentIndex != -1) {
			openDocumentIndex = -1;
			fclose(DocumentIndexHA);
		}
	}
#endif


#define CurrentDocumentIndexVersion 4

// DocumentIndex strukt, er rutiner som jobber p� en di struct
void DIS_delete(struct DocumentIndexFormat *DocumentIndexPost) {
	DocumentIndexPost->htmlSize2 = 0;
	DocumentIndexPost->htmlSize = 0;
	DocumentIndexPost->imageSize = 0;
	DocumentIndexPost->RepositoryPointer = 0;
	DocumentIndexPost->ResourcePointer = 0;
	DocumentIndexPost->SummaryPointer = 0;
	DocumentIndexPost->SummarySize = 0;

}

int DIS_isDeleted(struct DocumentIndexFormat *DocumentIndexPost) {

	//dokumentet er slettet hvis alle f�lgende pekere og st�relse er 0
	if (
	   ((DocumentIndexPost->htmlSize2 == 0) && (DocumentIndexPost->htmlSize == 0))
	&& (DocumentIndexPost->imageSize == 0)
	&& (DocumentIndexPost->RepositoryPointer == 0)
	&& (DocumentIndexPost->ResourcePointer == 0)
	&& (DocumentIndexPost->SummaryPointer == 0)
	&& (DocumentIndexPost->SummarySize == 0)
	) {
		return 1;
	}
	else {
		return 0;
	}
}
int DIPostAdress(unsigned int DocID) {

	int adress = -1;

	int LotNr;

	//finner lot for denne DocIDen
	LotNr = rLotForDOCid(DocID);

	#ifdef BLACK_BOX
		adress = (sizeof(struct DocumentIndexFormat) + sizeof(unsigned int))* (DocID - LotDocIDOfset(LotNr));
	#else
		adress = sizeof(struct DocumentIndexFormat) * (DocID - LotDocIDOfset(LotNr));
	#endif

	return adress;


}
/*
finner riktig fil og S�ker seg frem til riktig adresse, slik at man bare kan lese/skrive
*/
FILE *GetFileHandler (unsigned int DocID,char type,char subname[], char *diname) {

	#ifndef DI_FILE_CASHE
		FILE *DocumentIndexHA = NULL;
	#endif
	int LotNr;
	char FileName[128];
	char FilePath[128];
	
	//finner lot for denne DocIDen
	LotNr = rLotForDOCid(DocID);

	//hvis filen ikke er open �pner vi den
	//segfeiler en skjelden gang
	#ifdef DI_FILE_CASHE
	if ((LotNr == openDocumentIndex) && (diname == NULL || strcmp(openName,diname) == 0)) {

	}
	#else
	if(0) {

	}
	#endif
	else {		

		GetFilPathForLot(FilePath,LotNr,subname);
		
		strncpy(FileName,FilePath,128);
		strncat(FileName,diname == NULL ? "DocumentIndex" : diname,128);

		#ifdef DI_FILE_CASHE
			printf("openig di file \"%s\"\n",FileName);
		#endif

		#ifdef DI_FILE_CASHE

			//hvis vi har en open fil lukkes denne
			if (openDocumentIndex != -1) {
				//segfeiler her for searchkernel
				//18,okt segefeiler her igjen ????
				fclose(DocumentIndexHA);
			}
		#endif
				
		
		//pr�ver f�rst � �pne for lesing
		if (type == 'c') {
			//temp: setter filopning til r+ for � f� til � samarbeid melom DIRead og DIwrite
			//dette gj�r at s�k ikke funker p� web p� grun av rettighter :-(
			if ((DocumentIndexHA = fopen(FileName,"r+b")) == NULL) {
				printf("%d: cant open file %s for c\n", __LINE__, FileName);
				perror(FileName);
			    return NULL;
			}
		}
		else if (type == 'r') {
			//temp: setter filopning til r+ for � f� til � samarbeid melom DIRead og DIwrite
			//dette gj�r at s�k ikke funker p� web p� grun av rettighter :-(
			if ((DocumentIndexHA = fopen(FileName,"r+b")) == NULL) {
				printf("%d: cant open file %s for r\n",__LINE__, FileName);
				perror(FileName);
			    return NULL;
			}
		}
		else if (type == 's') {
			//en ekte r read
			if ((DocumentIndexHA = fopen(FileName,"rb")) == NULL) {
				printf("%d: cant open file %s for rb\n", __LINE__, FileName);
				perror(FileName);
			    return NULL;
			}
		}

		else if (type == 'w'){
			if ((DocumentIndexHA = fopen(FileName,"r+b")) == NULL) {
				//hvis det ikke g�r lager vi og �pne filen
				makePath(FilePath);
				if ((DocumentIndexHA = fopen(FileName,"w+b")) == NULL) {
					perror(FileName);

					return NULL;
				}
			}
		}

		#ifdef DI_FILE_CASHE
			openDocumentIndex = LotNr;
			strscpy(openName,diname == NULL ? "DocumentIndex" : diname,sizeof(openName));
		#endif
	}

	//s�ker til riktig post
	if (fseek(DocumentIndexHA,DIPostAdress(DocID),0) != 0) {
		perror("Can't seek");
		exit(1);
	}


	return DocumentIndexHA;

}

//sjeker om det fins en DocumentIndex fro denne loten
int DIHaveIndex (int lotNr,char subname[]) {

	char FilePath[512];
	FILE *FH;

        GetFilPathForLot(FilePath,lotNr,subname);

        strncat(FilePath,"DocumentIndex",sizeof(FilePath));

	if ((FH = fopen(FilePath,"r")) == NULL) {
		return 0;
	}
	else {
		fclose(FH);
		return 1;
	}
}

/*
skriver en post til DocumentIndex
*/
void DIWrite (struct DocumentIndexFormat *DocumentIndexPost, unsigned int DocID,char subname[], char *diname) {


	FILE *file;
	
	if ((file = GetFileHandler(DocID,'w',subname, diname)) != NULL) {

		#ifdef BLACK_BOX
			unsigned int CurrentDocumentIndexVersionAsUInt = CurrentDocumentIndexVersion;

			if (fwrite(&CurrentDocumentIndexVersionAsUInt,sizeof(unsigned int),1,file) != 1) {
	                        perror("Can't write");
        	                exit(1);
                	}
		#endif
		//skriver posten
		if (fwrite(DocumentIndexPost,sizeof(struct DocumentIndexFormat),1,file) != 1) {
			perror("Can't write");
			exit(1);
		}
		#ifndef WITHOUT_DIWRITE_FSYNC
			fsync(fileno(file));
		#endif
	}
	else {
		printf("Cant get fh\n");
	}

	//hvis vi ikke har p� DI_FILE_CASHE m� vi lokke filen
	#ifndef DI_FILE_CASHE
		fclose(file);
	#endif
}

/*
tar et lott nr inn og henter neste post

Den vil retunere 1 s� lenge det er data og lese. slik at man kan ha en lopp slik

while (DIRGetNext(LotNr,DocumentIndexPost)) {

        ..gj�r noe med ReposetoryData..

}
*/

int DIGetNext (struct DocumentIndexFormat *DocumentIndexPost, int LotNr,unsigned int *DocID,char subname[]) {


        static FILE *LotFileOpen;
        static int LotOpen = -1;
	static unsigned LastDocID;
	int n;
	char FileName[128];



        //tester om reposetoriet allerede er open, eller ikke
        if (LotOpen != LotNr) {
                //hvis den har v�rt open, lokker vi den. Hvi den er -1 er den ikke brukt enda, s� ingen vits � � lokke den da :-)
                if (LotOpen != -1) {
                        fclose(LotFileOpen);
                }
                GetFilPathForLot(FileName,LotNr,subname);
                strncat(FileName,"DocumentIndex",128);

		#ifdef DEBUG
	                printf("%s:%i: Opending lot %s\n",__FILE__,__LINE__,FileName);
		#endif

                if ( (LotFileOpen = fopen(FileName,"rb")) == NULL) {
			#ifdef DEBUG
                        	perror(FileName);
			#endif

			LotOpen = -1;
			return 0;
                }
                LotOpen = LotNr;
		LastDocID = GetStartDocIFForLot(LotNr);
        }
	else {
		++LastDocID;
	}

	(*DocID) = LastDocID;

	

        //hvis det det er data igjen i filen leser vi den
        if (!feof(LotFileOpen)) {
         

        	//leser posten
		//mystisk at vi f�r "Can't reed DocumentIndexPost: Success", ved eof her,
		//i steden for at vi  f�r eof lenger opp
		#ifdef BLACK_BOX
                        unsigned int CurrentDocumentIndexVersionAsUInt;
			if ((fread(&CurrentDocumentIndexVersionAsUInt,sizeof(unsigned int),1,LotFileOpen)) != 1) {
				#ifdef DEBUG
				perror("CurrentDocumentIndexVersionAsUInt");
				#endif

				//stnger ned filen
        	                fclose(LotFileOpen);
	                        LotOpen = -1;

				return 0;
			}
		#endif

        	if ((n=fread(DocumentIndexPost,sizeof(struct DocumentIndexFormat),1,LotFileOpen)) != 1) {
			if (feof(LotFileOpen)) {
				printf("hit eof for DocumentIndex\n");
			}
			else {
                		printf("Can't reed DocumentIndexPost. n: %i, eof %i\n",n,feof(LotFileOpen));
				perror("fread() DocumentIndexPost");
			}
			//stnger ned filen
			fclose(LotFileOpen);
			LotOpen = -1;

			return 0;
        	}
		else {
	       		return 1;
		}

        }
        else {
		//hvis vi er tom for data stenger vi filen, og retunerer en 0 som sier at vi er ferdig.
		#ifdef DEBUG
                	printf("ferdig\n");
		#endif
                fclose(LotFileOpen);
		LotOpen = -1;
                return 0;
        }
}

int DIRead_post_i(struct DocumentIndexFormat *DocumentIndexPost, int file) {

	int forReturn = 0;

		#ifdef BLACK_BOX
                        unsigned int CurrentDocumentIndexVersionAsUInt;
                        if ((read(file,&CurrentDocumentIndexVersionAsUInt,sizeof(unsigned int))) < 0) {
				#ifdef DEBUG
                                perror("CurrentDocumentIndexVersionAsUInt");
				#endif
                                forReturn = 0;
                        }
                #endif


        	//lesr posten
        	if (read(file,DocumentIndexPost,sizeof(*DocumentIndexPost)) < 0) {
			#ifdef DEBUG
                	perror("Can't reed");
			#endif
			//selv om vi ikke fikk lest fra filen m� vi lokke den, s� vi kan ikke kalle retun directe her
			forReturn =  0;
        	}
		else {
			forReturn  = 1;
		}

        if ((*DocumentIndexPost).htmlSize != 0) {
                (*DocumentIndexPost).htmlSize2 = (*DocumentIndexPost).htmlSize;
        }

	return forReturn;
		
}

int DIRead_post_fh(struct DocumentIndexFormat *DocumentIndexPost, FILE *file) {

	#ifdef DEBUG
		printf("DIRead_post_fh()\n");
	#endif

	int forReturn = 0;

		#ifdef BLACK_BOX
                        unsigned int CurrentDocumentIndexVersionAsUInt;
                        if ((fread(&CurrentDocumentIndexVersionAsUInt,sizeof(unsigned int),1,file)) != 1) {
				#ifdef DEBUG
                                perror("CurrentDocumentIndexVersionAsUInt");
				#endif
                                forReturn = 0;
                        }
                #endif


        	//lesr posten
        	if (fread(DocumentIndexPost,sizeof(*DocumentIndexPost),1,file) != 1) {
			#ifdef DEBUG
				perror("Can't reed");
			#endif
			//selv om vi ikke fikk lest fra filen m� vi lokke den, s� vi kan ikke kalle retun directe her
			forReturn =  0;
        	}
		else {
			forReturn  = 1;
		}


	        if ((*DocumentIndexPost).htmlSize != 0) {
	                (*DocumentIndexPost).htmlSize2 = (*DocumentIndexPost).htmlSize;
	        }


	return forReturn;
		
}

int DIRead_fmode (struct DocumentIndexFormat *DocumentIndexPost, int DocID,char subname[], char filemode) {

	FILE *file;
	int forReturn = 0;

	#ifdef DEBUG
		printf("DIRead_fmode(DocID=%i, subname=\"%s\")\n",DocID,subname);
	#endif

	#ifdef DISK_PROTECTOR
		dp_lock(rLotForDOCid(DocID));
	#endif

	if ((file = GetFileHandler(DocID,filemode,subname, NULL)) != NULL) {

		if (DIRead_post_fh(DocumentIndexPost,file)) {
			forReturn = 1;
		}

		//hvis vi ikke har p� DI_FILE_CASHE m� vi lokke filen
		#ifndef DI_FILE_CASHE
			fclose(file);
		#endif
		
        }
        else {
		printf("can't open DocumentIndexPost for DocID %u.\n",DocID);
        }


        if ((*DocumentIndexPost).htmlSize != 0) {
                (*DocumentIndexPost).htmlSize2 = (*DocumentIndexPost).htmlSize;
        }


	#ifdef DISK_PROTECTOR
		dp_unlock(rLotForDOCid(DocID));
	#endif

	return forReturn;

}

int DIRead_fh(struct DocumentIndexFormat *DocumentIndexPost, int DocID,char subname[], FILE *file) {

	int forReturn = 0;


	if (file == NULL) {
		#ifdef DEBUG
			printf("DIRead_fh: file isent open.\n");
		#endif
		forReturn = DIRead_fmode(DocumentIndexPost,DocID,subname,'r');
	}
	else {
		#ifdef DISK_PROTECTOR
			dp_lock(rLotForDOCid(DocID));
		#endif

		

		//s�ker til riktig post
		if (fseek(file,DIPostAdress(DocID),0) != 0) {
			perror("Can't seek");
			exit(1);
		}

		if (DIRead_post_fh(DocumentIndexPost,file)) {
			forReturn = 1;
		}
		#ifdef DISK_PROTECTOR
			dp_unlock(rLotForDOCid(DocID));
		#endif

	}

        if ((*DocumentIndexPost).htmlSize != 0) {
                (*DocumentIndexPost).htmlSize2 = (*DocumentIndexPost).htmlSize;
        }


	return forReturn;
}

int DIRead_i(struct DocumentIndexFormat *DocumentIndexPost, int DocID,char subname[], int file) {

	int forReturn = 0;


	if (file == -1) {
		#ifdef DEBUG
			printf("DIRead_fh: file isent open.\n");
		#endif
		forReturn = DIRead_fmode(DocumentIndexPost,DocID,subname,'r');
	}
	else {
		#ifdef DISK_PROTECTOR
			dp_lock(rLotForDOCid(DocID));
		#endif

		

		//s�ker til riktig post
		if (lseek(file,DIPostAdress(DocID),SEEK_SET) < 0) {
			perror("Can't lseek");
			forReturn = 0;
		}
		else if (DIRead_post_i(DocumentIndexPost,file)) {
			forReturn = 1;
		}
		#ifdef DISK_PROTECTOR
			dp_unlock(rLotForDOCid(DocID));
		#endif

	}

        if ((*DocumentIndexPost).htmlSize != 0) {
                (*DocumentIndexPost).htmlSize2 = (*DocumentIndexPost).htmlSize;
        }


	return forReturn;
}

/*
leser en post fra DocumentIndex

runarb: 24.10.2007:
Tradisjonelt s� har DIRead �pnet filen r+ for � kunne samarbeide med DIWrite.
Jeg har n� d�pt den �pningen om til c, og laget en DIRead_fmode der man kan spesifisere opningsmode selv, slik at man kan
f� en ekte read
*/
int DIRead (struct DocumentIndexFormat *DocumentIndexPost, int DocID,char subname[]) {
	return DIRead_fmode(DocumentIndexPost,DocID,subname,'c');
}

//stenger ned filer
void DIClose(FILE *DocumentIndexHA) {
	fclose(DocumentIndexHA);
}
