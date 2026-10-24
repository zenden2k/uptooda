// parser.cpp
// Этот файл содержит реализацию класса TParser,
// осуществляющего разбор заданного математического выражения.
// Written by Chaos Master, 11th of Febrary, 2000.

// Includes

#include <cmath>
#include <boost/format.hpp>

#include "parser.h"


using namespace std;
// Defines
// operations...
#define OP_PLUS          0
#define OP_MINUS         1
#define OP_MULTIPLY      2
#define OP_DIVIDE        3
#define OP_PERCENT       4
#define OP_POWER         5
#define OP_UMINUS        6

#define OP_SIN           10
#define OP_COS           11
#define OP_TG            12
#define OP_CTG           13
#define OP_ARCSIN        14
#define OP_ARCCOS        15
#define OP_ARCTG         16
#define OP_ARCCTG        17
#define OP_SH            18
#define OP_CH            19
#define OP_TH            20
#define OP_CTH           21
#define OP_EXP           22
#define OP_LG            23
#define OP_LN            24
#define OP_SQRT          25
#define OP_X             26

// PI...
#define M_PI             3.1415926535897932384626433832795

// Member functions
TParserNode *TParser::CreateNode(double _value, TParserNode *_left, TParserNode *_right)
{
   TParserNode *pNode = new TParserNode(_value, _left, _right);
   history.push_back(pNode);
   return pNode;
}

void TParser::SendError(int errNum)
{
   std::string errs[7] = { "",
                            "",
                            "Unexpected end of expression",
                            "End of expression expected",
                            "'(' or '[' expected",
                            "')' or ']' expected",
                            ""
                         };
   
   int len = strlen(curToken);
   
   if(*curToken=='\0')
      strcpy(curToken, "EOL");

   switch(errNum)
   {
      case 0:
         errs[0] = str(boost::format("Unknown keyword: '%s'") % curToken);
         break;

      case 1:
         errs[1] = str(boost::format("Unknown symbol : '%s'") % curToken);
         break;

      case 6:
         errs[6] = str(boost::format("Unexpected '%s'") % curToken);
         break;
   }

   TParserError error(errs[errNum], pos-len);

   for(size_t i=0; i<history.size(); i++)
      delete history[i];
   history.clear();

   root = NULL;

   throw error;
}
   
bool TParser::GetToken(void)
{
   *curToken = '\0';
   
   while(expr[pos]==' ') pos++;
   
   if(expr[pos]=='\0')
   {
      curToken[0] = '\0';
      typToken = PARSER_END;
      return true;
   }
   else if(IsDelim())
   {
      curToken[0] = expr[pos++];
      curToken[1] = '\0';
      switch(*curToken)
      {
         case '+': typToken = PARSER_PLUS; return true;
         case '-': typToken = PARSER_MINUS; return true;
         case '*': typToken = PARSER_MULTIPLY; return true;
         case '/': typToken = PARSER_DIVIDE; return true;
         case '%': typToken = PARSER_PERCENT; return true;
         case '^': typToken = PARSER_POWER; return true;
         case '[':
         case '(': typToken = PARSER_L_BRACKET; return true;
         case ']':
         case ')': typToken = PARSER_R_BRACKET; return true;
      }
   }
   else if(IsLetter())
   {
      int i=0;
      while (IsLetter() && pos < MAX_TOKEN_LEN) {
          curToken[i++] = expr[pos++];
      }
      curToken[i] = '\0';

      int len = strlen(curToken);
      for(i=0; i<len; i++)
         if(curToken[i]>='A' && curToken[i]<='Z')
            curToken[i] += 'a' - 'A';

      if(!strcmp(curToken, "x"))           { typToken = PARSER_X; return true; }
      else if(!strcmp(curToken, "pi"))     { typToken = PARSER_PI; return true; }
      else if(!strcmp(curToken, "e"))      { typToken = PARSER_E; return true; }
      else if(!strcmp(curToken, "sin"))    { typToken = PARSER_SIN; return true; }
      else if(!strcmp(curToken, "cos"))    { typToken = PARSER_COS; return true; }
      else if(!strcmp(curToken, "tg"))     { typToken = PARSER_TG; return true; }
      else if(!strcmp(curToken, "ctg"))    { typToken = PARSER_CTG; return true; }
      else if(!strcmp(curToken, "arcsin")) { typToken = PARSER_ARCSIN; return true; }
      else if(!strcmp(curToken, "arccos")) { typToken = PARSER_ARCCOS; return true; }
      else if(!strcmp(curToken, "arctg"))  { typToken = PARSER_ARCTG; return true; }
      else if(!strcmp(curToken, "arcctg")) { typToken = PARSER_ARCCTG; return true; }
      else if(!strcmp(curToken, "sh"))     { typToken = PARSER_SH; return true; }
      else if(!strcmp(curToken, "ch"))     { typToken = PARSER_CH; return true; }
      else if(!strcmp(curToken, "th"))     { typToken = PARSER_TH; return true; }
      else if(!strcmp(curToken, "cth"))    { typToken = PARSER_CTH; return true; }
      else if(!strcmp(curToken, "exp"))    { typToken = PARSER_EXP; return true; }
      else if(!strcmp(curToken, "lg"))     { typToken = PARSER_LG; return true; }
      else if(!strcmp(curToken, "ln"))     { typToken = PARSER_LN; return true; }
      else if(!strcmp(curToken, "sqrt"))   { typToken = PARSER_SQRT; return true; }
      else SendError(0);
   }
   else if(IsDigit() || IsPoint())
   {
      int i=0;
      while (IsDigit() && i < MAX_TOKEN_LEN) {
          curToken[i++] = expr[pos++];
      }
      if(IsPoint())
      {
         curToken[i++] = expr[pos++];
         while (IsDigit() && i < MAX_TOKEN_LEN) {
             curToken[i++] = expr[pos++];
         }
      }
      curToken[i] = '\0';
      typToken = PARSER_NUMBER;
      return true;
   }
   else
   {
      curToken[0] = expr[pos++];
      curToken[1] = '\0';
      SendError(1);
   }

   return false;
}      

bool TParser::Compile(const char *_expr)
{
   pos = 0;
   expr = (char*)_expr;
   *curToken = '\0';
   if(root!=NULL)
   {
      DelTree(root);
      root = NULL;
   }

   history.clear();

   GetToken();
   if(typToken==PARSER_END) SendError(2);
   root = Expr();
   if(typToken!=PARSER_END) SendError(3);

   history.clear();

   return true;
}

TParserNode *TParser::Expr(void)
{
   TParserNode *temp = Expr1();

   while(1)
   {
      if(typToken==PARSER_PLUS)
      {
         GetToken();
         temp = CreateNode(OP_PLUS, temp, Expr1());
      }
      else if(typToken==PARSER_MINUS)
      {
         GetToken();
         temp = CreateNode(OP_MINUS, temp, Expr1());
      }
      else break;
   }

   return temp;
}
   
TParserNode *TParser::Expr1(void)
{
   TParserNode *temp = Expr2();

   while(1)
   {
      if(typToken==PARSER_MULTIPLY)
      {
         GetToken();
         temp = CreateNode(OP_MULTIPLY, temp, Expr2());
      }
      else if(typToken==PARSER_DIVIDE)
      {
         GetToken();
         temp = CreateNode(OP_DIVIDE, temp, Expr2());
      }
      else if(typToken==PARSER_PERCENT)
      {
         GetToken();
         temp = CreateNode(OP_PERCENT, temp, Expr2());
      }
      else break;
   }

   return temp;
}

TParserNode *TParser::Expr2(void)
{
   TParserNode *temp = Expr3();

   while(1)
   {
      if(typToken==PARSER_POWER)
      {
         GetToken();
         temp = CreateNode(OP_POWER, temp, Expr2());
      }
      else break;
   }

   return temp;
}

TParserNode *TParser::Expr3(void)
{
   TParserNode *temp;

   if(typToken==PARSER_PLUS)
   {
      GetToken();
      temp = Expr4();
   }
   else if(typToken==PARSER_MINUS)
   {
      GetToken();
      temp = CreateNode(OP_UMINUS, Expr4());
   }
   else
      temp = Expr4();

   return temp;      
}

TParserNode *TParser::Expr4(void)
{
   TParserNode *temp;

   if(typToken>=PARSER_SIN && typToken<=PARSER_X)
   {
      temp = CreateNode(OP_SIN-PARSER_SIN+typToken);
      GetToken();
      if(typToken!=PARSER_L_BRACKET) SendError(4);
      GetToken();
      temp->left = Expr();
      if(typToken!=PARSER_R_BRACKET) SendError(5);
      GetToken();
   }
   else
      temp = Expr5();

   return temp;
}

TParserNode *TParser::Expr5(void)
{
   TParserNode *temp = 0;
   
   switch(typToken)
   {
      case PARSER_NUMBER:
         temp = CreateNode((double)atof(curToken));
         GetToken();
         break;

      case PARSER_PI:
         temp = CreateNode((double)M_PI);
         GetToken();
         break;

      case PARSER_E:
         temp = CreateNode((double)exp((double)1));
         GetToken();
         break;

      case PARSER_L_BRACKET:
         GetToken();
         temp = Expr();
         if(typToken!=PARSER_R_BRACKET) SendError(5);
         GetToken();
         break;

      default:
         SendError(6);
   }

   return temp;         
}

double TParser::Evaluate(void)
{
   result = CalcTree(root);
   return result;
}

double TParser::CalcTree(TParserNode *tree)
{
   static double temp;
   
   if(tree->left==NULL && tree->right==NULL)
      return tree->value;
   else
      switch((int)tree->value)
      {
         case OP_PLUS:
            return CalcTree(tree->left)+CalcTree(tree->right);

         case OP_MINUS:
            return CalcTree(tree->left)-CalcTree(tree->right);

         case OP_MULTIPLY:
            return CalcTree(tree->left)*CalcTree(tree->right);

         case OP_DIVIDE:
            return CalcTree(tree->left)/CalcTree(tree->right);

         case OP_PERCENT:
            return (int)CalcTree(tree->left)%(int)CalcTree(tree->right);

         case OP_POWER:
            return (double)pow(CalcTree(tree->left),CalcTree(tree->right));

         case OP_UMINUS:
            return -CalcTree(tree->left);

         case OP_SIN:
            return sin(CalcTree(tree->left));

         case OP_COS:
            return cos(CalcTree(tree->left));

         case OP_TG:
            return tan(CalcTree(tree->left));

         case OP_CTG:
            return 1.0/tan(CalcTree(tree->left));

         case OP_ARCSIN:
            return asin(CalcTree(tree->left));

         case OP_ARCCOS:
            return acos(CalcTree(tree->left));

         case OP_ARCTG:
            return atan(CalcTree(tree->left));

         case OP_ARCCTG:
            return M_PI/2.0-atan(CalcTree(tree->left));

         case OP_SH:
            temp = CalcTree(tree->left);
            return (exp(temp)-exp(-temp))/2.0;

         case OP_CH:
            temp = CalcTree(tree->left);
            return (exp(temp)+exp(-temp))/2.0;

         case OP_TH:
            temp = CalcTree(tree->left);
            return (exp(temp)-exp(-temp))/(exp(temp)+exp(-temp));

         case OP_CTH:
            temp = CalcTree(tree->left);
            return (exp(temp)+exp(-temp))/(exp(temp)-exp(-temp));

         case OP_EXP:
            return exp(CalcTree(tree->left));

         case OP_LG:
            return log10(CalcTree(tree->left));

         case OP_LN:
            return log(CalcTree(tree->left));

         case OP_SQRT:
            return sqrt(CalcTree(tree->left));

         case OP_X:
            return x[int(CalcTree(tree->left))];
      }

   return 0;
}

void TParser::DelTree(TParserNode *tree)
{
   if(tree==NULL) return;

   DelTree(tree->left);
   DelTree(tree->right);

   delete tree;

   return;
}