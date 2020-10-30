#define L0 -1
#define L1 131
#define L2 147
#define L3 165
#define L4 175
#define L5 196
#define L6 221
#define L7 248

#define M1 262
#define M2 294
#define M3 330
#define M4 350
#define M5 393
#define M6 441
#define M7 495

#define H1 525
#define H2 589
#define H3 661
#define H4 700
#define H5 786
#define H6 882
#define H7 990

#define WHOLE 1
#define HALF 0.5
#define QUARTER 0.25
#define EIGHTH 0.25
#define SIXTEENTH 0.625

int tune[] =
{
    M6,H1,H2,H2,H2,H2,H2,H3,H4,H4,H3, //难道这就是你分手的借口
    H1,H2,H3,H3,H5,H6,H6,H1, //如果让你重新来过
    H1,H3,H2,H1,H2,H3, //你会不会爱我
    H1,H2,H3,H3,H5,H6,H6,H1, //爱情让人拥有快乐
    H1,H3,H2,H1,M7,H1, //也会带来折磨
    H1,H2,H3,H3,H5,H6,H6,H1, //曾经和你一起走过
    H1,H3,H2,H1,H1,H2,H3, //传说中的爱河
    H1,H2,H3,H3,H5,H6,H6,H1, //已经被我泪水淹没
    H1,H3,H2,H1,H1,M7,H1 //变成痛苦的爱河
};

float duration[] =
{
    0.25,0.25,0.25,0.25,0.25,0.25,0.5,0.25,0.25,0.5,0.5,
    0.5,0.5,0.5,0.5,0.5,0.25,0.25+0.5,0.5,
    0.5,0.5,1,0.5,0.5,1,
    0.5,0.5,0.5,0.5,0.5,0.25,0.25+0.5,0.5,
    0.5,0.5,1,0.5,0.5,1,
    0.5,0.5,0.5,0.5,0.5,0.25,0.25+0.5,0.5,
    0.5,0.5,0.5,0.5,0.5,0.5,1,
    0.5,0.5,0.5,0.5,0.5,0.25,0.25+0.5,0.5,
    0.5,0.5,0.5,0.25,0.25+0.5,1,0.5+1
};