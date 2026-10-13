#pragma once
#include "resnet50/engine/polyphase_convolution.hpp"
struct PublicStride {unsigned stride,support;};
inline PublicStride r50_stride(unsigned id,unsigned oi){if(id==0&&aqcf_polyphase458::enabled()){if(oi>=128)throw std::runtime_error("458 output range");return {0,0};}switch(id){
case 0:
if(oi<22)return {0,0};
if(oi<44)return {0,0};
if(oi<66)return {0,0};
if(oi<79)return {0,0};
if(oi<101)return {0,0};
if(oi<123)return {0,0};
if(oi<145)return {0,0};
if(oi<158)return {0,0};
if(oi<180)return {0,0};
if(oi<202)return {0,0};
if(oi<224)return {0,0};
if(oi<237)return {0,0};
if(oi<259)return {0,0};
if(oi<281)return {0,0};
if(oi<303)return {0,0};
if(oi<316)return {0,0};
if(oi<338)return {0,0};
if(oi<360)return {0,0};
if(oi<382)return {0,0};
if(oi<395)return {0,0};
if(oi<417)return {0,0};
if(oi<439)return {0,0};
if(oi<461)return {0,0};
if(oi<474)return {0,0};
if(oi<496)return {0,0};
if(oi<518)return {0,0};
if(oi<540)return {0,0};
if(oi<553)return {0,0};
throw std::runtime_error("public output index");
case 1:
if(oi<64)return {6272,1};
throw std::runtime_error("public output index");
case 2:
if(oi<64)return {1792,4};
if(oi<107)return {1344,6};
throw std::runtime_error("public output index");
case 3:
if(oi<32)return {3136,2};
throw std::runtime_error("public output index");
case 4:
if(oi<32)return {0,0};
throw std::runtime_error("public output index");
case 5:
if(oi<64)return {1792,4};
if(oi<107)return {1344,6};
throw std::runtime_error("public output index");
case 6:
if(oi<256)return {6272,1};
throw std::runtime_error("public output index");
case 7:
if(oi<32)return {1792,4};
if(oi<64)return {1344,6};
throw std::runtime_error("public output index");
case 8:
if(oi<32)return {0,0};
throw std::runtime_error("public output index");
case 9:
if(oi<64)return {1792,4};
if(oi<107)return {1344,6};
throw std::runtime_error("public output index");
case 10:
if(oi<256)return {6272,1};
throw std::runtime_error("public output index");
case 11:
if(oi<32)return {1792,4};
if(oi<64)return {1344,6};
throw std::runtime_error("public output index");
case 12:
if(oi<32)return {0,0};
throw std::runtime_error("public output index");
case 13:
if(oi<64)return {1792,4};
if(oi<107)return {1344,6};
throw std::runtime_error("public output index");
case 14:
if(oi<256)return {6272,1};
throw std::runtime_error("public output index");
case 15:
if(oi<103)return {784,10};
throw std::runtime_error("public output index");
case 16:
if(oi<64)return {1792,4};
if(oi<107)return {1344,6};
throw std::runtime_error("public output index");
case 17:
if(oi<64)return {0,0};
throw std::runtime_error("public output index");
case 18:
if(oi<52)return {784,10};
throw std::runtime_error("public output index");
case 19:
if(oi<171)return {3136,3};
throw std::runtime_error("public output index");
case 20:
if(oi<64)return {784,10};
throw std::runtime_error("public output index");
case 21:
if(oi<43)return {0,0};
throw std::runtime_error("public output index");
case 22:
if(oi<43)return {3136,3};
throw std::runtime_error("public output index");
case 23:
if(oi<52)return {784,10};
throw std::runtime_error("public output index");
case 24:
if(oi<171)return {3136,3};
throw std::runtime_error("public output index");
case 25:
if(oi<64)return {784,10};
throw std::runtime_error("public output index");
case 26:
if(oi<43)return {0,0};
throw std::runtime_error("public output index");
case 27:
if(oi<52)return {784,10};
throw std::runtime_error("public output index");
case 28:
if(oi<171)return {3136,3};
throw std::runtime_error("public output index");
case 29:
if(oi<64)return {784,10};
throw std::runtime_error("public output index");
case 30:
if(oi<43)return {0,0};
throw std::runtime_error("public output index");
case 31:
if(oi<52)return {784,10};
throw std::runtime_error("public output index");
case 32:
if(oi<171)return {3136,3};
throw std::runtime_error("public output index");
case 33:
if(oi<103)return {196,40};
throw std::runtime_error("public output index");
case 34:
if(oi<52)return {784,10};
throw std::runtime_error("public output index");
case 35:
if(oi<86)return {0,0};
throw std::runtime_error("public output index");
case 36:
if(oi<52)return {196,40};
throw std::runtime_error("public output index");
case 37:
if(oi<171)return {1372,6};
throw std::runtime_error("public output index");
case 38:
if(oi<64)return {196,40};
throw std::runtime_error("public output index");
case 39:
if(oi<29)return {0,0};
throw std::runtime_error("public output index");
case 40:
if(oi<52)return {196,40};
throw std::runtime_error("public output index");
case 41:
if(oi<171)return {1372,6};
throw std::runtime_error("public output index");
case 42:
if(oi<64)return {196,40};
throw std::runtime_error("public output index");
case 43:
if(oi<29)return {0,0};
throw std::runtime_error("public output index");
case 44:
if(oi<52)return {196,40};
throw std::runtime_error("public output index");
case 45:
if(oi<171)return {1372,6};
throw std::runtime_error("public output index");
case 46:
if(oi<64)return {196,40};
throw std::runtime_error("public output index");
case 47:
if(oi<29)return {0,0};
throw std::runtime_error("public output index");
case 48:
if(oi<52)return {196,40};
throw std::runtime_error("public output index");
case 49:
if(oi<171)return {1372,6};
throw std::runtime_error("public output index");
case 50:
if(oi<64)return {196,40};
throw std::runtime_error("public output index");
case 51:
if(oi<29)return {0,0};
throw std::runtime_error("public output index");
case 52:
if(oi<52)return {196,40};
throw std::runtime_error("public output index");
case 53:
if(oi<171)return {1372,6};
throw std::runtime_error("public output index");
case 54:
if(oi<64)return {196,40};
throw std::runtime_error("public output index");
case 55:
if(oi<29)return {0,0};
throw std::runtime_error("public output index");
case 56:
if(oi<52)return {196,40};
throw std::runtime_error("public output index");
case 57:
if(oi<171)return {1372,6};
throw std::runtime_error("public output index");
case 58:
if(oi<76)return {49,162};
throw std::runtime_error("public output index");
case 59:
if(oi<64)return {196,40};
throw std::runtime_error("public output index");
case 60:
if(oi<57)return {0,0};
throw std::runtime_error("public output index");
case 61:
if(oi<50)return {49,164};
throw std::runtime_error("public output index");
case 62:
if(oi<171)return {637,12};
throw std::runtime_error("public output index");
case 63:
if(oi<47)return {49,165};
throw std::runtime_error("public output index");
case 64:
if(oi<32)return {0,0};
throw std::runtime_error("public output index");
case 65:
if(oi<50)return {49,164};
throw std::runtime_error("public output index");
case 66:
if(oi<171)return {637,12};
throw std::runtime_error("public output index");
case 67:
if(oi<47)return {49,165};
throw std::runtime_error("public output index");
case 68:
if(oi<32)return {0,0};
throw std::runtime_error("public output index");
case 69:
if(oi<50)return {49,164};
throw std::runtime_error("public output index");
case 70:
if(oi<171)return {637,12};
throw std::runtime_error("public output index");
case 71:
if(oi<11)return {1,8190};
throw std::runtime_error("public output index");
default:throw std::runtime_error("public affine index");}}
