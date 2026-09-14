#include "Palette/ColorPartTable.h"

namespace {

struct Part
{
	unsigned char chara;
	unsigned char sub;
	unsigned char part;
	unsigned char count;
	const char* name;
	const unsigned char* entries;
};

const unsigned char kChr000P0Part1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 80, 81, 82, 83, 84, 85, 118, 119, 120, 121, 122, 123, 124, 125, 134, 135, 136, 137, 138, 139, 140, 141 };
const unsigned char kChr000P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr000P0Part3[] = { 112, 113, 114, 115, 116 };
const unsigned char kChr000P0Part4[] = { 11, 12, 13, 55, 56, 57, 58, 59, 60, 61 };
const unsigned char kChr000P0Part5[] = { 176, 177, 178, 179, 180, 192, 193, 194, 195, 196, 208, 209, 210, 211, 212 };
const unsigned char kChr001P0Part1[] = { 1, 2, 3, 4, 5, 6, 73, 74, 75, 76, 77, 192, 193, 194, 195, 196, 197 };
const unsigned char kChr001P0Part2[] = { 16, 17, 18, 19, 20, 22, 32, 33, 34, 35 };
const unsigned char kChr001P0Part3[] = { 128, 129, 130, 131, 132, 134, 135, 136, 137, 138, 144, 145, 146, 147, 148 };
const unsigned char kChr001P0Part4[] = { 25, 26, 27, 28, 29, 41, 42, 43, 44, 45 };
const unsigned char kChr001P0Part5[] = { 11, 12, 13, 64, 65, 66, 67, 68, 96, 97, 98, 99, 100, 221, 222, 223, 224, 225, 226, 227, 228, 230, 231, 232, 233, 234, 205, 206, 207 };
const unsigned char kChr002P0Part1[] = { 1, 2, 3, 4, 5, 6, 160, 161, 162, 163, 164, 166, 167, 168, 169, 170, 176, 177, 178, 179, 180 };
const unsigned char kChr002P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr002P0Part3[] = { 128, 129, 130, 131, 132, 134, 135, 136, 137, 138 };
const unsigned char kChr002P0Part4[] = { 70, 71, 72, 73, 74 };
const unsigned char kChr002P0Part5[] = { 11, 12, 13, 15, 24, 25, 26, 27, 28 };
const unsigned char kChr003P0Part1[] = { 1, 2, 3, 4, 5, 6, 55, 56, 57, 58, 59, 71, 72, 73, 74, 75 };
const unsigned char kChr003P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr003P0Part3[] = { 48, 49, 50, 51, 52, 128, 129, 130, 131, 132, 135, 136, 137, 138, 139, 144, 145, 146, 147, 148 };
const unsigned char kChr003P0Part4[] = { 80, 81, 82, 83, 84, 87, 88, 89, 90, 91 };
const unsigned char kChr003P0Part5[] = { 176, 177, 178, 179, 180, 183, 184, 185, 186, 187, 208, 209, 210, 211, 212, 215, 216, 217, 218, 219, 231, 232, 233, 234, 235, 11, 12, 13, 15, 27, 28, 29, 31 };
const unsigned char kChr004P0Part1[] = { 1, 2, 3, 4, 5, 6, 25, 26, 27, 28, 29 };
const unsigned char kChr004P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr004P0Part3[] = { 64, 65, 66, 67, 68, 208, 209, 210, 211, 212 };
const unsigned char kChr004P0Part4[] = { 86, 87, 88, 89, 90, 96, 97, 98, 99, 100, 102, 103, 104, 105, 106, 160, 161, 162, 163, 164, 192, 193, 194, 195, 196, 198, 199, 200, 201, 202, 214, 215, 216, 217, 218, 240, 241, 242, 243, 244, 249 };
const unsigned char kChr004P0Part5[] = { 11, 12, 13, 48, 49, 50, 51, 52, 128, 129, 130, 131, 132, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 203, 204, 205, 206, 207, 220, 221, 222, 230, 231, 232, 233, 234 };
const unsigned char kChr005P0Part1[] = { 1, 2, 3, 4, 5, 6, 86, 87, 88, 89, 90, 91, 102, 103, 104, 105, 106, 107 };
const unsigned char kChr005P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr005P0Part3[] = { 166, 167, 169, 170, 171, 172, 173 };
const unsigned char kChr005P0Part4[] = { 70, 71, 72, 73, 74 };
const unsigned char kChr005P0Part5[] = { 11, 12, 13, 15, 144, 145, 146, 147, 148, 150, 151, 153, 154, 155, 156, 157, 192, 193, 194, 195, 196, 198, 199, 201, 202, 203, 204, 205 };
const unsigned char kChr006P0Part1[] = { 1, 2, 3, 4, 5, 6, 86, 87, 88, 89, 90, 91, 102, 103, 104, 105, 106, 107 };
const unsigned char kChr006P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr006P0Part3[] = { 48, 49, 50, 51, 52, 144, 145, 146, 147, 148 };
const unsigned char kChr006P0Part4[] = { 7, 8, 9, 11, 12, 13, 15, 96, 97, 98, 99, 100, 112, 113, 114, 115, 116, 118, 119, 120, 121, 122, 176, 177, 178, 179, 180 };
const unsigned char kChr006P0Part5[] = { 54, 55, 56, 57, 58, 70, 71, 72, 73, 74, 75, 80, 81, 82, 83, 84, 208, 209, 210, 211, 212, 214, 215, 216, 217, 218, 224, 225, 226, 227, 228 };
const unsigned char kChr008P0Part1[] = { 1, 2, 3, 4, 5, 6, 112, 113, 114, 115, 116, 117 };
const unsigned char kChr008P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr008P0Part3[] = { 60, 61, 62, 63, 64, 65, 66, 67, 68 };
const unsigned char kChr008P0Part4[] = { 11, 12, 13, 80, 81, 82, 83, 84, 160, 161, 162, 163, 164, 166, 167, 168, 169, 170, 172, 173, 174, 175, 176, 177, 178, 179, 180, 182, 183, 184, 185, 186 };
const unsigned char kChr008P0Part5[] = { 70, 71, 72, 73, 74, 128, 129, 130, 131, 132, 134, 135, 136, 137, 138, 140 };
const unsigned char kChr009P0Part1[] = { 1, 2, 3, 4, 5, 6, 86, 87, 88, 89, 90, 166, 167, 169, 170, 171, 172, 173 };
const unsigned char kChr009P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr009P0Part3[] = { 112, 113, 114, 115, 116 };
const unsigned char kChr009P0Part4[] = { 54, 55, 56, 57, 58, 80, 81, 82, 83, 84, 160, 161, 162, 163, 164 };
const unsigned char kChr009P0Part5[] = { 64, 65, 66, 67, 68, 128, 129, 130, 131, 132 };
const unsigned char kChr010P0Part1[] = { 1, 2, 3, 4, 5, 6, 96, 97, 98, 99, 100, 101, 103, 104, 105, 106, 107, 108, 134, 135, 136, 137, 138 };
const unsigned char kChr010P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr010P0Part3[] = { 128, 129, 130, 131, 132 };
const unsigned char kChr010P0Part4[] = { 11, 12, 13, 55, 56, 57, 58, 59, 60, 61 };
const unsigned char kChr010P0Part5[] = { 150, 151, 152, 153, 154, 179, 180, 182, 183, 184, 185, 186 };
const unsigned char kChr011P0Part1[] = { 80, 81, 82, 83, 84, 1, 2, 3, 4, 5, 6, 112, 113, 114, 115, 116, 117, 214, 215, 216, 217, 218 };
const unsigned char kChr011P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr011P0Part3[] = { 64, 65, 66, 67, 68, 208, 209, 210, 211, 212 };
const unsigned char kChr011P0Part4[] = { 54, 55, 56, 57, 58, 11, 12, 13, 70, 71, 72, 73, 74, 92, 93, 94, 95 };
const unsigned char kChr011P0Part5[] = { 128, 129, 130, 131, 132, 134, 135, 136, 137, 138, 140, 160, 161, 162, 163, 164, 166, 167, 168, 169, 170, 198, 199, 200, 201, 202, 76, 77, 78, 79, 92, 93, 94, 95 };
const unsigned char kChr011P1Part1[] = { 1, 2, 3, 4, 5, 6, 80, 81, 82, 83, 84, 85, 112, 113, 114, 115, 116, 117 };
const unsigned char kChr011P1Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr011P1Part3[] = { 64, 65, 66, 67, 68, 72, 73, 74, 75, 76, 96, 97, 98, 99, 100, 101, 102 };
const unsigned char kChr011P1Part4[] = { 11, 12, 13, 88, 89, 90, 91, 92, 93, 94 };
const unsigned char kChr011P1Part5[] = { 160, 161, 162, 163, 164, 165, 166, 169, 170, 171, 172, 173, 174, 175, 198, 199, 200, 201, 202, 208, 209, 210, 211, 212, 214, 215, 216, 217, 218, 219, 220, 224, 225, 226, 227, 228, 230, 231, 232, 233, 234, 235, 236, 238, 240, 241, 242, 243, 244 };
const unsigned char kChr012P0Part1[] = { 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 15, 24, 25, 26, 27, 28 };
const unsigned char kChr012P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr012P0Part3[] = { 176, 177, 178, 179, 180, 204, 205, 206, 207 };
const unsigned char kChr012P0Part4[] = { 64, 65, 66, 67, 68, 70, 72, 73, 74, 75, 76, 128, 129, 130, 131, 132, 144, 145, 146, 147, 148, 139, 140, 141, 142, 143 };
const unsigned char kChr012P0Part5[] = { 112, 113, 114, 115, 116, 157, 158, 159, 171, 172, 173, 174, 175 };
const unsigned char kChr013P0Part1[] = { 1, 2, 3, 4, 5, 6, 144, 145, 146, 147, 148, 149, 160, 161, 162, 163, 164, 165, 176, 177, 178, 179, 180, 181 };
const unsigned char kChr013P0Part2[] = { 16, 17, 18, 19, 20 };
const unsigned char kChr013P0Part3[] = { 86, 87, 88, 89, 90, 102, 103, 104, 105, 106, 112, 113, 114, 115, 116 };
const unsigned char kChr013P0Part4[] = { 64, 65, 66, 67, 68, 118, 119, 120, 121, 122, 208, 209, 210, 211, 212, 214, 215, 216, 217, 218, 224, 225, 226, 227, 228 };
const unsigned char kChr013P0Part5[] = { 11, 12, 13, 15, 24, 25, 26, 27, 28, 152, 153, 154, 155, 156, 168, 169, 170, 171, 172, 184, 185, 186, 187, 188 };
const unsigned char kChr014P0Part1[] = { 1, 2, 3, 4, 5, 6, 128, 129, 130, 131, 132 };
const unsigned char kChr014P0Part2[] = { 16, 17, 18, 19, 20, 22, 11, 12, 13, 15 };
const unsigned char kChr014P0Part3[] = { 80, 81, 82, 83, 84 };
const unsigned char kChr014P0Part4[] = { 54, 55, 56, 57, 58, 70, 71, 72, 73, 74, 86, 87, 88, 89, 90, 160, 161, 162, 163, 164, 167, 168, 169, 170, 171, 172 };
const unsigned char kChr014P0Part5[] = { 24, 25, 26, 27, 28, 62, 63, 78, 79 };
const unsigned char kChr015P0Part1[] = { 16, 17, 18, 19, 20, 22, 11, 12, 13, 15 };
const unsigned char kChr015P0Part2[] = { 64, 65, 66, 67, 68 };
const unsigned char kChr015P0Part3[] = { 112, 113, 114, 115, 116 };
const unsigned char kChr015P0Part4[] = { 128, 129, 130, 131, 132 };
const unsigned char kChr015P0Part5[] = { 118, 119, 120, 121, 122, 144, 145, 146, 147, 148, 208, 209, 210, 211, 212, 213, 215, 216, 218, 219, 220, 221, 222, 224, 225, 226, 227, 228 };
const unsigned char kChr016P0Part1[] = { 1, 2, 3, 4, 5, 6, 80, 81, 82, 83, 84, 85, 112, 113, 114, 115, 116, 117 };
const unsigned char kChr016P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr016P0Part3[] = { 64, 65, 66, 67, 68, 72, 73, 74, 75, 76, 96, 97, 98, 99, 100, 101, 102 };
const unsigned char kChr016P0Part4[] = { 11, 12, 13, 88, 89, 90, 91, 92, 93, 94 };
const unsigned char kChr016P0Part5[] = { 56, 57, 58, 59, 60, 160, 161, 162, 163, 164, 165, 166, 169, 170, 171, 172, 173, 174, 175, 198, 199, 200, 201, 202, 238, 240, 241, 242, 243, 244 };
const unsigned char kChr017P0Part1[] = { 48, 49, 50, 51, 52, 53 };
const unsigned char kChr017P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr017P0Part3[] = { 96, 97, 98, 99, 100, 134, 135, 136, 137, 138 };
const unsigned char kChr017P0Part4[] = { 11, 12, 13, 86, 87, 88, 89, 90, 91, 128, 129, 130, 131, 132, 144, 145, 146, 147, 148, 176, 177, 178, 179, 180, 182, 183, 184, 185, 186, 187, 188, 189, 190 };
const unsigned char kChr017P0Part5[] = { 102, 103, 104, 105, 106, 107 };
const unsigned char kChr017P1Part1[] = { 54, 55, 56, 57, 58, 70, 71, 72, 73, 74, 86, 87, 88, 89, 90 };
const unsigned char kChr017P1Part2[] = { 8, 9, 22, 23, 25, 26, 27, 28, 29, 96, 97, 98, 99, 100 };
const unsigned char kChr017P1Part3[] = { 60, 61, 62, 63, 64, 65, 66, 67, 68 };
const unsigned char kChr017P1Part4[] = { 11, 12, 13, 80, 81, 82, 83, 84, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 200, 201, 202, 203, 204, 205, 206 };
const unsigned char kChr017P1Part5[] = { 128, 129, 130, 131, 132, 133, 144, 145, 146, 147, 148, 150, 151, 152, 153, 154 };
const unsigned char kChr017P2Part1[] = { 1, 2, 3, 4, 5, 6, 112, 113, 114, 115, 116, 117 };
const unsigned char kChr017P2Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr017P2Part3[] = { 60, 61, 62, 63, 64, 65, 66, 67, 68 };
const unsigned char kChr017P2Part4[] = { 11, 12, 13, 80, 81, 82, 83, 84, 160, 161, 162, 163, 164, 166, 167, 168, 169, 170, 172, 173, 174, 175, 176, 177, 178, 179, 180, 182, 183, 184, 185, 186 };
const unsigned char kChr017P2Part5[] = { 70, 71, 72, 73, 74, 128, 129, 130, 131, 132, 134, 135, 136, 137, 138, 140 };
const unsigned char kChr018P0Part1[] = { 54, 55, 56, 57, 58, 70, 71, 72, 73, 74, 86, 87, 88, 89, 90 };
const unsigned char kChr018P0Part2[] = { 8, 9, 22, 23, 25, 26, 27, 28, 29, 96, 97, 98, 99, 100 };
const unsigned char kChr018P0Part3[] = { 60, 61, 62, 63, 64, 65, 66, 67, 68 };
const unsigned char kChr018P0Part4[] = { 11, 12, 13, 80, 81, 82, 83, 84, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 200, 201, 202, 203, 204, 205, 206, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220 };
const unsigned char kChr018P0Part5[] = { 128, 129, 130, 131, 132, 133, 144, 145, 146, 147, 148, 150, 151, 152, 153, 154 };
const unsigned char kChr018P1Part1[] = { 54, 55, 56, 57, 58, 70, 71, 72, 73, 74, 86, 87, 88, 89, 90 };
const unsigned char kChr018P1Part2[] = { 8, 9, 22, 23, 25, 26, 27, 28, 29, 96, 97, 98, 99, 100 };
const unsigned char kChr018P1Part3[] = { 60, 61, 62, 63, 64, 65, 66, 67, 68 };
const unsigned char kChr018P1Part4[] = { 11, 12, 13, 80, 81, 82, 83, 84, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 200, 201, 202, 203, 204, 205, 206, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220 };
const unsigned char kChr018P1Part5[] = { 128, 129, 130, 131, 132, 133, 144, 145, 146, 147, 148, 150, 151, 152, 153, 154 };
const unsigned char kChr018P2Part1[] = { 1, 2, 3, 4, 5, 6, 112, 113, 114, 115, 116, 117 };
const unsigned char kChr018P2Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr018P2Part3[] = { 60, 61, 62, 63, 64, 65, 66, 67, 68 };
const unsigned char kChr018P2Part4[] = { 11, 12, 13, 80, 81, 82, 83, 84, 160, 161, 162, 163, 164, 166, 167, 168, 169, 170, 172, 173, 174, 175, 176, 177, 178, 179, 180, 182, 183, 184, 185, 186 };
const unsigned char kChr018P2Part5[] = { 70, 71, 72, 73, 74, 128, 129, 130, 131, 132, 134, 135, 136, 137, 138, 140 };
const unsigned char kChr019P0Part1[] = { 1, 2, 3, 4, 5, 6, 80, 81, 82, 83, 84, 85, 121, 122, 123, 124, 125 };
const unsigned char kChr019P0Part2[] = { 16, 17, 18, 19, 20, 22, 32, 33, 34, 35, 36, 128, 129, 130, 131, 132 };
const unsigned char kChr019P0Part3[] = { 112, 113, 114, 115, 116 };
const unsigned char kChr019P0Part4[] = { 7, 8, 9, 10, 11, 12, 13, 15, 63, 55, 56, 58, 59, 60, 61, 89, 90, 91, 92, 93 };
const unsigned char kChr019P0Part5[] = { 192, 193, 194, 195, 196, 95, 169, 170, 171, 172, 173, 185, 186, 187, 188, 189, 201, 202, 203, 204, 205 };
const unsigned char kChr019P1Part1[] = { 1, 2, 3, 4, 5, 6, 80, 81, 82, 83, 84, 85, 160, 161, 163, 164, 165, 166, 167 };
const unsigned char kChr019P1Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr019P1Part3[] = { 112, 113, 114, 115, 116 };
const unsigned char kChr019P1Part4[] = { 7, 8, 9, 10, 11, 12, 13, 15, 57, 58, 59, 60, 61, 105, 106, 107, 108, 109 };
const unsigned char kChr019P1Part5[] = { 192, 193, 195, 196, 197, 198, 199, 201, 202, 203, 204, 205, 89, 90, 91, 92, 93 };
const unsigned char kChr020P0Part1[] = { 1, 2, 3, 4, 5, 6, 96, 97, 98, 99, 100, 128, 129, 130, 131, 132, 144, 145, 146, 147, 148, 150, 151, 152, 153, 154 };
const unsigned char kChr020P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr020P0Part3[] = { 134, 135, 137, 138, 139, 140, 141, 232, 233, 235, 236, 237, 238, 239, 192, 193, 194, 195, 196, 227, 228, 203, 204, 205, 206, 207 };
const unsigned char kChr020P0Part4[] = { 7, 8, 9, 11, 12, 13, 15, 56, 57, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 70, 71, 72, 73, 74 };
const unsigned char kChr020P0Part5[] = { 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 86, 87, 88, 89, 90, 199, 200, 201, 224, 225 };
const unsigned char kChr021P0Part1[] = { 1, 2, 3, 4, 5, 6, 64, 65, 66, 67, 68, 86, 87, 88, 89, 90, 91, 176, 177, 178, 179, 180 };
const unsigned char kChr021P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr021P0Part3[] = { 54, 55, 56, 57, 58, 192, 193, 194, 195, 196, 235, 236, 237, 238, 239 };
const unsigned char kChr021P0Part4[] = { 7, 8, 9, 11, 12, 13, 15, 48, 49, 50, 51, 52, 112, 113, 114, 115, 116, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132 };
const unsigned char kChr021P0Part5[] = { 24, 25, 26, 27, 28, 160, 161, 162, 163, 164, 224, 225, 226, 227, 228, 230, 231, 232, 233, 234 };
const unsigned char kChr022P0Part1[] = { 1, 2, 3, 4, 5, 6, 48, 49, 50, 51, 52, 144, 145, 146, 147, 148, 192, 193, 194, 195, 196, 197 };
const unsigned char kChr022P0Part2[] = { 16, 17, 18, 19, 20, 22 };
const unsigned char kChr022P0Part3[] = { 7, 8, 9, 11, 12, 13, 15, 80, 81, 82, 83, 84 };
const unsigned char kChr022P0Part4[] = { 102, 103, 104, 105, 106, 224, 225, 226, 227, 228 };
const unsigned char kChr022P0Part5[] = { 112, 113, 114, 115, 116, 230, 231, 232, 233, 234 };

const Part kParts[] = {
	{ 0, 0, 1, 31, "Skin, hands, legs", kChr000P0Part1 },
	{ 0, 0, 2, 6, "Hair", kChr000P0Part2 },
	{ 0, 0, 3, 5, "Skirt", kChr000P0Part3 },
	{ 0, 0, 4, 10, "Eyes, necklace", kChr000P0Part4 },
	{ 0, 0, 5, 15, "Shoes", kChr000P0Part5 },
	{ 1, 0, 1, 17, "Skin, chest ribbon, legs", kChr001P0Part1 },
	{ 1, 0, 2, 10, "Hair", kChr001P0Part2 },
	{ 1, 0, 3, 15, "Apron", kChr001P0Part3 },
	{ 1, 0, 4, 10, "Headband", kChr001P0Part4 },
	{ 1, 0, 5, 29, "Eyes, sleeves, collar, shoes", kChr001P0Part5 },
	{ 2, 0, 1, 21, "Skin, stockings, socks", kChr002P0Part1 },
	{ 2, 0, 2, 6, "Hair", kChr002P0Part2 },
	{ 2, 0, 3, 10, "Skirt", kChr002P0Part3 },
	{ 2, 0, 4, 5, "Tie", kChr002P0Part4 },
	{ 2, 0, 5, 9, "Eyes, headband", kChr002P0Part5 },
	{ 3, 0, 1, 16, "Skin, gakuran hooks, inner shirt", kChr003P0Part1 },
	{ 3, 0, 2, 6, "Hair", kChr003P0Part2 },
	{ 3, 0, 3, 20, "Gakuran collar, trousers", kChr003P0Part3 },
	{ 3, 0, 4, 10, "Sleeves", kChr003P0Part4 },
	{ 3, 0, 5, 33, "Shoes, knife, eyes", kChr003P0Part5 },
	{ 4, 0, 1, 11, "Skin, ribbon", kChr004P0Part1 },
	{ 4, 0, 2, 6, "Hair", kChr004P0Part2 },
	{ 4, 0, 3, 10, "Apron, Amber hood", kChr004P0Part3 },
	{ 4, 0, 4, 41, "Obi, Amber ribbon, broom, China star, molotov string", kChr004P0Part4 },
	{ 4, 0, 5, 36, "Eyes, tabi, sandals, plant, China shoes", kChr004P0Part5 },
	{ 5, 0, 1, 18, "Skin, undershirt, gloves", kChr005P0Part1 },
	{ 5, 0, 2, 6, "Hair", kChr005P0Part2 },
	{ 5, 0, 3, 7, "Trousers", kChr005P0Part3 },
	{ 5, 0, 4, 5, "Collar, sleeves", kChr005P0Part4 },
	{ 5, 0, 5, 28, "Eyes, belt, shoes", kChr005P0Part5 },
	{ 6, 0, 1, 18, "Skin", kChr006P0Part1 },
	{ 6, 0, 2, 6, "Hair", kChr006P0Part2 },
	{ 6, 0, 3, 10, "Trousers, collar", kChr006P0Part3 },
	{ 6, 0, 4, 27, "Eyes, gloves, bandages", kChr006P0Part4 },
	{ 6, 0, 5, 31, "Shoulders, chest line, coat", kChr006P0Part5 },
	{ 8, 0, 1, 12, "Skin", kChr008P0Part1 },
	{ 8, 0, 2, 6, "Hair", kChr008P0Part2 },
	{ 8, 0, 3, 9, "Clothing trim", kChr008P0Part3 },
	{ 8, 0, 4, 32, "Eyes, sleeves, halberd", kChr008P0Part4 },
	{ 8, 0, 5, 16, "Collar, boots", kChr008P0Part5 },
	{ 9, 0, 1, 18, "Skin, clothing trim, neck ornament", kChr009P0Part1 },
	{ 9, 0, 2, 6, "Hair", kChr009P0Part2 },
	{ 9, 0, 3, 5, "Trousers", kChr009P0Part3 },
	{ 9, 0, 4, 15, "Sleeves, sash, coat", kChr009P0Part4 },
	{ 9, 0, 5, 10, "Tie, gloves, boots", kChr009P0Part5 },
	{ 10, 0, 1, 23, "Skin, gloves", kChr010P0Part1 },
	{ 10, 0, 2, 6, "Hair", kChr010P0Part2 },
	{ 10, 0, 3, 5, "Skirt", kChr010P0Part3 },
	{ 10, 0, 4, 10, "Eyes, necklace", kChr010P0Part4 },
	{ 10, 0, 5, 12, "Legs, shoes", kChr010P0Part5 },
	{ 11, 0, 1, 22, "Sleeves, skin, stockings, socks", kChr011P0Part1 },
	{ 11, 0, 2, 6, "Hair", kChr011P0Part2 },
	{ 11, 0, 3, 10, "Clothing trim, uniform skirt", kChr011P0Part3 },
	{ 11, 0, 4, 17, "Collar, eyes, necklace", kChr011P0Part4 },
	{ 11, 0, 5, 34, "Shoes, Black Keys, uniform ribbon, metal, cross", kChr011P0Part5 },
	{ 11, 1, 1, 18, "Skin, arms, thighs, stockings", kChr011P1Part1 },
	{ 11, 1, 2, 6, "Hair", kChr011P1Part2 },
	{ 11, 1, 3, 17, "Gloves, skirt", kChr011P1Part3 },
	{ 11, 1, 4, 10, "Eyes, boot metal", kChr011P1Part4 },
	{ 11, 1, 5, 49, "Weapons, metal, snake sword grip, Seventh Scripture line, rifle", kChr011P1Part5 },
	{ 12, 0, 1, 18, "Skin, eyes, ribbon", kChr012P0Part1 },
	{ 12, 0, 2, 6, "Hair", kChr012P0Part2 },
	{ 12, 0, 3, 9, "Drawers", kChr012P0Part3 },
	{ 12, 0, 4, 26, "Armour, casual stockings", kChr012P0Part4 },
	{ 12, 0, 5, 13, "Front apron, skirt, casual boots", kChr012P0Part5 },
	{ 13, 0, 1, 24, "Skin, thighs, knee socks, socks", kChr013P0Part1 },
	{ 13, 0, 2, 5, "Hair", kChr013P0Part2 },
	{ 13, 0, 3, 15, "Shirt, half pants buttons, half pants", kChr013P0Part3 },
	{ 13, 0, 4, 25, "Clothes, sleeves, half pants cuffs, shoes", kChr013P0Part4 },
	{ 13, 0, 5, 24, "Eyes, ribbon, panda fur, panda pattern, Nanatsu-Yoru", kChr013P0Part5 },
	{ 14, 0, 1, 11, "Skin, thighs", kChr014P0Part1 },
	{ 14, 0, 2, 10, "Hair, eyebrows, pupils", kChr014P0Part2 },
	{ 14, 0, 3, 5, "Jacket, boots", kChr014P0Part3 },
	{ 14, 0, 4, 26, "Inner, spots, boot corners, knee socks", kChr014P0Part4 },
	{ 14, 0, 5, 9, "Ribbon, choker", kChr014P0Part5 },
	{ 15, 0, 1, 10, "Hair, eyebrows, pupils", kChr015P0Part1 },
	{ 15, 0, 2, 5, "Shirt", kChr015P0Part2 },
	{ 15, 0, 3, 5, "Pants", kChr015P0Part3 },
	{ 15, 0, 4, 5, "Pants pattern", kChr015P0Part4 },
	{ 15, 0, 5, 28, "Belt buckle, shoes, bag", kChr015P0Part5 },
	{ 16, 0, 1, 18, "Skin, arms, thighs, stockings", kChr016P0Part1 },
	{ 16, 0, 2, 6, "Hair", kChr016P0Part2 },
	{ 16, 0, 3, 17, "Gloves, skirt", kChr016P0Part3 },
	{ 16, 0, 4, 10, "Eyes, boot metal", kChr016P0Part4 },
	{ 16, 0, 5, 30, "Boots, weapon, metal, snake sword grip, Seventh Scripture line, rifle", kChr016P0Part5 },
	{ 17, 0, 1, 6, "Hood", kChr017P0Part1 },
	{ 17, 0, 2, 6, "Hair", kChr017P0Part2 },
	{ 17, 0, 3, 10, "Trousers", kChr017P0Part3 },
	{ 17, 0, 4, 33, "Eyes, gauntlets, shoes", kChr017P0Part4 },
	{ 17, 0, 5, 6, "Tights", kChr017P0Part5 },
	{ 17, 1, 1, 15, "Cape", kChr017P1Part1 },
	{ 17, 1, 2, 14, "Eyebrows, hood shadow, gloves", kChr017P1Part2 },
	{ 17, 1, 3, 9, "Clothing trim", kChr017P1Part3 },
	{ 17, 1, 4, 37, "Eyes, sleeves, weapon", kChr017P1Part4 },
	{ 17, 1, 5, 16, "Boots", kChr017P1Part5 },
	{ 17, 2, 1, 12, "Skin", kChr017P2Part1 },
	{ 17, 2, 2, 6, "Hair", kChr017P2Part2 },
	{ 17, 2, 3, 9, "Clothing trim", kChr017P2Part3 },
	{ 17, 2, 4, 32, "Eyes, sleeves, halberd", kChr017P2Part4 },
	{ 17, 2, 5, 16, "Collar, boots", kChr017P2Part5 },
	{ 18, 0, 1, 15, "Cape", kChr018P0Part1 },
	{ 18, 0, 2, 14, "Hood shadow, gloves", kChr018P0Part2 },
	{ 18, 0, 3, 9, "Clothing trim", kChr018P0Part3 },
	{ 18, 0, 4, 49, "Eyes, sleeves, weapon", kChr018P0Part4 },
	{ 18, 0, 5, 16, "Boots", kChr018P0Part5 },
	{ 18, 1, 1, 15, "Cape", kChr018P1Part1 },
	{ 18, 1, 2, 14, "Hood shadow, gloves", kChr018P1Part2 },
	{ 18, 1, 3, 9, "Clothing trim", kChr018P1Part3 },
	{ 18, 1, 4, 49, "Eyes, sleeves, weapon", kChr018P1Part4 },
	{ 18, 1, 5, 16, "Boots", kChr018P1Part5 },
	{ 18, 2, 1, 12, "Skin", kChr018P2Part1 },
	{ 18, 2, 2, 6, "Hair", kChr018P2Part2 },
	{ 18, 2, 3, 9, "Clothing trim", kChr018P2Part3 },
	{ 18, 2, 4, 32, "Eyes, sleeves, halberd", kChr018P2Part4 },
	{ 18, 2, 5, 16, "Collar, boots", kChr018P2Part5 },
	{ 19, 0, 1, 17, "Skin, hands, thighs", kChr019P0Part1 },
	{ 19, 0, 2, 16, "Hair, eyebrows, ear fur, tail", kChr019P0Part2 },
	{ 19, 0, 3, 5, "Skirt", kChr019P0Part3 },
	{ 19, 0, 4, 20, "Eyes, glasses, sleeves", kChr019P0Part4 },
	{ 19, 0, 5, 21, "Shoes, train window, props", kChr019P0Part5 },
	{ 19, 1, 1, 19, "Skin, gloves, stockings", kChr019P1Part1 },
	{ 19, 1, 2, 6, "Hair, eyebrows", kChr019P1Part2 },
	{ 19, 1, 3, 5, "Skirt frill", kChr019P1Part3 },
	{ 19, 1, 4, 18, "Eyes, ribbon", kChr019P1Part4 },
	{ 19, 1, 5, 17, "Shoes, sleeves", kChr019P1Part5 },
	{ 20, 0, 1, 26, "Skin, belly, thighs, knee socks", kChr020P0Part1 },
	{ 20, 0, 2, 6, "Hair, eyebrows", kChr020P0Part2 },
	{ 20, 0, 3, 26, "Waist armour, shield rim, disc, grip", kChr020P0Part3 },
	{ 20, 0, 4, 24, "Eyes, chest, arms", kChr020P0Part4 },
	{ 20, 0, 5, 20, "Chest accents, upper body, shield metal, gem", kChr020P0Part5 },
	{ 21, 0, 1, 22, "Skin, inner, hands, knee socks", kChr021P0Part1 },
	{ 21, 0, 2, 6, "Hair, eyebrows", kChr021P0Part2 },
	{ 21, 0, 3, 15, "Waist cord, socks, loincloth", kChr021P0Part3 },
	{ 21, 0, 4, 32, "Eyes, cloak, waist wraps, metal", kChr021P0Part4 },
	{ 21, 0, 5, 20, "Crown, hilt, spats, sword", kChr021P0Part5 },
	{ 22, 0, 1, 22, "Skin, hat, inner shirt, hands", kChr022P0Part1 },
	{ 22, 0, 2, 6, "Hair, eyebrows", kChr022P0Part2 },
	{ 22, 0, 3, 12, "Eyes, cloak", kChr022P0Part3 },
	{ 22, 0, 4, 10, "Poncho, trousers", kChr022P0Part4 },
	{ 22, 0, 5, 10, "Scarf, shoes", kChr022P0Part5 },
};

const Part* Find(int chara, int sub, int part)
{
	for (const Part& entry : kParts)
	{
		if (entry.chara == chara && entry.sub == sub && entry.part == part)
			return &entry;
	}

	return nullptr;
}

int Owned(int chara, int sub, bool* owned)
{
	int found = 0;

	for (int part = 1; part < ColorPartTable::kParts; ++part)
	{
		const Part* const entry = Find(chara, sub, part);

		if (entry == nullptr)
			continue;

		++found;

		for (int i = 0; i < entry->count; ++i)
			owned[entry->entries[i]] = true;
	}

	return found;
}

}

bool ColorPartTable::Has(int chara, int sub)
{
	return Find(chara, sub, 1) != nullptr;
}

const char* ColorPartTable::Name(int chara, int sub, int part)
{
	if (part == 0)
		return "Base";

	const Part* const entry = Find(chara, sub, part);
	return entry != nullptr ? entry->name : "";
}

int ColorPartTable::Entries(int chara, int sub, int part, unsigned char* out)
{
	if (part < 0 || part >= kParts || out == nullptr)
		return 0;

	if (part > 0)
	{
		const Part* const entry = Find(chara, sub, part);

		if (entry == nullptr)
			return 0;

		for (int i = 0; i < entry->count; ++i)
			out[i] = entry->entries[i];

		return entry->count;
	}

	bool owned[kColours] = {};

	if (Owned(chara, sub, owned) == 0)
		return 0;

	int count = 0;

	for (int index = 1; index < kColours; ++index)
	{
		if (!owned[index])
			out[count++] = static_cast<unsigned char>(index);
	}

	return count;
}
