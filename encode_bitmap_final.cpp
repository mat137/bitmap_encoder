#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string>
#include <conio.h>
#include <iostream>
#include <bitset>
#include <fstream>
#include <streambuf>

using namespace std;
#define EOF_MARKER '0'

string ConvertDecimalToBinary(int number)
{
    return bitset<8>(number).to_string();
}

int ConvertBinaryToDecimal(string number)
{
    int result = 0, pow = 1;
    for ( int i = number.length() - 1; i >= 0; --i, pow <<= 1 )
        result += (number[i] - '0') * pow;

    return result;
}

/*	search for dataAddress, dataWidth, dataSize
	it is important not to encode data in header of bitmap
	between 10-14 byte dataAddress is placed (pointer where real data of bitmap begins)
	between 18-22 byte dataWidth is placed (how many data we can encode in one bitmap row. After that width
		there are checksum of row = dataWidth mod 4 - it should not be changed)
	between 34-38 byte dataSize is placed (it should be 8 times bigger than size of the input file)
*/
void ReadBitmapFile(string fileName, int& dataAddress, int& dataWidth, int& dataSize){

	FILE *fileHandler = fopen(fileName.c_str(), "rb");
	if (fileHandler == NULL)
	{
		cout << "File " << fileName << " does not exist";
		exit(-1);
	}
	
	fseek(fileHandler, 10, SEEK_SET);
	//between 10-14 byte dataAddress is placed (pointer where real data of bitmap begins)
	int byte;
	string tmpString = "";
	
	for(int i=0;i<4;i++){
		byte=getc(fileHandler);
		
		tmpString = ConvertDecimalToBinary(byte) + tmpString;
		//cout << tmpString << endl;
	}
	
	dataAddress = ConvertBinaryToDecimal(tmpString);
	

	fseek(fileHandler, 18, SEEK_SET);
	//between 18-22 byte dataWidth is placed (how many data we can encode in one bitmap row. After that width
	//there are checksum of row = dataWidth mod 4 - it should not be changed)
	tmpString = "";

	for(int i=0;i<4;i++){
		byte=getc(fileHandler);
		tmpString = ConvertDecimalToBinary(byte) + tmpString;
	}

	dataWidth = ConvertBinaryToDecimal(tmpString);


	fseek(fileHandler,34,SEEK_SET);
	//between 34-38 byte dataSize is placed (it should be 8 times bigger than size of the input file)
	tmpString = "";

	for(int i=0;i<4;i++){
		byte=getc(fileHandler);
		tmpString = ConvertDecimalToBinary(byte) + tmpString;
	}

	dataSize = ConvertBinaryToDecimal(tmpString);
	
	fclose(fileHandler);
} 
	
//odczytanie z bitmapy i zapisanie do pliku txt
void SaveDecodedTextToFile(string textFileName, string bitmapFileName, int dataWidth, int dataAddress, int dataSize, string key){

	FILE *textFileHandler = fopen(textFileName.c_str(), "wt+");

	FILE *bitmapFileHandler = fopen(bitmapFileName.c_str(), "rb");
	if (bitmapFileHandler == NULL)
	{
		cout << "File " << bitmapFileName << " does not exist";
		exit(-1);
	}

	int checkSumWidth = dataWidth % 4;
	int oryginalDataCounter = 0;

	string tmpByte = "";
	int currentValue, readBytesCounter;
	bool endOfFile = false;

	fseek(bitmapFileHandler, dataAddress, SEEK_SET);

	for(int i=0;i<dataSize;i++){
		currentValue = getc(bitmapFileHandler);
		readBytesCounter++;//zmienna dzieki ktorej nie zczytamy nieznaczacych zer
		
		if(readBytesCounter <= dataWidth){//zeby nie wchodzic na nieznaczace zera
			string tmpStr = ConvertDecimalToBinary(currentValue);
			tmpByte += tmpStr[tmpStr.length() - 1];//zawsze i tak pobieramy siodmy bo najmdlodszy
			//zbieramy najmlodsze bity z pixeli az do 8
			
			if(tmpByte.length() == 8){
				int tmpVal = ConvertBinaryToDecimal(tmpByte);
				tmpByte = "";				
				
			    if(char(tmpVal ^ key[oryginalDataCounter % key.size()]) == EOF_MARKER)//tutaj uzywamy markera jako znak konca tekstu
					endOfFile = true;
				//zeby nie zczytywac "smieci"
				if(!endOfFile){
					cout << char(tmpVal ^ key[oryginalDataCounter % key.size()]);
					fprintf(textFileHandler,"%c", char(tmpVal ^ key[oryginalDataCounter % key.size()]));
				}	
				
				oryginalDataCounter++;				
			}
		}
		
		if (readBytesCounter==dataWidth+checkSumWidth)
			readBytesCounter=0;
	}
	
	fclose(textFileHandler);
	fclose(bitmapFileHandler);
}

// xor
string ReturnEncryptedStringFromFile(string textFileToEncode, string key){

	ifstream textFileStream;
    string result = "";
    // otwieramy plik do odzytu jako stream
    textFileStream.open (textFileToEncode.c_str(), ios::in | ios::binary);
    //czyta strumien pliku i konwertuje na string
    string content((istreambuf_iterator<char>(textFileStream)), istreambuf_iterator<char>());
    //dodajemy marker na koncu
    content += EOF_MARKER;

        
    for(int i=0; i<content.size();i++)
    {    
		result += char(content[i] ^ key[i % key.size()]);
	}
     
    textFileStream.close ();
    //zwraca stringa ktory jest zaxorowanym plikiem
    return result;
}
//tekst xorowany do nowej bitmapy
void CopyEncodedTextToNewBitmap(string newBitmapFileName, string textFileToEncode, string bitmapFileName, int dataWidth, int dataSize, int dataAddress, string key){
	
	int checkSumWidth = dataWidth % 4;
	int value, readBytesCounter = 0;
	
	string textToSave = ReturnEncryptedStringFromFile(textFileToEncode, key);
	
	cout << textToSave;
	//sprawdza czy sie zmiesci tekst w bitmapie.
	if ((textToSave.length())*8>dataSize){ 
		cout << "Text file is too long to encode it in that bitmap file";
		exit(-1);
	}
		
	FILE *newBitmapFileHandler=fopen(newBitmapFileName.c_str(),"wb");//nowa bitmapa

	FILE *bitmapFileHandler = fopen(bitmapFileName.c_str(), "rb");//stara bitmapa
	if (bitmapFileHandler == NULL)
	{
		cout << "File " << bitmapFileName << " does not exist";
		exit(-1);
	}

//kopiowanie
	for(int i=0;i<dataAddress;i++){
		putc(getc(bitmapFileHandler),newBitmapFileHandler);
	}
	//zmienna ktora zawierac bedzie to co trzeba zakodowac w bitmapie
	string valueToEncode = "";
	int currentValue = 0, textIterator = 0;
	
	bool endOfFile = false;
	
	for(int i=0;i<dataSize;i++){
		
		//na poczatku valueToEncode pusty wiec wchodzimy do srodka i pobieramy w nim pierwszy znak z tekstu do zakodowania
		//(potem bedziemy pobierali kolejne ale na poczatku pobbieramy pierwszy)
		//jak go pobierzemy to konwertujemy go binarnie
		if(valueToEncode.length() == 0){
			currentValue = 0;
			valueToEncode = "";
			if (readBytesCounter<dataWidth)
			{
				if(textToSave.length() > textIterator){
					currentValue=textToSave[textIterator];
					textIterator++;
				}
				else{
					endOfFile = true;
				}
			}
				
			valueToEncode = ConvertDecimalToBinary(currentValue);
		}

		//tutaj czytamy element bitmapy, do ktorego chcemy przepisac do najmniej znaczacego bitu to co jest w valueToEncode
		//ale valueToEncode posiada 8 elementow (zakodowany binarnie znak), wiec musimy pobierac 8 razy znak z bitmapy i 8 razy zapisywac
		//element z valueToEncode. Dopeiro wtedy mozemy pobrac kolejny element z tekstu do zapsiania
		value=getc(bitmapFileHandler);
		readBytesCounter++;
		
		if ((readBytesCounter<=dataWidth) && !endOfFile)
		{
			//jesli nie weszlismy w zera na koncu wiersza ani nie skonczylismy pliku to konwertujemy przeczytany z bitmapy znak binarnie
			//i potem w jego najmniej znaczacy bit zapisujemy PIERWSZY element ze stringa valueToEncode(w ktorym jest 8 bitow zakodowanego znaku ze stringa ktorychcemy zaodowac)
			//kasujemy pierwszy element ze stringa valueToEncode (juz teraz valueToEncode ma 7 lub w najtepnych krokach mniejsza dlugosc) bo juz zapisalismy ta wartosc
			string orgValueFromBitmap = ConvertDecimalToBinary(value);
			orgValueFromBitmap[7] = valueToEncode[0];
			valueToEncode.erase(0,1);
			
			putc(ConvertBinaryToDecimal(orgValueFromBitmap), newBitmapFileHandler);
		} else {	
			
			//jesli plik tekstowy do zakodowania w bitmapie sie skonczyl to rpzepisujemy reszte bitmapy tak jak oryginalnie byla
			putc(value,newBitmapFileHandler);
		}
		
		if (readBytesCounter==dataWidth+checkSumWidth)
			readBytesCounter=0;
			
		
	}

	fclose(newBitmapFileHandler);
	fclose(bitmapFileHandler);
}




//-----------------------------------------------

int main(void){
	int choice, dataAddress, dataWidth, dataSize;
	string inBitmapName, outBitmapName, textFileName, key;

	cout << "What do you want to do with file?" << endl;
	cout << "1. Hide text" << endl << "2. Restore text" << endl;

	cin >> choice;

	if (choice==1){
		cout << "Type name of the file with input bitmap: " << endl;
		cin >> inBitmapName;
		
		cout << "Type name of the file with input text: " << endl;
		cin >> textFileName;
		
		cout << "Type name of the file with output bitmap: " << endl;
		cin >> outBitmapName;
		
		cout << "Type key value to encrypt the file: " << endl;
		cin >> key;


		ReadBitmapFile(inBitmapName, dataAddress, dataWidth, dataSize);
		
		//cout << "ADDRESS: " << dataAddress << endl << "WIDTH: " << dataWidth << endl << "SIZE: " << dataSize << endl;

		CopyEncodedTextToNewBitmap(outBitmapName, textFileName, inBitmapName, dataWidth, dataSize, dataAddress, key);
		
	} 
	else	
	{
		cout << "Type name of the file with input bitmap: " << endl;
		cin >> inBitmapName;
		
		cout << "Type name of the file with output text: " << endl;
		cin >> textFileName;
		
		cout << "Type key value to decrypt the file: " << endl;
		cin >> key;
		
		ReadBitmapFile(inBitmapName, dataAddress, dataWidth, dataSize);
		
		SaveDecodedTextToFile(textFileName, inBitmapName, dataWidth, dataAddress, dataSize, key);
	}
	
	return 0;
}
