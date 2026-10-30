// parser.h
// Этот файл содержит описание класса TParser,
// осуществляющего разбор заданного математического выражения.
// Written by Chaos Master, 11th of Febrary, 2000.
// 2015 - Sergey Svistunov

#if !defined(__SOLVER_H)
#define __SOLVER_H

// Includes
#include <string.h>
#include <vector>

struct TParserNode
{
   double value;
   TParserNode *left;
   TParserNode *right;

   TParserNode(double _value=0.0, TParserNode *_left=NULL, TParserNode *_right=NULL)
      { value = _value; left = _left; right = _right; }
};

class TParserError : public std::runtime_error {
public:
    TParserError(const std::string& msg, int pos = -1) : std::runtime_error(msg), pos_(pos) {}
    TParserError(const TParserError& ex) : std::runtime_error(ex), pos_(ex.pos_) {}
    int pos() {
        return pos_;
    }
protected:
    int pos_;
};

class TParser
{
  private:
   TParserNode *root;
   char *expr;
   enum { MAX_EXPR_LEN =  255, MAX_TOKEN_LEN  = 80};
   char curToken[MAX_TOKEN_LEN];
   enum { PARSER_PLUS, PARSER_MINUS, PARSER_MULTIPLY, PARSER_DIVIDE, PARSER_PERCENT, PARSER_POWER,
          PARSER_SIN, PARSER_COS, PARSER_TG, PARSER_CTG, PARSER_ARCSIN, PARSER_ARCCOS, PARSER_ARCTG, PARSER_ARCCTG, PARSER_SH, PARSER_CH, PARSER_TH, PARSER_CTH,
          PARSER_EXP, PARSER_LG, PARSER_LN, PARSER_SQRT, PARSER_X, PARSER_L_BRACKET, PARSER_R_BRACKET, PARSER_E, PARSER_PI, PARSER_NUMBER, PARSER_END } typToken;
   int pos;

   const double *x;
   double result;

   std::vector<TParserNode *> history;
   TParserNode *CreateNode(double _value=0.0, TParserNode *_left=NULL, TParserNode *_right=NULL);

   TParserNode *Expr(void);
   TParserNode *Expr1(void);
   TParserNode *Expr2(void);
   TParserNode *Expr3(void);
   TParserNode *Expr4(void);
   TParserNode *Expr5(void);

   bool GetToken(void);
   bool IsDelim(void) { return (strchr("+-*/%^()[]", expr[pos])!=NULL); }
   bool IsLetter(void) { return ((expr[pos]>='a' && expr[pos]<='z') ||
                                 (expr[pos]>='A' && expr[pos]<='Z')); }
   bool IsDigit(void) { return (expr[pos]>='0' && expr[pos]<='9'); }
   bool IsPoint(void) { return (expr[pos]=='.'); }

   double CalcTree(TParserNode *tree);
   void  DelTree(TParserNode *tree);

   void SendError(int errNum);

  public:
   TParser() { result = 0.0; x = NULL; root = NULL; }
   ~TParser() { DelTree(root); root=NULL; }
   void SetX(const double *_x) { x=_x; }
   //void SetY(double _y) { y=_y; }

   bool Compile(const char *expr);
   void Decompile() { DelTree(root); root=NULL; }

   double Evaluate();
   double Evaluate(double *_x) { SetX(_x); return Evaluate(); }
   double Evaluate(double x, ...) { SetX(&x); return Evaluate(); }

   double GetResult(void) { return result; }
};

#endif