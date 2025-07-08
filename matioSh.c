#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "matio.h"

int dimMatrix(char*matrixText,int numText){
	for(int i =0;i<numText;i++){
		if(matrixText[i]!='['){
			return i;
		}
	}
	return 0;
}
int countCommas(char*text,int numText){
	int commas=0;
	for(int i=0;i<numText;i++){
		if(text[i]==','){
			commas++;
		}
		if(text[i]=='\0'){
			break;
		}
	}
	return commas;
}
int sizeMatrixInText(char*text,int numText){
	int layer=0;
	int matrixLen=0;
	for(int i=0;i<numText;i++){
		if(text[i]=='['){
			layer++;
		}
		if(text[i]==']'){
			layer--;
		}
		if(layer==0){
			matrixLen=i+1;
			break;
		}
	}
	return matrixLen;
}
char* filterMatrixInText(char*text,int numText){
	int matrixLen=sizeMatrixInText(text,numText);
	char*matrixText=(char*)malloc((matrixLen+1)*sizeof(char));
	strncpy(matrixText,text,matrixLen);
	matrixText[matrixLen]='\0';
	return matrixText;
}
size_t* sizeMatrix(char*matrixText,int numText){
	char*subMatrix;
	int dimension=dimMatrix(matrixText,numText);
	size_t*size=(size_t*)malloc(dimension*sizeof(size_t));
	int*commas=(int*)malloc(dimension*sizeof(int));
	for(int i=0;i<dimension;i++){
		subMatrix=filterMatrixInText(matrixText+i,numText);
		commas[i]=countCommas(subMatrix,sizeMatrixInText(matrixText+i,numText));
		free(subMatrix);
	}
	for(int i=0;i<dimension;i++){
		if(i==0){
			size[dimension-1-i]=commas[dimension-1-i]+1;
		}
		if(i>0){
			size[dimension-1-i]=(commas[dimension-1-i]+1)/(commas[dimension-i]+1);

		}
	}
	free(commas);
	return size;
}
int fileSize(char*fileName){
	FILE*toRead=fopen(fileName,"r");
	char currChar;
	for(int i=0;;i++){
		if(fgetc(toRead)==EOF){
			fclose(toRead);
			return i+1;
		}
	}
	free(toRead);
	return 0;
}
int totalEntries(char*matrixText,int numText){
	int numEntries=1;
	int dimension=dimMatrix(matrixText,numText);
	size_t* size=sizeMatrix(matrixText,numText);
	
	for(int i=0;i<dimension;i++){
		numEntries*=(int)size[i];
	}
	free(size);
	return numEntries;
}
int numberCharacter(char character){
	char special[20]="0123456789.";
	for(int i=0;;i++){
		if(character==special[i]){
			return 1;
		}
		if(special[i]=='\0'){
			return 0;
		}
	}
}
double* getMatrix(char*matrixText,int numText){
	int position=0;
	int matPos=0;	
	char numStr[50];
	int numEntries=totalEntries(matrixText,numText);
	double*matrix=(double*)malloc(numEntries*sizeof(double));
	for(int i=0;i<numText;i++){
		if(!numberCharacter(matrixText[i])&& position!=0){
			numStr[position]='\0';
			sscanf(numStr,"%lf",matrix+matPos);
			matPos++;
			position=0;
		}
		if(numberCharacter(matrixText[i])){
			numStr[position]=matrixText[i];
			position++;
		}
	}
	return matrix;
}

int specifiedMatFile(char*fileName){
	char strToFind[20]=".mat";
	int pos=0;
	for(int i=0;i<1024;i++){
		if(strToFind[pos]==fileName[i]){
			pos++;
		}
		else{
			pos=0;
		}
		if(pos>=5){
			return 1;
		}
		if(fileName[i]=='\0'){
			break;
		}
	}
	return 0;
}
int fileExists(char* fileName){
	FILE*sFile=fopen(fileName,"r");
	int exists=0;
	if(sFile!=NULL){
		fclose(sFile);
		exists=1;
	}
	return exists;
}
void writeMAT(char*variableName,char*matName,double*matrix,size_t*size,int dimension){
	printf("d%d\n",dimension);
	matvar_t*matvar=Mat_VarCreate(variableName,MAT_C_DOUBLE,MAT_T_DOUBLE,dimension,size,matrix,MAT_FT_DEFAULT);
	mat_t*matfp;
	if(fileExists(matName)){
		matfp=Mat_Open(matName,MAT_FT_DEFAULT);

	}
	else{
		matfp=Mat_CreateVer(matName,NULL,MAT_FT_DEFAULT);

	}
	Mat_VarWrite(matfp,matvar,MAT_COMPRESSION_ZLIB);
	Mat_Close(matfp);
	Mat_VarFree(matvar);
}
matvar_t* readMAT(char*fileName,char*varName){
	mat_t*matfp=Mat_Open(fileName,MAT_ACC_RDONLY);
	matvar_t* matvar=Mat_VarRead(matfp,varName);
	Mat_Close(matfp);
	return matvar;
}
int numFrontiers(int index,size_t*size,int rank){
	int frontiers=0;
	int product=1;
	for(int i=0;i<rank;i++){
		product*=(int)size[rank-1-i];
		if(index%product==0){
			frontiers++;
		}
	}
	return frontiers;
}
void writeTXT(char*fileName,double*data,size_t*size,int rank){
	FILE*toWrite=fopen(fileName,"w");
	int numEntries=1;
	for(int i=0;i<rank;i++){
		numEntries*=(int)size[i];
		fprintf(toWrite,"[");
		
	}
	for(int i=0;i<numEntries;i++){
		fprintf(toWrite,"%lf",data[i]);
		if(i==numEntries-1){
			break;
		}
		for(int j=0;j<numFrontiers(i+1,size,rank);j++){
			fprintf(toWrite,"]");
		}

		fprintf(toWrite,",");
		for(int j=0;j<numFrontiers(1+i,size,rank);j++){
			fprintf(toWrite,"[");
		}
	}
	for(int i=0;i<rank;i++){
		numEntries*=size[i];
		fprintf(toWrite,"]");
	}
	fclose(toWrite);
}




int main(int argc,char**argv){
	if(specifiedMatFile(argv[1])){
		int numMatrix=fileSize(argv[2]);
		FILE*toRead=fopen(argv[2],"r");
		char*matrixText=(char*)malloc(	(numMatrix+1)*sizeof(char));
		fgets(matrixText,(numMatrix+1)*sizeof(char),toRead);
		int dimension=dimMatrix(matrixText,numMatrix);
		printf("%d\n",dimension);
		size_t*size=sizeMatrix(matrixText,numMatrix);
		double* matrix=getMatrix(matrixText,numMatrix);
		printf("%d",totalEntries(matrixText,numMatrix));
		writeMAT(argv[3],argv[1],matrix,size,dimension);
		free(matrix);
		free(size);
		free(matrixText);
		fclose(toRead);
		return 0;
	}
	if(specifiedMatFile(argv[2])){
		matvar_t* matvar=readMAT(argv[2],argv[3]);
		int rank=matvar->rank;
		size_t*size=matvar->dims;
		double*data=matvar->data;
		Mat_VarFree(matvar);
		writeTXT(argv[1],data,size,rank);
	}
}
