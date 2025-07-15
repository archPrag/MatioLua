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
void stack_dump(lua_State *L) {
    int top = lua_gettop(L);
    printf("--- Stack Dump (top: %d) ---\n", top);
    for (int i = 1; i <= top; i++) {
        int t = lua_type(L, i);
        printf("%d: %s ", i, lua_typename(L, t));
        switch (t) {
            case LUA_TSTRING:
                printf("'%s'", lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("%g", lua_tonumber(L, i));
                break;
            default:
                printf("%p", lua_topointer(L, i));
                break;
        }
        printf("\n");
    }
    printf("---------------------------\n");
}
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
int numFrontiersRaw(int index, int rank,size_t*dims){
	int frontiers=0;
	int product=1;
	for(int i=0;i<rank;i++){
		product*=dims[rank-1-i];
		if((index+1)%product==0){
			frontiers++;
		}
	}
	return frontiers;
}
void dispTensor(tensor data){
	printf("dims:");
	for(int i=0;i<data.rank;i++){
		printf("%d ",data.dims[i]);
	}
	printf("\ndata:");
	int entries=totalEntries(data);
	for(int i=0;i<data.dims[0];i++){
		printf("%lf ",data.data[i]);
	}
	printf("...\n");
}
tensor permuteIndices(tensor x){
	tensor y;
	int entries;
	int newIndex;
	int oldIndex;
	int*indexPermute;
	int*divisors;
	int*multiplier;
	y.rank=x.rank;
	indexPermute=(int*)malloc(y.rank*sizeof(int));
	y.dims=(size_t*)malloc(y.rank*sizeof(size_t));
	for(int i=0;i<y.rank/2;i++){
		indexPermute[2*i]=2*i+1;
		indexPermute[2*i+1]=2*i;
	}
	if(y.rank%2==1){
		indexPermute[y.rank-1]=y.rank-1;
	}
	for(int i=0;i<y.rank;i++){
		y.dims[indexPermute[i]]=x.dims[i];
	}
	entries=totalEntries(x);
	y.data=(double*)malloc(entries*sizeof(double));
	divisors=(int*)malloc(y.rank*sizeof(int));
	multiplier=(int*)malloc(y.rank*sizeof(int));
	for(int i=0;i<y.rank;i++){
		divisors[i]=1;
		multiplier[indexPermute[i]]=1;
		for(int j=0;j<i;j++){
			divisors[i]*=x.dims[j];
			multiplier[indexPermute[i]]*=x.dims[j];
		}
	}
	for(int i=0;i<entries;i++){
		newIndex=0;
		for(int j=0;j<y.rank;j++){
			oldIndex=(i/divisors[j])%(x.dims[j]);
			newIndex+=multiplier[j]*oldIndex;
		}
		y.data[newIndex]=x.data[i];
	}
	free(multiplier);
	free(indexPermute);
	free(divisors);
	return y;
}
tensor getLuaTensor(lua_State*L,int index){
	tensor tensorData;
	int entries;
	int lastDim;
	int rank=0;
	int*currInd;
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
	tensorData.dims=(size_t*)malloc(rank*sizeof(size_t));
	for(int i=0;i<rank;i++){
		tensorData.dims[i]=lua_rawlen(L,-1);
		if (tensorData.dims[i]==0){
			break;
		}
		lua_rawgeti(L,-1,1);
	}
	entries=totalEntries(tensorData);
	tensorData.data=(double*)malloc(sizeof(double)*entries);
	currInd=(int*)malloc(sizeof(int)*rank);
	for(int i=0;i<rank;i++){
		currInd[i]=1;
	}
	for(int i=0;i<entries;i++){
		tensorData.data[i]=lua_tonumber(L,-1);
		lua_pop(L,1);
		for(int j=0;j<numFrontiersRaw(i,tensorData.rank,tensorData.dims);j++){
			lua_pop(L,1);
			currInd[j]=0;
		}

		if(i+1==entries){
			break;
		}
		currInd[0]++;
		for(int j=0;j<numFrontiersRaw(i,tensorData.rank,tensorData.dims);j++){
			currInd[j+1]++;
			lua_rawgeti(L,-1,i%currInd[j+1]+1);
		}
		lua_rawgeti(L,-1,currInd[0]);
	}

	lua_pop(L,1);
	free(currInd);
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
int fileExists(const char* fileName){
	FILE*sFile=fopen(fileName,"r");
	int exists=0;
	if(sFile!=NULL){
		fclose(sFile);
		exists=1;
	}
	return exists;
}
int varExists(mat_t*matfp,const char* varName){
	matvar_t*matvar=Mat_VarReadInfo(matfp,varName);
	if(matvar!=NULL){
		Mat_VarFree(matvar);
		return 1;
	}
	return 0;
}
void writeMAT(const char*fileName,const char*variableName,tensor data){
	matvar_t*matvar=Mat_VarCreate(variableName,MAT_C_DOUBLE,MAT_T_DOUBLE,data.rank,data.dims,data.data,MAT_FT_DEFAULT);
	mat_t*matfp;
	if(fileExists(fileName)){
		matfp=Mat_Open(fileName,MAT_ACC_RDWR);
	}
	else{
		matfp=Mat_CreateVer(fileName,NULL,MAT_FT_DEFAULT);
	}
	if(varExists(matfp,fileName)){
		Mat_VarDelete(matfp,variableName);
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
	data.data=(double*)malloc(sizeof(double)*entries);
	memcpy(data.data,matvar->data,entries*sizeof(double));
	Mat_VarFree(matvar);
	Mat_Close(matfp);
	return data;
}

void appendTensor(lua_State*L,tensor data){
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
		currKey[0]++;
		for(int j=0;j<numFrontiers(i,data)-(i+1>=entries);j++){
			lua_settable(L,-3);
			currKey[j+1]++;
			currKey[j]=1;
		}
		if(i+1>=entries){
			break;
		}
		for(int j=0;j<numFrontiers(i,data);j++){
			lua_pushnumber(L,currKey[j+1]);
			lua_newtable(L);
		}
	}
	free(currKey);
}
tensor getLuaNumber(lua_State*L,int index){
	tensor data;
	data.rank=1;
	data.dims=(size_t*)malloc(sizeof(size_t));
	data.data=(double*)malloc(sizeof(double));
	data.dims[0]=1;
	data.data[0]=luaL_checknumber(L,index);
	return data;
}
static int lua_writeMatio(lua_State*L){
	const char*fileName=luaL_checkstring(L,1);
	tensor data;
	tensor x;
	const char*varName=luaL_checkstring(L,3);
	if(lua_istable(L,2)){
		data=getLuaTensor(L,2);
	}
	else{
		data=getLuaNumber(L,2);
	}
	writeMAT(fileName,varName,data);
	freeTensor(data);
	return 0;
}
static int lua_readMatio(lua_State*L){
	const char*fileName=luaL_checkstring(L,1);
	const char*varName=luaL_checkstring(L,2);
	if(!fileExists(fileName)){
		fprintf(stderr,"file to read does not exists");
	}
	tensor dataP =readMat(fileName,varName);
	
	if(totalEntries(dataP)==1){
		lua_pushnumber(L,dataP.data[0]);
	}
	else{
		tensor data =permuteIndices(dataP);
		appendTensor(L,data);
		freeTensor(data);
	}
	freeTensor(dataP);
	return 1;
}

static const struct luaL_Reg matio_funcs[] = {
    {"load", lua_readMatio},
    {"save", lua_writeMatio},
    {NULL, NULL} 
};
int luaopen_matioLua(lua_State *L) {
    luaL_newlib(L, matio_funcs);

    return 1;
}




