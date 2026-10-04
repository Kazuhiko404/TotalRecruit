#pragma once


class TripleInt
{
public:
    int value[0x3]; // 0x0 Member
    TripleInt(const TripleInt& who);// public RVA = 0x8AF30
    TripleInt* _CONSTRUCTOR(const TripleInt& who);// public RVA = 0x8AF30
    TripleInt(int a, int b, int c);// public RVA = 0x6DF10
    TripleInt* _CONSTRUCTOR(int a, int b, int c);// public RVA = 0x6DF10
    TripleInt();// public RVA = 0x2B49D0
    TripleInt* _CONSTRUCTOR();// public RVA = 0x2B49D0
    const TripleInt& operator=(const TripleInt& a);// public RVA = 0x6DF30
    int operator[](int i) const;// public RVA = 0x64530
    int& operator[](int i);// public RVA = 0x6DF50
    static TripleInt ZERO; // RVA = 0x212E9A0 Static Member
};