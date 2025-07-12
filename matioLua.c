#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "matio.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
typedef struct{
	int rank;
	size_t*dims;
	double*data;
}tensor;	
void freeTensor(tensor data){
	free(data.dims);
	free(data.data);
}
int totalEntries(tensor data){
	int numEntries=1;
	for(int i=0;i<data.rank;i++){
		numEntries*=(int)data.dims[i];
	}
	return numEntries;
}
int numFrontiers(int index, tensor data){
	int frontiers=0;
	int product=1;
	for(int i=0;i<data.rank;i++){
		product*=data.dims[i];
		if((index+1)%product==0){
			frontiers++;
		}
	}
	return frontiers;
}
tensor getLuaTensor(lua_State*L,int index){
	tensor tensorData;
	int entries;
	int lastDim;
	int rank=0;
	lua_pushvalue(L,index);
	while(lua_istable(L,-1)){
		rank++;
		if (lua_rawlen(L,-1)==0){
			break;
		}
		lua_rawgeti(L,-1,1);
	}
	tensorData.rank=rank;
	lua_pop(L,rank);
	lua_pushvalue(L,index);
	tensorData.dims=(size_t*)malloc(tensorData.rank*sizeof(size_t));
	for(int i=0;i<rank;i++){
		tensorData.dims[i]=lua_rawlen(L,-1);
		lua_rawgeti(L,-1,1);
	}
	lua_pop(L,tensorData.rank);
	entries=totalEntries(tensorData);
	tensorData.data=(double*)malloc(sizeof(int)*totalEntries(tensorData));
	for(int i=0;i<entries;i++){
		for(int j=0;j<rank;j++){
			lua_rawgeti(L,-1,i%tensorData.dims[j]);
		}
		tensorData.data[i]=lua_tonumber(L,-1);
		lua_pop(L,rank);
	}
	return tensorData;
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




tensor readMat(const char*fileName,const char*varName){
	mat_t*matfp=Mat_Open(fileName,MAT_ACC_RDONLY);
	matvar_t* matvar=Mat_VarRead(matfp,varName);
	tensor data;
	int entries;
	data.rank=matvar->rank;
	data.dims=(size_t*)malloc(sizeof(size_t)*matvar->rank);
	memcpy(data.dims,matvar->dims,data.rank*sizeof(size_t));
	entries=totalEntries(data);
	memcpy(data.data,matvar->data,entries*sizeof(double));
	Mat_VarFree(matvar);
	Mat_Close(matfp);
	return data;
}
static int lua_readMatio(lua_State*L){
	const char*fileName=luaL_checkstring(L,1);
	const char*varName=luaL_checkstring(L,2);
	tensor data =readMat(fileName,varName);
	int entries=totalEntries(data);
	int*currKey=(int*)malloc(data.rank*sizeof(int));
	for(int i=0;i<data.rank;i++){
		lua_newtable(L);
		currKey[data.rank-1-i]=1;
		lua_pushnumber(L,currKey[data.rank-1-i]);
	}
	lua_pop(L,1);
	for(int i=0;i<entries;i++){
		lua_pushnumber(L,currKey[0]);
		lua_pushnumber(L,data.data[i]);
		lua_settable(L,-3);
		lua_pop(L,2);
		currKey[0]++;
		for(int j=0;j<numFrontiers(i,data);j++){
			lua_settable(L,-3);
			lua_pop(L,2);
			currKey[j+1]++;
			currKey[j]=1;
		}
		for(int j=0;j<numFrontiers(i,data);j++){
			lua_pushnumber(L,currKey[j+1]);
			lua_newtable(L);
		}
	}
	lua_pop(L,data.rank-1);
	freeTensor(data);
	free(currKey);
	return 1;
}

static const struct luaL_Reg matio_funcs[] = {
    {"load", lua_readMatio},
    {NULL, NULL} 
};
int luaopen_matioLua(lua_State *L) {
    luaL_newlib(L, matio_funcs);

    return 1;
}




