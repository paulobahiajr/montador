/* Trabalho 1 de Software Básico
Autores:  Henrique de Sousa e Silva Balbino - 11/0061390
          Paulo Bahia - 10/

Descrição: Este trabalho tem como função processar e analisar um código em Assembly inventado,
para então sintetizar seu respectivo código objeto caso nenhum erro seja encontrado.
O código é dividido em três grandes partes. Preprocessamento, processamento de MACROs e montagem.
As partes que serão executadas dependem de parâmetros na chamada do programa.
Cada uma dessas partes gera um arquivo de saída correspondente.
A parte de preprocessamento é responsável por remover os comentários e processar os IFs e EQUs,
mantendo as instruções com EQU 1 e removendo as com EQU 0.
A parte de processamento de MACROs insere o código na definição da MACRO no corpo do programa
(SECTION TEXT) substituindo os respectivos argumentos de entrada.
A parte de montagem é responsável por analisar o código após ser preprocessado e com as MACROs
inseridas, procurando por possíveis erros léxicos, sintáticos e semânticos. Após feita a análise,
caso nenhum erro tenha sido encontrado, ocorre a montagem que gera o código objeto a partir dos
mnemônicos das instruções em Assembly.

Para auxiliar a execução do código, foram criadas algumas funções e structs.
A struct equ, que contem uma label e seu valor.
A struct macro, que contem uma label, as variáveis e o código.
A struct token, que contem um nome, um valor para indicar seu tipo e seu endereço no código.
As funções getTokens e Tokenize são responsáeis por separar o código todo em uma string em várias
strings menores que são usadas como tokens. Cada token pode representar um número, uma label, uma
instrução ou um argumento.
As funções intToStr e uintToStr recebem um número inteiro e o transforma em uma string cujos caracteres
são os algarismos do número.
A função hexToInt recebe uma string com os dígitos de um número hexadecimal e retorna uma variável int com
seu valor decimal correspondente.
As funções insertLines, removeLines, removeStoreLines e restoreLines são responsáveis por inserirem e
removerem os números das linhas do código no final de cada linha da string do código. Os números das
linhas são necessários para identificar onde ocorreram os erros do programa.
A função getCurrentLine encontra a linha no código original para uma dada posição no estado atual do código
A função codeGenerator gera o código objeto a partir do código Assembly processado.
*/

#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <fstream>
#include <cmath>
#include <cstdlib>

using namespace std;

#define HEADER       0  // Defines para indicar a seção do código
#define SECTION_TEXT 1
#define SECTION_DATA 2

#define NOT_DEFINDED  0  // Defines para indicar o tipo do token
#define DIRECTIVE     1
#define LABEL         2
#define INSTRUCTION   3
#define OPERAND       4
#define DATA          5

struct equ  // Struct que contem as informações de uma EQU
{
	string label;
	int value;
};

struct macro  // Struct que contem as informações de uma MACRO
{
	string label, code;
	vector<string> variables;
};

struct Token  // Struct que contem as informações de um token
{
  string name;
  int address, type;
};

struct TokensList
{
    string label;
    string value;
    int address;
};

void Tokenize ( const string&, vector<Token>&, const string& );  // Obtem os tokens de uma linha
void getTokens ( const string&, vector< vector<Token> >&, const string& );  // Separa cada linha do código para obter os tokens
int hexToInt( string );  // Converte uma string com os dígitos de um número hexadecimal em um int decimal
string intToStr ( int );  // Converte um inteiro para uma string com seus algarismos
string uintToStr ( uint );  // Converte um inteiro sem sinal para uma string com seus algarismos
void insertLines ( string & );  // Insere os números das linhas no final de cada linha da string
void removeLines ( string & );  // Remove os números das linhas inseridos no final de cada linha da string
void removeStoreLines ( string &, vector<uint> & );  // Remove os números das linhas inseridos no final de cada linha da string e armazena os números em um vector
void restoreLines ( string &, vector<uint> & );  // Restaura os números das linhas no final de cada linha da string a partir de um vector
uint getCurrentLine ( uint, string );  // Retorna a linha atual da string do código baseado na posição atual do iterador
bool isInLabelTable ( vector<Token>, Token );  // Retorna se o Token está na tabela de labels
int getLabel ( vector< vector<Token> >, string );  // Retorna o token para onde a label inseridda aponta

void codeGenerator ( string&, vector<vector<Token> >& );  // Gera o código objeto a partir do código em Assembly
void displayMatrix ( vector< vector<Token> > tokens );  // Mostra os tokens
int acharOpCode ( string& ,int&, int& );  // Identifica o opcode correspondente ao nome da instrução
void displayVet ( vector<uint> );
void displayStructTokensList ( vector<struct TokensList> );
void writeOnObjectFile( fstream&, vector< vector<Token> >& );
bool is_number( const string& );

int main ( int argc, char* argv[] )
{
  if ( argc < 2 || (strcmp(argv[1], "-p") && strcmp(argv[1], "-o")) )
  {
    cout << "ERRO! O argumento utilizado não indica nenhuma operação válida. O programa será encerrado!" << endl;
    return 0;
  }

  if ( argc < 3 || (!strstr(argv[2], ".asm") && !strstr(argv[2], ".pre")) )
  {
    cout << "ERRO! Nome do arquivo de entrada inválido. O programa será encerrado!" << endl;
    return 0;
  }

  if ( argc > 5 )
  {
    cout << "ERRO! Número de arquivos maior que o permitido. O programa será encerrado!" << endl;
    return 0; 
  }

  for ( uint fileNumber = 0; fileNumber < (uint) argc - 2; fileNumber++ )
  {
    fstream input ( argv[fileNumber + 2], fstream::in );  // Abre o arquivo de entrada

    if ( !(input.is_open()) )
    {
      cout << "ERRO! Arquivo de entrada inexistente. O programa será encerrado!" << endl;
      return 0;
    }

    input.seekg ( 0, input.end );  // Encontra a quantidade de caracteres dentro do arquivo
    int inputLength = input.tellg();
    input.seekg ( 0, input.beg );

    char *buffer = new char [inputLength];  // Armazena todo o conteúdo do arquivo em uma string
    input.read ( buffer, inputLength );
    input.seekg ( 0, input.beg );
    input.close();
    string inputString, original ( buffer );
    inputString = original;
    delete[] buffer;

    fstream output;

    for ( uint i = 0; i < inputString.size(); i++ )
    {
      if ( inputString[i] >= 'a' && inputString[i] <= 'z' )
        inputString[i] = inputString[i] + 'A' - 'a';
    }

    uint strPos = 0;  // Posição na string que contem o código Assembly vindo do arquivo de entrada
    uint aux/*, aux2*/;  // Contadores auxiliares para navegar na string que contem o código Assembly vindo do arquivo de entrada
    uint section = HEADER;  // Seção do código, HEADER = 0, SECTION TEXT = 1, SECTION DATA = 2
    uint existBegin = 0, existEnd = 0;  // Indicam a existência das diretivas BEGIN e END
    int text = -1, data = -1;  // Posição na string que contém o código Assembly de onde começa as seções text e data
    bool erased;  // Indica se houve uso da função erase
    string label;  // Armazena o nome de uma label
    string variable;   // Armazena o nome de uma variável de uma macro
    string macroCode;  // Armazena o código de uma MACRO antes de inserir no código Assembly
    vector<string> macroVariables;
    vector<equ> equs;	 // Contem todas as EQUs do programa
    vector<macro> macros;  // Contem todas as MACROs do programa
    vector<uint> codeLines;  // Contem os números das linhas do código
    vector< vector<Token> > tokens;  // Contem os tokens de cada linha do código
    vector<Token> labelTable;  // Contém a tabela de labels
    vector<string> errors;  // Contem os erros que impediram a montagem do programa

    if ( !strcmp ( argv[1], "-p" ) || !strcmp ( argv[1], "-o" ) )
    {
      cout << "Executando preprocessamento" << endl;

      insertLines ( inputString );

      while ( strPos < inputString.size() )
      {
        erased = false;
        if ( strPos < inputString.size() - 13
        && inputString[strPos] ==    'S'
        && inputString[strPos+1] ==  'E'
        && inputString[strPos+2] ==  'C'
        && inputString[strPos+3] ==  'T'
        && inputString[strPos+4] ==  'I'
        && inputString[strPos+5] ==  'O'
        && inputString[strPos+6] ==  'N'
        && inputString[strPos+7] ==  ' '
        && inputString[strPos+8] ==  'T'
        && inputString[strPos+9] ==  'E'
        && inputString[strPos+10] == 'X'
        && inputString[strPos+11] == 'T' )  // Verifica se chegou na SECTION TEXT e armazena onde ela começa
        {
          if ( section == SECTION_DATA )
            errors.push_back("Erro semântico. SECTION DATA antes de SECTION TEXT.");

          section = SECTION_TEXT;
          text = strPos + 13;
        }

        else if ( strPos < inputString.size() - 13
        && inputString[strPos] ==    'S'
        && inputString[strPos+1] ==  'E'
        && inputString[strPos+2] ==  'C'
        && inputString[strPos+3] ==  'T'
        && inputString[strPos+4] ==  'I'
        && inputString[strPos+5] ==  'O'
        && inputString[strPos+6] ==  'N'
        && inputString[strPos+7] ==  ' '
        && inputString[strPos+8] ==  'D'
        && inputString[strPos+9] ==  'A'
        && inputString[strPos+10] == 'T'
        && inputString[strPos+11] == 'A' )  // Verifica se chegou na SECTION DATA e armazena onde ela começa
        {
          section = SECTION_DATA;
          data = strPos + 13;
        }

      	if ( inputString[strPos] == ';' )  // Remove os comentários a partir do ; até o final da linha ou do conteúdo
        {
          do
          {
            inputString.erase ( inputString.begin() + strPos );
          } while( inputString[strPos] != '\n' && inputString[strPos] != '/' && strPos < inputString.size() );
        }

        if ( strPos < inputString.size() - 3 && inputString[strPos] == 'E'  && inputString[strPos+1] == 'Q' && inputString[strPos+2] == 'U' )  // Verifica se tem um EQU
        {
        	aux = strPos;
        	equs.push_back ( equ() );

        	while ( aux > 0 && inputString[aux] != '\n' )  // Recua até o começo da linha ou do arquivo
        		aux--;

        	if ( inputString[aux] == '\n' )	 // Pula o \n
        		aux++;

        	while ( inputString[aux] != ':' )  // Identifica a label do EQU e armazena no vector
        	{
        		if ( inputString[aux] != ' ' || inputString[aux] != '\t' )  // Ignora os espaços
        			equs.back().label += inputString[aux];
        		aux++;
        	}

        	if ( section != HEADER )
          {
            errors.push_back( "Erro de preprocessamento na linha " + uintToStr ( getCurrentLine ( strPos, inputString ) ) + ". EQU \'" + equs.back().label + "\' fora do cabeçalho." );
          }

        	while ( inputString[strPos+3] == ' ' || inputString[strPos+3] == '\t' )
        		strPos++;

        	if ( inputString[strPos+3] == '0' )	 // Identifica o valor do EQU e armazena no vector
        		equs.back().value = 0;
        	else
        		equs.back().value = 1;

        	while ( strPos > 0 && inputString[--strPos] != '\n' ) ;  // Retorna ao início da linha

        	if ( inputString[strPos] == '\n' )
        		strPos++;

        	while ( strPos < inputString.size() && inputString[strPos] != '\n' )  // Apaga a linha do EQU
        		inputString.erase ( inputString.begin() + strPos );
        	if ( strPos < inputString.size() && inputString[strPos] == '\n' )
  				{
            inputString.erase ( inputString.begin() + strPos );
            erased = true;
          }
        }

        if ( strPos < inputString.size() - 2 && inputString[strPos] == 'I'  && inputString[strPos+1] == 'F' )  // Verifica se tem um IF
        {
        	uint labelFlag = 0;
        	aux = strPos + 2;
        	label.clear();

        	while ( aux < inputString.size() && (inputString[aux] == ' ' || inputString[aux] == '\t') )  // Pula os espaços
        		aux++;

  				while ( aux < inputString.size() && inputString[aux] != ' ' && inputString[aux] != '\t' && inputString[aux] != '\n' && inputString[aux] != '\r' && inputString[aux] != '/' )  // Armazena a label do IF
  					label += inputString[aux++];

  				for ( uint i = 0; i < equs.size(); i++ )  // Processa os IFs, comparando o label atual com os armazenados com EQU
  				{
  					if ( !equs[i].label.compare(label) )
  					{
  						while ( strPos > 0 && inputString[--strPos] != '\n' ) ;

  						strPos++;

  						while (  strPos < inputString.size() && inputString[strPos] != '\n' )  // Apaga a linha do IF
  							inputString.erase ( inputString.begin() + strPos );
  						if (  strPos < inputString.size() && inputString[strPos] == '\n' )
  							inputString.erase ( inputString.begin() + strPos );

  						if ( !equs[i].value )
  						{
  							while (  strPos < inputString.size() && inputString[strPos] != '\n' )  // Apaga a linha após o IF se o valor for 0
  								inputString.erase ( inputString.begin() + strPos );
  							if (  strPos < inputString.size() && inputString[strPos] == '\n' )
  							{
  								inputString.erase ( inputString.begin() + strPos );
  								erased = true;
  							}
  						}
  						labelFlag = 1;
  					}
  				}

  				if ( labelFlag == 0 )
  				{
  					errors.push_back ( "Erro de preprocessamento na linha " + uintToStr ( getCurrentLine ( strPos, inputString ) ) + ". Label \'" + label + "\' utilizada no IF não existe." );
  				}
        }

        if ( !erased )
          strPos++;
      }

      if ( text < 0 )
        errors.push_back ( "Erro semântico. SECTION TEXT inexistente" );
      if ( data < 0 )
        errors.push_back ( "Erro semântico. SECTION DATA inexistente" );

      equs.clear();

      buffer = new char [strlen ( argv[fileNumber + 2] )];
      strncpy ( buffer, argv[fileNumber + 2], strlen( argv[fileNumber + 2] ) - 3 ); // "nome_do_arquivo.pre"
      buffer[strlen ( argv[fileNumber + 2] ) - 3]  = 'p';
      buffer[strlen ( argv[fileNumber + 2] ) - 2]  = 'r';
      buffer[strlen ( argv[fileNumber + 2] ) - 1]  = 'e';
      buffer[strlen ( argv[fileNumber + 2] )]     = '\0';

      removeStoreLines ( inputString, codeLines );
      output.open ( buffer, fstream::out | fstream::trunc );
  		output.write ( inputString.c_str(), inputString.size() );
  		output.close();
  		delete[] buffer;

      restoreLines ( inputString, codeLines );
    }

    strPos = 0;
    section = HEADER;

    /*if ( !strcmp ( argv[1], "-m" ) || !strcmp ( argv[1], "-o" ) )
    {
    	cout << "Executando processamento de macros" << endl;

      uint macroInserted = 1;
      text = data = 0;

    	while ( strPos < inputString.size() )
    	{
    	  if ( strPos == 0 )
          macroInserted = 0;

        if ( strPos < inputString.size() - 13
        && inputString[strPos] ==    'S'
        && inputString[strPos+1] ==  'E'
        && inputString[strPos+2] ==  'C'
        && inputString[strPos+3] ==  'T'
        && inputString[strPos+4] ==  'I'
        && inputString[strPos+5] ==  'O'
        && inputString[strPos+6] ==  'N'
        && inputString[strPos+7] ==  ' '
        && inputString[strPos+8] ==  'T'
        && inputString[strPos+9] ==  'E'
        && inputString[strPos+10] == 'X'
        && inputString[strPos+11] == 'T' )  // Verifica se chegou na SECTION TEXT e armazena onde ela começa
        {
          section = SECTION_TEXT;
          text = strPos + 13;
        }

        else if ( strPos < inputString.size() - 13
        && inputString[strPos] ==    'S'
        && inputString[strPos+1] ==  'E'
        && inputString[strPos+2] ==  'C'
        && inputString[strPos+3] ==  'T'
        && inputString[strPos+4] ==  'I'
        && inputString[strPos+5] ==  'O'
        && inputString[strPos+6] ==  'N'
        && inputString[strPos+7] ==  ' '
        && inputString[strPos+8] ==  'D'
        && inputString[strPos+9] ==  'A'
        && inputString[strPos+10] == 'T'
        && inputString[strPos+11] == 'A' )  // Verifica se chegou na SECTION DATA e armazena onde ela começa
        {
          section = SECTION_DATA;
          data = strPos + 13;
        }

    		if ( strPos > 0 && strPos < inputString.size() - 5
    		&& inputString[strPos] ==   'M'
    		&& inputString[strPos+1] == 'A'
    		&& inputString[strPos+2] == 'C'
    		&& inputString[strPos+3] == 'R'
    		&& inputString[strPos+4] == 'O'
    		&& inputString[strPos-1] != 'D' )  // Verifica se tem uma MACRO e não é ENDMACRO
    		{
    			variable.clear();
    			macros.push_back ( macro() );
    			aux = strPos;
    			strPos += 5;

    			while ( strPos < inputString.size() && (inputString[strPos] == ' ' || inputString[strPos] == '\t') ) // Pula os espaços
    				strPos++;

    			while ( strPos < inputString.size() && inputString[strPos] != '\n' )  // Armazena todas as variáveis da MACRO
    			{
    				if ( inputString[strPos] == '&' )
    				{
              strPos++;
    					while ( strPos < inputString.size() && inputString[strPos] != ' '  && inputString[strPos] != '\t' && inputString[strPos] != '\r' && inputString[strPos] != '\n' && inputString[strPos] != '/' )
    						variable += inputString[strPos++];

    					macros.back().variables.push_back ( variable );
    				}
            strPos++;
    				variable.clear();
    			}

    			if ( inputString[strPos] == '\n' )
    				strPos++;

    			while ( aux > 0 && inputString[--aux] != '\n' ) ;  // Retorna ao início da linha

    			if ( inputString[aux] == '\n')
    				aux++;

          aux2 = aux;

    			while ( inputString[aux] != ':' )  // Identifica a label da MACRO e armazena no vector
        	{
        		if ( inputString[aux] != ' ' || inputString[aux] != '\t' )  // Ignora os espaços
        			macros.back().label += inputString[aux];
        		aux++;
        	}

        	while ( strPos < inputString.size() - 9
      		&& (inputString[strPos+1]!= 'E'
      		|| inputString[strPos+2] != 'N'
      		|| inputString[strPos+3] != 'D'
      		|| inputString[strPos+4] != 'M'
      		|| inputString[strPos+5] != 'A'
      		|| inputString[strPos+6] != 'C'
      		|| inputString[strPos+7] != 'R'
      		|| inputString[strPos+8] != 'O') )  // Enquanto não for ENDMACRO, insere o código na MACRO enquanto remove a definição da MACRO no código
        	{
        		macros.back().code += inputString[strPos];
        		inputString.erase ( inputString.begin() + strPos );
        	}

          inputString.erase ( inputString.begin() + strPos );  // Remove ENDMACRO
          while (inputString[strPos] != '\n')
            inputString.erase ( inputString.begin() + strPos );
          inputString.erase ( inputString.begin() + strPos );

          while ( aux2 < inputString.size() && inputString[aux2] != '\n' )  // Remove label da MACRO
          {
            inputString.erase( inputString.begin() + aux2 );
            strPos--;
          }

          if ( inputString[aux2] == '\n' )
          {
            inputString.erase( inputString.begin() + aux2 );
            strPos--;
          }

          strPos--;
    		}

        if ( strPos < inputString.size() - 1 && (inputString[strPos] == '\n' || inputString[strPos] == ':') && section != HEADER )  // Verifica se tem uma MACRO na linha atual
        {
          strPos++;
          label.clear();

          while ( strPos < inputString.size() && (inputString[strPos] == ' ' || inputString[strPos] == '\t' || inputString[strPos] == '\r') )  // Pula os espaços
            strPos++;

          while ( strPos < inputString.size() && inputString[strPos] != ' ' && inputString[strPos] != '\t' && inputString[strPos] != '\n' && inputString[strPos] != '\r' )
            label += inputString[strPos++];

          for ( uint i = 0; i < macros.size(); i++ )
          {
            if ( !macros[i].label.compare(label) )  // Compara a label atual com as labels das MACROs
            {
              macroInserted = 1;
              if ( section == SECTION_DATA )
              {
                errors.push_back( "Erro de MACRO na linha " + uintToStr ( getCurrentLine ( strPos, inputString ) ) + ". MACRO \'" + label + "\' utilizada na SECTION DATA. O programa será encerrado!" );
              }

              while ( strPos < inputString.size() && (inputString[strPos] == ' ' || inputString[strPos] == '\t') )  // Pula os espaços
                strPos++;

              while ( strPos < inputString.size() && inputString[strPos] != '\n' && inputString[strPos] != '/' )  // Armazena todas as variáveis da MACRO
              {

                while ( strPos < inputString.size() && inputString[strPos] != ' '  && inputString[strPos] != '\t' && inputString[strPos] != '\r' && inputString[strPos] != '\n' && inputString[strPos] != '/' )
                  variable += inputString[strPos++];

                macroVariables.push_back ( variable );
                variable.clear();

                if ( inputString[strPos] == '\n' )
                  break;

                strPos++;
              }

              while ( strPos > 0 && inputString[--strPos] != '\n' && inputString[strPos] != ':' ) ;  // Volta pro começo da linha

              if ( inputString[strPos]== '\n' || inputString[strPos] == ':' )
                strPos++;

              while ( strPos < inputString.size() && inputString[strPos] != '\n' )  // Apaga a linha
                inputString.erase ( inputString.begin() + strPos );
              macroCode = macros[i].code;

              aux = 1;

              for (uint j = 0; j < macros[i].variables.size() ; j++ )  // Substitui as variáveis da MACRO
              {
                while ( (aux = macroCode.find( macros[i].variables[j].c_str(), aux, macros[i].variables[j].size() ) + 1) != 0 )
                {
                  aux--;
                  macroCode.replace ( aux, macros[i].variables[j].size(), macroVariables[j] );
                  aux += macroVariables[j].size();
                  if (aux >= macroCode.size())
                    break;
                }
              }

              inputString.insert ( strPos, macroCode );  // Insere a MACRO no código Assembly
              macroCode.clear();
            }
          }
        }

        strPos++;

        if ( strPos >= inputString.size() && macroInserted )
        {
          strPos = text;
          macroInserted = 0;
        }
    	}

    	for ( uint i = 0; i < macros.size(); i++ )
    	{
    		cout << endl << macros[i].label;
    		for ( uint j = 0; j < macros[i].variables.size(); j++ )
    			cout << " " << macros[i].variables[j];
    		cout << endl << macros[i].code << endl;
    	}

      buffer = new char [strlen ( argv[3] ) + 4];
      strcpy ( buffer, argv[3] );
      buffer[strlen ( argv[3] )]  = '.';
      buffer[strlen ( argv[3] ) + 1]  = 'm';
      buffer[strlen ( argv[3] ) + 2]  = 'c';
      buffer[strlen ( argv[3] ) + 3]  = 'r';
      buffer[strlen ( argv[3] ) + 4]  = '\0';

      codeLines.clear();
      removeStoreLines ( inputString, codeLines );
      output.open ( buffer, fstream::out | fstream::trunc );  // Salva o código com as macros processadas no arquivo com extensão .mcr
  		output.write ( inputString.c_str(), inputString.size() );
  		output.close();
      restoreLines ( inputString, codeLines );

      delete[] buffer;
    }*/

    if ( !strcmp ( argv[1], "-o" ) )
    {
      cout << "Executando montagem" << endl;

      uint currentAddress = 0;  // Variável para indicar o endereço atual ao percorrer os tokens
      uint labelLine;  // Variável para identificar se na linha atual foi colocada uma label
      uint invalidToken;  // Variável para identificar se um token é inválido
      section = HEADER;


      codeLines.clear();
      getTokens ( inputString, tokens, " \t\r/" );  // Separa a string toda do código em várias strings tokens com os separadores definidos


      cout << "Executando análise léxica" << endl;

      for ( uint i = 0; i < tokens.size(); i++ )  // Percorre todos os tokens de todas as linhas do código
      {
        labelLine = 0;
        codeLines.push_back ( atoi ( tokens[i].back().name.c_str() ) );
        tokens[i].pop_back();
        
        while ( i < tokens.size() - 1 && tokens[i].size() == 0 )
        {
          tokens.erase( tokens.begin() + i );
          codeLines.push_back ( atoi ( tokens[i].back().name.c_str() ) );
          tokens[i].pop_back();
        }

        for ( uint j = 0; j < tokens[i].size(); j++ )
        {
          invalidToken = 0;
          if ( !tokens[i][j].name.compare ( "SECTION" ) )  // Identifica SECTION TEXT e DATA
          {
            tokens[i][j].type = DIRECTIVE;
            if ( !tokens[i][j + 1].name.compare ( "TEXT" ) )
            {
              section = SECTION_TEXT;
              tokens[i][j + 1].type = DIRECTIVE;
            }
            if ( !tokens[i][j + 1].name.compare ( "DATA" ) )
            {
              section = SECTION_DATA;
              tokens[i][j + 1].type = DIRECTIVE;
            }
            j++;
          }

          else if ( !tokens[i][j + 1].name.compare ( "BEGIN" ) || !tokens[i][j + 1].name.compare ( "END" ) )  // Identifica BEGIN e END
          {
            if ( !tokens[i][j + 1].name.compare ( "BEGIN" ) )
              existBegin = 1;
            else if ( !tokens[i][j + 1].name.compare ( "END" ) )
            {
              existEnd = 1;
              if ( !existBegin )
                errors.push_back ( "Erro semântico na linha " + uintToStr( codeLines[i] ) + ". Diretiva END antes da existência de diretiva BEGIN." );
            }
            tokens[i][j + 1].type = DIRECTIVE;
            j++;
          }

          else
          {
            if ( tokens[i][j].name[tokens[i][j].name.size() - 1] == ':' )  // Identifica o token como label e armazena na tabela de labels
            {
              tokens[i][j].type = LABEL;
              labelTable.push_back ( tokens[i][j] );
              labelTable.back().address = currentAddress;
              labelLine = 1;
            }
            else if ( ((labelLine == 0 && j == 0) || (labelLine == 1 && j == 1)) && section == SECTION_TEXT )  // Identifica o token como instrução
            {
              tokens[i][j].type = INSTRUCTION;
              tokens[i][j].address = currentAddress;
              currentAddress++;
            }
            else if ( ((labelLine == 0 && j > 0) || (labelLine == 1 && j > 1)) && section == SECTION_TEXT )  // Identifica o token como operando
            {
              tokens[i][j].type = OPERAND;
              tokens[i][j].address = currentAddress;
              currentAddress++;
            }
            else if ( !tokens[i][j].name.compare( "CONST" ) && section == SECTION_DATA )  // Identifica o token como constante
            {
              tokens[i][j].type = DATA;
              tokens[i][j].address = currentAddress;
              if ( j < tokens[i].size() - 1 )
              {
                tokens[i][j + 1].type = DATA;
                tokens[i][j + 1].address = currentAddress;
                if ( tokens[i][j + 1].name.size() > 3 &&
                   ((tokens[i][j + 1].name[0] == '0' && tokens[i][j + 1].name[1] == 'X' && is_number( tokens[i][j + 1].name.substr( 2, tokens[i][j + 1].name.size() - 1 ) ) ) ||
                   ( tokens[i][j + 1].name[0] == '-' && tokens[i][j + 1].name[1] == '0' && tokens[i][j + 1].name[1] == 'X' && is_number( tokens[i][j + 1].name.substr( 3, tokens[i][j + 1].name.size() - 1 ) ) ) ) )
                  tokens[i][j + 1].name = intToStr ( hexToInt ( tokens[i][j + 1].name ) );  // Veriica se é hexadecimal e converte para decimal
                else if ( atoi ( tokens[i][j + 1].name.c_str() ) == 0 && (!tokens[i][j + 1].name.compare( "0" ) || !tokens[i][j + 1].name.compare( "00" )) )
                  invalidToken = 1;

              }
              currentAddress++;
            }
            else if ( !tokens[i][j].name.compare( "SPACE" ) && section == SECTION_DATA )  // Identifica o token como variável e verifica a quantidade de espaços
            {
              tokens[i][j].address = currentAddress;
              tokens[i][j].type = DATA;
              if ( j == tokens[i].size() - 1 )
              {
                currentAddress++;
              }
              else
              {
                tokens[i][j + 1].type = DATA;
                if ( tokens[i][j + 1].name.size() > 1 && tokens[i][j + 1].name[0] == '0' &&  tokens[i][j + 1].name[1] == 'X' && is_number( tokens[i][j + 1].name.substr( 2, tokens[i][j + 1].name.size() - 1 ) ) ) 
                {
                  currentAddress += hexToInt ( tokens[i][j + 1].name );  // Verifica se é hexadecimal e converte para decimal
                  tokens[i][j + 1].name = intToStr ( hexToInt ( tokens[i][j + 1].name ) );
                }
                else if ( atoi ( tokens[i][j + 1].name.c_str() ) == 0 && (!tokens[i][j + 1].name.compare( "0" ) || !tokens[i][j + 1].name.compare( "00" )) )
                  invalidToken = 1;
                else
                  currentAddress += atoi ( tokens[i][j + 1].name.c_str() );
                j++;
              }
            }

            if ( !(!(tokens[i][j].name[0] < '0' || tokens[i][j].name[0] > '9')  // Verifica se o token é válido
            && tokens[i][j].type == DATA) && (tokens[i][j].name[0] != '_'
            && (tokens[i][j].name[0] < 'A' || tokens[i][j].name[0] > 'Z')) )
            {
              if ( !tokens[i][j].name.compare("2") ) cout << tokens[i][j].type << endl;
              invalidToken = 1;
            }
            for ( uint k = 1; k < tokens[i][j].name.size(); k++ )
            {
              if ( tokens[i][j].name[k] != '_' && tokens[i][j].name[k] != ':'
              && ((tokens[i][j].name[k] < 'A' || tokens[i][j].name[k] > 'Z'))
              &&  (tokens[i][j].name[k] < '0' || tokens[i][j].name[k] > '9')
              &&   tokens[i][j].name[k] != ',' )
                invalidToken = 1;
            }
            if ( invalidToken == 1 )
            {
              errors.push_back ( "Erro léxico na linha " + uintToStr( codeLines[i] ) + ". Token \'" + tokens[i][j].name + "\' com nome inválido." );
            }
          }
        }

        if ( tokens[i].size() == 0 )
          tokens.erase ( tokens.begin() + i );
      }

      cout << "Executando análise sintática" << endl;

      int instrOperands, opcode, aLabel;
      section = HEADER;
      for ( uint i = 0; i < tokens.size(); i++ )
      {
        for ( uint j = 0; j < tokens[i].size(); j++ )
        {
          // Diretiva SECTION Inválida
          if ( !tokens[i][j].name.compare ( "SECTION" ) && (tokens[i].size() < 2 || (tokens[i][j + 1].name.compare ( "TEXT" ) && tokens[i][j + 1].name.compare ( "DATA" ))) )
            errors.push_back ( "Erro sintático na linha " + uintToStr ( codeLines[i] ) + ". Diretiva SECTION que não é nem TEXT e nem DATA." );

          else if ( tokens[i].size() >= 2 && !tokens[i][j].name.compare ( "SECTION" ) && !tokens[i][j + 1].name.compare ( "TEXT" ) )
            section = SECTION_TEXT;

          else if ( tokens[i].size() >= 2 && !tokens[i][j].name.compare ( "SECTION" ) && !tokens[i][j + 1].name.compare ( "DATA" ) )
            section = SECTION_DATA;

          if ( tokens[i][j].type == INSTRUCTION )
          {
            aLabel = -1;
            if ( tokens[i].size() >= 2 )
              aLabel = getLabel(tokens,tokens[i][j + 1].name);

            // Instrução Inválida
            if ( acharOpCode(tokens[i][j].name, opcode, instrOperands ) )
              errors.push_back ( "Erro sintático na linha " + uintToStr ( codeLines[i] ) + ". A instrução \'" + tokens[i][j].name + "\' não é uma instrução válida." );

            // Instruções com quantidade de operando inválidas
            else if ( (tokens[i].size() - j) != (uint)instrOperands )
              errors.push_back ( "Erro sintático na linha " + uintToStr ( codeLines[i] ) + ". A instrução \'" + tokens[i][j].name + "\' não possui a quantidade válida de operandos." );

            // Tipo de Argumento Inválido
            else if ( (!tokens[i][j].name.compare( "ADD" ) || !tokens[i][j].name.compare( "SUB" ) || !tokens[i][j].name.compare( "MULT" )
            || !tokens[i][j].name.compare( "DIV" ) || !tokens[i][j].name.compare( "LOAD" ) || !tokens[i][j].name.compare( "STORE" )
            || !tokens[i][j].name.compare( "INPUT" ) || !tokens[i][j].name.compare( "OUTPUT" ) || !tokens[i][j].name.compare( "STORE" ))
            && aLabel != -1 && tokens[aLabel/10][aLabel%10].type != DATA )
            errors.push_back ( "Erro sintático na linha " + uintToStr ( codeLines[i] ) + ". A instrução \'" + tokens[i][j].name + "\' utiliza operando de tipo inválido." );

            // Copy sem vírgula
            if ( !tokens[i][j].name.compare ( "COPY" ) && tokens[i][j + 1].name[tokens[i][j + 1].name.size() - 1] != ',' )
              errors.push_back ( "Erro sintático na linha " + uintToStr ( codeLines[i] ) + ". Instrução COPY com primeiro operando sem vírgula." );
          }
        }

        // Seção inválida, nem text, nem data
        if ( tokens[i][0].type != DIRECTIVE && section != SECTION_TEXT && section != SECTION_DATA )
             errors.push_back ( "Erro sintático na linha " + uintToStr ( codeLines[i] ) + ". Sessão inválida, não é SECTION TEXT e nem SECTION DATA");
      }

      cout << "Executando análise semântica" << endl;

      section = HEADER;
      int haveStop = 0;

      for ( uint i = 0; i < tokens.size(); i++ )
      {
        if ( existBegin && !existEnd )
          errors.push_back ( "Erro semântico. Existência de diretiva BEGIN sem existência de diretiva END." );
        else if ( !existBegin && existEnd )
          errors.push_back ( "Erro semântico. Existência de diretiva END sem existência de diretiva BEGIN." );

        for ( uint j = 0; j < tokens[i].size(); j++ )
        {
          if ( tokens[i].size() >= 2 && !tokens[i][j].name.compare( "SECTION" ) && !tokens[i][j + 1].name.compare ( "TEXT" ) )
            section = SECTION_TEXT;

          else if ( tokens[i].size() >= 2 && !tokens[i][j].name.compare ( "SECTION" ) && !tokens[i][j + 1].name.compare ( "DATA" ) )
            section = SECTION_DATA;

          if ( tokens[i][j].type == INSTRUCTION )
          {
            // Instruções na sessão errada
            if ( section != SECTION_TEXT )
              errors.push_back ( "Erro semântico na linha " + uintToStr(codeLines[i]) + ". A instrução \'" + tokens[i][j].name + "\' está fora da SECTION TEXT." );

            aLabel = -1;
            if ( tokens[i].size() >= 2 )
              aLabel = getLabel(tokens, tokens[i][j + 1].name);

            // Modificação de um valor Constante
            if ( !tokens[i][j].name.compare( "STORE" ) && aLabel != -1 && !(tokens[aLabel/10][aLabel%10].name.compare ( "CONST" )) )
              errors.push_back ( "Erro semântico na linha " + uintToStr(codeLines[i]) + ". STORE modificando um dado CONST." );

            // Divisão por 0
            if ( !tokens[i][j].name.compare( "DIV" ) && aLabel != -1 && (uint)aLabel%10 < tokens[i].size() && !atoi( tokens[aLabel/10][1 + aLabel%10].name.c_str() ) )
              errors.push_back ( "Erro semântico na linha " + uintToStr(codeLines[i]) + ". Divisão por zero." );

            // Pulos para rótulos Inválidos
            if ( (!tokens[i][j].name.compare( "JMP" ) || !tokens[i][j].name.compare( "JMPN" ) || !tokens[i][j].name.compare( "JMPP" ) || !tokens[i][j].name.compare( "JMPZ" ))
            && (aLabel == -1 || tokens[aLabel/10][aLabel%10].type != INSTRUCTION) )
              errors.push_back ( "Erro semântico na linha " + uintToStr(codeLines[i]) + ". Pulo para rótulo inválido." );
          }

          // Diretivas na sessão errada
          if ( tokens[i][j].type == DATA || !tokens[i][j].name.compare ( "CONST" ) || !tokens[i][j].name.compare ( "SPACE" ) )
          {
            if ( section != SECTION_DATA )
              errors.push_back ( "Erro semântico na linha " + uintToStr(codeLines[i]) + ". A diretiva \'" + tokens[i][j].name + "\' está fora da SECTION DATA." );
          }

          if ( !tokens[i][j].name.compare("STOP") )
            haveStop = 1;
        }
      }

      // Código sem instrução stop
      if ( haveStop == 0 )
        errors.push_back ( "Erro semântico. Não há instrução STOP no programa." );

      for ( uint i = 0; i < tokens.size() - 1; i++ )
        for ( uint j = 0; j < tokens[i].size(); j++ )
        {
          for ( uint k = 0; k < tokens[i][j].name.size(); k++ )
            if ( tokens[i][j].name[k] == ',' )
              tokens[i][j].name.erase( tokens[i][j].name.begin() + k );

          if ( tokens[i][j].name.size() == 0 )
              tokens[i].erase ( tokens[i].begin() + j-- );
        }

      cout << "Executando síntese do código objeto" << endl;

      string objectCode;

      if ( errors.size() == 0 )
      {
        codeGenerator ( objectCode, tokens );

        buffer = new char [strlen ( argv[fileNumber + 2] ) - 2];
        strncpy ( buffer, argv[fileNumber + 2], strlen( argv[fileNumber + 2] ) - 3 ); // "nome_do_arquivo.o"
        buffer[strlen ( argv[fileNumber + 2] ) - 3]  = 'o';
        buffer[strlen ( argv[fileNumber + 2] ) - 2]  = '\0';

        output.open ( buffer, fstream::out | fstream::trunc );
        writeOnObjectFile ( output, tokens );
        output.close();

        cout << "Código objeto gerado com sucesso no arquivo " << buffer << endl;
        delete[] buffer;
      }
      else
      {
       cout << "O código apresenta erros, não foi possível gerar o código objeto." << endl;
       for ( uint i = 0; i < errors.size(); i++ )
          cout << errors[i] << endl;
      }

      codeLines.clear();
      removeLines ( inputString );
    }

    inputString.clear();
  }

  return 0;
}

void Tokenize ( const string& str, vector<Token>& tokens, const string& delimiters = " " )  // Obtem os tokens de uma linha
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of( delimiters, 0 );
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of( delimiters, lastPos );

    while ( string::npos != pos || string::npos != lastPos ) {
        // Found a token, add it to the vector.
        tokens.push_back ( Token() );
        tokens.back().name = str.substr ( lastPos, pos - lastPos );
        tokens.back().address = -1;
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of ( delimiters, pos );
        // Find next "non-delimiter"
        pos = str.find_first_of ( delimiters, lastPos );
    }
}

void getTokens ( const string& str, vector< vector<Token> >& tokens, const string& delimiters = " "  )  // Separa cada linha do código para obter os tokens
{
  uint iterator = 0, back = 0;

  while ( iterator < str.size() )
  {
    if ( str[iterator] == '\n' )
    {
      tokens.push_back( vector<Token>() );
      Tokenize ( str.substr ( back, iterator - back ), tokens.back(), delimiters );
      back = ++iterator;
    }

    else if ( iterator == str.size() - 1 )
    {
      tokens.push_back( vector<Token>() );
      Tokenize ( str.substr ( back, iterator - back + 1 ), tokens.back(), delimiters );
      back = ++iterator;
    }

    iterator++;
  }
}

int hexToInt(string hex)
{
    int isNegative = 0;
    if ( hex[0] == '-' )
    {
      isNegative = 1;
      hex.erase( hex.begin() );
    }
    int num = 0;
    int pow16 = 1;
    string alpha("0123456789ABCDEF");
    for(int i = hex.length() - 1; i >= 0; --i)
    {
      if ( hex[i] == 'x' || hex[i] == 'X' )
        break;
      num += alpha.find(toupper(hex[i])) * pow16;
      pow16 *= 16;
    }
    if ( isNegative )
      num = -num;

    return num;
}

string intToStr ( int number )  // Converte um inteiro para uma string com seus algarismos
{
  uint decimals, copy;
  string algarisms;

  if ( number < 0 )
  {
    copy = -number;
    algarisms += '-';
  }
  else
    copy = number;

  decimals = log10( copy );

  for ( int i = decimals; i >= 0; i-- )
  {
    algarisms += (copy/(uint)pow(10, i) + '0');
    copy = copy%(uint)pow(10, i);
  }

  return algarisms;
}

string uintToStr ( uint number )  // Converte um inteiro sem sinal para uma string com seus algarismos
{
  uint decimals = log10(number), copy = number;
  string algarisms;

  for ( int i = decimals; i >= 0; i-- )
  {
    algarisms += (copy/(uint)pow(10, i) + '0');
    copy = copy%(uint)pow(10, i);
  }

  return algarisms;
}

void insertLines ( string &text )  // Insere os números das linhas no final de cada linha da string
{
  uint iterator = 0, lineNumber = 1;
  string lineString;
  while ( iterator < text.size() )
  {
    if ( text[iterator] == '\n' )
    {
      lineString = uintToStr( lineNumber );
      text.insert( iterator, "//" + lineString );
      iterator += lineString.size() + 2;
      lineNumber++;
    }

    if ( iterator == text.size() - 1 )
    {
      text += "//" + uintToStr( lineNumber );
      break;
    }

    iterator++;
  }
}

void removeLines ( string &text )  // Remove os números das linhas inseridos no final de cada linha da string
{
  uint iterator = 0;
  while ( iterator < text.size() - 2 )
  {
    if ( text[iterator] == '/' && text[iterator + 1] == '/' )
    {
      do
      {
        text.erase( text.begin() + iterator );
      } while ( text[iterator] != '\n' && iterator < text.size() );
    }
    iterator++;
  }
}

void removeStoreLines ( string &text, vector<uint> &lines )
{
  uint iterator = 0;
  string lineString;
  while ( iterator < text.size() - 2 )
  {
    if ( text[iterator] == '/' && text[iterator + 1] == '/' )
    {
      do
      {
        if ( text[iterator] >= '0' && text[iterator] <= '9' )
          lineString.push_back( text[iterator] );

        text.erase( text.begin() + iterator );
      } while ( text[iterator] != '\n' && iterator < text.size() );

      lines.push_back( (uint)atoi( lineString.c_str() ) );

      lineString.clear();
    }
    iterator++;
  }
}

void restoreLines ( string &text, vector<uint> &lines )
{
  uint iterator = 0, lineNumber = 0;
  string lineString;

  while ( iterator < text.size() )
  {
    if ( text[iterator] == '\n' )
    {

      lineString = uintToStr( lines[lineNumber] );
      text.insert( iterator, "//" + lineString );
      iterator += lineString.size() + 2;
      lineNumber++;
    }

    if ( iterator == text.size() - 1 )
    {
      text += "//" + uintToStr( lines[lineNumber] );
      break;
    }

    iterator++;
  }
}

uint getCurrentLine ( uint strPos, string inputString )
{
  uint beg;
  while ( strPos < inputString.size() && inputString[strPos] != '\n' )
    strPos++;

  beg = strPos;
  while ( beg >= 1 && inputString[beg] != '/' && inputString[beg - 1] != '/' )
    beg--;

  return (uint) atoi ( inputString.substr(beg, strPos).c_str() );
}

bool isInLabelTable ( vector<Token> labelTable, Token myToken )
{
  bool isInside = false;

  for ( uint i = 0; i < labelTable.size(); i++ )
    if ( !myToken.name.compare( labelTable[i].name ) )
    {
      isInside = true;
      break;
    }

  return isInside;
}

int getLabel ( vector< vector<Token> > tokens, string labelName )
{
  int position = -1;
  for ( uint i = 0; i < tokens.size(); i++ )
  {
    for ( uint j = 0; j < tokens[i].size() - 1; j++ )
    {
      if ( tokens[i][j].type == LABEL && !labelName.compare ( tokens[i][j].name.substr ( 0, tokens[i][j].name.size() - 1 ) ) )
      {
        position = 10*i+j+1;
        return position;
      }
    }
  }

  return position;
}

void codeGenerator (string &objectCode, vector< vector<Token> > &tokens )
{
int nLinhas;
    string tokenAtual;
    int opCode=0, currentLine=0;

    nLinhas=tokens.size();

    vector<unsigned int> codeLines(nLinhas + 1,0);
    vector<TokensList> TokensListAtual;

    int j=0, k=0, iat;

    for ( unsigned int i = 0; i < tokens.size(); i++ )
    {
      if((tokens[i].size()>=2)&&(tokens[i][0].name.compare("SECTION")==0)&&(tokens[i][1].name.compare("TEXT")==0))
      {
        iat=i+1;
        break;
      }
      iat=i+1;
    }

    for (unsigned int i = iat; i < tokens.size(); i++ )
    {
      if((tokens[i].size()>=2)&&(tokens[i][0].name.compare("SECTION")==0)&&(tokens[i][1].name.compare("DATA")==0))
      {
        codeLines[i+1]=codeLines[i];
        iat=i+1;
        break;
      }


      tokenAtual.assign(tokens[i][j].name);

      //Verifica se o token é um mnemonico ou uma label
      if(acharOpCode(tokenAtual, opCode, currentLine)==0)
      {
        tokens[i][j].name=uintToStr(opCode);
      }else

      {
        //Se não for um mnemonico
        TokensListAtual.push_back(TokensList());
        TokensListAtual[k].label=tokens[i][j].name.substr(0, tokens[i][j].name.size() - 1);
        TokensListAtual[k].value=uintToStr(0);
        TokensListAtual[k].address=codeLines[i];

        tokenAtual.assign(tokens[i][j+1].name);

        // Verificando se o proximo token é um mnemonico
        if(acharOpCode(tokenAtual, opCode, currentLine)==0)
        {
          tokens[i][j+1].name=uintToStr(opCode);
        }

        k++;
      }

      if (codeLines.size() >= i-1)
        codeLines[i+1]=codeLines[i]+currentLine;
    }


    j=0;
    for (unsigned int i = iat; i < tokens.size(); i++ )
    {
      if ((j<(int)tokens[i].size()-2)&&(tokens[i][j+1].name.compare("SPACE")==0))
      {
        cout << tokens[i][j+1].name << " " << tokens[i][j+2].name << endl;
        unsigned int zeros = (unsigned int)atoi(tokens[i][j+2].name.c_str());
        for (unsigned int spaces = 0; spaces < zeros; spaces++)
        {
          TokensListAtual.push_back(TokensList());
          TokensListAtual.back().label=tokens[i][j].name.substr(0, tokens[i][j].name.size() - 1);
          TokensListAtual.back().value=uintToStr(0);
          TokensListAtual.back().address=codeLines[i];

          if ((unsigned int)j+2+spaces < tokens[i].size())
          {
            tokens[i][j+2+spaces].name=uintToStr(0);
          }
          else
          {
            tokens[i].push_back(Token());
            tokens[i].back().name = "0";
            tokens[i].back().type = NOT_DEFINDED;
          }
        }
      }
      else if((tokens[i].size()>=(unsigned int)j+2)&&(tokens[i][j+1].name.compare("SPACE")==0))
      {
        TokensListAtual.push_back(TokensList());
        TokensListAtual.back().label=tokens[i][j].name.substr(0, tokens[i][j].name.size() - 1);
        TokensListAtual.back().value=uintToStr(0);
        TokensListAtual.back().address=codeLines[i];
        tokens[i][j+1].name=uintToStr(0);
      }else

      if((tokens[i].size()>=(unsigned int)j+2)&&(tokens[i][j+1].name.compare("CONST")==0))
      {
        TokensListAtual.push_back(TokensList());
        TokensListAtual.back().label=tokens[i][j].name.substr(0, tokens[i][j].name.size() - 1);
        TokensListAtual.back().value=tokens[i][j+1].name;
        TokensListAtual.back().address=codeLines[i];
      }

      codeLines[i+1]=codeLines[i]+1;
   }

     //displayStructTokensList(TokensListAtual);

//-------------Substituindo as constantes no código--------------------------//

      for ( unsigned int i = 0; i < tokens.size(); i++ )
      {
        for ( unsigned int j = 0; j < tokens[i].size(); j++ )
        {
            tokenAtual.assign(tokens[i][j].name);
            for (unsigned int k = 0; k < TokensListAtual.size(); k++ )
            {
              if(tokenAtual.compare(TokensListAtual[k].label)==0)
                tokens[i][j].name=uintToStr(TokensListAtual[k].address);
            }
        }
      }


  //displayVet(codeLines);

}


void writeOnObjectFile( fstream &outfile, vector< vector<Token> > &tokens )
{
    string tokenAtual;

    for ( uint i = 0; i < tokens.size(); i++ )
    {
      for ( uint j = 0; j < tokens[i].size(); j++ )
      {
        tokenAtual.assign(tokens[i][j].name);

        if(is_number(tokenAtual)==1)
        {
          outfile << tokenAtual << " ";
        }
      }
    }
}

bool is_number( const string& s )
{
    string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void displayStructTokensList ( vector<struct TokensList> TokensListAtual )
{
    for (uint i = 0; i < TokensListAtual.size(); i++ )
    {
      cout<<"i= "<< i<<endl;
      cout<<"label= "<<TokensListAtual[i].label <<endl;
      cout<<"value= "<< TokensListAtual[i].value <<endl;
      cout<<"adress= "<<TokensListAtual[i].address <<endl;
      cout<<"\n";
    }
}

void displayMatrix ( vector< vector<Token> > tokens )
{
  for ( uint i = 0; i < tokens.size(); i++ )
  {
    for ( uint j = 0; j < tokens[i].size(); j++ )
    {
      cout << tokens[i][j].name<<" ";
    }
    cout<<endl;
  }
}


int acharOpCode (string &token, int &opCode, int &currentLine)
{
  int out=0;

  if(token.compare("ADD")==0)
  {
    opCode=1;
    currentLine=2;
  } else

  if(token.compare("SUB")==0)
  {
    opCode=2;
    currentLine=2;
  } else

  if(token.compare("MULT")==0)
  {
    opCode=3;
    currentLine=2;
  } else

  if(token.compare("DIV")==0)
  {
    opCode=4;
    currentLine=2;
  } else

  if(token.compare("JMP")==0)
  {
    opCode=5;
    currentLine=2;
  } else

  if(token.compare("JMPN")==0)
  {
    opCode=6;
    currentLine=2;} else

  if(token.compare("JMPP")==0)
  {
    opCode=7;
    currentLine=2;
  } else

  if(token.compare("JMPZ")==0)
  {
    opCode=8;
    currentLine=2;
  } else

  if(token.compare("COPY")==0)
  {
    opCode=9;
    currentLine=3;
  } else

  if(token.compare("LOAD")==0)
  {
    opCode=10;
    currentLine=2;
  } else

  if(token.compare("STORE")==0)
  {
    opCode=11;
    currentLine=2;
  } else

  if(token.compare("INPUT")==0)
  {
    opCode=12;
    currentLine=2;
  } else

  if(token.compare("OUTPUT")==0)
  {
    opCode=13;
    currentLine=2;
  } else

  if(token.compare("STOP")==0)
  {
    opCode=14;
    currentLine=1;
  } else
  {
      out=1;
      //cout<<"\n Op-code nao encontrado!\n";
      currentLine=0;
  }

  return (out);
}



void displayVet (vector<uint> vet){

for (vector<uint>::const_iterator i = vet.begin(); i != vet.end(); ++i)
   {
    cout << *i << ' ';
   }
   cout << "\n";
}
