//
//  Linker.cpp
//  OSLab1
//
//  Created by Yinuo Chang on 9/20/21.
//

#include <iostream>
#include <cstring> //for strtok()
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <iomanip>


struct Tokens{
    int lineNum;
    int tokenOffset;
    std::string token; //

};
//std::vector<Tokens> tokenList;
struct Symbol{
    int definedTimes = 0;
    bool usedOrNot = false;
    std::string symbolName;
    int initialsymbolAddress;
    int symbolAddress;
    int noModule;
    bool appearedInE = false;
};

struct codeCountPair{
    char codeType;
    int opcode; //op/1000
    int oprand; //op%1000
    int noModule;
    std::string errorInfo ="";
};

std::vector<Symbol> useListWarning;
int moduleSize[100];

void __parseerror(int errcode, Tokens errorToken) {
    static char* errstr[] = {
        (char*)"NUM_EXPECTED", // Number expect, anything >= 2^30 is not a number either
        (char*)"SYM_EXPECTED", // Symbol Expected
        (char*)"ADDR_EXPECTED", // Addressing Expected which is A/E/I/R
        (char*)"SYM_TOO_LONG", // Symbol Name is too long
        (char*)"TOO_MANY_DEF_IN_MODULE", // > 16
        (char*)"TOO_MANY_USE_IN_MODULE", // > 16
        (char*)"TOO_MANY_INSTR"  //total num_instr exceeds memory size (512)
    };
        

    printf("Parse Error line %d offset %d: %s\n", errorToken.lineNum,errorToken.tokenOffset, errstr[errcode]);
    exit(0);
}

//10
int readInt(Tokens inputToken){
    for(char a:inputToken.token){
        if(!isdigit(int(a))){
            __parseerror(0, inputToken);
//            std::cout<<"是:"<<a<<"退出"<<'\n';
//            return false;
        }
        else{
            continue;
        }
    }
    return std::stoi(inputToken.token);
}

/*isymbol：
 1.16个character
 2.[a-Z][a-Z0-9]*
 */
Symbol readSymbol(Tokens inputToken){
    int len = inputToken.token.length();
    if(len>16){
        __parseerror(3, inputToken); //length
    }
    if(!isalpha(inputToken.token[0])){
        __parseerror(1, inputToken);//firstdigit
    }
    else{
        for(int i = 1; i<len;i++){
            if(!isalnum(inputToken.token[i])){
                __parseerror(1, inputToken); //the following
            }
            else{
                continue;
            }
            
        }
    }
    Symbol s;
    s.symbolName = inputToken.token;
    return s;
}

//is IAER?
char readIAER(Tokens inputToken){
    if(inputToken.token.length()!=1){
        __parseerror(2, inputToken);
    }
    if(inputToken.token.compare("I")!=0 && inputToken.token.compare("A")!=0 && inputToken.token.compare("E")!=0 && inputToken.token.compare("R")!=0 ){
        __parseerror(2, inputToken);
    }
    return inputToken.token[0];
}


std::vector<Tokens> getToken(std::string str){
    std::ifstream fin(str);
    std::string s;
    int i = 0;
    std::vector<Tokens> tokenList;
    int finalOff = 0;
    while( getline(fin,s) ){
        finalOff = 2;
        i++;
        char *sentence=(char*)s.c_str();
        char *tokens = strtok(sentence, " \t\n");
        int offset = 1;
        
        while(tokens != NULL){
            Tokens tokens1;
            tokens1.lineNum = i;
            tokens1.token = tokens;
            str = tokens1.token;
            str.erase(remove(str.begin(), str.end(), '\t'), str.end());
            tokens1.token = str;
            tokens1.tokenOffset = offset;
            tokenList.push_back(tokens1);
//            std::cout << "Token: " << tokens1.lineNum << ":" << tokens1.tokenOffset << " : "<< tokens1.token<<" 处理后："<<str<<'\n';
            offset = offset + tokens1.token.length() + 1;
            tokens = strtok(NULL, " ");
            finalOff = offset; //这一行暂时没想好怎么归入

        }
            
        }
    fin.close();
    Tokens LastToken;
    LastToken.lineNum = i;
    LastToken.tokenOffset = finalOff -1;
    LastToken.token = "0ItLast";
    tokenList.push_back(LastToken);
    return tokenList;

     }






////Symbol Table
//void printSymbolTable(std::vector<Symbol> s){
//    std::cout<<"Symbol Table"<<'\n';
//    for(int i = 0; i<s.size();i++){
//        std::cout<<s[i].symbolName<<"="<<s[i].symbolAddress<<'\n';
//    }
//    std::cout<<'\n'<<'\n';
//
//    std::cout<<"Memory Map"<<'\n';
//}

//Print final result
void printFinalResult(std::vector<Symbol> s,std::vector<codeCountPair> c){
    
    std::cout<<"Symbol Table"<<'\n';
    for(int i = 0; i<s.size();i++){
        if(s[i].definedTimes>=1){
            std::cout<<s[i].symbolName<<"="<<s[i].symbolAddress;
            if(s[i].definedTimes>1){
                std::cout<<" Error: This variable is multiple times defined; first value used";
            }
            std::cout<<'\n';
        }

    }
    std::cout<<'\n';
    std::cout<<"Memory Map"<<'\n';

    for(int j = 0;j<c.size();j++){

        std::cout<< std::setfill ('0') << std::setw (3)<<j;
        if(c[j].opcode>=10){
            c[j].errorInfo = " Error: Illegal opcode; treated as 9999";
            c[j].oprand = 999;
            c[j].opcode = 9;
        }
        std::cout<<": "<<std::setfill ('0') << std::setw (4)<<c[j].opcode*1000+c[j].oprand;
        std::cout<<c[j].errorInfo<<'\n';

        for(int i = 0; i<useListWarning.size();i++){
            if(!useListWarning[i].appearedInE && useListWarning[i].noModule == c[j].noModule && ((j==c.size()-1)||(c[j+1].noModule != c[j].noModule))){
                std::cout<<"Warning: Module "<<useListWarning[i].noModule<<": "<<useListWarning[i].symbolName<<" appeared in the uselist but was not actually used"<<'\n';
            }
            else{
                continue;
            }
        }
    }
    
    for(int h=0;h<s.size();h++){
        if(s[h].usedOrNot){
            continue;
        }
        else{
            std::cout<<'\n'<<"Warning: Module "<<s[h].noModule<<": "<<s[h].symbolName<<" was defined but never used"<<'\n';
        }
    }

}

std::vector<Symbol> Pass1(std::string str){
    std::vector<Tokens> token1 = getToken(str);
    std::vector<Symbol> Symbolist;
    std::vector<Symbol> useList;
    int sumOfInst = 0;
    int noOfModule = 1;
    int j = 0; //进行到第几个Token啦
    int Address = 0; //用于记录每一个module的开始address
    while(j<token1.size()-1){
        int defcount;
        try {
            defcount = readInt(token1[j]);
            if(defcount>16){
                __parseerror(4,token1[j]);
            }
            j++;
        } catch (std::exception E) {
            __parseerror(0,token1[token1.size()-1]);
        }

        
        for(int m = 0;m<defcount;m++){
            Symbol s1;
            try {
                s1= readSymbol(token1[j++]);
            } catch (std::exception E) {
                __parseerror(1, token1[token1.size()-1]);
            }
            
            s1.noModule = noOfModule;
            
            try {
                s1.initialsymbolAddress = readInt(token1[j++]);
            } catch (std::exception E) {
                __parseerror(0, token1[token1.size()-1]);
            }
            
            s1.symbolAddress = s1.initialsymbolAddress+Address;
            s1.usedOrNot = false;
            s1.definedTimes = 1;
            bool notExist = true;
            for(int ti = 0;ti<Symbolist.size();ti++){
                if(s1.symbolName.compare(Symbolist[ti].symbolName)==0){
                    Symbolist[ti].definedTimes++;
                    notExist = false;
                }
                else{
                    continue;
                }
            }
            if(notExist){
                Symbolist.push_back(s1);}
        }
        
        int usecount;
        try {
            usecount= readInt(token1[j]);
            if(usecount>16){
                __parseerror(5, token1[j]);
            }
            j++;
        } catch (std::exception E) {
            __parseerror(0,token1[token1.size()-1]);
        }
        
        
        for(int m = 0; m<usecount;m++){
            Symbol s2;
            try {
                s2 = readSymbol(token1[j++]);
            } catch (std::exception E) {
                __parseerror(1,token1[token1.size()-1]);
            }
            
            useList.push_back(s2);
        }
        
        int instcount;
        try {
            instcount = readInt(token1[j]);
            sumOfInst = sumOfInst + instcount;
            if(sumOfInst>512){
                __parseerror(6, token1[j]);
            }
            j++;
        } catch (std::exception E) {
            __parseerror(0,token1[token1.size()-1]);
        }

        
        for(int m = 0;m<instcount;m++){
            char addressmode;
            try {
                addressmode = readIAER(token1[j++]);
            } catch (std::exception E) {
                __parseerror(2,token1[token1.size()-1]);
            }
            
            int op;
            try {
                op = readInt(token1[j]);
    //            if(op<1000){
    //                __parseerror(2, token1[j]);
    //            }
                j++;
            } catch (std::exception E) {
                __parseerror(0,token1[token1.size()-1]);
            }

            int opcode = op/1000;
            int oprand = op%1000;
        }
        
        Address = Address + instcount;
        moduleSize[noOfModule-1] = instcount;
        for(int i = 0;i<Symbolist.size();i++){
            if(Symbolist[i].noModule == noOfModule){
                if(Symbolist[i].initialsymbolAddress>instcount-1){
                    std::cout<<"Warning: Module "<<noOfModule<<": "<<Symbolist[i].symbolName<<" too big "<<Symbolist[i].initialsymbolAddress<<" (max="<<instcount-1<<") assume zero relative"<<'\n';
                    Symbolist[i].symbolAddress = Address-instcount;
                }
            }
            else{
                continue;
            }
        }
        
        noOfModule++;
    }

    for(int st = 0; st<Symbolist.size();st++){
        for(int ul = 0; ul<useList.size();ul++){
            if(Symbolist[st].symbolName.compare(useList[ul].symbolName)==0){
                Symbolist[st].usedOrNot = true;
                break;
            }
            else{
                continue;
            }
        }
    }
    
    
//    for(int ul = 0; ul<useList.size();ul++){
//        Symbol u = useList[ul];
//        int flag = 0;
//        for(int st = 0; st<Symbolist.size();st++){
//            if(Symbolist[st].symbolName.compare(useList[ul].symbolName)==0){
//                flag = 1;
//                break;
//            }
//            else{
//                continue;
//            }
//        }
//        if(flag == 1){
//            continue;
//        }
//        else{
//            u.definedTimes = 0;
//            u.symbolAddress = 0;
//            Symbolist.push_back(u);
//        }
//    }
    return Symbolist;
    
}

std::vector<codeCountPair> Pass2(std::string str,std::vector<Symbol> symbolTable){
    std::vector<Tokens> token1 = getToken(str);
    std::vector<Symbol> Symbolist;
    std::vector<codeCountPair> codePairs;
    int sumOfInst = 0;
    int noOfModule = 1;
    int j = 0; //which Token
    int Address = 0; //the begin of every module
    
    std::vector<Symbol> useList; //新+
    
    while(j<token1.size()-1){
        int defcount = readInt(token1[j]);
        if(defcount>16){
            __parseerror(4,token1[j]);
        }
        j++;
        
        for(int m = 0;m<defcount;m++){
            Symbol s1 = readSymbol(token1[j++]);
            s1.noModule = noOfModule;
            s1.symbolAddress = readInt(token1[j++])+Address;
            s1.usedOrNot = false;
            s1.definedTimes = 1;
            bool notExist = true;
            for(int ti = 0;ti<Symbolist.size();ti++){
                if(s1.symbolName.compare(Symbolist[ti].symbolName)==0){
                    Symbolist[ti].definedTimes++;
                    notExist = false;
                }
                else{
                    continue;
                }
            }
            if(notExist){
                Symbolist.push_back(s1);}
        }
        
        std::vector<Symbol> useList;
        int usecount = readInt(token1[j]);
        if(usecount>16){
            __parseerror(5, token1[j]);
        }
        j++;
        
        
        Symbol s2;
        for(int m = 0; m<usecount;m++){
            s2 = readSymbol(token1[j++]);
            s2.noModule = noOfModule;
            int defnumber = 0;
            for(int k = 0;k<symbolTable.size();k++){
                if(symbolTable[k].symbolName.compare(s2.symbolName)==0){
                    defnumber++;
                }
                else{
                    continue;
                }
            }
            s2.definedTimes = defnumber;
            useList.push_back(s2);
            
        }
        
        int instcount = readInt(token1[j]);
        sumOfInst = sumOfInst + instcount;
        if(sumOfInst>512){
            __parseerror(6, token1[j]);
        }
        j++;
        
        for(int m = 0;m<instcount;m++){
            codeCountPair pair;
            pair.codeType = readIAER(token1[j++]);
            pair.noModule = noOfModule;
            int op = readInt(token1[j++]);
            if(pair.codeType =='I'){
                if(op>=10000){
                    pair.opcode = 9;
                    pair.oprand = 999;
                    pair.errorInfo = " Error: Illegal immediate value; treated as 9999";
                }
                else{
                    pair.opcode = op/1000;
                    pair.oprand = op%1000;
                }

            }
            else if (pair.codeType =='A'){
                pair.opcode = op/1000;
                if(op%1000>512){
                    pair.oprand = 0;
                    pair.errorInfo = " Error: Absolute address exceeds machine size; zero used";
                }
                else{
                    pair.oprand = op%1000;
                }
            }
            else if(pair.codeType =='E'){
                pair.opcode = op/1000;
                int index = op%1000;
                if(index<useList.size()){
                    useList[index].appearedInE = true;
                    if(useList[index].definedTimes == 0){
                        pair.oprand = 0;
                        pair.errorInfo = " Error: "+useList[index].symbolName+" is not defined; zero used";
                        
                    }
                    else{
                        for(int k = 0;k<symbolTable.size();k++){
                            if(symbolTable[k].symbolName.compare(useList[index].symbolName)==0){
                                pair.oprand = symbolTable[k].symbolAddress;
                            }
                            else{
                                continue;
                            }

                    }
                    }
                }
                else{
                    pair.oprand = index;
                    pair.errorInfo=" Error: External address exceeds length of uselist; treated as immediate";
                }
//                std::cout<<"index: "<<index<<" useList:"<<useList[index].symbolName<<'\n';
            }
            else{
                pair.opcode = op/1000;
                if(op%1000<instcount){pair.oprand = op%1000+Address;}
                else{
                    pair.oprand = Address;
                    pair.errorInfo =" Error: Relative address exceeds module size; zero used";
                }
                
            }


            codePairs.push_back(pair);

            
            
        }
        for(int i = 0;i<useList.size();i++){
            useListWarning.push_back(useList[i]);

        }
        Address = Address + instcount;

        noOfModule++;
        
        
    }
    
    
    return codePairs;
}

    

int main(int argc, const char * argv[]) {
    std::string fileLink = "/Users/yinuochang/Desktop/testFormal.txt";
//    if(argc!=2)
//    {
//        std::cout<<" Wrong Inputs \n";
//        exit(0);
//    }
//    std::string fileLink  = argv[1];
    std::vector<Symbol> symbolPass1 = Pass1(fileLink);
//    printSymbolTable(symbolPass1);
    std::vector<codeCountPair> MemoryMap = Pass2(fileLink, symbolPass1);
    printFinalResult(symbolPass1,MemoryMap);


    return 0;
}




