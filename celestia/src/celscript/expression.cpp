// expression.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/expression.h>

using namespace std;
using namespace celx;


typedef Value (BinaryOperatorFunc)(const Value&, const Value&);
typedef Value (UnaryOperatorFunc)(const Value&);

static BinaryOperatorFunc AddFunc;
static BinaryOperatorFunc SubtractFunc;
static BinaryOperatorFunc MultiplyFunc;
static BinaryOperatorFunc DivideFunc;
static BinaryOperatorFunc EqualFunc;
static BinaryOperatorFunc NotEqualFunc;
static BinaryOperatorFunc LesserFunc;
static BinaryOperatorFunc GreaterFunc;
static BinaryOperatorFunc LesserEqualFunc;
static BinaryOperatorFunc GreaterEqualFunc;

static Value ErrorValue = Value();


static BinaryOperatorFunc* BinaryOperatorFunctions[BinaryExpression::OperatorCount] =
{
    AddFunc,
    SubtractFunc,
    MultiplyFunc,
    DivideFunc,
    EqualFunc,
    NotEqualFunc,
    LesserFunc,
    GreaterFunc,
    LesserEqualFunc,
    GreaterEqualFunc,
    NULL,
};


static UnaryOperatorFunc NegateFunc;
static UnaryOperatorFunc LogicalNotFunc;
static UnaryOperatorFunc* UnaryOperatorFunctions[UnaryExpression::OperatorCount] =
{
    NegateFunc,
    LogicalNotFunc,
    NULL,
};


BinaryExpression::BinaryExpression(Operator _op,
                                   Expression* _left,
                                   Expression* _right) :
    op(_op),
    left(_left),
    right(_right)
{
}

BinaryExpression::~BinaryExpression()
{
    if (left != NULL)
        delete left;
    if (right != NULL)
        delete right;
}

Value BinaryExpression::eval()
{
    Value a = left->eval();
    Value b = right->eval();
    return BinaryOperatorFunctions[op](a, b);
}


UnaryExpression::UnaryExpression(Operator _op,
                                 Expression* _expr) :
    op(_op),
    expr(_expr)
{
}

UnaryExpression::~UnaryExpression()
{
    if (expr != NULL)
        delete expr;
}

Value UnaryExpression::eval()
{
    Value v = expr->eval();
    return UnaryOperatorFunctions[op](v);
}


ConstantExpression::ConstantExpression(const Value& _value) :
    value(_value)
{
}

ConstantExpression::~ConstantExpression()
{
}

Value ConstantExpression::eval()
{
    return value;
}


NameExpression::NameExpression(const string& _name) :
    name(_name)
{
}

NameExpression::~NameExpression()
{
}

Value NameExpression::eval()
{
    return Value();
}


Value AddFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 + x1);
    }
    else
    {
        return ErrorValue;
    }
}

Value SubtractFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 - x1);
    }
    else
    {
        return ErrorValue;
    }
}

Value MultiplyFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 * x1);
    }
    else
    {
        return ErrorValue;
    }
}

Value DivideFunc(const Value& a, const Value& b)
{
    if (a.getType() == NumberType && b.getType() == NumberType)
    {
        double x0 = 0.0;
        double x1 = 0.0;
        a.numberValue(x0);
        b.numberValue(x1);
        return Value(x0 / x1);
    }
    else
    {
        return ErrorValue;
    }
}


Value EqualFunc(const Value& a, const Value& b)
{
    return Value(a == b);
}


Value NotEqualFunc(const Value& a, const Value& b)
{
    return Value(a != b);
}


Value LesserFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() < b.toNumber());
}


Value GreaterFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() > b.toNumber());
}


Value LesserEqualFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() <= b.toNumber());
}


Value GreaterEqualFunc(const Value& a, const Value& b)
{
    return Value(a.toNumber() >= b.toNumber());
}



Value NegateFunc(const Value& v)
{
    return Value(-v.toNumber());
}

Value LogicalNotFunc(const Value& v)
{
    return Value(!v.toBoolean());
}
