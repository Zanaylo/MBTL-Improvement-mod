#include "Palette/EffectTable.h"

namespace {

struct Row
{
	unsigned short pattern;
	const char* code;
	const char* name;
	const char* spawnedBy;
	const unsigned char* entries;
	unsigned char count;
};

const unsigned char kEntries000_0[] = { 241 };
const unsigned char kEntries000_1[] = { 242 };
const unsigned char kEntries000_2[] = { 244 };
const unsigned char kEntries000_3[] = { 241, 244 };
const unsigned char kEntries000_4[] = { 250, 251 };
const unsigned char kEntries000_5[] = { 249 };
const unsigned char kEntries000_6[] = { 241, 242, 253 };

const Row kEffects000[] = {
	{ 101, "", "A", "", kEntries000_0, 1 },
	{ 102, "", "\xe3\x81\x9f\xe3\x82\x81\x42", "", kEntries000_0, 1 },
	{ 103, "", "\xe3\x81\x9f\xe3\x82\x81\x43", "", kEntries000_0, 1 },
	{ 104, "", "2A", "", kEntries000_0, 1 },
	{ 105, "", "\x32\x42\xe7\x85\x99", "", nullptr, 0 },
	{ 106, "", "2C", "", kEntries000_0, 1 },
	{ 107, "", "JA", "", kEntries000_0, 1 },
	{ 108, "", "JB1", "", kEntries000_0, 1 },
	{ 109, "", "JB2", "", kEntries000_0, 1 },
	{ 110, "", "JC", "AirSC", kEntries000_0, 1 },
	{ 111, "", "B", "StdB_End", kEntries000_0, 1 },
	{ 112, "", "C", "StdC_End", kEntries000_0, 1 },
	{ 113, "", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x91\xe3\x83\xbc", "StdSC", kEntries000_0, 1 },
	{ 114, "", "\x34\x43\xe7\x85\x99", "214BC_Hit, 4C", nullptr, 0 },
	{ 115, "", "\x32\x30\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "ExSC", kEntries000_0, 1 },
	{ 116, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries000_1, 1 },
	{ 117, "", "\xe3\x82\xaf\xe3\x83\xad\xe3\x82\xb9", "AirDiveSC, DiveSC", kEntries000_0, 1 },
	{ 164, "", "J2B", "J2B, J2B_End", kEntries000_0, 1 },
	{ 172, "", "J2C", "J2C", kEntries000_0, 1 },
	{ 203, "", "236>Add4", "236_Add4", kEntries000_2, 1 },
	{ 204, "", "\xe3\x81\x9f\xe3\x82\x81\x32\x33\x36", "236A", kEntries000_2, 1 },
	{ 205, "", "236", "236A_End, 236B, 236BC", kEntries000_2, 1 },
	{ 206, "", "236>Add", "236BC, 236_Add", kEntries000_2, 1 },
	{ 207, "", "236>AddFinA", "236BC, 236_236_Add236A", kEntries000_2, 1 },
	{ 208, "", "236>AddFinB", "236_236_Add214A", kEntries000_2, 1 },
	{ 210, "", "236EX1", "236EX", kEntries000_2, 1 },
	{ 211, "", "236EX2", "236EX", kEntries000_3, 2 },
	{ 212, "", "236EX3", "236EX", kEntries000_2, 1 },
	{ 213, "", "\x32\x33\x36\x45\x58\xe4\xb8\xad\xe6\xae\xb5", "236EX", kEntries000_2, 1 },
	{ 235, "", "\xe6\xae\x8b\xe5\x83\x8f", "214EX_Hit", nullptr, 0 },
	{ 236, "", "\xe6\xae\x8b\xe5\x83\x8f", "214EX_Hit", nullptr, 0 },
	{ 237, "", "\xe6\xae\x8b\xe5\x83\x8f", "214EX_Hit", nullptr, 0 },
	{ 240, "", "214", "214A, 214B, 214BC, 214EX", kEntries000_0, 1 },
	{ 242, "", "", "214EX_Hit", kEntries000_0, 1 },
	{ 243, "", "", "214EX_Hit", kEntries000_0, 1 },
	{ 244, "", "", "214EX_Hit", kEntries000_0, 1 },
	{ 245, "", "C", "214B_Hit", kEntries000_0, 1 },
	{ 254, "", "623", "623A", kEntries000_2, 1 },
	{ 255, "", "623", "623A, 623B", kEntries000_2, 1 },
	{ 256, "", "623EX", "623EX", kEntries000_2, 1 },
	{ 257, "", "623Add", "623A_Add", kEntries000_2, 1 },
	{ 258, "", "623AddEX", "623EX", kEntries000_2, 1 },
	{ 259, "", "623", "623BC", kEntries000_2, 1 },
	{ 275, "", "\xe5\xaf\xbe\xe7\xa9\xba", "0202A, 0202B, 0202BC, 0202EX, J0202A, J0202B, J0202BC", kEntries000_2, 1 },
	{ 277, "", "\xe5\xaf\xbe\xe7\xa9\xba", "J0202EX", kEntries000_2, 1 },
	{ 283, "", "J66", "0202_JAdd, 0202_JAddEX", kEntries000_2, 1 },
	{ 325, "", "\xe3\x81\xa8\xe3\x81\xa9\xe3\x82\x81\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries000_4, 2 },
	{ 328, "", "\xe5\xb2\xa9", "SPTuta", kEntries000_0, 1 },
	{ 329, "", "\xe6\x89\x8b\xe5\x89\x8d\xe8\x94\xa6", "SPTuta", nullptr, 0 },
	{ 360, "", "\xe5\xae\x87\xe5\xae\x99", "LA_Main", nullptr, 0 },
	{ 365, "", "\xe5\x88\x87\xe3\x82\x8a\xe6\x9b\xbf\xe3\x81\x88\xe9\xa2\xa8\x28\x32\x30\x46\x29", "", nullptr, 0 },
	{ 366, "", "\xe6\x97\x8b\xe9\xa2\xa8", "", kEntries000_5, 1 },
	{ 367, "", "\xe5\x9c\xa7\xe7\xb8\xae\xe9\x9b\x86\xe4\xb8\xad", "", kEntries000_6, 3 },
	{ 373, "", "\xe3\x83\x81\xe3\x83\xaa", "", nullptr, 0 },
	{ 374, "", "\xe4\xb8\xad\xe5\xa4\xae\xe5\x85\x89\xe6\x98\x8e\xe6\xbb\x85", "", nullptr, 0 },
};

const unsigned char kEntries001_0[] = { 1, 3, 96, 97 };
const unsigned char kEntries001_1[] = { 1, 3, 96, 98 };
const unsigned char kEntries001_2[] = { 250 };
const unsigned char kEntries001_3[] = { 1, 96 };
const unsigned char kEntries001_4[] = { 198, 253 };
const unsigned char kEntries001_5[] = { 253 };
const unsigned char kEntries001_6[] = { 189, 190, 253 };
const unsigned char kEntries001_7[] = { 189, 190 };
const unsigned char kEntries001_8[] = { 198, 199, 253 };
const unsigned char kEntries001_9[] = { 249 };
const unsigned char kEntries001_10[] = { 245, 247, 248 };
const unsigned char kEntries001_11[] = { 247, 248, 249 };
const unsigned char kEntries001_12[] = { 245, 246, 247 };
const unsigned char kEntries001_13[] = { 245, 246, 247, 250 };
const unsigned char kEntries001_14[] = { 245, 249, 251 };
const unsigned char kEntries001_15[] = { 251 };
const unsigned char kEntries001_16[] = { 246, 247 };
const unsigned char kEntries001_17[] = { 241, 243, 246, 249 };

const Row kEffects001[] = {
	{ 101, "", "", "", kEntries001_0, 4 },
	{ 102, "", "", "", kEntries001_1, 4 },
	{ 103, "", "\xe7\xab\x8b\xe3\x81\xa1\xe5\xbc\xb7\xe6\x94\xbb\xe6\x92\x83", "P_AtkB, StdC_End", kEntries001_2, 1 },
	{ 104, "", "", "", kEntries001_1, 4 },
	{ 105, "", "", "", kEntries001_1, 4 },
	{ 106, "", "", "", nullptr, 0 },
	{ 107, "", "", "", kEntries001_3, 2 },
	{ 108, "", "", "", kEntries001_4, 2 },
	{ 115, "", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x91\xe3\x83\xbc", "StdSC", kEntries001_5, 1 },
	{ 116, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries001_5, 1 },
	{ 158, "", "\xe6\xb0\xb4\xe3\x81\x9f\xe3\x81\xbe\xe3\x82\x8a", "", nullptr, 0 },
	{ 164, "", "", "6C", kEntries001_6, 3 },
	{ 165, "", "", "6C_6C", kEntries001_6, 3 },
	{ 166, "", "", "6C_6C_6C", kEntries001_7, 2 },
	{ 177, "", "", "AirSC, JB_JB", kEntries001_8, 3 },
	{ 194, "", "\xe3\x81\x9f\xe3\x81\xbe\xe3\x82\x8b\xe6\xb0\xb4\xe3\x81\x9f\xe3\x81\xbe\xe3\x82\x8a", "4B, P_AtkA", nullptr, 0 },
	{ 195, "", "\xe3\x81\x98\xe3\x82\x87\xe3\x81\x86\xe3\x82\x8d\xe6\xb0\xb4\xe6\x8a\x9c\xe3\x81\x91", "4B_End, P_AtkA", kEntries001_9, 1 },
	{ 196, "", "\xe6\xb0\xb4\xe3\x81\x9f\xe3\x81\xbe\xe3\x82\x8a\xe3\x82\xbf\xe3\x83\xa1", "P_AtkA", nullptr, 0 },
	{ 197, "", "\xe6\xb0\xb4\xe3\x81\x9f\xe3\x81\xbe\xe3\x82\x8a", "4B_End", nullptr, 0 },
	{ 208, "", "\xe7\x99\xbe\xe5\x88\x97\xe6\x89\x8b", "236A, 236B, 236BC, 236EX", nullptr, 0 },
	{ 210, "", "", "236A, 236B, 236BC, 236EX", kEntries001_5, 1 },
	{ 215, "", "", "236_Add", kEntries001_5, 1 },
	{ 260, "", "\xe3\x83\x9b\xe3\x82\xb3\xe3\x83\xaa\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae\xe7\x94\x9f\xe6\x88\x90", "Tama623A, Tama623B, Tama623BC, Tama623EX", nullptr, 0 },
	{ 261, "", "\xe3\x83\x9b\xe3\x82\xb3\xe3\x83\xaa\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "", nullptr, 0 },
	{ 280, "", "\xe5\xbc\x81\xe5\xbd\x93\xe7\x88\x86\xe7\x99\xba", "BentouAB, BentouBC, BentouBC2", kEntries001_10, 3 },
	{ 300, "", "", "J236A, J236B, J236BC, J236EX", kEntries001_5, 1 },
	{ 315, "", "\xe3\x82\xbf\xe3\x83\x83\xe3\x83\x81\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "M_ChangeTouch, P_ChangeTouch", nullptr, 0 },
	{ 352, "", "\x20\x3e\xe9\x9b\x86\xe4\xb8\xad", "Atumaru_tama", kEntries001_11, 3 },
	{ 353, "", "\xe9\xa3\x9b\xe3\x81\xb6\xe3\x81\x9f\xe3\x81\xbe\xe6\xb6\x88\xe6\xbb\x85", "Ball41236SP, BallPow41236SP", kEntries001_12, 3 },
	{ 354, "", "\xe9\xa3\x9b\xe3\x81\xb6\xe3\x81\x9f\xe3\x81\xbe\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Ball41236SP, BallPow41236SP", kEntries001_12, 3 },
	{ 355, "", "\xe3\x81\x8a\xe3\x81\x84\xe3\x81\xa6\xe3\x81\x84\xe3\x81\x8f\xe3\x83\x95\xe3\x83\xac\xe3\x82\xa2", "Ball41236SP, BallPow41236SP", kEntries001_13, 4 },
	{ 356, "", "\xe3\x81\xa8\xe3\x81\xa9\xe3\x82\x81\xe7\x88\x86\xe7\x99\xba", "Ball41236SP, BallPow41236SP", kEntries001_14, 3 },
	{ 358, "", "\xe3\x83\xac\xe3\x83\xb3\xe3\x82\xba\xe3\x83\x95\xe3\x83\xac\xe3\x82\xa2", "41236SP", kEntries001_15, 1 },
	{ 359, "", "\xe5\x9c\xb0\xe9\x9d\xa2\xe7\x85\x99", "41236SP", nullptr, 0 },
	{ 360, "", "\x20\x3e\xe7\x85\x99\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 361, "", "\x20\x20\x3e\xe6\x89\x8b\xe5\x89\x8d\xe7\x85\x99", "", nullptr, 0 },
	{ 362, "", "\xe5\xbc\xbe\xe3\x81\xae\xe5\xbe\x8c\xe3\x82\x8d\xe3\x82\xb4\xe3\x82\xb4\xe3\x82\xb4", "Atumaru_tama", kEntries001_16, 2 },
	{ 363, "", "\xe5\xbc\xbe\xe3\x81\xae\xe5\xbe\x8c\xe3\x82\x8d\xe3\x82\xb4\xe3\x82\xb4\xe3\x82\xb4", "Ball41236SP, BallPow41236SP", kEntries001_16, 2 },
	{ 380, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\x8b\xe5\x88\x80\xe5\x89\x8d", "jump_otoko", nullptr, 0 },
	{ 381, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\x8b\xe5\x88\x80\xe5\xbe\x8c", "", nullptr, 0 },
	{ 382, "", "\xe6\x8d\xa8\xe3\x81\xa6\xe3\x82\x8b\xe9\x9e\x98", "", nullptr, 0 },
	{ 383, "", "\xe8\x90\xbd\xe3\x81\xa8\xe3\x81\x99\xe5\x88\x80", "", nullptr, 0 },
	{ 395, "", "", "", kEntries001_17, 4 },
	{ 396, "", "", "", kEntries001_17, 4 },
	{ 401, "", "\xe5\x85\x89\xe3\x82\x8b\xe7\x9c\xbc", "", nullptr, 0 },
	{ 404, "", "\xe5\x88\x80\xe3\x82\xad\xe3\x83\xa3\xe3\x83\x83\xe3\x83\x81", "LastArcMng", nullptr, 0 },
};

const unsigned char kEntries002_0[] = { 208 };
const unsigned char kEntries002_1[] = { 241 };
const unsigned char kEntries002_2[] = { 241, 244 };
const unsigned char kEntries002_3[] = { 241, 242 };
const unsigned char kEntries002_4[] = { 240, 241, 242 };
const unsigned char kEntries002_5[] = { 241, 248 };
const unsigned char kEntries002_6[] = { 248 };

const Row kEffects002[] = {
	{ 101, "", "", "", kEntries002_0, 1 },
	{ 102, "", "B", "StdB_End", kEntries002_1, 1 },
	{ 103, "", "\xe3\x81\x9f\xe3\x82\x81\x42", "", kEntries002_1, 1 },
	{ 104, "", "2A", "", kEntries002_1, 1 },
	{ 105, "", "2B", "", kEntries002_1, 1 },
	{ 106, "", "", "", kEntries002_0, 1 },
	{ 107, "", "JA", "", kEntries002_1, 1 },
	{ 108, "", "JB", "", kEntries002_1, 1 },
	{ 116, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries002_1, 1 },
	{ 117, "", "B_B", "StdSC", kEntries002_1, 1 },
	{ 211, "", "\xe6\x94\xbb\xe6\x92\x83\xe9\x83\xa8\xe5\x88\x86", "Ball236A, Ball236B, Ball236BC, BallJ236B", kEntries002_2, 2 },
	{ 212, "", "\xe3\x82\xa2\xe3\x83\x8b\xe3\x83\xa1\xe7\xb5\x82\xe4\xba\x86\xe6\xb6\x88\xe6\xbb\x85", "Ball236A, Ball236B, Ball236BC, BallJ236B", kEntries002_2, 2 },
	{ 213, "", "\xe5\x96\xb0\xe3\x82\x89\xe3\x81\xa3\xe3\x81\xa6\xe6\xb6\x88\xe6\xbb\x85", "Ball236A, Ball236B, Ball236BC, BallJ236B", kEntries002_2, 2 },
	{ 215, "", "\xe6\x94\xbb\xe6\x92\x83\xe9\x83\xa8\xe5\x88\x86", "Ball236EX", kEntries002_2, 2 },
	{ 216, "", "\xe3\x82\xa2\xe3\x83\x8b\xe3\x83\xa1\xe7\xb5\x82\xe4\xba\x86\xe6\xb6\x88\xe6\xbb\x85", "Ball236EX", kEntries002_2, 2 },
	{ 217, "", "\xe5\x96\xb0\xe3\x82\x89\xe3\x81\xa3\xe3\x81\xa6\xe6\xb6\x88\xe6\xbb\x85", "Ball236EX", kEntries002_2, 2 },
	{ 239, "", "A", "214A", kEntries002_2, 2 },
	{ 240, "", "B", "214A, 214B", kEntries002_2, 2 },
	{ 241, "", "EX", "214BC, 214EX", kEntries002_2, 2 },
	{ 312, "", "", "63214EX_Hit", kEntries002_1, 1 },
	{ 360, "", "", "41236SP", kEntries002_1, 1 },
	{ 361, "", "\xe5\x9c\xb0\xe9\x9d\xa2", "41236SP", kEntries002_1, 1 },
	{ 363, "", "", "41236SP_Hit", kEntries002_3, 2 },
	{ 365, "", "\xe7\x81\xab\xe6\x9f\xb1", "41236SP_Hit", kEntries002_4, 3 },
	{ 366, "", "\xe7\x81\xab\xe6\x9f\xb1\xe6\x8a\x9c\xe3\x81\x91", "41236SP_End", kEntries002_4, 3 },
	{ 368, "", "", "", kEntries002_1, 1 },
	{ 369, "", "", "", kEntries002_1, 1 },
	{ 371, "", "\xe6\x8a\x95\xe3\x81\x92\xe9\x9b\x86\xe4\xb8\xad", "41236SP", kEntries002_1, 1 },
	{ 372, "", "\xe6\x8a\x95\xe3\x81\x92\xe7\x88\x86\xe7\x99\xba", "41236SP", kEntries002_1, 1 },
	{ 409, "", "\xe7\xab\x9c\xe5\xb7\xbb\xe7\x94\xa8\xe3\x83\x95\xe3\x83\xac\xe3\x82\xa2", "", kEntries002_5, 2 },
	{ 410, "", "\xe5\x9c\xb0\xe9\x9d\xa2\xe3\x81\xbc\xe3\x82\x8f\xe3\x81\xbc\xe3\x82\x8f", "LastArc_Hit", kEntries002_3, 2 },
	{ 411, "", "\xe5\xb7\xbb\xe3\x81\x8d\xe4\xb8\x8a\xe3\x81\x92", "LastArc_Hit", kEntries002_1, 1 },
	{ 415, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries002_5, 2 },
	{ 416, "", "\xe6\x8c\x9f\xe3\x82\x80\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries002_1, 1 },
	{ 417, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe7\x94\xa8\xe3\x83\x95\xe3\x83\xac\xe3\x82\xa2", "", kEntries002_6, 1 },
	{ 419, "", "\xe7\x9b\xb8\xe6\x89\x8b\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "", kEntries002_3, 2 },
	{ 420, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries002_1, 1 },
	{ 421, "", "\xe5\xbe\x8c\xe3\x82\x8d\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries002_1, 1 },
	{ 425, "", "\xe6\x89\x8b\xe5\x89\x8d\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe7\x94\xa8\xe3\x83\x95\xe3\x83\xac\xe3\x82\xa2", "", kEntries002_6, 1 },
	{ 426, "", "\xe6\x89\x8b\xe5\x89\x8d\xe7\xa0\xb4\xe7\x89\x87\xe3\x83\xab\xe3\x83\xbc\xe3\x83\x97", "", kEntries002_1, 1 },
	{ 429, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe5\x91\xbc\xe3\x81\xb3\xe5\x87\xba\xe3\x81\x97", "", nullptr, 0 },
	{ 430, "", "\xe7\xb4\xb0\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb6\xe3\x83\xbc", "", kEntries002_5, 2 },
	{ 431, "", "\xe5\xa4\xaa\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb6\xe3\x83\xbc", "", kEntries002_5, 2 },
	{ 432, "", "\xe7\xb4\xb0\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb6\xe3\x83\xbc", "", kEntries002_5, 2 },
	{ 433, "", "\xe7\xb4\xb0\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb6\xe3\x83\xbc", "", kEntries002_5, 2 },
	{ 434, "", "\xe5\xa4\xaa\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb6\xe3\x83\xbc", "", kEntries002_5, 2 },
	{ 437, "", "\xe6\xb6\x88\xe6\xbb\x85\xe5\x8a\xb9\xe6\x9e\x9c", "", nullptr, 0 },
};

const unsigned char kEntries003_0[] = { 177, 178 };
const unsigned char kEntries003_1[] = { 88, 178 };
const unsigned char kEntries003_2[] = { 243, 246, 247 };
const unsigned char kEntries003_3[] = { 2, 4 };
const unsigned char kEntries003_4[] = { 243, 247 };
const unsigned char kEntries003_5[] = { 246, 247, 250 };
const unsigned char kEntries003_6[] = { 246, 247 };
const unsigned char kEntries003_7[] = { 241, 247, 250, 252 };

const Row kEffects003[] = {
	{ 101, "", "", "", kEntries003_0, 2 },
	{ 102, "", "", "", kEntries003_1, 2 },
	{ 103, "", "C", "StdC_End", kEntries003_2, 3 },
	{ 104, "", "", "", kEntries003_3, 2 },
	{ 105, "", "2B", "", kEntries003_2, 3 },
	{ 106, "", "2B2", "", kEntries003_2, 3 },
	{ 107, "", "2C", "", kEntries003_2, 3 },
	{ 108, "", "JB", "", kEntries003_2, 3 },
	{ 109, "", "JC", "AirC_End, AirDiveSC, AirSC, DiveSC", kEntries003_4, 2 },
	{ 110, "", "", "", kEntries003_3, 2 },
	{ 111, "", "\xe6\x8a\x95\xe3\x81\x92", "0202D_Throw_F_Hit", kEntries003_2, 3 },
	{ 112, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92", "", kEntries003_2, 3 },
	{ 114, "", "\xe7\x99\xbb\xe5\xa0\xb4\xe3\x83\x8a\xe3\x82\xa4\xe3\x83\x95\xe5\x85\x89", "", kEntries003_5, 3 },
	{ 116, "", "BB", "214A_Add, 214BC, 214EX, StdSC", kEntries003_2, 3 },
	{ 117, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries003_2, 3 },
	{ 118, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries003_2, 3 },
	{ 209, "", "6BC1", "236BC", kEntries003_2, 3 },
	{ 210, "", "236", "236A, 236B, 236EX", kEntries003_2, 3 },
	{ 213, "", "\x32\x33\x36\x45\x58\xe3\x81\xa8\xe3\x81\xa9\xe3\x82\x81", "236EX", kEntries003_2, 3 },
	{ 216, "", "6BC2", "236BC", kEntries003_2, 3 },
	{ 240, "", "\xe3\x82\xb9\xe3\x83\xa9", "214A, 214B, 214BC", kEntries003_6, 2 },
	{ 241, "", "\xe3\x82\xb9\xe3\x83\xa9", "214EX", kEntries003_6, 2 },
	{ 260, "", "623", "623A, 623B, 623BC, J236A, J236B, J236BC", kEntries003_2, 3 },
	{ 262, "", "623EX", "0202EX, 623EX, J236EX", kEntries003_2, 3 },
	{ 320, "", "", "0202A_Hit, 0202BC_Hit", kEntries003_2, 3 },
	{ 321, "", "", "0202B", kEntries003_2, 3 },
	{ 325, "", "", "0202C, 236BC", kEntries003_2, 3 },
	{ 361, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries003_2, 3 },
	{ 363, "", "\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe6\x96\xac\xe3\x82\x8a\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "41236SP_Hit", kEntries003_2, 3 },
	{ 364, "", "\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe6\x96\xac\xe3\x82\x8a\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\xe5\x88\x9d\xe6\xae\xb5", "41236SP", kEntries003_2, 3 },
	{ 366, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe8\xa1\x80", "", kEntries003_7, 4 },
	{ 953, "", "\xe3\x80\x80\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe3\x83\x80\xe3\x83\x9f\xe3\x83\xbc", "", nullptr, 0 },
};

const unsigned char kEntries004_0[] = { 1, 2 };
const unsigned char kEntries004_1[] = { 160, 249 };
const unsigned char kEntries004_2[] = { 160, 161 };
const unsigned char kEntries004_3[] = { 136, 137, 249 };
const unsigned char kEntries004_4[] = { 192 };
const unsigned char kEntries004_5[] = { 160, 161, 249 };
const unsigned char kEntries004_6[] = { 247, 248 };
const unsigned char kEntries004_7[] = { 246, 247, 248 };
const unsigned char kEntries004_8[] = { 247, 249 };
const unsigned char kEntries004_9[] = { 160 };

const Row kEffects004[] = {
	{ 101, "", "", "", kEntries004_0, 2 },
	{ 102, "", "B", "", kEntries004_1, 2 },
	{ 103, "", "C", "", kEntries004_1, 2 },
	{ 104, "", "2B", "", kEntries004_1, 2 },
	{ 105, "", "2B", "", kEntries004_1, 2 },
	{ 106, "", "2C", "", kEntries004_1, 2 },
	{ 107, "", "", "", kEntries004_0, 2 },
	{ 108, "", "JB", "AirSC", kEntries004_1, 2 },
	{ 109, "", "JC", "AirC_End, AirDiveSC, DiveSC, P_AtkC", kEntries004_1, 2 },
	{ 110, "", "", "", kEntries004_2, 2 },
	{ 112, "", "\xe6\x8a\x9c\xe5\x88\x80\xe5\x85\x89", "236A, 236A_End, 236B, 236BC, 236B_End, 236EX", nullptr, 0 },
	{ 113, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries004_3, 3 },
	{ 114, "", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\xa0\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe8\x90\xbd\xe3\x81\xa8\xe3\x81\x99\xe7\xae\x92\xef\xbc\x92", "", nullptr, 0 },
	{ 115, "", "\xe6\x95\x97\xe5\x8c\x97\xe8\x90\xbd\xe3\x81\xa8\xe3\x81\x99\xe7\xae\x92", "", nullptr, 0 },
	{ 116, "", "\xe6\x95\x97\xe5\x8c\x97\xe3\x81\xb3\xe3\x81\xa3\xe3\x81\x8f\xe3\x82\x8a", "", nullptr, 0 },
	{ 118, "", "\xe7\x99\xbb\xe5\xa0\xb4\xe3\x83\x93\xe3\x83\xb3", "", kEntries004_4, 1 },
	{ 119, "", "\xe7\x99\xbb\xe5\xa0\xb4\xe7\x85\x99", "", nullptr, 0 },
	{ 121, "", "\xe5\x8b\x9d\xe5\x88\xa9\xe3\x82\xa6\xe3\x82\xa3\xe3\x83\xb3\xe3\x82\xaf", "", nullptr, 0 },
	{ 123, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "", nullptr, 0 },
	{ 127, "", "", "StdSC", kEntries004_5, 3 },
	{ 128, "", "\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\xbc\xe3\x83\xb3", "2C_2C, 6C", nullptr, 0 },
	{ 129, "", "\xe3\x83\x8f\xe3\x83\xaa\xe3\x82\xbb\xe3\x83\xb3", "6C, 6C_End", kEntries004_1, 2 },
	{ 182, "", "\xe9\xa3\x9b\xe3\x81\xb6\xe6\x96\xac\xe6\x92\x83\x31", "Ball236A", kEntries004_6, 2 },
	{ 183, "", "\xe9\xa3\x9b\xe3\x81\xb6\xe6\x96\xac\xe6\x92\x83\x31\xe6\xb6\x88\xe6\xbb\x85", "Ball236A", kEntries004_7, 3 },
	{ 184, "", "\xe9\xa3\x9b\xe3\x81\xb6\xe6\x96\xac\xe6\x92\x83\x32", "Ball236B", kEntries004_6, 2 },
	{ 185, "", "\xe9\xa3\x9b\xe3\x81\xb6\xe6\x96\xac\xe6\x92\x83\x32\xe6\xb6\x88\xe6\xbb\x85", "Ball236B", kEntries004_7, 3 },
	{ 205, "", "236A", "236A, 236A_End, 236BC", kEntries004_8, 2 },
	{ 206, "", "236B", "236B, 236BC, 236B_End", kEntries004_8, 2 },
	{ 207, "", "236[B]", "236B", kEntries004_8, 2 },
	{ 208, "", "\x32\x33\x36\xe8\xbf\xbd\xe5\x8a\xa0\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x91\xe3\x83\xbc", "236_Add", kEntries004_1, 2 },
	{ 209, "", "236EX1", "236BC", kEntries004_8, 2 },
	{ 210, "", "236EX2", "236BC, 236EX_Hit", kEntries004_8, 2 },
	{ 211, "", "236EX3", "236BC, 236EX_Hit", kEntries004_8, 2 },
	{ 212, "", "\x32\x33\x36\x45\x58\xe7\x94\x9f\xe6\x88\x90", "236EX", nullptr, 0 },
	{ 289, "", "\xe3\x83\x89\xe3\x83\xaa\xe3\x83\xab\xe5\x9b\x9e\xe8\xbb\xa2\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "DrillTree, DrillTree1, DrillTree2", nullptr, 0 },
	{ 290, "", "\xe5\x9b\x9e\xe8\xbb\xa2\xe5\x9c\x9f\xe7\x85\x99", "DrillTree, DrillTree1, DrillTree2", nullptr, 0 },
	{ 291, "", "\xe9\xa3\x9b\xe3\x81\xb3\xe4\xb8\x8a\xe3\x81\x8c\xe3\x82\x8a\xe7\x85\x99", "DrillTree, DrillTree1, DrillTree2", nullptr, 0 },
	{ 293, "", "\xe5\x9b\x9e\xe8\xbb\xa2\xe6\xa4\x8d\xe6\x9c\xa8\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "GroundSpinTree, GroundSpinTreePow", nullptr, 0 },
	{ 295, "", "\xe3\x82\xb5\xe3\x83\x9c\xe3\x83\x86\xe3\x83\xb3\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xef\xbc\x91", "PunchingTree", nullptr, 0 },
	{ 296, "", "\xe3\x82\xb5\xe3\x83\x9c\xe3\x83\x86\xe3\x83\xb3\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xef\xbc\x92", "PunchingTree", nullptr, 0 },
	{ 297, "", "\xe3\x82\xb5\xe3\x83\x9c\xe3\x83\x86\xe3\x83\xb3\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xef\xbc\x91\xef\xbc\x8b", "PunchingTree", nullptr, 0 },
	{ 360, "", "\xe7\x85\x99", "41236SP_Guard, 41236SP_Hit", nullptr, 0 },
	{ 361, "", "\xe7\x85\x99\xe5\xb9\x95\xe3\x83\x93\xe3\x83\xb3", "41236SP_Guard, 41236SP_Hit", kEntries004_4, 1 },
	{ 367, "", "\x41\x44\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\xef\xbc\x91", "41236SP_Hit", kEntries004_9, 1 },
	{ 368, "", "\x41\x44\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\xef\xbc\x92", "41236SP_Hit", kEntries004_9, 1 },
	{ 369, "", "\x41\x44\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\xef\xbc\x93", "41236SP_Hit", kEntries004_9, 1 },
	{ 425, "", "\xe6\xb1\xba\xe3\x82\x81\xe5\x8a\xb9\xe6\x9e\x9c\xe7\xb7\x9a", "", nullptr, 0 },
};

const unsigned char kEntries005_0[] = { 170, 202 };
const unsigned char kEntries005_1[] = { 170 };
const unsigned char kEntries005_2[] = { 81, 82, 103, 104 };
const unsigned char kEntries005_3[] = { 103, 104 };
const unsigned char kEntries005_4[] = { 170, 201, 202, 243, 244 };
const unsigned char kEntries005_5[] = { 242, 243, 244 };
const unsigned char kEntries005_6[] = { 243, 244, 245 };
const unsigned char kEntries005_7[] = { 243, 244, 252 };
const unsigned char kEntries005_8[] = { 202, 243 };
const unsigned char kEntries005_9[] = { 243, 244, 245, 247, 248, 252 };
const unsigned char kEntries005_10[] = { 242, 243, 244, 245 };
const unsigned char kEntries005_11[] = { 243 };
const unsigned char kEntries005_12[] = { 243, 244 };
const unsigned char kEntries005_13[] = { 243, 244, 245, 247 };
const unsigned char kEntries005_14[] = { 245 };
const unsigned char kEntries005_15[] = { 243, 244, 245, 252, 254 };
const unsigned char kEntries005_16[] = { 243, 244, 245, 248, 252, 254 };
const unsigned char kEntries005_17[] = { 243, 244, 245, 254 };
const unsigned char kEntries005_18[] = { 243, 244, 245, 252 };
const unsigned char kEntries005_19[] = { 2, 81, 82, 103, 104 };
const unsigned char kEntries005_20[] = { 244, 254 };
const unsigned char kEntries005_21[] = { 243, 244, 245, 252, 253, 254 };

const Row kEffects005[] = {
	{ 102, "", "", "", kEntries005_0, 2 },
	{ 103, "", "", "", kEntries005_1, 1 },
	{ 104, "", "", "", kEntries005_2, 4 },
	{ 105, "", "", "", kEntries005_0, 2 },
	{ 106, "", "", "", kEntries005_1, 1 },
	{ 107, "", "", "", kEntries005_3, 2 },
	{ 108, "", "", "", kEntries005_0, 2 },
	{ 109, "", "", "6C, AirDiveSC, AirSC, DiveSC", kEntries005_4, 5 },
	{ 110, "", "", "", kEntries005_3, 2 },
	{ 111, "", "", "RapidRelayAtk", kEntries005_5, 3 },
	{ 115, "", "104", "2C_2C", kEntries005_1, 1 },
	{ 117, "", "\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3", "6C", kEntries005_6, 3 },
	{ 122, "", "\xe6\x8a\x95\xe3\x81\x92", "", nullptr, 0 },
	{ 123, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe8\xa1\x80", "", nullptr, 0 },
	{ 124, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92", "", nullptr, 0 },
	{ 126, "", "\xe6\xb5\xae\xe3\x81\x8b\xe3\x81\x9b\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries005_6, 3 },
	{ 161, "", "\x4a\x32\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "J2C", kEntries005_7, 3 },
	{ 180, "", "\xe3\x82\xb1\xe3\x83\xaa\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "6C_6C", kEntries005_8, 2 },
	{ 185, "", "\xe6\x89\x8b\xe3\x81\xae\xe5\x85\x89", "214A, 214BC, 214B_Add", kEntries005_6, 3 },
	{ 186, "", "\xe6\x89\x8b\xe3\x81\xae\xe5\x85\x89\x42", "214A_Add, 214A_Add_End, 214B, 214B_End, 214EX", kEntries005_6, 3 },
	{ 215, "", "\xe6\xb0\xb4\xe5\xb9\xb3\xe9\x9b\xb7\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Tama214A, Tama214B_Add", kEntries005_9, 6 },
	{ 217, "", "\xe9\x9b\xb7\xe7\x9d\x80\xe5\x9c\xb0", "Tama214A_Add, Tama214A_Add_Rapid, Tama214B, Tama214B_Rapid, Tama214EX", kEntries005_10, 4 },
	{ 218, "", "\xe3\x81\x9f\xe3\x82\x81\x42\xe6\x96\x9c\xe3\x82\x81\xe9\x9b\xb7\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Tama214A_Add, Tama214A_Add_Rapid, Tama214B, Tama214B_Rapid", kEntries005_10, 4 },
	{ 219, "", "\x45\x58\xe6\x96\x9c\xe3\x82\x81\xe9\x9b\xb7\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Tama214EX", kEntries005_10, 4 },
	{ 220, "", "\xef\xbc\x93\x57\x41\x59\xe9\x9b\xb7\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Tama214BC", kEntries005_9, 6 },
	{ 237, "", "\xe4\xbd\x93\xe3\x83\x93\xe3\x83\xaa\xe3\x83\x93\xe3\x83\xaa\x31", "623A, 623B, 623BC, 623EX", kEntries005_11, 1 },
	{ 238, "", "\xe4\xbd\x93\xe3\x83\x93\xe3\x83\xaa\xe3\x83\x93\xe3\x83\xaa\x32", "623A, 623B, 623BC, 623EX", kEntries005_12, 2 },
	{ 239, "", "\xe6\x94\xbe\xe5\x87\xba", "623A, 623B, 623BC, 623EX", kEntries005_6, 3 },
	{ 240, "", "\xe7\xaa\x81\xe9\x80\xb2\xe7\xa7\xbb\xe5\x8b\x95", "236A, 236A_Add, 236B, 236BC, 236B_End, 236EX", kEntries005_13, 4 },
	{ 241, "", "\xe9\x9b\xb7\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 307, "", "\xe3\x80\x80\xe3\x81\x8a\xe3\x81\x84\xe3\x81\xa6\xe3\x81\x84\xe3\x81\x8f\xe5\xb0\x8f\xe3\x81\x95\xe3\x81\x84\xe9\x9b\xb7", "41236SP_End", kEntries005_6, 3 },
	{ 310, "", "\x41\x44\xe8\x83\x8c\xe6\x99\xaf", "41236SP_Hit", kEntries005_14, 1 },
	{ 311, "", "\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3", "41236SP_Hit", kEntries005_6, 3 },
	{ 313, "", "\xe3\x81\xa8\xe3\x81\xa9\xe3\x82\x81\xe8\x90\xbd\xe9\x9b\xb7", "41236SP_Hit", kEntries005_15, 5 },
	{ 320, "", "\xe9\xa3\x9b\xe3\x81\xb3\xe9\x81\x93\xe5\x85\xb7", "41236SP", kEntries005_16, 6 },
	{ 321, "", "", "41236SP_Hit", kEntries005_17, 4 },
	{ 322, "", "", "41236SP_Hit", kEntries005_18, 4 },
	{ 358, "", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe3\x83\xad\xe3\x82\xa2\xef\xbc\x8b\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88\xe3\x82\x88\xe3\x81\xb3", "", nullptr, 0 },
	{ 360, "", "\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3", "LastArc_Hit", kEntries005_6, 3 },
	{ 361, "", "\xe7\x88\x86\xe7\x99\xba", "LastArc_Hit", kEntries005_6, 3 },
	{ 362, "", "", "LastArc_Hit", kEntries005_19, 5 },
	{ 364, "", "\xe8\x9b\x87", "", kEntries005_20, 2 },
	{ 366, "", "\xe5\xa4\xa7\xe8\x9b\x87\xe3\x83\xbb\xe7\xae\xa1\xe7\x90\x86", "", kEntries005_20, 2 },
	{ 367, "", "\xe3\x80\x80\xe5\xad\x90\xe8\x9b\x87\xef\xbc\x86\xe3\x83\x96\xe3\x83\xa9\xe3\x83\x83\xe3\x82\xaf\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "", kEntries005_21, 6 },
	{ 368, "", "\xe3\x80\x80\xe5\xad\x90\xe8\x9b\x87", "", nullptr, 0 },
};

const unsigned char kEntries006_0[] = { 97, 98 };
const unsigned char kEntries006_1[] = { 97 };
const unsigned char kEntries006_2[] = { 242, 243, 244, 245 };
const unsigned char kEntries006_3[] = { 242, 243, 244 };
const unsigned char kEntries006_4[] = { 87, 97, 102 };
const unsigned char kEntries006_5[] = { 242, 243 };
const unsigned char kEntries006_6[] = { 242, 243, 244, 245, 250 };
const unsigned char kEntries006_7[] = { 242, 243, 245 };
const unsigned char kEntries006_8[] = { 242, 245 };
const unsigned char kEntries006_9[] = { 242, 244 };
const unsigned char kEntries006_10[] = { 243, 244, 245 };
const unsigned char kEntries006_11[] = { 243 };
const unsigned char kEntries006_12[] = { 16, 64, 96, 145 };

const Row kEffects006[] = {
	{ 101, "", "", "", kEntries006_0, 2 },
	{ 102, "", "", "", kEntries006_1, 1 },
	{ 103, "", "003", "StdC_End", kEntries006_2, 4 },
	{ 104, "", "", "", kEntries006_1, 1 },
	{ 105, "", "", "", kEntries006_1, 1 },
	{ 106, "", "2C", "CroC_End", kEntries006_3, 3 },
	{ 107, "", "", "", kEntries006_0, 2 },
	{ 108, "", "", "", kEntries006_1, 1 },
	{ 109, "", "JC", "0202BC, AirC_End, AirDiveSC, AirSC, DiveSC", kEntries006_2, 4 },
	{ 111, "", "\xe6\x8a\x95\xe3\x81\x92", "", kEntries006_2, 4 },
	{ 112, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92", "", kEntries006_3, 3 },
	{ 114, "", "\xe6\x8d\xa8\xe3\x81\xa6\xe3\x82\x8b\xe6\x9c\x8d", "", nullptr, 0 },
	{ 115, "", "\xe5\x8b\x9d\xe5\x88\xa9", "", kEntries006_2, 4 },
	{ 116, "", "", "", kEntries006_2, 4 },
	{ 118, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries006_2, 4 },
	{ 120, "", "\xe7\x87\x83\xe7\x84\xbc", "214BC_Hit, 214EX_Hit, 214_Hit", kEntries006_2, 4 },
	{ 122, "", "\xe7\x88\x86\xe7\x99\xba\xe5\xa4\xa7", "214EX_Hit, 214_Hit", kEntries006_2, 4 },
	{ 128, "", "", "StdSC", kEntries006_3, 3 },
	{ 145, "", "", "", kEntries006_2, 4 },
	{ 146, "", "", "", kEntries006_2, 4 },
	{ 161, "", "", "StdC_Hit", kEntries006_4, 3 },
	{ 210, "", "236", "236A, 236B_End", kEntries006_2, 4 },
	{ 211, "", "236", "236BC, 236_Add, 236_Add4", kEntries006_2, 4 },
	{ 212, "", "236", "236BC, 236_236_Add, 236_236_Add4, 623_JAdd, J236BC, J236_JAdd", kEntries006_2, 4 },
	{ 213, "", "236[B]", "236B", kEntries006_2, 4 },
	{ 215, "", "236EX", "236EX", kEntries006_2, 4 },
	{ 216, "", "\x32\x33\x36\x45\x58\xe7\x88\x86\xe7\x99\xba", "236EX", kEntries006_2, 4 },
	{ 242, "", "", "214BC_Hit, 214_AddEX", kEntries006_2, 4 },
	{ 245, "", "", "214A, 214B, 214BC, 214EX", kEntries006_5, 2 },
	{ 247, "", "", "214EX_Hit", kEntries006_2, 4 },
	{ 248, "", "", "214EX_Hit", kEntries006_2, 4 },
	{ 260, "", "623", "623A, 623B, 623BC, J236A, J236B", kEntries006_2, 4 },
	{ 290, "", "\xe3\x83\x80\xe3\x83\x9f\xe3\x83\xbc", "0202B, 0202BC", nullptr, 0 },
	{ 328, "", "\x41\x44\xe3\x81\xa4\xe3\x81\x8b\xe3\x81\xbf\xe3\x82\xad\xe3\x83\xa9\xe3\x83\x83", "41236SP_Hit", kEntries006_6, 5 },
	{ 330, "", "\xe7\x82\x8e\xe9\x96\x8b\xe6\x94\xbe", "41236SP", kEntries006_2, 4 },
	{ 332, "", "\xe7\x87\x83\xe3\x81\x88\xe3\x82\x8b\xe6\x89\x8b", "41236SP", kEntries006_2, 4 },
	{ 334, "", "\xe6\x8a\x95\xe3\x81\x92\xe7\xaa\x81\xe3\x81\x8d\xe7\x88\x86\xe7\x99\xba", "41236SP_Hit", kEntries006_2, 4 },
	{ 336, "", "\xe6\x8c\x81\xe3\x81\xa1\xe4\xb8\x8a\xe3\x81\x92\xe3\x82\x8b\xe7\x82\x8e\xe3\x81\xae\xe6\x89\x8b", "41236SP_Hit", kEntries006_7, 3 },
	{ 338, "", "", "41236SP_Hit", kEntries006_2, 4 },
	{ 339, "", "", "41236SP_Hit", kEntries006_2, 4 },
	{ 406, "", "\xe5\xa4\x9a\xe6\xae\xb5\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "", nullptr, 0 },
	{ 410, "", "", "LastArc_Hit", kEntries006_2, 4 },
	{ 411, "", "", "LastArc_Hit", kEntries006_2, 4 },
	{ 412, "", "", "", kEntries006_2, 4 },
	{ 415, "", "\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 416, "", "\xe5\x8f\xa9\xe3\x81\x8d\xe3\x81\xa4\xe3\x81\x91\xe7\x88\x86\xe7\x99\xba", "", kEntries006_2, 4 },
	{ 417, "", "\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab\xe7\x94\x9f\xe6\x88\x90", "LastArc_Hit", nullptr, 0 },
	{ 420, "", "\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "", kEntries006_2, 4 },
	{ 421, "", "\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\xe3\x82\xa4\xe3\x83\xb3", "", kEntries006_8, 2 },
	{ 424, "", "\xe7\x88\x86\xe7\x99\xba\xe5\x86\x86", "", kEntries006_2, 4 },
	{ 425, "", "\xe7\x88\x86\xe7\x99\xba\xe5\x86\x86\xef\xbc\x92", "", kEntries006_9, 2 },
	{ 426, "", "BG", "", kEntries006_9, 2 },
	{ 427, "", "\xe6\x89\x8b\xe5\x89\x8d\xe7\x82\x8e", "", kEntries006_10, 3 },
	{ 428, "", "\xe6\x89\x8b\xe5\x89\x8d\xe3\x83\x81\xe3\x83\xaa", "", kEntries006_11, 1 },
	{ 430, "", "\xe7\x88\x86\xe7\x99\xba\xe5\x86\x86\xe6\x8a\x9c\xe3\x81\x91", "LastArc_End", kEntries006_2, 4 },
	{ 431, "", "\xe6\x89\x8b\xe5\x89\x8d\xe7\x82\x8e", "LastArc_End", kEntries006_10, 3 },
	{ 432, "", "", "", kEntries006_12, 4 },
	{ 433, "", "\xe6\x8a\x9c\xe3\x81\x91\x42\x47", "LastArc_End", kEntries006_9, 2 },
};

const unsigned char kEntries008_0[] = { 243, 244, 245 };
const unsigned char kEntries008_1[] = { 129, 243, 244, 245 };
const unsigned char kEntries008_2[] = { 242, 244, 245 };
const unsigned char kEntries008_3[] = { 242, 243, 244, 245 };
const unsigned char kEntries008_4[] = { 243, 245 };
const unsigned char kEntries008_5[] = { 129 };
const unsigned char kEntries008_6[] = { 129, 243, 245 };
const unsigned char kEntries008_7[] = { 160, 162, 166, 167 };
const unsigned char kEntries008_8[] = { 128, 129 };
const unsigned char kEntries008_9[] = { 129, 243, 244 };
const unsigned char kEntries008_10[] = { 49, 80, 81 };
const unsigned char kEntries008_11[] = { 160, 166, 244 };
const unsigned char kEntries008_12[] = { 160, 166, 243, 244 };
const unsigned char kEntries008_13[] = { 246 };

const Row kEffects008[] = {
	{ 4, "", "\xe3\x81\x97\xe3\x82\x83\xe3\x81\x8c\xe3\x81\xbf\xe5\xbc\xb1\xe6\x94\xbb\xe6\x92\x83", "AD_CutA, AD_CutB, AD_CutC, AD_CutD", nullptr, 0 },
	{ 100, "", "JC", "", kEntries008_0, 3 },
	{ 101, "", "A", "", kEntries008_0, 3 },
	{ 102, "", "B", "41236SP_Hit", kEntries008_1, 4 },
	{ 103, "", "C", "", kEntries008_2, 3 },
	{ 104, "", "2B", "", kEntries008_3, 4 },
	{ 105, "", "2B", "41236SP_Hit", kEntries008_4, 2 },
	{ 106, "", "2C", "41236SP_Hit", kEntries008_0, 3 },
	{ 107, "", "101", "", kEntries008_5, 1 },
	{ 108, "", "JB", "", kEntries008_6, 3 },
	{ 109, "", "JC", "AirC_End, AirDiveSC, AirSC, DiveSC", kEntries008_0, 3 },
	{ 110, "", "201", "", kEntries008_4, 2 },
	{ 111, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "41236SP, StdSC", kEntries008_3, 4 },
	{ 112, "", "\xe7\x88\x86\xe7\x99\xba", "", nullptr, 0 },
	{ 113, "", "\x33\x30\xe9\x80\xa3", "41236SP_Hit, RapidRelayAtk", kEntries008_4, 2 },
	{ 114, "", "\x33\x30\xe9\x80\xa3", "41236SP_Hit, RapidRelayAtk", kEntries008_4, 2 },
	{ 115, "", "\xe7\x99\xbb\xe5\xa0\xb4\x31", "41236SP_End, LastArc_End", kEntries008_7, 4 },
	{ 116, "", "\xe7\x99\xbb\xe5\xa0\xb4\x32", "41236SP_End, LastArc_End", kEntries008_7, 4 },
	{ 118, "", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\xa0\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97", "", nullptr, 0 },
	{ 120, "", "\xe6\x8a\x95\xe3\x81\x92", "", kEntries008_8, 2 },
	{ 121, "", "\xe6\x8a\x95\xe3\x81\x92", "", kEntries008_9, 3 },
	{ 122, "", "\xe6\x8a\x95\xe3\x81\x92", "", kEntries008_5, 1 },
	{ 145, "", "", "", kEntries008_0, 3 },
	{ 203, "", "\xe9\xbb\x92\xe9\x8d\xb5\xe6\x8a\x95\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "236A, 236B, 236BC, 236EX", kEntries008_10, 3 },
	{ 210, "", "\xe3\x82\xb5\xe3\x83\x9e\xe3\x83\xbc\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "0202A, 0202A_End, 0202B, 0202BC", kEntries008_0, 3 },
	{ 211, "", "\x45\x58\xe3\x82\xb5\xe3\x83\x9e\xe3\x83\xbc\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "0202EX", kEntries008_0, 3 },
	{ 240, "", "623B", "623A, 623B, 623BC, 623EX", kEntries008_0, 3 },
	{ 241, "", "\xe5\x9c\xb0\xe9\x9d\xa2\xe7\x88\x86\xe7\x99\xba", "214A_Add, 214B_Add, 214EX, 41236SP_Hit", nullptr, 0 },
	{ 250, "", "\xe9\xa3\x9b\xe3\x81\xb3\xe8\xb9\xb4\xe3\x82\x8a", "6C", kEntries008_6, 3 },
	{ 260, "", "", "214A, 214B, 214BC, 214EX", kEntries008_7, 4 },
	{ 261, "", "", "214A, 214B, 214BC, 214EX", kEntries008_4, 2 },
	{ 262, "", "", "214BC, 214B_Add, 214EX", kEntries008_0, 3 },
	{ 263, "", "", "214A_Add, 214B_Add, 214EX, 41236SP_Hit", kEntries008_7, 4 },
	{ 264, "", "", "214A_Add, 214B_Add, 214EX, 41236SP_Hit", kEntries008_0, 3 },
	{ 265, "", "\xe3\x81\xb2\xe3\x81\xa3\xe3\x81\xb1\xe3\x82\x8a", "214A, 214A_Hit, 214B, 214BC, 214B_Hit, 214EX", kEntries008_4, 2 },
	{ 298, "", "\xe9\xbb\x92\xe9\x8d\xb5\xe6\x8a\x95\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214BC, 236BC, 236EX, 236_AddA, 236_AddB, J236A, J236B, J236BC, J236EX", kEntries008_10, 3 },
	{ 324, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe8\xa1\x80", "", nullptr, 0 },
	{ 325, "", "\xe3\x81\xa8\xe3\x81\xa9\xe3\x82\x81\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries008_3, 4 },
	{ 329, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries008_0, 3 },
	{ 335, "", "\xe7\x81\xab\xe8\x8a\xb1\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 336, "", "\xe7\x81\xab\xe8\x8a\xb1\xe7\x94\x9f\xe6\x88\x90\x32", "41236SP", nullptr, 0 },
	{ 362, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\x8b\xe9\xbb\x92\xe9\x8d\xb5", "LastArc_Hit", kEntries008_11, 3 },
	{ 363, "", "\xe9\xbb\x92\xe9\x8d\xb5\xe3\x82\xb9\xe3\x83\xad\xe3\x83\xbc", "", kEntries008_11, 3 },
	{ 364, "", "\xe5\xb0\x8f\xe3\x81\x95\xe3\x81\x84\xe9\xbb\x92\xe9\x8d\xb5\xe2\x86\x90", "", kEntries008_11, 3 },
	{ 365, "", "\xe5\xb0\x8f\xe3\x81\x95\xe3\x81\x84\xe9\xbb\x92\xe9\x8d\xb5\xe2\x86\x92", "", kEntries008_11, 3 },
	{ 367, "", "\xe9\xbb\x92\xe9\x8d\xb5\xe8\x83\x8c\xe6\x99\xaf", "", nullptr, 0 },
	{ 370, "", "\xe8\x90\xbd\xe4\xb8\x8b\xe9\x9b\x86\xe4\xb8\xad\xe7\xb7\x9a", "LA_BigNoe", nullptr, 0 },
	{ 372, "", "\xe8\xb5\xa4", "", nullptr, 0 },
	{ 373, "", "\xe8\xa1\x80\xe3\x81\x97\xe3\x81\xb6\xe3\x81\x8d\xe5\x89\x8d", "", nullptr, 0 },
	{ 374, "", "\xe8\xa1\x80\xe3\x81\x97\xe3\x81\xb6\xe3\x81\x8d\xe5\xbe\x8c", "", nullptr, 0 },
	{ 376, "", "\xe8\xb5\xa4\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "", nullptr, 0 },
	{ 377, "", "\xe7\x9b\xb8\xe6\x89\x8b\xe6\x8b\x98\xe6\x9d\x9f", "", kEntries008_12, 4 },
	{ 380, "", "\xe5\xa4\x9a\xe6\xae\xb5\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "", nullptr, 0 },
	{ 382, "", "\xe9\xbb\x92\xe9\x8d\xb5\xe6\x80\xa5\xe9\x99\x8d\xe4\xb8\x8b\xe7\xb7\x9a", "", kEntries008_13, 1 },
	{ 383, "", "\xe9\x9b\xbb\xe6\x9f\xb1\xe5\x9c\x9f\xe7\x85\x99", "LA_Densin", nullptr, 0 },
	{ 385, "", "\xe4\xb8\x80\xe9\x96\x83\xe4\xbd\x99\xe9\x9f\xbb", "", kEntries008_0, 3 },
	{ 386, "", "\xe7\xb5\x90\xe7\x95\x8c\xe6\xb6\x88\xe6\xbb\x85", "", nullptr, 0 },
	{ 388, "", "\xe6\xb1\x8e\xe7\x94\xa8\xe9\xbb\x92\xe8\x83\x8c\xe6\x99\xaf", "", nullptr, 0 },
};

const unsigned char kEntries009_0[] = { 218 };
const unsigned char kEntries009_1[] = { 245, 246, 247, 248 };
const unsigned char kEntries009_2[] = { 241, 242, 243, 245, 246, 247, 248, 250, 251, 252 };
const unsigned char kEntries009_3[] = { 245, 246, 247, 248, 250, 251, 252 };
const unsigned char kEntries009_4[] = { 246, 247, 248 };
const unsigned char kEntries009_5[] = { 247, 248, 250, 251, 252 };
const unsigned char kEntries009_6[] = { 241, 247, 248 };
const unsigned char kEntries009_7[] = { 250 };
const unsigned char kEntries009_8[] = { 247, 250, 251, 252 };
const unsigned char kEntries009_9[] = { 247 };
const unsigned char kEntries009_10[] = { 247, 248 };
const unsigned char kEntries009_11[] = { 250, 251, 252 };
const unsigned char kEntries009_12[] = { 248 };
const unsigned char kEntries009_13[] = { 243, 247, 248 };

const Row kEffects009[] = {
	{ 101, "", "001", "", kEntries009_0, 1 },
	{ 102, "", "B", "", kEntries009_1, 4 },
	{ 103, "", "\xe6\xb5\xae\xe3\x81\x8b\xe3\x81\x9b", "StdC_End", kEntries009_2, 10 },
	{ 104, "", "101", "", kEntries009_0, 1 },
	{ 105, "", "2B", "", kEntries009_1, 4 },
	{ 106, "", "2C", "", kEntries009_3, 7 },
	{ 107, "", "201", "", kEntries009_0, 1 },
	{ 108, "", "JB", "AirSC", kEntries009_1, 4 },
	{ 109, "", "JC", "AirC_End", kEntries009_4, 3 },
	{ 110, "", "\x4a\x43\x28\x53\x43\xe7\x94\xa8\x29", "AirDiveSC, DiveSC", kEntries009_4, 3 },
	{ 111, "", "[C]", "", kEntries009_1, 4 },
	{ 112, "", "\xe5\x8b\x9d\xe5\x88\xa9\xe6\x81\xaf", "", nullptr, 0 },
	{ 113, "", "\xe7\x99\xbb\xe5\xa0\xb4\xe7\x85\x99", "", kEntries009_5, 5 },
	{ 115, "", "\xe5\x9c\xb0\xe4\xb8\x8a\xe3\x81\xa4\xe3\x81\x8b\xe3\x81\xbf\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "214A_Add_Add_Hit", kEntries009_6, 3 },
	{ 116, "", "\xe5\x99\x9b\xe3\x81\xbf\xe3\x81\xa4\xe3\x81\x8d", "", kEntries009_7, 1 },
	{ 124, "", "\x30\x33\x30\xe7\x85\x99", "030_fire, 030_ice", nullptr, 0 },
	{ 127, "", "\x41\xe9\x80\xa3\xe7\x88\x86\xe7\x99\xba\xe7\x82\x8e", "030_2_fire", kEntries009_8, 4 },
	{ 128, "", "\x41\xe9\x80\xa3\xe7\x88\x86\xe7\x99\xba\xe6\xb0\xb7", "030_2_ice", kEntries009_9, 1 },
	{ 166, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "6C", kEntries009_4, 3 },
	{ 212, "", "\xe6\xb0\xb7\xe8\xbb\x8c\xe8\xb7\xa1", "SpearSt, SpearStEX", kEntries009_10, 2 },
	{ 227, "", "\xe6\xae\x8b\xe5\x83\x8f", "214B, 214BC", nullptr, 0 },
	{ 234, "", "\x41\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A", kEntries009_4, 3 },
	{ 235, "", "\x42\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214B, 214B_End, 214EX, 214EX_Hit", kEntries009_4, 3 },
	{ 236, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A, 214B_End, 214EX_Hit", kEntries009_4, 3 },
	{ 237, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A_Add, 214EX_Hit", kEntries009_4, 3 },
	{ 238, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A_Add, 214BC, 214EX_Hit, 6C_End", kEntries009_4, 3 },
	{ 239, "", "\x42\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214BC", kEntries009_4, 3 },
	{ 243, "", "\x32\x31\x34\x45\x58\xe7\x82\x8e", "214EX_Hit", kEntries009_11, 3 },
	{ 244, "", "\x32\x31\x34\x45\x58\xe6\xb0\xb7", "214EX_Hit", kEntries009_10, 2 },
	{ 261, "", "\xe8\xb6\xb3\xe5\x85\x83\xe5\xaf\x92\xe6\xb0\x97", "320_fire, 320_ice, 325_ice", nullptr, 0 },
	{ 265, "", "\xe3\x82\xb9\xe3\x82\xab", "214A_Add_Add4", kEntries009_6, 3 },
	{ 266, "", "\xe3\x82\xb9\xe3\x82\xab\xe7\x85\x99", "214A_Add_Add4", nullptr, 0 },
	{ 362, "", "\xe3\x80\x80\xe3\x81\x8d\xe3\x82\x89\xe3\x81\x8d\xe3\x82\x89", "", kEntries009_12, 1 },
	{ 363, "", "\xe7\x99\xba\xe5\xb0\x84\xe5\x89\x8d", "", kEntries009_12, 1 },
	{ 364, "", "\xe7\x99\xba\xe5\xb0\x84", "", kEntries009_10, 2 },
	{ 365, "", "\xe3\x80\x80\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb6\xe3\x83\xbc\xe5\xbe\x85\xe6\xa9\x9f", "", kEntries009_9, 1 },
	{ 366, "", "\xe3\x80\x80\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb6\xe3\x83\xbc\xe7\x99\xba\xe5\xb0\x84", "", kEntries009_9, 1 },
	{ 370, "", "\xe6\x9c\x80\xe5\x88\x9d\xe3\x81\xae\xe8\xa1\x9d\xe6\x92\x83\xe6\xb3\xa2", "LastArc_Hit", kEntries009_4, 3 },
	{ 372, "", "\xe3\x82\x84\xe3\x82\x8a\xe5\x87\xba\xe7\x8f\xbe", "LastArc_Hit", nullptr, 0 },
	{ 373, "", "\xe3\x82\x84\xe3\x82\x8a\xe5\xbe\x85\xe6\xa9\x9f", "", nullptr, 0 },
	{ 375, "", "\xe6\xa7\x8d\xe5\x90\xb9\xe3\x81\x8d\xe9\xa3\x9b\xe3\x81\xb0\xe3\x81\x97\xe9\xa2\xa8", "", kEntries009_4, 3 },
	{ 381, "", "\xe7\x93\xa6\xe7\xa4\xab", "", kEntries009_9, 1 },
	{ 382, "", "\xe7\x93\xa6\xe7\xa4\xab", "", nullptr, 0 },
	{ 386, "", "\xe7\x88\x86\xe7\x99\xba", "BoundGareki", nullptr, 0 },
	{ 387, "", "\xe7\x88\x86\xe7\x99\xba\x32", "BoundGareki", nullptr, 0 },
	{ 391, "", "\xe5\xa4\x9a\xe6\xae\xb5\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "BoundGareki", nullptr, 0 },
	{ 395, "", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe7\xa7\xbb\xe5\x8b\x95", "", nullptr, 0 },
	{ 396, "", "\xe9\x9b\x86\xe4\xb8\xad", "", nullptr, 0 },
	{ 397, "", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe6\xa7\x8d", "", nullptr, 0 },
	{ 400, "", "\xe8\x83\x8c\xe6\x99\xaf", "", nullptr, 0 },
	{ 401, "", "\xe8\x83\x8c\xe6\x99\xaf", "", nullptr, 0 },
	{ 403, "", "\xe7\x9b\xb8\xe6\x89\x8b\xe5\x90\xb9\xe3\x81\x8d\xe9\xa3\x9b\xe3\x81\xb0\xe3\x81\x97", "", nullptr, 0 },
	{ 404, "", "\xe7\xaa\x81\xe3\x81\x8d\xe5\x88\xba\xe3\x81\x97", "", kEntries009_13, 3 },
	{ 405, "", "\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab", "", nullptr, 0 },
};

const unsigned char kEntries010_0[] = { 240, 246, 254 };
const unsigned char kEntries010_1[] = { 240, 242, 246, 254 };
const unsigned char kEntries010_2[] = { 240 };
const unsigned char kEntries010_3[] = { 240, 246 };
const unsigned char kEntries010_4[] = { 240, 242, 254 };
const unsigned char kEntries010_5[] = { 240, 244, 247, 254 };
const unsigned char kEntries010_6[] = { 242, 254 };
const unsigned char kEntries010_7[] = { 240, 243, 244, 247, 254 };
const unsigned char kEntries010_8[] = { 240, 243, 247, 250, 254 };
const unsigned char kEntries010_9[] = { 240, 243, 247, 254 };
const unsigned char kEntries010_10[] = { 244, 252 };
const unsigned char kEntries010_11[] = { 240, 244, 252, 254 };
const unsigned char kEntries010_12[] = { 241 };
const unsigned char kEntries010_13[] = { 240, 242 };

const Row kEffects010[] = {
	{ 101, "", "A", "", kEntries010_0, 3 },
	{ 102, "", "B", "", kEntries010_0, 3 },
	{ 103, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "StdC_End", kEntries010_1, 4 },
	{ 104, "", "2A", "", kEntries010_0, 3 },
	{ 105, "", "\x32\x42\xe7\x85\x99", "", kEntries010_2, 1 },
	{ 106, "", "2C", "", kEntries010_0, 3 },
	{ 107, "", "JA", "", kEntries010_0, 3 },
	{ 108, "", "JB1", "", kEntries010_0, 3 },
	{ 109, "", "JB2", "", kEntries010_0, 3 },
	{ 110, "", "204", "AirC_End, AirSC", kEntries010_2, 1 },
	{ 111, "", "B", "StdB_End", kEntries010_0, 3 },
	{ 112, "", "\xe3\x81\x9f\xe3\x82\x81\x43", "", kEntries010_0, 3 },
	{ 113, "", "204", "AirDiveSC, DiveSC", kEntries010_2, 1 },
	{ 115, "", "\xe7\xab\x8b\xe3\x81\xa1\x43\xe7\x85\x99", "", nullptr, 0 },
	{ 116, "", "\x32\x42\xe3\x83\x93\xe3\x83\xaa\xe3\x83\x93\xe3\x83\xaa\xe6\xae\x8b\xe3\x82\x8a", "", kEntries010_3, 2 },
	{ 117, "", "\xe3\x81\x97\xe3\x82\x83\xe3\x81\x8c\xe3\x81\xbf\x43\xe7\x85\x99", "", nullptr, 0 },
	{ 120, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries010_2, 1 },
	{ 122, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "StdSC", kEntries010_4, 3 },
	{ 123, "", "104", "2C_2C, 2C_2C_End", kEntries010_5, 4 },
	{ 125, "", "\x30\x33\x30\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries010_0, 3 },
	{ 126, "", "\x33\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214BC", kEntries010_4, 3 },
	{ 163, "", "\xe6\xae\x8b\xe5\x83\x8f", "623EX, 6C", nullptr, 0 },
	{ 175, "", "\x4a\x32\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "J2C", kEntries010_2, 1 },
	{ 200, "", "BC", "236BC", kEntries010_6, 2 },
	{ 201, "", "EX1", "236EX", kEntries010_4, 3 },
	{ 202, "", "EX2", "236EX", kEntries010_6, 2 },
	{ 203, "", "\xe6\x9c\xac\xe4\xbd\x93\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "236BC, 236EX", nullptr, 0 },
	{ 205, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "236B", kEntries010_4, 3 },
	{ 206, "", "\xe6\x9c\xac\xe4\xbd\x93\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214BC, 236A, 236B, 236EX", nullptr, 0 },
	{ 213, "", "", "214A, 214B, 214BC, 214EX", nullptr, 0 },
	{ 214, "", "", "214B, 214BC, 214EX, StdSC", nullptr, 0 },
	{ 240, "", "\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x31\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Ball214A, Ball214B, Ball214BC", kEntries010_7, 5 },
	{ 241, "", "\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x31\xe6\xb6\x88\xe6\xbb\x85", "Ball214A, Ball214B, Ball214BC, Ball214BC2", kEntries010_8, 5 },
	{ 242, "", "\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x32\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Ball214B2, Ball214BC2, Ball214BC3", kEntries010_7, 5 },
	{ 243, "", "\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x32\xe6\xb6\x88\xe6\xbb\x85", "Ball214B2, Ball214BC3", kEntries010_9, 4 },
	{ 244, "", "\x45\x58\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x31\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Ball214EX, Ball214EX3", kEntries010_7, 5 },
	{ 245, "", "\x45\x58\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x31\xe6\xb6\x88\xe6\xbb\x85", "Ball214EX, Ball214EX3", kEntries010_8, 5 },
	{ 246, "", "\x45\x58\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x32\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "Ball214EX2", kEntries010_7, 5 },
	{ 247, "", "\x45\x58\xe3\x82\x8f\xe3\x81\xa3\xe3\x81\x8b\x32\xe6\xb6\x88\xe6\xbb\x85", "Ball214EX2", kEntries010_9, 4 },
	{ 260, "", "\xe6\xae\x8b\xe5\x83\x8f", "623A, 623A_End, 623B, 623BC, 623B_End, 623EX", nullptr, 0 },
	{ 265, "", "\xe7\xaa\x81\xe9\x80\xb2\xe5\x88\x87\xe3\x82\x8a\xe8\xa3\x82\xe3\x81\x8d", "623EX, 6C", kEntries010_2, 1 },
	{ 267, "", "\xe5\xa5\xa5\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "623EX_Hit", kEntries010_10, 2 },
	{ 268, "", "\xe6\x89\x8b\xe5\x89\x8d\xe3\x83\x90\xe3\x83\x81\xe3\x83\xb3", "623EX_Hit", kEntries010_11, 4 },
	{ 269, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\x8b", "623EX_Hit", kEntries010_12, 1 },
	{ 300, "", "", "J214A, J214B, J214BC", nullptr, 0 },
	{ 301, "", "", "J214BC, J214B_JAdd, J214EX", nullptr, 0 },
	{ 349, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "", kEntries010_4, 3 },
	{ 360, "", "\xe7\x88\x86\xe7\x99\xba", "LastArc_Hit", kEntries010_13, 2 },
};

const unsigned char kEntries011_0[] = { 2, 3 };
const unsigned char kEntries011_1[] = { 128, 130 };
const unsigned char kEntries011_2[] = { 241, 246, 247 };
const unsigned char kEntries011_3[] = { 160 };
const unsigned char kEntries011_4[] = { 243, 244, 245 };
const unsigned char kEntries011_5[] = { 2, 3, 80, 81 };
const unsigned char kEntries011_6[] = { 128 };
const unsigned char kEntries011_7[] = { 246 };
const unsigned char kEntries011_8[] = { 246, 248 };
const unsigned char kEntries011_9[] = { 242, 243, 244, 245 };
const unsigned char kEntries011_10[] = { 247, 248 };
const unsigned char kEntries011_11[] = { 244, 245 };
const unsigned char kEntries011_12[] = { 243, 244, 245, 246, 247, 248, 250 };
const unsigned char kEntries011_13[] = { 243, 244, 245, 246, 247, 249, 250 };
const unsigned char kEntries011_14[] = { 250 };
const unsigned char kEntries011_15[] = { 248 };
const unsigned char kEntries011_16[] = { 243 };
const unsigned char kEntries011_17[] = { 245 };

const Row kEffects011[] = {
	{ 101, "", "001", "", kEntries011_0, 2 },
	{ 102, "", "002", "", kEntries011_1, 2 },
	{ 103, "", "C", "StdC_End", kEntries011_2, 3 },
	{ 104, "", "\x43\xe6\xb6\x88\xe3\x81\x88", "StdC_End", kEntries011_3, 1 },
	{ 105, "", "2B", "", kEntries011_4, 3 },
	{ 106, "", "2C", "", kEntries011_4, 3 },
	{ 107, "", "201", "", kEntries011_5, 4 },
	{ 108, "", "JB", "", kEntries011_4, 3 },
	{ 109, "", "JC", "AirDiveSC, AirSC, DiveSC", kEntries011_4, 3 },
	{ 110, "", "101", "", kEntries011_6, 1 },
	{ 111, "", "\x43\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "C_C", kEntries011_7, 1 },
	{ 112, "", "\x43\x43\xe6\xb6\x88\xe3\x81\x88", "C_C", kEntries011_3, 1 },
	{ 113, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "StdSC", kEntries011_7, 1 },
	{ 114, "", "\x42\x42\x42\xe6\xb6\x88\xe3\x81\x88", "", kEntries011_3, 1 },
	{ 117, "", "", "RapidRelayAtk", kEntries011_7, 1 },
	{ 118, "", "", "RapidRelayAtk", kEntries011_8, 2 },
	{ 210, "", "\xe3\x82\xb5\xe3\x83\x9e\xe3\x83\xbc\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "0202A, 0202B, 0202BC, 0202EX, J0202A, J0202B, J0202BC, J0202EX", kEntries011_9, 4 },
	{ 236, "", "", "214A, 214A_End, 214B, 214EX", kEntries011_10, 2 },
	{ 237, "", "", "214B_AddA, 214B_AddB, 214EX", kEntries011_10, 2 },
	{ 238, "", "", "214B_Add_AddB, 214EX", kEntries011_10, 2 },
	{ 240, "", "\xe3\x82\xb7\xe3\x83\xb3\xe3\x82\xab\xe3\x83\xbc\xe7\xaa\x81\xe9\x80\xb2", "623A, 623B, 623BC, 623EX, 623EX_End", kEntries011_7, 1 },
	{ 241, "", "\xe3\x82\xb7\xe3\x83\xb3\xe3\x82\xab\xe3\x83\xbc\xe6\xb6\x88\xe3\x81\x88", "623A, 623A_Hit, 623B, 623BC, 623BC_Hit, 623B_Hit, 623EX, 623EX_End, 623EX_Hit, 623EX_Hit2", kEntries011_3, 1 },
	{ 256, "", "\xe3\x82\xab\xe3\x82\xab\xe3\x83\x88\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "6C", kEntries011_11, 2 },
	{ 285, "", "", "214BC, 214B_Add_AddA, 236A_JAdd4, 236B_JAdd4, 236_JAddWall_Add, J214A, J214B, J214BC, J214B_JAdd, J214B_JAdd_JAdd, J214EX", kEntries011_10, 2 },
	{ 319, "", "\xef\xbc\xa0\xe7\x85\x99\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 322, "", "\xef\xbc\xa0\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe5\xbe\x8c\xe3\x80\x80\xe5\xb2\xa9\xef\xbc\x86\xe7\x85\x99\xe5\x91\xbc\xe3\x81\xb3", "41236SP_Hit", nullptr, 0 },
	{ 324, "", "\xe9\xa3\x9b\xe3\x81\xb3\xe6\x95\xa3\xe3\x82\x8b\xe9\xa0\x81", "", nullptr, 0 },
	{ 325, "", "\xe6\x8c\x81\xe3\x81\xa1\xe4\xb8\x8a\xe3\x81\x92\xe7\x85\x99", "", nullptr, 0 },
	{ 326, "", "\xe7\x99\xba\xe5\xb0\x84\xe7\x88\x86\xe7\x99\xba", "", kEntries011_12, 7 },
	{ 327, "", "\xe7\x99\xba\xe5\xb0\x84\xe8\xb6\xb3\xe5\x85\x83\xe5\xb2\xa9", "", nullptr, 0 },
	{ 330, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95\x42\x47", "41236SP_Hit", nullptr, 0 },
	{ 331, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95\x42\x47", "", nullptr, 0 },
	{ 360, "", "\xe7\x85\x99", "", nullptr, 0 },
	{ 361, "", "\xe5\xb2\xa9\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 364, "", "\xe8\xb5\xa4\xe3\x81\x84\x42\x47", "", nullptr, 0 },
	{ 365, "", "\xe7\xa0\xb4\xe5\xa3\x8a\xe5\xb2\xa9", "", nullptr, 0 },
	{ 367, "", "\xe7\x94\xbb\xe9\x9d\xa2\xe3\x81\xab\xe6\xb5\x81\xe3\x82\x8c\xe3\x82\x8b\xe5\x9c\x9f\xe7\x85\x99", "", nullptr, 0 },
	{ 368, "", "\xe8\x83\x8c\xe6\x99\xaf\xe6\x9a\x97\xe3\x81\x8f", "", nullptr, 0 },
	{ 370, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries011_13, 7 },
	{ 371, "", "\xe8\xa1\x9d\xe6\x92\x83\xe5\xb2\xa9", "", kEntries011_14, 1 },
	{ 372, "", "\xe7\x99\xba\xe5\xb0\x84\xe8\xa1\x9d\xe6\x92\x83\xe7\x85\x99", "", nullptr, 0 },
	{ 375, "", "\xe9\x9b\x86\xe4\xb8\xad\x42\x47", "", kEntries011_15, 1 },
	{ 376, "", "\xe7\x9f\xa2\xe9\xa3\x9b\xe3\x82\x93\xe3\x81\xa7\xe3\x81\x8f\xe3\x82\x8b", "", kEntries011_16, 1 },
	{ 381, "", "\xe7\xa7\xbb\xe5\x8b\x95\xe7\xb7\x9a", "", nullptr, 0 },
	{ 383, "", "\xe6\x9c\x80\xe5\xbe\x8c\xe3\x83\x95\xe3\x83\xa9\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5", "", kEntries011_17, 1 },
	{ 385, "", "\xe4\xb8\x80\xe7\x9e\xac\xe8\x83\x8c\xe6\x99\xaf\xe6\x9a\x97\xe3\x81\x8f", "", nullptr, 0 },
	{ 955, "", "\xe9\xbb\x92\xe9\x8d\xb5\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "", kEntries011_7, 1 },
};

const unsigned char kEntries012_0[] = { 246, 247, 248 };
const unsigned char kEntries012_1[] = { 241, 244, 245, 247, 248, 254 };
const unsigned char kEntries012_2[] = { 253 };
const unsigned char kEntries012_3[] = { 245, 250, 253 };
const unsigned char kEntries012_4[] = { 245, 250, 252, 253 };
const unsigned char kEntries012_5[] = { 243, 244, 249 };
const unsigned char kEntries012_6[] = { 252, 253 };
const unsigned char kEntries012_7[] = { 241, 253 };
const unsigned char kEntries012_8[] = { 245, 251, 252, 253 };

const Row kEffects012[] = {
	{ 127, "", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\xa0\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe7\x94\xa8\xe5\x89\xa3", "", nullptr, 0 },
	{ 128, "", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\xa0\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe7\x94\xa8\xe5\x89\xa3", "", nullptr, 0 },
	{ 252, "", "", "StrikeAirEX", kEntries012_0, 3 },
	{ 254, "", "\xe3\x82\xb9\xe3\x82\xa4\xe3\x83\xb3\xe3\x82\xb0", "0202EX", kEntries012_1, 6 },
	{ 330, "", "\xe7\x99\xba\xe5\x8b\x95\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\xa9", "41236SP", kEntries012_2, 1 },
	{ 331, "", "\xe5\xa5\xa5\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab", "41236SP", kEntries012_2, 1 },
	{ 332, "", "\x42\x47\xe8\xb6\xb3\xe5\x85\x83\xe5\x85\x89", "41236SP", kEntries012_2, 1 },
	{ 335, "", "\xe6\xb1\x8e\xe7\x94\xa8\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88\xe5\x91\xbc\xe3\x81\xb3", "41236SP", nullptr, 0 },
	{ 339, "", "\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab\xe7\x94\x9f\xe6\x88\x90", "41236SP", nullptr, 0 },
	{ 360, "", "\xe3\x82\xa2\xe3\x83\xb4\xe3\x82\xa1\xe3\x83\xad\xe3\x83\xb3\xe7\xb2\x92\xe5\xad\x90", "63214SP_Hit", kEntries012_2, 1 },
	{ 361, "", "\xe3\x82\xa8\xe3\x82\xaf\xe3\x82\xb9\xe3\x82\xab\xe3\x83\xaa\xe3\x83\x90\xe3\x83\xbc\xe5\xbe\x85\xe6\xa9\x9f", "63214SP_Hit", kEntries012_3, 3 },
	{ 362, "", "\xe3\x82\xa8\xe3\x82\xaf\xe3\x82\xb9\xe3\x82\xab\xe3\x83\xaa\xe3\x83\x90\xe3\x83\xbc\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries012_4, 4 },
	{ 410, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95", "LastArc_Hit", kEntries012_5, 3 },
	{ 411, "", "\xe7\xaa\x81\xe9\x80\xb2\xe3\x83\x90\xe3\x83\xaa\xe3\x82\xa2", "LastArc_Hit", kEntries012_6, 2 },
	{ 412, "", "", "LastArc_Hit", kEntries012_7, 2 },
	{ 413, "", "", "LastArc_Hit", kEntries012_2, 1 },
	{ 415, "", "\xe6\x89\x93\xe3\x81\xa1\xe4\xb8\x8a\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LastArc_Hit", kEntries012_2, 1 },
	{ 416, "", "\xe6\x89\x93\xe3\x81\xa1\xe4\xb8\x8a\xe3\x81\x92\xe7\x85\x99", "LastArc_Hit", kEntries012_2, 1 },
	{ 418, "", "\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "LastArc_Hit", nullptr, 0 },
	{ 419, "", "\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\xe3\x82\xa4\xe3\x83\xb3", "", nullptr, 0 },
	{ 421, "", "\xe3\x82\xa8\xe3\x82\xaf\xe3\x82\xb9\xe3\x82\xab\xe3\x83\xaa\xe3\x83\x90\xe3\x83\xbc", "", kEntries012_8, 4 },
	{ 423, "", "\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 424, "", "\xe7\xb2\x92\xe5\xad\x90", "", kEntries012_2, 1 },
	{ 425, "", "\xe5\xa4\xa9\xe3\x81\xb8\xe3\x82\xa8\xe3\x82\xaf\xe3\x82\xb9\xe3\x82\xab\xe3\x83\xaa\xe3\x83\x90\xe3\x83\xbc", "", kEntries012_8, 4 },
	{ 426, "", "\xe9\x9b\xb2\xe3\x81\xae\xe7\xa9\xb4", "", nullptr, 0 },
	{ 427, "", "\xe5\x85\x89\xe3\x81\xae\xe6\x9f\xb1\xe6\x89\x8b\xe5\x89\x8d\xe3\x81\xae\xe8\xbe\xbb", "", nullptr, 0 },
	{ 430, "", "\xe4\xb8\x8b\xe3\x81\x8b\xe3\x82\x89\xe3\x81\xae\xe9\xa2\xa8", "", kEntries012_6, 2 },
	{ 431, "", "\xe4\xb8\x8b\xe3\x81\x8b\xe3\x82\x89\xe3\x81\xae\xe7\xb2\x92\xe5\xad\x90", "", kEntries012_2, 1 },
	{ 432, "", "BG", "", nullptr, 0 },
	{ 441, "chr016_LA_Blade1", "", "", nullptr, 0 },
	{ 442, "", "", "chr016_LA_Blade1", nullptr, 0 },
};

const unsigned char kEntries013_0[] = { 242, 243, 250 };
const unsigned char kEntries013_1[] = { 2, 3, 48, 50, 64, 66, 160, 162 };
const unsigned char kEntries013_2[] = { 2, 3, 48, 50, 64, 66 };
const unsigned char kEntries013_3[] = { 250 };
const unsigned char kEntries013_4[] = { 49, 50, 64, 242, 243, 250 };
const unsigned char kEntries013_5[] = { 16, 18, 48, 50, 64, 66, 160, 162, 176, 178 };
const unsigned char kEntries013_6[] = { 242, 243, 250, 251 };
const unsigned char kEntries013_7[] = { 251, 252 };
const unsigned char kEntries013_8[] = { 242, 243, 250, 251, 252 };
const unsigned char kEntries013_9[] = { 243, 251, 252 };
const unsigned char kEntries013_10[] = { 243 };
const unsigned char kEntries013_11[] = { 252 };
const unsigned char kEntries013_12[] = { 16, 18, 48, 50, 64, 66, 160, 162, 176, 178, 208, 210 };
const unsigned char kEntries013_13[] = { 244, 250, 251, 252 };
const unsigned char kEntries013_14[] = { 244, 246 };

const Row kEffects013[] = {
	{ 101, "", "", "", kEntries013_0, 3 },
	{ 102, "", "", "", kEntries013_0, 3 },
	{ 103, "", "", "236A_AddA, 236BC", kEntries013_1, 8 },
	{ 104, "", "", "", kEntries013_0, 3 },
	{ 105, "", "", "", kEntries013_0, 3 },
	{ 106, "", "", "", kEntries013_0, 3 },
	{ 107, "", "", "", kEntries013_0, 3 },
	{ 108, "", "", "", kEntries013_0, 3 },
	{ 109, "", "", "AirDiveSC, AirSC, DiveSC", kEntries013_0, 3 },
	{ 111, "", "", "", kEntries013_2, 6 },
	{ 116, "", "\xe3\x81\xb5\xe3\x82\x93\xe3\x81\xb0\xe3\x82\x8a\xe7\x85\x99", "", kEntries013_3, 1 },
	{ 119, "", "", "StdSC", kEntries013_4, 6 },
	{ 120, "", "", "RapidRelayAtk", kEntries013_0, 3 },
	{ 121, "", "", "RapidRelayAtk", kEntries013_0, 3 },
	{ 123, "", "", "2C_2C, 2C_2C_End", kEntries013_0, 3 },
	{ 171, "", "", "6B", kEntries013_0, 3 },
	{ 205, "", "", "236A, 236B, 236BC", kEntries013_5, 10 },
	{ 206, "", "", "236EX", kEntries013_5, 10 },
	{ 207, "", "", "236A, 236B, 236BC", kEntries013_6, 4 },
	{ 208, "", "", "236EX", kEntries013_7, 2 },
	{ 215, "", "", "236A_AddC", kEntries013_0, 3 },
	{ 217, "", "\xe3\x81\xb5\xe3\x82\x93\xe3\x81\xb0\xe3\x82\x8a\xe7\x85\x99", "236A, 236B, 236BC, 236EX", kEntries013_3, 1 },
	{ 235, "", "", "214A, 214A_End, 214B, 214BC, 214B_End", kEntries013_8, 5 },
	{ 237, "", "", "214EX", kEntries013_9, 3 },
	{ 240, "", "\xe3\x81\xb5\xe3\x82\x93\xe3\x81\xb0\xe3\x82\x8a\xe7\x85\x99", "214A, 214A_End, 214B, 214BC, 214B_End, 214EX", kEntries013_3, 1 },
	{ 258, "", "", "623A, 623B, 623BC, J236B", kEntries013_0, 3 },
	{ 259, "", "", "623A, 623B, 623BC, J236B", kEntries013_0, 3 },
	{ 260, "", "", "623EX", kEntries013_7, 2 },
	{ 261, "", "", "623EX", kEntries013_7, 2 },
	{ 276, "", "", "236A, J236A, J236B, J236BC", kEntries013_10, 1 },
	{ 277, "", "\xe7\x85\x99", "236A, J236A, J236B, J236BC", kEntries013_10, 1 },
	{ 278, "", "", "J236EX", kEntries013_11, 1 },
	{ 279, "", "\xe7\x85\x99", "J236EX", kEntries013_7, 2 },
	{ 290, "", "", "214B, J214A, J214B, J214BC", kEntries013_0, 3 },
	{ 291, "", "", "J214EX", kEntries013_7, 2 },
	{ 307, "", "", "0202A, 0202B, 0202BC, 0202EX, 0202EX_End", kEntries013_12, 12 },
	{ 311, "Rc0202EX", "", "", kEntries013_13, 4 },
	{ 356, "", "\xe5\xb2\xa9", "AdPillar", nullptr, 0 },
	{ 357, "", "\xe8\x88\x9e\xe3\x81\x84\xe4\xb8\x8a\xe3\x81\x8c\xe3\x82\x8b\xe3\x82\xac\xe3\x83\xac\xe3\x82\xad", "AdPillar", nullptr, 0 },
	{ 358, "", "\xe4\xbd\x99\xe9\x9f\xbb\xe7\x85\x99", "AdPillar", nullptr, 0 },
	{ 420, "", "", "LA_Panda", kEntries013_0, 3 },
	{ 421, "", "", "LA_Panda", kEntries013_0, 3 },
	{ 422, "", "", "LA_Panda", kEntries013_0, 3 },
	{ 423, "", "", "LA_Panda", kEntries013_0, 3 },
	{ 424, "", "", "LA_Panda", kEntries013_0, 3 },
	{ 426, "", "\xe6\x9a\x97\xe8\xbb\xa2", "", kEntries013_14, 2 },
	{ 435, "", "\xe8\xa1\x9d\xe6\x92\x83\xe6\xb3\xa2\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 436, "", "\xe7\x85\x99\xe7\x94\x9f\xe6\x88\x90", "", nullptr, 0 },
	{ 935, "", "", "", nullptr, 0 },
	{ 936, "", "", "", nullptr, 0 },
	{ 937, "", "", "", kEntries013_2, 6 },
	{ 951, "", "\xe6\x8d\xa8\xe3\x81\xa6\xe3\x82\x8b\xe6\x9c\x8d", "", nullptr, 0 },
};

const unsigned char kEntries014_0[] = { 247, 250 };
const unsigned char kEntries014_1[] = { 247, 248, 250 };
const unsigned char kEntries014_2[] = { 80 };
const unsigned char kEntries014_3[] = { 48, 80, 250 };
const unsigned char kEntries014_4[] = { 192, 247, 250, 251 };
const unsigned char kEntries014_5[] = { 18, 26, 50, 82, 130, 162 };
const unsigned char kEntries014_6[] = { 247 };
const unsigned char kEntries014_7[] = { 243, 244, 247, 250 };
const unsigned char kEntries014_8[] = { 192, 241, 247, 248, 249, 255 };
const unsigned char kEntries014_9[] = { 241, 247, 248, 253 };
const unsigned char kEntries014_10[] = { 241 };
const unsigned char kEntries014_11[] = { 241, 247 };
const unsigned char kEntries014_12[] = { 192, 200, 208 };
const unsigned char kEntries014_13[] = { 249 };
const unsigned char kEntries014_14[] = { 194, 200, 210, 251 };
const unsigned char kEntries014_15[] = { 194, 200, 210 };

const Row kEffects014[] = {
	{ 101, "", "", "", kEntries014_0, 2 },
	{ 102, "", "", "", kEntries014_0, 2 },
	{ 103, "", "C", "", kEntries014_0, 2 },
	{ 104, "", "", "", kEntries014_0, 2 },
	{ 105, "", "2B", "", kEntries014_1, 3 },
	{ 106, "", "2C", "", kEntries014_0, 2 },
	{ 107, "", "JA", "", kEntries014_2, 1 },
	{ 108, "", "JB", "AirSC", kEntries014_0, 2 },
	{ 109, "", "JC", "AirC_End, AirDiveSC, DiveSC", kEntries014_0, 2 },
	{ 110, "", "\xe3\x81\x9f\xe3\x82\x81\x4a\x43", "", kEntries014_0, 2 },
	{ 111, "", "\xe6\x8a\x95\xe3\x81\x92", "", kEntries014_0, 2 },
	{ 112, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\xb9\xe3\x82\xab", "", kEntries014_2, 1 },
	{ 113, "", "\xe7\xa9\xba\xe6\x8a\x95\xe3\x81\x92\xe3\x82\xb9\xe3\x82\xab", "", kEntries014_2, 1 },
	{ 114, "", "3C", "StdSC", kEntries014_3, 3 },
	{ 115, "", "AA", "RapidRelayAtk", kEntries014_3, 3 },
	{ 116, "", "\xe9\x80\x9a\xe5\xb8\xb8\x43", "StdC_End", kEntries014_0, 2 },
	{ 183, "", "", "Ball_6C, Ball_i6C", kEntries014_4, 4 },
	{ 185, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95\xe3\x83\x96\xe3\x83\xa9\xe3\x83\xbc", "236_Add, 236_AddDash, J236_JAdd, J236_JAddDash", kEntries014_5, 6 },
	{ 186, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95\xe4\xb8\x80\xe9\x96\x83", "236_Add, J236_JAdd", kEntries014_6, 1 },
	{ 187, "", "\xe6\xae\x8b\xe5\x83\x8f", "236_Add, 236_AddDash, J236_JAdd, J236_JAddDash", nullptr, 0 },
	{ 260, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "623A, 623B, 623BC, 623EX", kEntries014_7, 4 },
	{ 261, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5", "623A, 623B, 623BC, 623EX", kEntries014_7, 4 },
	{ 355, "", "\xe6\x9c\x80\xe5\x88\x9d\xe3\x81\xae\xe5\x9c\xb0\xe9\x9d\xa2\xe3\x81\xa4\xe3\x81\x8d\xe3\x81\x95\xe3\x81\x97", "41236SP, 41236SP_Damage", kEntries014_6, 1 },
	{ 356, "", "\xe6\x9c\x80\xe5\x88\x9d\xe3\x81\xa4\xe3\x81\x8d\xe3\x81\x95\xe3\x81\x97\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "41236SP, 41236SP_Damage", kEntries014_0, 2 },
	{ 370, "", "\xe5\x9c\xb0\xe9\x9d\xa2\xe3\x82\xba\xe3\x83\x89\xe3\x83\x89\xe3\x83\x89", "AD_FirstLance, AD_Lance1, AD_Lance2, AD_Lance3, AD_MissLance, AD_MissLanceFin", nullptr, 0 },
	{ 372, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe7\x88\x86\xe7\x99\xba", "41236SP_Hit", kEntries014_8, 6 },
	{ 410, "", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x97\xe9\xad\x94\xe7\x9c\xbc", "LastArc_Hit", nullptr, 0 },
	{ 411, "", "\xe3\x80\x80\xe9\xad\x94\xe7\x9c\xbc\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "", kEntries014_9, 4 },
	{ 412, "", "\xe3\x80\x80\xe9\xad\x94\xe7\x9c\xbc\xe8\x8a\xb1\xe3\x81\xb3\xe3\x82\x89", "", kEntries014_10, 1 },
	{ 415, "", "\xe3\x83\x90\xe3\x83\xa9\xe8\x8a\xb1\xe3\x81\xb3\xe3\x82\x89", "LastArc_Hit", kEntries014_10, 1 },
	{ 416, "", "\x34\x30\x46\xe3\x83\x95\xe3\x82\xa7\xe3\x83\xbc\xe3\x83\x89\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "LastArc_Hit", kEntries014_10, 1 },
	{ 418, "", "\xe3\x83\x95\xe3\x82\xa7\xe3\x83\xbc\xe3\x83\x89\xe3\x82\xa4\xe3\x83\xb3", "LA_Box", kEntries014_10, 1 },
	{ 419, "", "\xe3\x81\x86\xe3\x81\x9a", "LA_Box", kEntries014_11, 2 },
	{ 425, "", "\xe5\xb7\xa8\xe5\xa4\xa7\xe3\x83\x8e\xe3\x82\xa8\xe3\x83\xab\xe7\xaa\x81\xe3\x81\x8d\xe5\x88\xba\xe3\x81\x97", "LA_Box", kEntries014_12, 3 },
	{ 426, "", "\xe7\x88\x86\xe7\x99\xba\xe3\x80\x9c\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "LA_Box", nullptr, 0 },
	{ 428, "", "\xe3\x82\xb9\xe3\x83\x9d\xe3\x83\x83\xe3\x83\x88\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\x88", "", nullptr, 0 },
	{ 432, "", "\xe3\x80\x80\xe8\x83\x8c\xe6\x99\xaf", "LA_BG_Color", kEntries014_13, 1 },
	{ 433, "", "\xe3\x80\x80\xe3\x83\x95\xe3\x82\xa7\xe3\x83\xbc\xe3\x83\x89\xe3\x82\xa4\xe3\x83\xb3", "", nullptr, 0 },
	{ 935, "", "\xe6\xbd\xb0\xe3\x81\x99\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "", kEntries014_14, 4 },
	{ 937, "", "\xe5\xb1\x8d", "", kEntries014_15, 3 },
};

const unsigned char kEntries015_0[] = { 112, 114, 144, 146 };
const unsigned char kEntries015_1[] = { 243, 248 };
const unsigned char kEntries015_2[] = { 112, 114 };
const unsigned char kEntries015_3[] = { 243, 244, 248, 250 };
const unsigned char kEntries015_4[] = { 243, 246, 248, 250 };
const unsigned char kEntries015_5[] = { 112, 114, 243, 248 };
const unsigned char kEntries015_6[] = { 246, 248, 255 };
const unsigned char kEntries015_7[] = { 241, 246, 248, 250 };
const unsigned char kEntries015_8[] = { 241, 243, 246, 248, 250 };
const unsigned char kEntries015_9[] = { 241, 243, 248, 250 };
const unsigned char kEntries015_10[] = { 243, 246, 250 };
const unsigned char kEntries015_11[] = { 16 };
const unsigned char kEntries015_12[] = { 248 };
const unsigned char kEntries015_13[] = { 243 };
const unsigned char kEntries015_14[] = { 243, 246 };

const Row kEffects015[] = {
	{ 1, "", "\xe7\xab\x8b\xe3\x81\xa1\xe5\xbc\xb1\xe6\x94\xbb\xe6\x92\x83", "Rc0202B, Rc0202BC, Rc0202EX", nullptr, 0 },
	{ 101, "", "", "", kEntries015_0, 4 },
	{ 102, "", "B", "", kEntries015_1, 2 },
	{ 103, "", "C", "StdC_End", kEntries015_1, 2 },
	{ 104, "", "2A", "", kEntries015_0, 4 },
	{ 105, "", "2B", "", kEntries015_1, 2 },
	{ 106, "", "2C", "", kEntries015_1, 2 },
	{ 107, "", "JA", "", kEntries015_2, 2 },
	{ 108, "", "JB", "", kEntries015_1, 2 },
	{ 109, "", "JC", "AirC_End", kEntries015_1, 2 },
	{ 116, "", "AA", "RapidRelayAtk", kEntries015_1, 2 },
	{ 120, "", "4C", "4C", kEntries015_1, 2 },
	{ 121, "", "6C", "6C, 6C_End", kEntries015_1, 2 },
	{ 122, "", "6C", "6C_6C", kEntries015_1, 2 },
	{ 123, "", "6C", "6C_6C_6C", kEntries015_1, 2 },
	{ 125, "", "SCBC", "ExSC", kEntries015_1, 2 },
	{ 126, "", "\x4c\x41\xe6\x94\xbb\xe6\x92\x83", "", kEntries015_1, 2 },
	{ 132, "", "3C", "StdSC", kEntries015_1, 2 },
	{ 167, "", "\x32\x43\x43\xe7\x88\x86\xe7\x99\xba", "2C_2C", kEntries015_3, 4 },
	{ 233, "", "\xe6\xb5\xae\xe9\x81\x8a\xe5\xbc\xbe\xe6\xb6\x88\xe6\xbb\x85\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "FloatMine, FloatMineBC1, FloatMineBC2, FloatMineBomb, WarpMineA, WarpMineB, WarpMineBC1, WarpMineBC2, WarpMineBomb, WarpMineC, WarpMineVanish", kEntries015_4, 4 },
	{ 234, "", "\xe6\xb5\xae\xe9\x81\x8a\xe5\xbc\xbe\xe7\x88\x86\xe7\x99\xba\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "FloatMineBomb, WarpMineBomb", kEntries015_4, 4 },
	{ 237, "", "\xe6\xb5\xae\xe9\x81\x8a\xe5\xbc\xbe\xe6\xb6\x88\xe6\xbb\x85\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "FloatMineBombEX, FloatMineEX", kEntries015_4, 4 },
	{ 238, "", "\xe6\xb5\xae\xe9\x81\x8a\xe5\xbc\xbe\xe7\x88\x86\xe7\x99\xba\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "FloatMineBombEX", kEntries015_4, 4 },
	{ 260, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe9\xa3\x9b\xe3\x81\xb0\xe3\x81\x99\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "623B, 6C_6C_6C_4C", kEntries015_1, 2 },
	{ 261, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe9\xa3\x9b\xe3\x81\xb0\xe3\x81\x99\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "623A, 6C_6C_6C_6C", kEntries015_1, 2 },
	{ 262, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe9\xa3\x9b\xe3\x81\xb0\xe3\x81\x99\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\x45\x58", "623BC, 623EX", kEntries015_1, 2 },
	{ 264, "", "\xe6\xb4\xbe\xe7\x94\x9f\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "623B_JAdd, 623EX_JHit", kEntries015_5, 4 },
	{ 279, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92\xe5\x8b\xa2\xe3\x81\x84", "Rc0202A", kEntries015_1, 2 },
	{ 280, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "0202A, 0202B, 0202BC, 0202EX", kEntries015_5, 4 },
	{ 363, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3", "41236SP_Hit", kEntries015_6, 3 },
	{ 365, "", "\xe8\xa1\x9d\xe6\x92\x83\xe6\xb3\xa2", "", nullptr, 0 },
	{ 366, "", "\xe6\x96\x9c\xe3\x82\x81\xe8\xa1\x9d\xe6\x92\x83\xe6\xb3\xa2", "", nullptr, 0 },
	{ 367, "", "\xe5\x88\x9d\xe6\xae\xb5\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "AD_Atk1", kEntries015_7, 4 },
	{ 368, "", "\xe4\xba\x8c\xe6\xae\xb5\xe7\x9b\xae\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "AD_Atk2", kEntries015_8, 5 },
	{ 369, "", "\xef\xbc\x93\xe6\xae\xb5\xe7\x9b\xae\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "AD_Atk3", kEntries015_9, 4 },
	{ 371, "", "\xe5\xbd\xb1", "41236SP", nullptr, 0 },
	{ 372, "", "\xe5\xbd\xb1", "", nullptr, 0 },
	{ 373, "", "\xe5\xbd\xb1", "41236SP_Hit", nullptr, 0 },
	{ 375, "", "\xe6\x9c\x80\xe5\xbe\x8c\xe7\x88\x86\xe7\x99\xba", "AD_Atk3", kEntries015_10, 3 },
	{ 376, "", "\xe3\x80\x80\xe9\x80\xa3\xe7\xb6\x9a\xe7\x88\x86\xe7\x99\xba\x53\x45\xe5\x91\xbc\xe3\x81\xb3", "", nullptr, 0 },
	{ 380, "", "\xe6\xb1\x8e\xe7\x94\xa8\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88\xe5\x91\xbc\xe3\x81\xb3", "", nullptr, 0 },
	{ 428, "", "\xe5\xb7\xa8\xe5\xa4\xa7\xe9\x9d\x92\xe5\xad\x90", "", kEntries015_11, 1 },
	{ 430, "", "\xe6\x89\x8b\xe5\x89\x8d\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3", "", kEntries015_12, 1 },
	{ 431, "", "\xe5\xa5\xa5\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3", "", kEntries015_13, 1 },
	{ 433, "", "\xe5\xbe\x8c\xe3\x82\x8d\xe8\x83\x8c\xe6\x99\xaf", "", kEntries015_14, 2 },
	{ 434, "", "\xe6\x89\x8b\xe5\x89\x8d\xe8\x8a\xb1\xe3\x81\xb3\xe3\x82\x89", "", nullptr, 0 },
	{ 435, "", "\xe5\xa5\xa5\xe8\x8a\xb1\xe3\x81\xb3\xe3\x82\x89", "", nullptr, 0 },
	{ 922, "", "\xe7\xbd\xae\xe3\x81\x8f\xe3\x81\x8b\xe3\x81\xb0\xe3\x82\x93", "", nullptr, 0 },
};

const unsigned char kEntries016_0[] = { 246, 247, 248 };
const unsigned char kEntries016_1[] = { 228, 234 };
const unsigned char kEntries016_2[] = { 56, 58, 96, 163 };
const unsigned char kEntries016_3[] = { 244 };
const unsigned char kEntries016_4[] = { 250, 252, 253 };
const unsigned char kEntries016_5[] = { 64, 248 };
const unsigned char kEntries016_6[] = { 243, 246, 247, 248, 251 };
const unsigned char kEntries016_7[] = { 241 };
const unsigned char kEntries016_8[] = { 233 };
const unsigned char kEntries016_9[] = { 241, 243, 250, 251, 254 };
const unsigned char kEntries016_10[] = { 244, 248 };
const unsigned char kEntries016_11[] = { 246, 247 };
const unsigned char kEntries016_12[] = { 228, 230, 246, 247 };
const unsigned char kEntries016_13[] = { 241, 243, 246, 247, 248, 252, 253, 254 };
const unsigned char kEntries016_14[] = { 241, 243, 246, 247, 248, 254 };
const unsigned char kEntries016_15[] = { 241, 243 };
const unsigned char kEntries016_16[] = { 241, 244 };
const unsigned char kEntries016_17[] = { 243, 247 };
const unsigned char kEntries016_18[] = { 247 };
const unsigned char kEntries016_19[] = { 246 };

const Row kEffects016[] = {
	{ 101, "", "001", "", kEntries016_0, 3 },
	{ 102, "", "002", "StdB_End", kEntries016_1, 2 },
	{ 103, "", "C", "", kEntries016_0, 3 },
	{ 104, "", "101", "", kEntries016_2, 4 },
	{ 105, "", "2B", "", kEntries016_0, 3 },
	{ 106, "", "2C", "CroC_End", kEntries016_0, 3 },
	{ 107, "", "201", "", kEntries016_0, 3 },
	{ 108, "", "202", "AirDiveSC, AirSC, DiveSC, J6B", kEntries016_0, 3 },
	{ 109, "", "JC", "AirC_End", kEntries016_0, 3 },
	{ 110, "", "JC2", "", kEntries016_0, 3 },
	{ 111, "", "\x4a\x43\xe6\xb6\x88\xe6\xbb\x85", "", kEntries016_3, 1 },
	{ 113, "", "", "RapidRelayAtk", kEntries016_3, 1 },
	{ 115, "", "\xe9\xad\x94\xe8\xa1\x93\xe5\x91\xbc\xe3\x81\xb3", "214A, 214A_End, 214B, 214BC, 214B_End, 214EX, J214A, J214A_End, J214B, J214BC, J214B_End, J214EX", kEntries016_4, 3 },
	{ 116, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries016_0, 3 },
	{ 117, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92\xe7\xa9\xba\xe6\x8c\xaf\xe3\x82\x8a", "", kEntries016_5, 2 },
	{ 119, "", "\xe7\xab\x8b\xe3\x81\xa1\xe3\x82\xb7\xe3\x83\xab\xe3\x82\xab\xe3\x83\xb3", "StdSC", kEntries016_6, 5 },
	{ 120, "", "\xe7\xab\x8b\xe3\x81\xa1\x53\x43\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab\xe6\xb6\x88\xe6\xbb\x85", "StdSC", kEntries016_3, 1 },
	{ 126, "", "\x43\xe6\xb6\x88\xe6\xbb\x85", "C_C", kEntries016_3, 1 },
	{ 127, "", "\x32\x43\xe6\xb6\x88\xe5\xa4\xb1", "CroC_End", kEntries016_3, 1 },
	{ 128, "", "\x32\x43\xe7\xaa\x81\xe3\x81\x8d\xe5\x88\xba\xe3\x81\x97", "2C_2C", nullptr, 0 },
	{ 129, "", "\x32\x43\xe7\xaa\x81\xe3\x81\x8d\xe5\x88\xba\xe3\x81\x97\xe6\xb6\x88\xe6\xbb\x85", "2C_2C", kEntries016_3, 1 },
	{ 132, "", "\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "", kEntries016_0, 3 },
	{ 133, "", "\xe7\x99\xba\xe5\xb0\x84", "", kEntries016_6, 5 },
	{ 134, "", "\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab\xe6\xb6\x88\xe6\xbb\x85", "", kEntries016_3, 1 },
	{ 136, "", "\xe6\xb6\x88\xe6\xbb\x85\xe3\x80\x80\x4d\x44", "", kEntries016_3, 1 },
	{ 171, "", "C2", "C_C", kEntries016_0, 3 },
	{ 172, "", "", "C_C", kEntries016_7, 1 },
	{ 179, "", "\x32\x43\x43\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe6\xb6\x88\xe6\xbb\x85", "236C, 236C_Finish, 236EX_Finish, 623B", kEntries016_3, 1 },
	{ 180, "", "\xe3\x82\xbf\xe3\x83\x83\xe3\x82\xaf\xe3\x83\xab\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab\xe5\x87\xba\xe7\x8f\xbe", "236C, 236C_Add_AddEX", kEntries016_3, 1 },
	{ 181, "", "\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab\xe6\xb6\x88\xe6\xbb\x85", "236C_Add_AddEX", kEntries016_3, 1 },
	{ 184, "", "102_1", "3B", kEntries016_0, 3 },
	{ 185, "", "102_2", "3B", kEntries016_0, 3 },
	{ 191, "", "\xe6\xb6\x88\xe6\xbb\x85", "236A_End", kEntries016_3, 1 },
	{ 192, "", "\xe6\xb6\x88\xe6\xbb\x85", "236A, 236A_AddA, 236BC", kEntries016_3, 1 },
	{ 193, "", "\xe8\x96\xac\xe8\x8e\xa2", "236A, 236A_End", nullptr, 0 },
	{ 199, "", "\xe4\xb9\xb1\xe5\xb0\x84\x31", "236A, 236A_AddA, 236BC", kEntries016_7, 1 },
	{ 200, "", "\xe4\xb9\xb1\xe5\xb0\x84\x32", "236A, 236A_AddA, 236BC", kEntries016_7, 1 },
	{ 201, "", "\xe4\xb9\xb1\xe5\xb0\x84\x33", "236A, 236A_AddA, 236BC", kEntries016_7, 1 },
	{ 202, "", "\xe3\x81\x90\xe3\x82\x8b", "236A_Add4, 236A_Add_Add", kEntries016_8, 1 },
	{ 203, "", "", "236A_Add4, 236A_Add_Add", kEntries016_9, 5 },
	{ 204, "", "\xe6\xb6\x88\xe6\xbb\x85", "236A_Add4, 236A_Add_Add", kEntries016_3, 1 },
	{ 211, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "236B, 236B_End", kEntries016_0, 3 },
	{ 212, "", "\xe6\xb6\x88\xe6\xbb\x85", "236B_End", kEntries016_10, 2 },
	{ 216, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "236B, 236B_AddB", kEntries016_0, 3 },
	{ 217, "", "\xe6\xb6\x88\xe6\xbb\x85", "236B_AddB", kEntries016_3, 1 },
	{ 221, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "236B, 236B_Add_Add", kEntries016_0, 3 },
	{ 222, "", "\xe6\xb6\x88\xe6\xbb\x85", "236B, 236B_Add_Add", kEntries016_3, 1 },
	{ 223, "", "\xe7\xaa\x81\xe3\x81\x8d\xe5\x88\xba\xe3\x81\x97\xe5\xb2\xa9", "236B, 236B_Add_Add", kEntries016_11, 2 },
	{ 245, "", "\xe5\x9b\x9e\xe8\xbb\xa2", "236C_Add_AddEX", kEntries016_12, 4 },
	{ 246, "", "\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab", "236C, 236C_Finish, 236EX_Finish", kEntries016_13, 8 },
	{ 247, "", "\x32\x43\x43\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab\xe5\xb2\xa9", "236C, 236C_Finish, 236EX_Finish", nullptr, 0 },
	{ 248, "", "\xe3\x83\x91\xe3\x82\xa4\xe3\x83\xab\xe6\xb6\x88\xe6\xbb\x85", "236C_Add_AddEX", kEntries016_3, 1 },
	{ 249, "", "\xe3\x82\xa2\xe3\x82\xb5\xe3\x83\xab\xe3\x83\x88\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\x95\xe3\x83\xab\xe5\x87\xba\xe7\x8f\xbe", "236C", kEntries016_3, 1 },
	{ 264, "", "", "623BC_Hit, 623BC_Hit_Shidou, 623EX_Hit", kEntries016_14, 6 },
	{ 265, "", "", "623BC_Hit, 623BC_Hit_Shidou, 623EX_Hit", nullptr, 0 },
	{ 266, "", "\xe6\xb6\x88\xe6\xbb\x85", "623BC_Hit, 623BC_Hit_Shidou, 623EX_Hit", kEntries016_3, 1 },
	{ 267, "", "\xe6\xb6\x88\xe6\xbb\x85", "J623A_Hit", kEntries016_3, 1 },
	{ 324, "", "\xe9\xa3\x9b\xe3\x81\xb3\xe6\x95\xa3\xe3\x82\x8b\xe9\xa0\x81", "41236SP_Hit, 623B", nullptr, 0 },
	{ 326, "", "\xe7\x99\xba\xe5\xb0\x84\xe7\x88\x86\xe7\x99\xba", "41236SP_Hit, 623B", kEntries016_13, 8 },
	{ 327, "", "\xe7\x99\xba\xe5\xb0\x84\xe8\xb6\xb3\xe5\x85\x83\xe5\xb2\xa9", "41236SP_Hit, 623B", nullptr, 0 },
	{ 335, "", "\xe3\x83\x9e\xe3\x82\xba\xe3\x83\xab", "", kEntries016_7, 1 },
	{ 336, "", "\xe3\x83\x9e\xe3\x82\xba\xe3\x83\xab", "", kEntries016_7, 1 },
	{ 337, "", "\xe3\x83\x9e\xe3\x82\xba\xe3\x83\xab", "", kEntries016_7, 1 },
	{ 339, "", "\xe3\x82\xb0\xe3\x83\xac\xe3\x83\x8d\xe3\x83\xbc\xe3\x83\x89", "", kEntries016_15, 2 },
	{ 340, "", "\xe3\x82\xb0\xe3\x83\xac\xe7\x88\x86\xe7\x99\xba", "", kEntries016_15, 2 },
	{ 359, "", "\xe3\x83\x9e\xe3\x82\xba\xe3\x83\xab\xe2\x99\xaa", "", kEntries016_16, 2 },
	{ 360, "", "\xe8\x83\x8c\xe9\x9d\xa2\xe9\xa3\x9b\xe3\x81\xb3\xe3\x82\xb7\xe3\x82\xa8\xe3\x83\xab\x36\x30\x46\xe3\x81\x90\xe3\x82\x89\xe3\x81\x84\xe2\x99\xaa", "", nullptr, 0 },
	{ 364, "", "\xe8\x83\x8c\xe6\x99\xaf\xe3\x82\xa2\xe3\x83\x8b\xe3\x83\xa1\xe2\x99\xaa", "LastArc_Hit", kEntries016_17, 2 },
	{ 365, "", "\xe3\x80\x80\xe3\x82\xae\xe3\x83\xad\xe3\x83\x81\xe3\x83\xb3\xe3\x81\xb8\xe2\x99\xaa", "", kEntries016_18, 1 },
	{ 370, "", "\xe6\x89\x8b\xe5\x89\x8d\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab", "", nullptr, 0 },
	{ 371, "", "\xe5\x9c\xb0\xe9\x9d\xa2", "", nullptr, 0 },
	{ 372, "", "\xe9\x99\x8d\xe3\x82\x8a\xe6\xb3\xa8\xe3\x81\x90\xe2\x99\xaa", "", nullptr, 0 },
	{ 374, "", "\xe5\x88\x87\xe3\x82\x8a\xe6\x9b\xbf\xe3\x81\x88\xe5\x89\x8d\xe3\x81\xae\xe7\x88\x86\xe7\x99\xba\x34\x30\x46\xe2\x99\xaa", "", nullptr, 0 },
	{ 375, "", "\xe8\x83\x8c\xe6\x99\xaf", "", nullptr, 0 },
	{ 380, "", "\xe6\x9c\x80\xe5\xbe\x8c\xe7\x88\x86\xe7\x99\xba\xe2\x99\xaa", "", kEntries016_19, 1 },
	{ 381, "", "\xe5\x89\xb2\xe3\x82\x8c\xe3\x82\x8b\xe5\xa4\xa7\xe5\x9c\xb0\xe2\x99\xaa", "", nullptr, 0 },
	{ 382, "", "", "", nullptr, 0 },
	{ 933, "", "\xe6\xb6\x88\xe3\x81\x88\xe3\x82\x8b\xe6\xad\xa6\xe5\x99\xa8", "", kEntries016_3, 1 },
};

const unsigned char kEntries017_0[] = { 128, 192 };
const unsigned char kEntries017_1[] = { 245, 252 };
const unsigned char kEntries017_2[] = { 245, 246, 252 };
const unsigned char kEntries017_3[] = { 128, 130, 192, 194, 245, 246 };
const unsigned char kEntries017_4[] = { 128, 130, 192, 194 };
const unsigned char kEntries017_5[] = { 245, 246 };
const unsigned char kEntries017_6[] = { 248, 249 };
const unsigned char kEntries017_7[] = { 251 };
const unsigned char kEntries017_8[] = { 249 };
const unsigned char kEntries017_9[] = { 241, 243, 247, 248, 251 };
const unsigned char kEntries017_10[] = { 247, 248, 251 };
const unsigned char kEntries017_11[] = { 248 };
const unsigned char kEntries017_12[] = { 97, 98, 192, 194 };

const Row kEffects017[] = {
	{ 101, "", "", "", kEntries017_0, 2 },
	{ 102, "", "", "", kEntries017_1, 2 },
	{ 103, "", "", "", kEntries017_2, 3 },
	{ 104, "", "", "", kEntries017_2, 3 },
	{ 105, "", "", "", kEntries017_2, 3 },
	{ 106, "", "2A", "", kEntries017_0, 2 },
	{ 107, "", "2C", "", kEntries017_2, 3 },
	{ 108, "", "JA", "", kEntries017_2, 3 },
	{ 109, "", "JB", "623EX_Hit", kEntries017_2, 3 },
	{ 110, "", "JC", "623EX_Hit, AirDiveSC, AirSC, DiveSC", kEntries017_2, 3 },
	{ 111, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\xb9\xe3\x82\xab", "", kEntries017_3, 6 },
	{ 112, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92\xe3\x82\xb9\xe3\x82\xab", "", kEntries017_4, 4 },
	{ 113, "", "\xe6\x8a\x95\xe3\x81\x92\xe6\x88\x90\xe7\xab\x8b\xe5\xbe\x8c", "", kEntries017_2, 3 },
	{ 114, "", "\xe6\x8a\x95\xe3\x81\x92\xe6\x88\x90\xe7\xab\x8b\xe5\xbe\x8c", "", kEntries017_2, 3 },
	{ 115, "", "020", "ExSC", kEntries017_2, 3 },
	{ 116, "", "030_1", "RapidRelayAtk", kEntries017_2, 3 },
	{ 117, "", "030_2", "623EX_Hit, RapidRelayAtk", kEntries017_5, 2 },
	{ 118, "", "\xe6\x8a\x95\xe3\x81\x92\xe6\x88\x90\xe7\xab\x8b\xe5\xbe\x8c", "", kEntries017_2, 3 },
	{ 119, "", "\xe6\x8a\x95\xe3\x81\x92\xe6\x88\x90\xe7\xab\x8b\xe5\xbe\x8c", "", kEntries017_2, 3 },
	{ 120, "", "\xe3\x81\x9f\xe3\x81\xa1\x53\x43", "StdSC", kEntries017_2, 3 },
	{ 121, "", "\x30\x33\x30\x5f\x32\x5f\xe6\xb6\x88\xe3\x81\x88\xe3\x81\xaa\xe3\x81\x84", "RapidRelayAtk", kEntries017_5, 2 },
	{ 132, "", "", "", kEntries017_2, 3 },
	{ 166, "", "\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "s806", kEntries017_6, 2 },
	{ 167, "", "\xe5\xb2\xa9", "s806", kEntries017_7, 1 },
	{ 171, "", "\xe6\x8a\x95\xe3\x81\x92\xe6\x96\xac\xe3\x82\x8a\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LA_RushSisterB, s807", kEntries017_6, 2 },
	{ 187, "", "\x4a\x32\x42\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J2B_End", kEntries017_2, 3 },
	{ 188, "", "\x4a\x32\x42\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88\xe5\xa4\xa7", "J2B", kEntries017_2, 3 },
	{ 364, "", "\xe8\x8a\xb1\xe7\x81\xab\xe7\x94\x9f\xe6\x88\x90", "OpAD1", nullptr, 0 },
	{ 369, "", "\xe7\x85\x99", "OpAD1, OpAD2", nullptr, 0 },
	{ 370, "", "\xe5\x88\x9d\xe6\xae\xb5\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "OpAD1, OpAD2", kEntries017_7, 1 },
	{ 371, "", "", "OpAD2", kEntries017_7, 1 },
	{ 372, "", "\xe6\xb5\xae\xe3\x81\x8b\xe3\x81\x9b", "OpAD2", kEntries017_7, 1 },
	{ 373, "", "\xe3\x81\x90\xe3\x82\x8b\xe3\x81\x90\xe3\x82\x8b", "OpAD2", kEntries017_8, 1 },
	{ 374, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8b\xe3\x82\x8d\xe3\x81\x97", "OpAD2", kEntries017_7, 1 },
	{ 376, "", "\x41\x44\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "OpAD2", kEntries017_9, 5 },
	{ 377, "", "\x36\x43\xe8\xb9\xb4\xe3\x82\x8a\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "OpAD2", kEntries017_7, 1 },
	{ 378, "", "\x36\x43\xe8\xb9\xb4\xe3\x82\x8a\xe9\xa2\xa8", "OpAD2", nullptr, 0 },
	{ 379, "", "\x36\x43\xe8\xb9\xb4\xe3\x82\x8a\xe5\x8b\xa2\xe3\x81\x84\xe7\xb7\x9a", "OpAD2", nullptr, 0 },
	{ 380, "", "\x42\x43\xe3\x82\xb7\xe3\x83\xab\xe3\x82\xab\xe3\x83\xb3\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "OpAD2", kEntries017_7, 1 },
	{ 385, "", "\xe6\x9c\x80\xe5\xbe\x8c\xe5\xbc\x95\xe3\x81\xa3\xe5\xbc\xb5\xe3\x82\x8a\xe9\xa2\xa8\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "OpAD2", nullptr, 0 },
	{ 386, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe9\xa2\xa8", "OpAD2", nullptr, 0 },
	{ 388, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "OpAD2", kEntries017_10, 3 },
	{ 411, "", "\x4c\x41\xe8\x83\x8c\xe6\x99\xaf", "LastArc_Hit", nullptr, 0 },
	{ 412, "", "\x4c\x41\xe8\x83\x8c\xe6\x99\xaf", "", nullptr, 0 },
	{ 416, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe7\x88\x86\xe7\x99\xba\x31", "LastArc_Hit2", nullptr, 0 },
	{ 417, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe7\x88\x86\xe7\x99\xba\x32", "LastArc_Hit2", nullptr, 0 },
	{ 425, "", "\xe6\xb6\x88\xe3\x81\x88\xe3\x82\x8b\xe6\xae\x8b\xe5\x83\x8f", "LA_BigSisterA, LA_BigSisterB, LA_BigSisterC, LA_BigSisterD", nullptr, 0 },
	{ 435, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LA_RushSisterA", kEntries017_11, 1 },
	{ 436, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LA_RushSisterA", kEntries017_11, 1 },
	{ 462, "Grp_Hit_SisterSlashFin", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe7\x94\xa8\xe3\x83\xbb\xe5\xba\xa7\xe6\xa8\x99\xe5\x9b\xba\xe5\xae\x9a", "LastArc_Hit2", nullptr, 0 },
	{ 505, "", "\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "s214A, s214A_End", kEntries017_6, 2 },
	{ 520, "", "", "s214B, s214B2", kEntries017_12, 4 },
	{ 521, "", "", "s214B, s214B2", kEntries017_12, 4 },
	{ 535, "", "", "LA_RushSisterC, s214BC, s214EX", kEntries017_6, 2 },
	{ 552, "", "\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LA_RushSisterD, s0202A, s214A", kEntries017_6, 2 },
	{ 558, "", "", "s0202B", kEntries017_6, 2 },
	{ 565, "", "", "s0202BC, s0202EX", kEntries017_11, 1 },
};

const unsigned char kEntries018_0[] = { 248 };
const unsigned char kEntries018_1[] = { 192, 254 };

const Row kEffects018[] = {
	{ 105, "", "", "P_AtkA, P_AtkEX", kEntries018_0, 1 },
	{ 106, "", "", "P_AtkA, P_AtkEX", kEntries018_0, 1 },
	{ 210, "", "", "P_AtkB, P_AtkBC, P_AtkPowB", kEntries018_0, 1 },
	{ 211, "", "", "P_AtkA_Add, P_AtkBC, P_AtkEX_Hit, P_AtkPowB", kEntries018_0, 1 },
	{ 212, "", "", "P_AtkA_Add, P_AtkBC, P_AtkPowB", kEntries018_0, 1 },
	{ 213, "", "", "P_BackAttack", kEntries018_1, 2 },
	{ 214, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5", "P_AtkEX_Hit", kEntries018_0, 1 },
};

const unsigned char kEntries019_0[] = { 241, 244, 248 };
const unsigned char kEntries019_1[] = { 252 };
const unsigned char kEntries019_2[] = { 245, 249 };
const unsigned char kEntries019_3[] = { 246, 252 };
const unsigned char kEntries019_4[] = { 249 };
const unsigned char kEntries019_5[] = { 241 };
const unsigned char kEntries019_6[] = { 241, 243 };
const unsigned char kEntries019_7[] = { 244 };
const unsigned char kEntries019_8[] = { 244, 247, 248 };
const unsigned char kEntries019_9[] = { 16 };
const unsigned char kEntries019_10[] = { 244, 252 };
const unsigned char kEntries019_11[] = { 241, 246, 248, 252 };
const unsigned char kEntries019_12[] = { 245 };

const Row kEffects019[] = {
	{ 1, "", "\xe7\xab\x8b\xe3\x81\xa1\xe5\xbc\xb1\xe6\x94\xbb\xe6\x92\x83", "Neco11, Neco12, Neco14, Neco15, Neco16, Neco17, Neco2, Neco9", nullptr, 0 },
	{ 2, "", "\xe7\xab\x8b\xe3\x81\xa1\xe4\xb8\xad\xe6\x94\xbb\xe6\x92\x83", "Neco3, Neco4, Neco8", nullptr, 0 },
	{ 3, "", "\xe7\xab\x8b\xe3\x81\xa1\xe5\xbc\xb7\xe6\x94\xbb\xe6\x92\x83", "Neco3", nullptr, 0 },
	{ 62, "", "\xe6\x8a\x95\xe3\x81\x92\xe7\x88\x86\xe7\x99\xba\xe8\xa1\xa8", "", kEntries019_0, 3 },
	{ 63, "", "\xe6\x8a\x95\xe3\x81\x92\xe7\x88\x86\xe7\x99\xba\xe8\xa3\x8f", "", kEntries019_0, 3 },
	{ 64, "", "\xe6\x8a\x95\xe3\x81\x92\xe8\x82\x89\xe7\x90\x83", "", kEntries019_1, 1 },
	{ 65, "", "\xe6\x8a\x95\xe3\x81\x92\xe5\x9c\x9f\xe7\x85\x99", "", nullptr, 0 },
	{ 101, "", "\xe7\xab\x8b\xe3\x81\xa1\xe5\xbc\xb1", "", kEntries019_2, 2 },
	{ 102, "", "\xe7\xab\x8b\xe3\x81\xa1\xe4\xb8\xad", "StdB_End", kEntries019_2, 2 },
	{ 103, "", "\xe7\xab\x8b\xe3\x81\xa1\xe5\xbc\xb7", "", kEntries019_3, 2 },
	{ 104, "", "\xe3\x81\x97\xe3\x82\x83\xe3\x81\x8c\xe3\x81\xbf\xe5\xbc\xb1", "", kEntries019_2, 2 },
	{ 105, "", "\xe3\x81\x97\xe3\x82\x83\xe3\x81\x8c\xe3\x81\xbf\xe4\xb8\xad", "CroB_End", nullptr, 0 },
	{ 106, "", "\xe3\x81\x97\xe3\x82\x83\xe3\x81\x8c\xe3\x81\xbf\xe5\xbc\xb7", "", kEntries019_3, 2 },
	{ 107, "", "\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\xb3\xe3\x83\x97\xe5\xbc\xb1", "", kEntries019_2, 2 },
	{ 108, "", "\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\xb3\xe3\x83\x97\xe4\xb8\xad", "AirSC", kEntries019_2, 2 },
	{ 109, "", "\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\xb3\xe3\x83\x97\xe5\xbc\xb7", "AirC_End", kEntries019_4, 1 },
	{ 110, "", "\xe3\x82\xbf\xe3\x83\xa1\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\xb3\xe3\x83\x97\xe5\xbc\xb7", "", kEntries019_4, 1 },
	{ 111, "", "\xe3\x83\xad\xe3\x82\xb1\xe3\x83\x83\xe3\x83\x88\xe5\x99\xb4\xe5\xb0\x84", "", kEntries019_5, 1 },
	{ 112, "", "", "", nullptr, 0 },
	{ 113, "", "", "", nullptr, 0 },
	{ 117, "", "\xe3\x82\xbf\xe3\x83\xa1\xe7\xab\x8b\xe3\x81\xa1\xe4\xb8\xad", "", kEntries019_1, 1 },
	{ 118, "", "\xe3\x83\xa9\xe3\x83\x94\xe3\x83\x83\xe3\x83\x89\xe3\x83\x93\xe3\x83\xbc\xe3\x83\x88", "RapidRelayAtk", kEntries019_2, 2 },
	{ 121, "", "\x42\x43\xe3\x82\xb7\xe3\x83\xab\xe3\x82\xab\xe3\x83\xb3", "ExSC", nullptr, 0 },
	{ 122, "", "\x42\x43\xe3\x82\xb7\xe3\x83\xab\xe3\x82\xab\xe3\x83\xb3", "ExSC", nullptr, 0 },
	{ 129, "", "3C", "StdSC", kEntries019_1, 1 },
	{ 133, "", "\xe3\x82\xaa\xe3\x83\xa9\xe3\x82\xaa\xe3\x83\xa9\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5", "RapidRelay_Finish", kEntries019_2, 2 },
	{ 144, "", "", "6A_Throw_Hit, AirDiveSC, DiveSC", kEntries019_2, 2 },
	{ 152, "", "\xe3\x82\xbf\xe3\x83\xa1\xe7\xab\x8b\xe3\x81\xa1\xe4\xb8\xad", "LastArc_Atk", kEntries019_1, 1 },
	{ 168, "", "", "6C", kEntries019_2, 2 },
	{ 169, "", "", "6C_6C", kEntries019_2, 2 },
	{ 171, "", "", "6C_6C_6C", kEntries019_2, 2 },
	{ 174, "", "", "J2B", nullptr, 0 },
	{ 179, "", "", "J2C", kEntries019_6, 2 },
	{ 186, "", "", "4C", kEntries019_2, 2 },
	{ 228, "", "\xe3\x82\x82\xe3\x81\x90\xe3\x82\x8a\xef\xbc\x88\xe7\x9f\xad\xef\xbc\x89", "214A, 214B, 214BC, 214C", nullptr, 0 },
	{ 229, "", "\xe3\x82\x82\xe3\x81\x90\xe3\x82\x8a", "DummyNeco", nullptr, 0 },
	{ 230, "", "\xe5\x87\xba\xe3\x81\xa6\xe3\x81\x8f\xe3\x82\x8b", "214A, 214B, 214C, DummyNeco, UndergroundInNeco, calldummy", nullptr, 0 },
	{ 231, "", "\xe5\x87\xba\xe3\x81\xa6\xe3\x81\x8f\xe3\x82\x8b\xef\xbc\x88\xe7\x9f\xad\xef\xbc\x89", "214BC", nullptr, 0 },
	{ 256, "", "\xe5\x85\x89\xe4\xbd\x93\xe5\x8c\x96\xe7\xb2\x92\xe5\xad\x90", "623EX_Hit", nullptr, 0 },
	{ 257, "", "\xe5\x85\x89\xe4\xbd\x93\xe5\x8c\x96\x42\x47", "623EX_Hit", nullptr, 0 },
	{ 258, "", "\xe5\x85\x89\xe4\xbd\x93\xe5\x8c\x96\xe7\x99\xba\xe5\x85\x89", "623EX_Hit", nullptr, 0 },
	{ 279, "", "\xe3\x82\xb8\xe3\x82\xa7\xe3\x83\x83\xe3\x83\x88\xe5\x99\xb4\xe5\xb0\x84\x28\xe9\xab\x98\xe9\x80\x9f\x29", "J214BC", kEntries019_5, 1 },
	{ 280, "", "\xe3\x82\xb4\xe3\x83\x83\xe3\x83\x89\xe3\x82\xad\xe3\x83\xa3\xe3\x83\x83\xe3\x83\x88", "J214EX", kEntries019_7, 1 },
	{ 282, "", "\xe3\x82\xb8\xe3\x82\xa7\xe3\x83\x83\xe3\x83\x88\xe5\x99\xb4\xe5\xb0\x84", "AbaddonBeret, DummyNeco, J214A, J214B, J214_JAdd2, J214_JAdd4, J214_JAdd6, J214_JAdd8, Neco7, SOSNeco", kEntries019_5, 1 },
	{ 308, "", "\xe3\x82\xbf\xe3\x83\xa1\xe3\x83\x96\xe3\x83\xac\xe3\x82\xb9\xe5\x9c\x9f\xe7\x85\x99", "0202A, Neco8", nullptr, 0 },
	{ 309, "", "\xe3\x82\xbf\xe3\x83\xa1\xe3\x83\x96\xe3\x83\xac\xe3\x82\xb9\xe5\x9c\x9f\xe7\x85\x99\xe7\xb6\x9a\xe3\x81\x8d", "", nullptr, 0 },
	{ 351, "", "\xe8\xbb\xa2\xe3\x81\x8c\xe3\x82\x8b\xe3\x82\xa8\xe3\x82\xaf\xe3\x82\xb9\xe3\x82\xab\xe3\x83\xaa\xe3\x83\x90\xe3\x83\xab\xe3\x83\xbc\xe3\x83\xb3", "", nullptr, 0 },
	{ 354, "", "\xe7\x84\xa1\xe9\x99\x90\xe3\x83\x8d\xe3\x82\xb3", "41236SP", nullptr, 0 },
	{ 355, "", "\xe3\x83\x8d\xe3\x82\xb3\xe3\x83\x95\xe3\x83\xac\xe3\x82\xa2", "", kEntries019_8, 3 },
	{ 356, "", "\xe5\x9c\x9f\xe7\x85\x99", "41236SP", nullptr, 0 },
	{ 357, "", "\xe9\xa2\xa8\xe8\x88\xb9\xe7\xa0\xb4\xe8\xa3\x82", "41236SP", nullptr, 0 },
	{ 359, "", "\xe7\x84\xa1\xe9\x99\x90\xe3\x83\x8d\xe3\x82\xb3\xe7\xb5\x82\xe4\xba\x86", "", nullptr, 0 },
	{ 360, "", "\xe3\x83\x8d\xe3\x82\xb3\xe3\x83\x95\xe3\x83\xac\xe3\x82\xa2\xe7\xb5\x82\xe4\xba\x86", "", kEntries019_8, 3 },
	{ 361, "", "\xe5\x9c\x9f\xe7\x85\x99\xe7\xb5\x82\xe4\xba\x86", "", nullptr, 0 },
	{ 363, "", "\xe5\xb4\xa9\xe3\x82\x8c\xe3\x81\x9d\xe3\x81\x86\xe3\x81\xaa\xe3\x83\x8d\xe3\x82\xb3", "41236SP", nullptr, 0 },
	{ 364, "", "\xe5\xb4\xa9\xe3\x82\x8c\xe3\x82\x8b\xe3\x83\x8d\xe3\x82\xb3", "", kEntries019_5, 1 },
	{ 365, "", "\xe8\xb5\xb0\xe3\x82\x8a\xe5\x8e\xbb\xe3\x82\x8a\xe5\x9c\x9f\xe7\x85\x99", "", nullptr, 0 },
	{ 366, "", "\xe8\xb5\xb0\xe3\x82\x8a\xe5\x8e\xbb\xe3\x82\x8a\xe5\x9c\x9f\xe7\x85\x99", "", nullptr, 0 },
	{ 367, "", "\xe3\x83\x87\xe3\x83\x90\xe3\x83\x95", "", kEntries019_7, 1 },
	{ 368, "", "MC_RATE_DOWN", "", nullptr, 0 },
	{ 373, "", "\xe3\x81\xab\xe3\x82\x83\xe3\x82\x93\xe3\x81\xb7\xe3\x81\x97\xe3\x83\xbc", "63214SP", kEntries019_9, 1 },
	{ 374, "", "\xe3\x81\xab\xe3\x82\x83\xe3\x82\x93\xe3\x81\xb7\xe3\x81\x97\xe3\x83\xbc\xe6\xb6\x88\xe6\xbb\x85", "", kEntries019_9, 1 },
	{ 375, "", "\xe5\x9c\x9f\xe7\x85\x99", "", nullptr, 0 },
	{ 376, "", "\xe5\x9c\x9f\xe7\x85\x99\xe6\xb6\x88\xe6\xbb\x85", "", nullptr, 0 },
	{ 377, "", "\xe9\xa2\xa8", "", nullptr, 0 },
	{ 378, "", "\xe9\xa2\xa8", "", nullptr, 0 },
	{ 405, "", "\xe7\xaa\x93\x28\x70\x61\x6c\xe3\x81\x8c\xe9\x81\x95\xe3\x81\x86\xe3\x81\xae\xe3\x81\xa7\xe5\x88\xa5\x70\x61\x74\x29", "LA_Rocket", nullptr, 0 },
	{ 406, "", "\xe4\xb9\x97\xe3\x82\x8a\xe8\xbe\xbc\xe3\x82\x93\xe3\x81\xa0\xe6\x99\x82\xe3\x81\xae\xe7\xaa\x93", "LA_Rocketlaunch", nullptr, 0 },
	{ 407, "", "\xe9\xa3\x9b\xe3\x81\xb6\xe3\x83\xad\xe3\x82\xb1\xe3\x83\x83\xe3\x83\x88\xe7\xaa\x93", "", nullptr, 0 },
	{ 412, "", "\xe5\xae\x87\xe5\xae\x99\xe7\xaa\x93", "LA_RocketUniverse", nullptr, 0 },
	{ 413, "", "\xe3\x83\xad\xe3\x82\xb1\xe3\x83\x83\xe3\x83\x88\xe7\x85\x99", "", nullptr, 0 },
	{ 414, "", "\xe3\x83\xad\xe3\x82\xb1\xe3\x83\x83\xe3\x83\x88\xe7\x82\x8e", "", nullptr, 0 },
	{ 416, "", "\xe9\xbb\x92\xe3\x83\x95\xe3\x82\xa7\xe3\x83\xbc\xe3\x83\x89", "", nullptr, 0 },
	{ 417, "", "\xe7\x99\xbd\xe3\x83\x95\xe3\x82\xa7\xe3\x83\xbc\xe3\x83\x89", "", nullptr, 0 },
	{ 418, "", "\xe5\x9c\xb0\xe7\x90\x83", "", nullptr, 0 },
	{ 419, "", "\xe3\x83\x8d\xe3\x82\xb3\xe7\xbc\xb6", "", nullptr, 0 },
	{ 421, "", "\xe9\x8a\x80\xe6\xb2\xb3", "", nullptr, 0 },
	{ 423, "", "\xe9\x8a\x80\xe6\xb2\xb3\xe7\x88\x86\xe7\x99\xba", "", nullptr, 0 },
	{ 437, "", "\xe3\x82\xa8\xe3\x82\xb3\xe7\x99\xbb\xe5\xa0\xb4", "EcoArc", kEntries019_3, 2 },
	{ 438, "", "\xe3\x82\xa8\xe3\x82\xb3\xe6\x92\xa4\xe9\x80\x80", "EcoArc", kEntries019_3, 2 },
	{ 446, "", "\xe7\x82\x8e\xe4\xb8\x8a", "421EX_Tmitter", kEntries019_0, 3 },
	{ 455, "", "\xe6\x90\xba\xe5\xb8\xaf\xe9\x9b\xbb\xe6\xb3\xa2", "421B, 421BC", nullptr, 0 },
	{ 468, "", "\xe5\x90\xb9\xe3\x81\x8d\xe5\x87\xba\xe3\x81\x97", "icon_fgo, icon_light, icon_necocan, icon_sos, icon_tmitter, icon_train, icon_vtuber", nullptr, 0 },
	{ 480, "", "\xe6\xb1\x8e\xe7\x94\xa8\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88\xe5\x91\xbc\xe3\x81\xb3", "Altria", nullptr, 0 },
	{ 481, "", "\xe7\x99\xba\xe5\x8b\x95\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\xa9", "Altria", kEntries019_7, 1 },
	{ 482, "", "\xe5\xa5\xa5\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab", "Altria", kEntries019_7, 1 },
	{ 483, "", "\x42\x47\xe8\xb6\xb3\xe5\x85\x83\xe5\x85\x89", "Altria", kEntries019_7, 1 },
	{ 505, "", "\x4e\x65\x63\x6f\x49\x74\x65\x6d\xe5\x8f\x96\xe5\xbe\x97", "ItemPear, ItemStone", kEntries019_10, 2 },
	{ 516, "", "\xe3\x83\x8d\xe3\x82\xb3\xe7\xbc\xb6\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae\xef\xbc\x88\xe8\x90\xbd\xe4\xb8\x8b\xef\xbc\x89", "Necocan", nullptr, 0 },
	{ 517, "", "\xe3\x83\x8d\xe3\x82\xb3\xe7\xbc\xb6\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae\xef\xbc\x88\xe5\x90\xb9\xe3\x81\x8d\xe9\xa3\x9b\xe3\x81\xb3\xef\xbc\x89", "Necocan", nullptr, 0 },
	{ 543, "", "\xe9\xaf\x96\xe9\x80\x80\xe5\xa0\xb4\xe8\xa1\xa8", "", kEntries019_7, 1 },
	{ 544, "", "\xe9\xaf\x96\xe9\x80\x80\xe5\xa0\xb4\xe8\xa3\x8f", "", kEntries019_7, 1 },
	{ 562, "", "\xe3\x82\xa2\xe3\x82\xa4\xe3\x83\x86\xe3\x83\xa0\xe5\x8f\x96\xe5\xbe\x97", "ItemCurry, ItemMapo, Necocan", kEntries019_11, 4 },
	{ 583, "", "00", "Eff_hpgain_Lv1, Eff_hpgain_Lv2, Eff_hpgain_Lv3", nullptr, 0 },
	{ 699, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe3\x82\xb8\xe3\x83\xa3\xe3\x83\x96", "6A", kEntries019_2, 2 },
	{ 700, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe3\x82\xb9\xe3\x83\x88\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x88", "6A", kEntries019_2, 2 },
	{ 701, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe5\xb7\xa6\xe3\x82\xad\xe3\x83\x83\xe3\x82\xaf", "6A", kEntries019_2, 2 },
	{ 702, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe6\x89\x93\xe3\x81\xa1\xe4\xb8\x8b\xe3\x82\x8d\xe3\x81\x97", "6A", kEntries019_2, 2 },
	{ 703, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe5\x8f\xb3\xe3\x83\xad\xe3\x83\xbc\xe3\x82\xad\xe3\x83\x83\xe3\x82\xaf", "6A", kEntries019_2, 2 },
	{ 704, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81", "6A", kEntries019_2, 2 },
	{ 705, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe8\xa3\x8f\xe5\x9b\x9e\xe3\x82\x8a\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81", "6A", kEntries019_2, 2 },
	{ 706, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe5\xb4\xa9\xe3\x82\x8c\xe3\x81\x95\xe3\x81\x9b\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81", "6A", kEntries019_2, 2 },
	{ 707, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x91\xe3\x83\xbc", "6A", kEntries019_2, 2 },
	{ 708, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe7\xa9\xba\xe4\xb8\xad\xe5\xbb\xbb\xe3\x81\x97\xe8\xb9\xb4\xe3\x82\x8a", "6A", kEntries019_2, 2 },
	{ 709, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe7\xa9\xba\xe4\xb8\xad\xe8\x86\x9d\xe8\xb9\xb4\xe3\x82\x8a", "6A", kEntries019_2, 2 },
	{ 710, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe7\xa9\xba\xe4\xb8\xad\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92", "6A", kEntries019_2, 2 },
	{ 711, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe7\xa9\xba\xe4\xb8\xad\xe3\x83\x89\xe3\x83\xad\xe3\x83\x83\xe3\x83\x97\xe3\x82\xad\xe3\x83\x83\xe3\x82\xaf", "6A", kEntries019_2, 2 },
	{ 712, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe7\xa9\xba\xe4\xb8\xad\xe3\x82\xa2\xe3\x83\x83\xe3\x83\x91\xe3\x83\xbc", "6A", kEntries019_2, 2 },
	{ 713, "", "\x31\x37\xe9\x80\xa3\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\x5f\xe7\xa9\xba\xe4\xb8\xad\xe3\x82\xb8\xe3\x82\xa7\xe3\x83\x83\xe3\x83\x88", "6A", kEntries019_5, 1 },
	{ 808, "", "\xe3\x81\x90\xe3\x82\x8b\xe3\x81\x90\xe3\x82\x8b\xe3\x81\xb1\xe3\x82\x93\xe3\x81\xa1", "ThrowMiss", kEntries019_12, 1 },
	{ 809, "", "\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe5\x9c\x9f\xe7\x85\x99", "ThrowMiss", nullptr, 0 },
	{ 810, "", "\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe5\x9c\x9f\xe7\x85\x99\xe7\xb5\x82\xe3\x82\x8f\xe3\x82\x8a", "", nullptr, 0 },
	{ 935, "", "", "", nullptr, 0 },
	{ 953, "", "\xe3\x82\xb8\xe3\x82\xa7\xe3\x83\x83\xe3\x83\x88\xe5\x99\xb4\xe5\xb0\x84", "", kEntries019_5, 1 },
	{ 954, "", "\xe3\x83\xa9\xe3\x82\xa6\xe3\x83\xb3\xe3\x83\x89\xe5\x8b\x9d\xe3\x81\xa1\xe3\x83\xa2\xe3\x83\xbc\xe3\x82\xb7\xe3\x83\xa7\xe3\x83\xb3\xe3\x81\xae\xe8\xa6\x8b\xe3\x81\x9f\xe7\x9b\xae", "", nullptr, 0 },
};

const unsigned char kEntries020_0[] = { 70, 72 };
const unsigned char kEntries020_1[] = { 219, 247, 253 };
const unsigned char kEntries020_2[] = { 247, 253 };
const unsigned char kEntries020_3[] = { 243, 246, 248 };
const unsigned char kEntries020_4[] = { 241, 247, 253 };
const unsigned char kEntries020_5[] = { 244 };
const unsigned char kEntries020_6[] = { 70, 247, 253 };
const unsigned char kEntries020_7[] = { 246, 253 };
const unsigned char kEntries020_8[] = { 246 };
const unsigned char kEntries020_9[] = { 243, 246, 247, 248, 252 };
const unsigned char kEntries020_10[] = { 219, 245, 252, 253 };
const unsigned char kEntries020_11[] = { 243, 245, 247, 253 };
const unsigned char kEntries020_12[] = { 219, 245, 252 };
const unsigned char kEntries020_13[] = { 245, 252 };
const unsigned char kEntries020_14[] = { 247, 252, 253 };

const Row kEffects020[] = {
	{ 101, "", "", "", kEntries020_0, 2 },
	{ 102, "", "", "", kEntries020_1, 3 },
	{ 103, "", "", "StdC_End", kEntries020_1, 3 },
	{ 105, "", "", "", kEntries020_1, 3 },
	{ 106, "", "", "", kEntries020_1, 3 },
	{ 107, "", "", "", kEntries020_2, 2 },
	{ 108, "", "", "", kEntries020_1, 3 },
	{ 109, "", "JC", "AirC_End, AirDiveSC, AirSC, DiveSC", kEntries020_1, 3 },
	{ 110, "", "J[C]", "", kEntries020_1, 3 },
	{ 115, "", "RB", "RapidRelayAtk", kEntries020_2, 2 },
	{ 116, "", "RB", "RapidRelayAtk", kEntries020_2, 2 },
	{ 117, "", "SCA", "StdSC", kEntries020_2, 2 },
	{ 120, "", "SCBC", "ExSC", kEntries020_2, 2 },
	{ 122, "", "\xe5\x9c\xb0\xe4\xb8\x8a\xe6\x8a\x95\xe3\x81\x92\xe5\x9c\xb0\xe9\x9d\xa2\xe9\x99\xa5\xe6\xb2\xa1", "", nullptr, 0 },
	{ 123, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries020_1, 3 },
	{ 132, "", "", "", kEntries020_2, 2 },
	{ 182, "", "\xe7\x85\x99", "6C_End", nullptr, 0 },
	{ 183, "", "\xe7\x85\x99\xe3\x83\xbb\xe5\xa4\xa7", "6C", nullptr, 0 },
	{ 185, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "6C_End", kEntries020_1, 3 },
	{ 186, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\xe3\x83\xbb\xe5\xa4\xa7", "6C", kEntries020_1, 3 },
	{ 191, "", "\xe3\x80\x80\xe9\xa2\xa8\xe7\x94\x9f\xe6\x88\x90", "236A, 236EX", nullptr, 0 },
	{ 194, "", "\xe3\x80\x80\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x86\xe3\x82\xa3\xe3\x82\xaf\xe3\x83\xab\xe7\x94\x9f\xe6\x88\x90", "236A, 236EX", nullptr, 0 },
	{ 210, "", "", "236A, 236B, 236BC, 236B_End, 236EX", kEntries020_1, 3 },
	{ 211, "", "", "236B, 236BC, 236B_End", kEntries020_1, 3 },
	{ 212, "", "\xe7\x85\x99", "236B, 236BC, 236B_End", nullptr, 0 },
	{ 215, "aura_236EX", "\xe7\xaa\x81\xe9\x80\xb2\xe7\x9b\xbe\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\xa9\xe3\x83\xab\xe3\x83\xbc\xe3\x83\x97", "236A, 236EX", kEntries020_3, 3 },
	{ 216, "", "\xe7\xaa\x81\xe9\x80\xb2\xe7\x9b\xbe\xe3\x82\xaa\xe3\x83\xbc\xe3\x83\xa9\xe6\xb6\x88\xe6\xbb\x85", "236A, 236B, 236BC, 236B_End", kEntries020_3, 3 },
	{ 229, "", "\xe9\xa3\x9b\xe3\x81\xb3\xe4\xb8\x8a\xe3\x81\x8c\xe3\x82\x8b\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\x45\x58\xe6\x9c\x80\xe5\xbe\x8c\xe7\x94\xa8", "", kEntries020_4, 3 },
	{ 230, "", "\xe9\xa3\x9b\xe3\x81\xb3\xe4\xb8\x8a\xe3\x81\x8c\xe3\x82\x8b\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "623A, 623B, 623BC, 623EX", kEntries020_4, 3 },
	{ 231, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe6\x8a\x9c\xe3\x81\x8d", "623A, 623B, 623BC, 623EX", kEntries020_1, 3 },
	{ 270, "", "\xe5\x88\x87\xe3\x82\x8a\xe6\x89\x95\xe3\x81\x84\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A, 214A_JAdd, 214BC, 214B_JAdd, 214EX", kEntries020_1, 3 },
	{ 271, "", "\xe5\x88\x9d\xe6\xae\xb5\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A, 214BC, 214EX", kEntries020_1, 3 },
	{ 273, "", "\xe5\x88\x9d\xe6\xae\xb5\x45\x58\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214B", kEntries020_1, 3 },
	{ 291, "", "\xe5\x9c\xb0\xe9\x9d\xa2\xe9\x99\xa5\xe6\xb2\xa1", "J236A, J236A_JHit, J236B, J236BC, J236BC_JHit, J236B_JHit, J236EX, J236EX_JHit", nullptr, 0 },
	{ 295, "", "\xe8\x90\xbd\xe4\xb8\x8b\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J236A, J236B, J236BC, J236BC_JHit, J236B_JHit, J236EX, J236EX_JHit", kEntries020_2, 2 },
	{ 296, "", "\xe8\x90\xbd\xe4\xb8\x8b\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J236A, J236A_JHit", kEntries020_2, 2 },
	{ 313, "", "A", "J214A, J214B, J214BC, J214EX", kEntries020_1, 3 },
	{ 316, "", "", "J214A, J214B, J214BC, J214EX", nullptr, 0 },
	{ 332, "", "\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "0202A, 0202B, 0202BC, 0202EX", kEntries020_1, 3 },
	{ 333, "", "\xe7\x85\x99", "0202A, 0202B, 0202BC, 0202EX", nullptr, 0 },
	{ 340, "", "\xe8\xa1\x9d\xe6\x92\x83\xe5\xb2\xa9", "Rc0202A, Rc0202B", nullptr, 0 },
	{ 356, "", "\xe3\x80\x80\xe8\xbc\xaa\xe3\x81\xa3\xe3\x81\x8b\xe7\x94\x9f\xe6\x88\x90", "ADThrowShield, EXThrowShield", nullptr, 0 },
	{ 360, "", "\xe5\xa3\x8a\xe3\x82\x8c\xe3\x82\x8b\xe5\x9c\xb0\xe9\x9d\xa2", "41236SP_Hit", nullptr, 0 },
	{ 361, "", "\xe6\x97\x8b\xe9\xa2\xa8", "41236SP_Hit", nullptr, 0 },
	{ 364, "", "\xe3\x80\x80\xe9\xa2\xa8\xe7\x94\x9f\xe6\x88\x90", "41236SP", nullptr, 0 },
	{ 373, "", "\xe7\x9b\xbe\xe6\x8a\x95\xe3\x81\x92\xe9\xa2\xa8", "41236SP, 421EX", kEntries020_5, 1 },
	{ 374, "", "\xe7\xaa\x81\xe3\x81\x8d", "41236SP", kEntries020_6, 3 },
	{ 375, "", "\xe7\x9b\xbe\xe5\x87\xba\xe7\x8f\xbe", "41236SP, 421EX", kEntries020_7, 2 },
	{ 378, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe5\xbe\x8c\xe7\x85\x99", "41236SP_End", nullptr, 0 },
	{ 419, "", "\xe7\x9b\xbe\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3", "", kEntries020_8, 1 },
	{ 421, "", "\xe7\x9b\xbe\xe3\x83\x89\xe3\x82\xb9\xe3\x83\xb3\xe7\x85\x99", "BigMash", nullptr, 0 },
	{ 422, "", "\xe7\x9b\xbe\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3", "BigMash", kEntries020_8, 1 },
	{ 423, "", "\xe3\x80\x80\xe7\x9b\xbe\xe9\xad\x94\xe6\xb3\x95\xe5\xa5\xa5\xe7\xaa\x81\xe9\xa2\xa8", "", nullptr, 0 },
	{ 424, "", "\xe3\x80\x80\xe9\xad\x94\xe6\xb3\x95\xe9\x99\xa3\xe7\x99\xba\xe5\xb0\x84\xe5\x9c\x9f\xe7\x85\x99", "", nullptr, 0 },
	{ 426, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe5\xbe\x8c\xe7\x85\x99", "LastArc_End", nullptr, 0 },
	{ 431, "", "\xe8\xb6\x85\xe6\x8a\x80\x42\x47\x32", "", kEntries020_9, 5 },
	{ 435, "", "\xe3\x80\x80\xe3\x80\x80\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe9\xa2\xa8\x31", "LastArc_Hit", nullptr, 0 },
	{ 436, "", "\xe3\x80\x80\xe3\x80\x80\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe9\xa2\xa8\x32", "LastArc_Hit", nullptr, 0 },
	{ 438, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe5\x88\x9d\xe6\xae\xb5\xe3\x83\x90\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5", "LastArc_Hit", kEntries020_10, 4 },
	{ 439, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe5\x88\x9d\xe6\xae\xb5\xe8\xa1\x9d\xe6\x92\x83\xe6\xb3\xa2", "LastArc_Hit", kEntries020_11, 4 },
	{ 440, "", "\xe4\xb9\xb1\xe8\x88\x9e\x4a\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LastArc_Hit", kEntries020_12, 3 },
	{ 441, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe5\x88\x87\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LastArc_Hit", kEntries020_13, 2 },
	{ 442, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe5\x88\x87\xe3\x82\x8a\xe6\x89\x95\xe3\x81\x84\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "LastArc_Hit", kEntries020_13, 2 },
	{ 443, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries020_14, 3 },
	{ 444, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe6\x80\xa5\xe9\x99\x8d\xe4\xb8\x8b\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "LastArc_Hit", kEntries020_13, 2 },
	{ 445, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe6\x80\xa5\xe9\x99\x8d\xe4\xb8\x8b\xe5\x9c\xb0\xe9\x9d\xa2\xe9\x99\xa5\xe6\xb2\xa1", "LastArc_Hit", nullptr, 0 },
	{ 446, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe6\x8a\x95\xe3\x81\x92\xe5\x9c\xb0\xe9\x9d\xa2\xe9\x99\xa5\xe6\xb2\xa1", "", nullptr, 0 },
	{ 447, "", "\xe4\xb9\xb1\xe8\x88\x9e\xe8\xb9\xb4\xe3\x82\x8a\xe4\xb8\x8a\xe3\x81\x92\xe7\x85\x99", "", nullptr, 0 },
	{ 935, "", "\xe7\x9b\xbe\xe6\xb6\x88\xe6\xbb\x85", "", kEntries020_7, 2 },
	{ 936, "", "\xe7\x9b\xbe\xe6\xb6\x88\xe6\xbb\x85\xef\xbc\x92", "", kEntries020_7, 2 },
};

const unsigned char kEntries021_0[] = { 24, 26, 59, 61 };
const unsigned char kEntries021_1[] = { 245, 247, 254 };
const unsigned char kEntries021_2[] = { 59, 60, 61 };
const unsigned char kEntries021_3[] = { 245, 247, 253 };
const unsigned char kEntries021_4[] = { 59, 61 };
const unsigned char kEntries021_5[] = { 86, 87, 89 };
const unsigned char kEntries021_6[] = { 81, 83, 86, 87 };
const unsigned char kEntries021_7[] = { 247, 250 };
const unsigned char kEntries021_8[] = { 245, 246, 247, 249, 250 };
const unsigned char kEntries021_9[] = { 245, 247 };
const unsigned char kEntries021_10[] = { 59, 61, 166, 168 };
const unsigned char kEntries021_11[] = { 245, 249, 250 };
const unsigned char kEntries021_12[] = { 3, 18, 26, 56, 61, 81, 162 };
const unsigned char kEntries021_13[] = { 246, 247, 251 };
const unsigned char kEntries021_14[] = { 247, 253 };
const unsigned char kEntries021_15[] = { 245 };
const unsigned char kEntries021_16[] = { 245, 246, 249, 250 };
const unsigned char kEntries021_17[] = { 245, 247, 249 };
const unsigned char kEntries021_18[] = { 18, 26, 56, 81, 178 };
const unsigned char kEntries021_19[] = { 245, 247, 249, 250 };
const unsigned char kEntries021_20[] = { 241, 247 };
const unsigned char kEntries021_21[] = { 246 };
const unsigned char kEntries021_22[] = { 247, 250, 253 };
const unsigned char kEntries021_23[] = { 245, 246, 247, 249, 253 };
const unsigned char kEntries021_24[] = { 241, 247, 250 };
const unsigned char kEntries021_25[] = { 250 };

const Row kEffects021[] = {
	{ 101, "", "A", "", kEntries021_0, 4 },
	{ 102, "", "B", "", kEntries021_1, 3 },
	{ 103, "", "C", "41236SP_Hit, StdC_End", kEntries021_1, 3 },
	{ 104, "", "2A", "", kEntries021_2, 3 },
	{ 105, "", "2B", "", kEntries021_1, 3 },
	{ 106, "", "2C", "", kEntries021_3, 3 },
	{ 107, "", "JA", "", kEntries021_4, 2 },
	{ 108, "", "JB", "AirDiveSC, DiveSC", kEntries021_1, 3 },
	{ 109, "", "JC", "AirC_End, AirSC", kEntries021_1, 3 },
	{ 110, "", "[C]", "", kEntries021_1, 3 },
	{ 115, "", "RB1", "RapidRelayAtk", kEntries021_1, 3 },
	{ 116, "", "RB2", "RapidRelayAtk", kEntries021_1, 3 },
	{ 117, "", "RB3", "41236SP_Hit, RapidRelayAtk", kEntries021_1, 3 },
	{ 118, "", "RB4", "", kEntries021_1, 3 },
	{ 119, "", "RB5", "", kEntries021_1, 3 },
	{ 121, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\xb9\xe3\x82\xab", "", kEntries021_5, 3 },
	{ 122, "", "\xe6\x8a\x95\xe3\x81\x92\xe9\xae\xae\xe8\xa1\x80", "", nullptr, 0 },
	{ 123, "", "\xe6\x8a\x95\xe3\x81\x92\xe3\x82\xb9\xe3\x82\xab", "", kEntries021_5, 3 },
	{ 124, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92", "", kEntries021_6, 4 },
	{ 132, "air_spin", "\xe7\xab\x9c\xe5\xb7\xbb", "StdSC", kEntries021_7, 2 },
	{ 133, "", "\x33\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "StdSC", kEntries021_7, 2 },
	{ 136, "", "\x33\x43\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries021_7, 2 },
	{ 186, "", "\xe5\x9c\xb0\xe9\x9d\xa2\xe7\x9d\x80\xe5\x9c\xb0\xe8\xa1\x9d\xe6\x92\x83\xe6\xb3\xa2", "", kEntries021_8, 5 },
	{ 198, "", "\xe6\x97\xa9\xe9\xa7\x86\xe3\x81\x91\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe9\xa2\xa8", "236A, 236B, 236BC, 236EX", kEntries021_9, 2 },
	{ 207, "", "\xe7\xab\x9c\xe5\xb7\xbb", "236EX_Hit", kEntries021_7, 2 },
	{ 216, "", "\xe5\xbc\xa7\xe6\x9c\x88\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "236A_JAddA", kEntries021_9, 2 },
	{ 220, "", "\xe8\xb6\xb3\xe6\x89\x95\xe3\x81\x84\x31", "236B_End", kEntries021_10, 4 },
	{ 221, "", "\xe8\xb6\xb3\xe6\x89\x95\xe3\x81\x84\x32", "236B_End", kEntries021_1, 3 },
	{ 224, "", "\xe5\x9e\x82\xe7\x9b\xb4\xe8\x90\xbd\xe4\xb8\x8b\xe4\xb8\x80\xe9\x96\x83", "236A_JAddC, J236BC, J236EX", kEntries021_11, 3 },
	{ 225, "", "\xe6\x80\xa5\xe9\x99\x8d\xe4\xb8\x8b\xe6\xae\x8b\xe5\x83\x8f", "236A_JAddC, J236BC, J236EX", kEntries021_12, 7 },
	{ 227, "", "\xe6\xa7\x8b\xe3\x81\x88\xe3\x82\xad\xe3\x83\xa9", "214A, 214B, 623B", kEntries021_13, 3 },
	{ 236, "", "\xe4\xb8\x80\xe9\x96\x83\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A_AddB, 214B, 214EX, 214EX_Hit", kEntries021_7, 2 },
	{ 239, "", "\xe6\x98\x87\xe7\xab\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "623A", kEntries021_3, 3 },
	{ 240, "", "\xe6\x98\x87\xe7\xab\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214A_AddA, 41236SP_Hit, 623B, 623BC, 623EX", kEntries021_3, 3 },
	{ 244, "", "\x32\x31\x34\x42\x43\xe6\x98\x9f\xe7\x94\x9f\xe6\x88\x90", "214BC", nullptr, 0 },
	{ 245, "", "\x32\x31\x34\x45\x58\xe5\x88\x9d\xe6\xae\xb5\xe4\xb8\x80\xe9\x96\x83", "214EX_Hit", kEntries021_14, 2 },
	{ 247, "", "\xe3\x83\x81\xe3\x83\xa3\xe3\x82\xad\xe3\x83\xb3", "214BC, 214EX_Hit", nullptr, 0 },
	{ 250, "", "\x32\x31\x34\x45\x58\xe8\x83\x8c\xe6\x99\xaf", "214EX_Hit", kEntries021_15, 1 },
	{ 251, "", "\x32\x31\x34\x45\x58\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "214EX_Hit", kEntries021_16, 4 },
	{ 259, "", "\x36\x32\x33\xe6\xae\x8b\xe5\x83\x8f", "623A, 623B, 623BC, 623EX", nullptr, 0 },
	{ 263, "", "\xe5\x9b\x9e\xe8\xbb\xa2\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\x32", "236B, 236BC", kEntries021_9, 2 },
	{ 264, "", "\xe6\x8c\xaf\xe3\x82\x8a\xe4\xb8\x8b\xe3\x82\x8d\xe3\x81\x97\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "236BC", kEntries021_1, 3 },
	{ 266, "", "\xe3\x81\x90\xe3\x82\x8b\xe3\x81\x90\xe3\x82\x8b\xe3\x83\xbb\xe7\x9f\xad", "236B, 236BC", kEntries021_9, 2 },
	{ 274, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95\xe8\xa1\x9d\xe6\x92\x83\xe6\xb3\xa2", "0202B, 0202BC, 0202C, 214A_AddB, 214B, 214EX", kEntries021_17, 3 },
	{ 275, "", "\xe6\xae\x8b\xe5\x83\x8f", "0202A, 0202B, 0202BC, 0202C, J236B", nullptr, 0 },
	{ 276, "", "\xe6\xae\x8b\xe5\x83\x8f", "236A_JAddC, J236BC, J236EX", nullptr, 0 },
	{ 277, "", "\xe5\xbc\xb5\xe3\x82\x8a\xe4\xbb\x98\xe3\x81\x8d\xe6\xae\x8b\xe5\x83\x8f\x32\x32\x41", "0202A, 0202B", kEntries021_18, 5 },
	{ 278, "22dummy", "\xe6\xae\x8b\xe5\x83\x8f\xe2\x80\xbb\x50\x41\x54\xe7\x95\xaa\xe5\xa4\x89\xe6\x9b\xb4\xe4\xb8\x8d\xe5\x8f\xaf", "0202C", kEntries021_12, 7 },
	{ 279, "", "\x32\x33\x36\x41\x3e\x42\xe3\x81\xae\xe6\xae\x8b\xe5\x83\x8f\xef\xbc\x88\xe6\x8c\xaf\xe3\x82\x8a\xe5\x90\x91\xe3\x81\x8d\xe3\x81\x82\xe3\x82\x8a\xef\xbc\x89", "236A_JAddB", kEntries021_18, 5 },
	{ 281, "", "\xe7\x9e\xac\xe9\x96\x93\xe7\xa7\xbb\xe5\x8b\x95\xe9\x96\x8b\xe5\xa7\x8b", "0202A, 0202B, 0202BC, 0202C, 623A, 623B, 623BC, 623EX, J236B", kEntries021_17, 3 },
	{ 282, "", "\xe7\xa9\xba\xe4\xb8\xad\xe7\x9e\xac\xe9\x96\x93\xe7\xa7\xbb\xe5\x8b\x95\xe9\x96\x8b\xe5\xa7\x8b", "J0202A, J236A, J236B, J236BC, J236EX", kEntries021_17, 3 },
	{ 290, "", "\xe6\xae\x8b\xe5\x83\x8f", "J0202A, J236A, J236B, J236EX", nullptr, 0 },
	{ 291, "", "", "J236A, J236B", kEntries021_18, 5 },
	{ 293, "", "\xe7\xa9\xba\xe4\xb8\xad\xe8\xb6\xb3\xe5\xa0\xb4", "0202BC, 0202C, 236A_JAddC, J236BC, J236EX", kEntries021_19, 4 },
	{ 303, "", "\x32\x31\x34\x45\x58\xe6\x98\x9f\xe7\x94\x9f\xe6\x88\x90", "214EX_Hit", nullptr, 0 },
	{ 320, "", "\x41\x44\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "", kEntries021_20, 2 },
	{ 322, "", "\xe9\xab\x98\xe9\x80\x9f\x42\x47\xe3\x82\xb9\xe3\x82\xaf\xe3\x83\xad\xe3\x83\xbc\xe3\x83\xab", "", kEntries021_21, 1 },
	{ 334, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries021_22, 3 },
	{ 335, "", "\xe5\x9b\x9e\xe8\xbb\xa2\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "41236SP_Hit", kEntries021_9, 2 },
	{ 340, "", "\xe6\xae\x8b\xe5\x83\x8f", "41236SP", nullptr, 0 },
	{ 341, "", "\xe6\xae\x8b\xe5\x83\x8f", "J41236SP", nullptr, 0 },
	{ 343, "", "\xe7\xb8\xa6\xe4\xb8\x80\xe9\x96\x83", "", kEntries021_14, 2 },
	{ 344, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5", "", kEntries021_23, 5 },
	{ 346, "", "\x41\x44\xe7\x94\xa8\xe7\xa9\xba\xe4\xb8\xad\xe8\xb6\xb3\xe5\xa0\xb4", "ZanzouWallToEnemy", kEntries021_19, 4 },
	{ 347, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95\xe4\xb8\x80\xe9\x96\x83", "", kEntries021_7, 2 },
	{ 348, "", "\xe5\x88\x9d\xe5\x9b\x9e\xe4\xb8\x80\xe9\x96\x83\xef\xbc\x88\xe7\x9b\xb8\xe6\x89\x8b\xe4\xbd\x8d\xe7\xbd\xae\xe3\x81\xab\xe5\x87\xba\xe3\x82\x8b\xef\xbc\x89", "", kEntries021_24, 3 },
	{ 360, "", "\xe9\xab\x98\xe9\x80\x9f\xe3\x82\xb9\xe3\x82\xaf\xe3\x83\xad\xe3\x83\xbc\xe3\x83\xab\xe5\x9c\xb0\xe9\x9d\xa2", "LastArc_Hit", nullptr, 0 },
	{ 361, "", "\xe7\x9d\x80\xe6\xb0\xb4\xe3\x82\xa2\xe3\x83\x8b\xe3\x83\xa1", "LastArc_Hit", nullptr, 0 },
	{ 363, "", "\xe5\x88\x86\xe8\xba\xab\x31", "LastArc_Hit", nullptr, 0 },
	{ 364, "", "\xe5\x88\x86\xe8\xba\xab\x32", "LastArc_Hit", nullptr, 0 },
	{ 365, "", "\xe5\x88\x86\xe8\xba\xab\x33", "LastArc_Hit", nullptr, 0 },
	{ 366, "", "\xe5\x88\x86\xe8\xba\xab\x34", "LastArc_Hit", nullptr, 0 },
	{ 368, "", "\xe6\x89\x8b\xe5\x89\x8d\xe9\xa2\xa8", "LastArc_Hit", nullptr, 0 },
	{ 370, "", "\xe9\xab\x98\xe9\x80\x9f\x42\x47\x32", "", nullptr, 0 },
	{ 371, "", "\xe5\x87\xba\xe7\x8f\xbe\xe9\xa2\xa8", "", nullptr, 0 },
	{ 374, "", "\xe3\x80\x80\xe6\x89\x8b\xe5\x89\x8d", "", nullptr, 0 },
	{ 375, "", "\xe3\x80\x80\xe5\xbe\x8c\xe5\x8d\x8a\xe6\x89\x8b\xe5\x89\x8d\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries021_9, 2 },
	{ 376, "", "\xe3\x81\x9f\xe3\x81\x8f\xe3\x81\x95\xe3\x82\x93\xe8\xa3\x8f", "", kEntries021_25, 1 },
	{ 378, "", "\xef\xbc\x98\xe4\xba\xba\xe7\x99\xbb\xe5\xa0\xb4\xe6\x89\x8b\xe5\x89\x8d\xe9\xa2\xa8", "", nullptr, 0 },
	{ 380, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe3\x83\x80\xe3\x83\x9f\xe3\x83\xbc", "", nullptr, 0 },
	{ 381, "", "\xe3\x83\x95\xe3\x82\xa3\xe3\x83\x8b\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe3\x81\x97\xe3\x81\xbe\xe3\x81\x86", "", kEntries021_14, 2 },
	{ 382, "", "\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88", "", kEntries021_25, 1 },
	{ 383, "", "\xe9\xab\x98\xe9\x80\x9f\x42\x47\xe3\x82\x92\xe6\xb6\x88\xe3\x81\x99", "", nullptr, 0 },
	{ 940, "", "", "", kEntries021_14, 2 },
	{ 941, "", "", "", kEntries021_14, 2 },
	{ 942, "", "", "", kEntries021_14, 2 },
	{ 943, "", "", "", nullptr, 0 },
	{ 944, "", "", "", kEntries021_14, 2 },
	{ 945, "", "", "", nullptr, 0 },
};

const unsigned char kEntries022_0[] = { 247, 248, 249 };
const unsigned char kEntries022_1[] = { 248, 249 };
const unsigned char kEntries022_2[] = { 248, 249, 250 };
const unsigned char kEntries022_3[] = { 49, 81, 97, 99, 161, 225, 231 };
const unsigned char kEntries022_4[] = { 247 };
const unsigned char kEntries022_5[] = { 247, 248 };
const unsigned char kEntries022_6[] = { 247, 248, 249, 250, 252 };
const unsigned char kEntries022_7[] = { 247, 248, 249, 250 };
const unsigned char kEntries022_8[] = { 17, 97, 99, 104, 161, 193, 225, 227, 231, 233 };
const unsigned char kEntries022_9[] = { 49, 97, 99, 161, 193, 225 };
const unsigned char kEntries022_10[] = { 247, 248, 249, 250, 254 };
const unsigned char kEntries022_11[] = { 247, 248, 249, 254 };
const unsigned char kEntries022_12[] = { 246, 247, 248, 249 };
const unsigned char kEntries022_13[] = { 248 };
const unsigned char kEntries022_14[] = { 246, 247, 248 };
const unsigned char kEntries022_15[] = { 247, 249, 254 };
const unsigned char kEntries022_16[] = { 249 };
const unsigned char kEntries022_17[] = { 245, 247, 248, 249 };
const unsigned char kEntries022_18[] = { 246, 248, 249, 251 };
const unsigned char kEntries022_19[] = { 247, 249 };
const unsigned char kEntries022_20[] = { 246, 247, 248, 249, 252 };

const Row kEffects022[] = {
	{ 101, "", "A", "", kEntries022_0, 3 },
	{ 102, "", "B", "", kEntries022_0, 3 },
	{ 103, "", "C1", "", kEntries022_1, 2 },
	{ 104, "", "2A", "", kEntries022_0, 3 },
	{ 105, "", "2B", "", kEntries022_0, 3 },
	{ 106, "", "2C", "", kEntries022_2, 3 },
	{ 107, "", "JA", "", kEntries022_0, 3 },
	{ 109, "", "JC", "AirC_End, AirSC", kEntries022_2, 3 },
	{ 110, "", "J[C]", "", kEntries022_2, 3 },
	{ 111, "", "\x36\x32\x33\x45\x58\xe5\x8f\xa9\xe3\x81\x8d\xe3\x81\xa4\xe3\x81\x91", "", kEntries022_2, 3 },
	{ 112, "", "\xe7\xa9\xba\xe4\xb8\xad\xe6\x8a\x95\xe3\x81\x92", "", kEntries022_0, 3 },
	{ 113, "", "\xe5\x9c\xb0\xe4\xb8\x8a\xe6\x8a\x95\xe3\x81\x92\xe6\xae\x8b\xe5\x83\x8f", "", kEntries022_3, 7 },
	{ 114, "", "\xe9\x80\x9a\xe5\xb8\xb8\xe6\x8a\x95\xe3\x81\x92\xe6\x88\x90\xe7\xab\x8b", "", kEntries022_4, 1 },
	{ 116, "", "3C", "214EX, StdSC", kEntries022_2, 3 },
	{ 117, "", "\xe9\x80\xa3\xe6\x89\x93\xe3\x82\xb3\xe3\x83\xb3\xe3\x83\x9c\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "RapidRelayAtk", kEntries022_5, 2 },
	{ 120, "", "C2", "", kEntries022_2, 3 },
	{ 123, "", "\x52\x42\xe7\x99\xbe\xe5\x88\x97\xe6\x89\x8b", "RapidRelayAtk", kEntries022_5, 2 },
	{ 175, "", "4C", "4C", kEntries022_2, 3 },
	{ 176, "", "4C", "4C", kEntries022_2, 3 },
	{ 235, "", "\xe9\x80\x9a\xe5\xb8\xb8\xe5\x8f\xa9\xe3\x81\x8d\xe3\x81\xa4\xe3\x81\x91", "214A_Add_Add_Hit", kEntries022_0, 3 },
	{ 238, "", "\xe3\x82\xad\xe3\x83\xa3\xe3\x83\x83\xe3\x83\x81", "214A_Add_Add_Hit", kEntries022_6, 5 },
	{ 240, "", "\x32\x31\x34\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\x31", "214A, 214BC, 214EX", kEntries022_2, 3 },
	{ 241, "", "\x32\x31\x34\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89\x32", "214A_Add, 214B, 214BC, 214EX", kEntries022_7, 4 },
	{ 242, "", "\xe6\xae\x8b\xe5\x83\x8f\xe3\x83\x96\xe3\x83\xa9\xe3\x83\xbc", "214A_Add_Add, 214A_Add_Add6, 214EX", kEntries022_8, 10 },
	{ 243, "", "\x32\x31\x34\x41\x41\x41\xe3\x82\xa6\xe3\x82\xba", "214A_Add_Add", kEntries022_7, 4 },
	{ 244, "", "\xe6\xae\x8b\xe5\x83\x8f\xe3\x83\x96\xe3\x83\xa9\xe3\x83\xbc", "214A_Add_Add4", kEntries022_9, 6 },
	{ 245, "", "", "214A_Add_Add4", kEntries022_0, 3 },
	{ 246, "", "", "214A_Add_Add6, 214A_Add_Add_Hit", kEntries022_0, 3 },
	{ 247, "", "\xe5\x8f\xa9\xe3\x81\x8d\xe3\x81\xa4\xe3\x81\x91", "214A_Add_Add_Hit", kEntries022_10, 5 },
	{ 259, "", "\x45\x58\xe6\x98\x87\xe7\xab\x9c", "623EX", kEntries022_0, 3 },
	{ 260, "", "\xe6\x98\x87\xe7\xab\x9c", "623A, 623B, 623BC", kEntries022_0, 3 },
	{ 261, "", "\xe6\x98\x87\xe7\xab\x9c\xe5\x9c\xb0\xe9\x9d\xa2", "623A, 623B, 623BC, 623EX", kEntries022_11, 4 },
	{ 302, "", "\x32\x31\x34\x45\x58\xe6\xae\x8b\xe5\x83\x8f\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\x31", "J214EX_Hit", kEntries022_4, 1 },
	{ 312, "", "\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xe6\xae\x8b\xe5\x83\x8f", "J214A, J214B, J214BC", kEntries022_3, 7 },
	{ 313, "", "\x32\x31\x34\x45\x58\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J214EX", kEntries022_12, 4 },
	{ 314, "", "\x32\x31\x34\x45\x58\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J214EX_Hit", kEntries022_12, 4 },
	{ 315, "", "\x32\x31\x34\x45\x58\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J214EX_Hit", kEntries022_12, 4 },
	{ 316, "", "\x32\x31\x34\x45\x58\xe3\x83\x91\xe3\x83\xb3\xe3\x83\x81\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J214EX_Hit", kEntries022_12, 4 },
	{ 318, "", "\xe9\x80\x9a\xe3\x82\x8a\xe9\x81\x8e\xe3\x81\x8e\xe3\x82\x8b\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J214EX_Hit", kEntries022_0, 3 },
	{ 319, "", "\xe9\x80\x9a\xe3\x82\x8a\xe9\x81\x8e\xe3\x81\x8e\xe3\x82\x8b\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J214EX_Hit", kEntries022_0, 3 },
	{ 320, "", "\xe9\x80\x9a\xe3\x82\x8a\xe9\x81\x8e\xe3\x81\x8e\xe3\x82\x8b\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "J214EX_Hit", kEntries022_0, 3 },
	{ 330, "", "\xe8\xb6\xb3\xe5\x85\x83\xe3\x83\x93\xe3\x83\xaa\xe3\x83\x93\xe3\x83\xaa", "0202EX", kEntries022_4, 1 },
	{ 331, "", "\xe3\x82\xb9\xe3\x83\x91\xe3\x83\xbc\xe3\x82\xaf", "0202EX", kEntries022_13, 1 },
	{ 360, "", "\xe3\x83\x80\xe3\x83\x83\xe3\x82\xb7\xe3\x83\xa5\xe7\xba\x8f\xe3\x81\x86\xe7\x82\x8e", "41236SP", kEntries022_1, 2 },
	{ 366, "", "\xe3\x82\xad\xe3\x83\xa3\xe3\x83\x83\xe3\x83\x81\xe6\x88\x90\xe5\x8a\x9f", "41236SP_Hit", kEntries022_6, 5 },
	{ 368, "", "\xe5\x85\x89\xe3\x81\xae\xe7\x8e\x89", "41236SP_Hit", kEntries022_14, 3 },
	{ 369, "", "\xe3\x81\xa4\xe3\x81\x8b\xe3\x81\xbf\xe4\xb8\xad\xe9\xbb\x92\xe3\x81\x84\xe7\x82\x8e", "41236SP_Hit", kEntries022_15, 3 },
	{ 371, "", "\xe3\x81\xa4\xe3\x81\x8b\xe3\x81\xbf\xe9\x96\x8b\xe5\xa7\x8b\x42\x47", "41236SP_Hit", kEntries022_16, 1 },
	{ 372, "", "\xe3\x81\xa4\xe3\x81\x8b\xe3\x81\xbf\xe4\xb8\xad\x42\x47", "", kEntries022_16, 1 },
	{ 374, "", "\xe7\x82\x8e\xe4\xb8\x8a", "41236SP_Hit", kEntries022_17, 4 },
	{ 375, "", "\xe7\x82\x8e\xe4\xb8\x8a\xe6\x89\x8b\xe5\x85\x83", "41236SP_Hit", kEntries022_18, 4 },
	{ 376, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\x31", "", kEntries022_11, 4 },
	{ 377, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\x32", "", kEntries022_11, 4 },
	{ 380, "", "\xe5\x8f\xa9\xe3\x81\x8d\xe3\x81\xa4\xe3\x81\x91\xe3\x83\x96\xe3\x83\xac\xe3\x83\xbc\xe3\x83\x89", "", kEntries022_1, 2 },
	{ 381, "", "\xe5\x9c\xb0\xe9\x9d\xa2\xe7\xa0\xb4\xe5\xa3\x8a", "", nullptr, 0 },
	{ 382, "", "\xe6\xb8\xa9\xe6\xb3\x89", "", kEntries022_0, 3 },
	{ 385, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "", kEntries022_0, 3 },
	{ 386, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "", kEntries022_0, 3 },
	{ 416, "", "\xe7\xaa\x81\xe9\x80\xb2\xe5\x8b\x95\xe3\x81\x8d", "", nullptr, 0 },
	{ 417, "", "\xe3\x80\x80\xe3\x81\xa8\xe3\x81\xa3\xe3\x81\x97\xe3\x82\x93\xe3\x83\x80\xe3\x83\x9f\xe3\x83\xbc", "", kEntries022_4, 1 },
	{ 418, "", "\xe3\x80\x80\xe7\xaa\x81\xe9\x80\xb2\xe7\xa7\xbb\xe5\x8b\x95\xe3\x82\xa8\xe3\x83\x95\xe3\x82\xa7\xe3\x82\xaf\xe3\x83\x88", "", kEntries022_19, 2 },
	{ 420, "", "\xe7\x82\x8e\xe3\x81\xbe\xe3\x81\xa8\xe3\x81\x84\xe3\x82\xa2\xe3\x83\x8b\xe3\x83\xa1", "LastArc_Hit", kEntries022_0, 3 },
	{ 422, "", "\x42\x47\xe3\x82\xa2\xe3\x83\x8b\xe3\x83\xa1", "", nullptr, 0 },
	{ 423, "", "\x42\x47\xe3\x82\xa2\xe3\x83\x8b\xe3\x83\xa1\xe3\x83\xab\xe3\x83\xbc\xe3\x83\x97\xe3\x81\xae\xe3\x81\xbf", "", nullptr, 0 },
	{ 426, "", "\xe9\xbb\x92\xe6\x9d\xbf", "", nullptr, 0 },
	{ 428, "", "\xe9\xab\x98\xe9\x80\x9f\xe7\xa7\xbb\xe5\x8b\x95\xe5\x8b\x95\xe3\x81\x8d", "", kEntries022_20, 5 },
	{ 430, "", "\xe2\x98\x85\xe4\xb9\xb1\xe8\x88\x9e\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "", nullptr, 0 },
	{ 431, "", "\xe2\x98\x85\x31\x32\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "", nullptr, 0 },
	{ 432, "", "\xef\xbc\xa0\xe4\xb8\x80\xe6\x96\x89\xe3\x81\xab\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0", "", kEntries022_1, 2 },
	{ 434, "", "\xe3\x80\x80\xe6\x89\x8b\xe5\x89\x8d", "", kEntries022_4, 1 },
	{ 435, "", "\xe3\x80\x80\xe5\xa5\xa5", "", kEntries022_4, 1 },
	{ 436, "", "\xe3\x80\x80\xe5\xa5\xa5", "", kEntries022_4, 1 },
	{ 437, "", "\xe3\x80\x80\xe5\xa5\xa5", "", nullptr, 0 },
	{ 438, "", "\xe3\x80\x80\xe5\xa5\xa5", "", kEntries022_4, 1 },
	{ 439, "", "\xe3\x80\x80\xe5\xa5\xa5", "", kEntries022_4, 1 },
	{ 440, "", "\xe3\x80\x80\xe5\xa5\xa5", "", kEntries022_4, 1 },
	{ 444, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88\xe6\x99\x82\xe8\x83\x8c\xe6\x99\xaf", "", kEntries022_1, 2 },
	{ 445, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe3\x83\x92\xe3\x83\x83\xe3\x83\x88", "", kEntries022_1, 2 },
	{ 446, "", "\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe5\x90\x8c\xe6\x99\x82\xe7\x99\xba\xe5\xb0\x84", "", kEntries022_19, 2 },
	{ 450, "", "\x46\xe3\x83\x93\xe3\x83\xbc\xe3\x83\xa0\xe4\xba\xa4\xe5\xb7\xae\xe3\x83\x9b\xe3\x83\xaf\xe3\x82\xa4\xe3\x83\x88\x34\x30\x46", "", kEntries022_5, 2 },
	{ 451, "", "\xe3\x80\x80\xe5\x85\x89\xe3\x81\xab\xe6\xba\xb6\xe3\x81\x91\xe3\x82\x8b", "", nullptr, 0 },
};

struct Chara
{
	int number;
	const Row* effects;
	int count;
};

const Chara kCharas[] = {
	{ 0, kEffects000, 55 },
	{ 1, kEffects001, 46 },
	{ 2, kEffects002, 47 },
	{ 3, kEffects003, 32 },
	{ 4, kEffects004, 47 },
	{ 5, kEffects005, 45 },
	{ 6, kEffects006, 58 },
	{ 8, kEffects008, 58 },
	{ 9, kEffects009, 54 },
	{ 10, kEffects010, 48 },
	{ 11, kEffects011, 47 },
	{ 12, kEffects012, 31 },
	{ 13, kEffects013, 52 },
	{ 14, kEffects014, 40 },
	{ 15, kEffects015, 48 },
	{ 16, kEffects016, 78 },
	{ 17, kEffects017, 57 },
	{ 18, kEffects018, 7 },
	{ 19, kEffects019, 115 },
	{ 20, kEffects020, 70 },
	{ 21, kEffects021, 89 },
	{ 22, kEffects022, 81 },
};

const Chara* FindChara(int chara)
{
	for (const Chara& entry : kCharas)
	{
		if (entry.number == chara)
			return &entry;
	}

	return nullptr;
}

}

int EffectTable::GetCount(int chara)
{
	const Chara* const found = FindChara(chara);
	return found != nullptr ? found->count : 0;
}

bool EffectTable::Get(int chara, int index, Effect& out)
{
	const Chara* const found = FindChara(chara);

	if (found == nullptr || index < 0 || index >= found->count)
		return false;

	const Row& row = found->effects[index];
	out = { row.pattern, row.code, row.name, row.spawnedBy, row.entries, row.count };
	return true;
}

int EffectTable::CountUsing(int chara, int entry)
{
	const Chara* const found = FindChara(chara);

	if (found == nullptr)
		return 0;

	int count = 0;

	for (int i = 0; i < found->count; ++i)
	{
		const Row& row = found->effects[i];

		for (int k = 0; k < row.count; ++k)
		{
			if (row.entries[k] != entry)
				continue;

			++count;
			break;
		}
	}

	return count;
}
